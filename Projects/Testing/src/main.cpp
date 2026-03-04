#include <chrono>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#include<core/type.hpp>
#include<util/string_generator.hpp>

#include<util/logger.hpp>

//#include<util/DSL_util.hpp>
#include<core/GDSL.hpp>

#include<util/cog.hpp>

// type sub_num {
//     int x;
//     int y;
// }

// type num {
//     int a;
//     int b;
//     sub_num n;
//     int o;
// }

// num c;
// num d;

// c.a = 1; d.a = 4;
// c.b = 3; d.b = 6;
// c.n.x = 8; d.n.x = 16;
// print("c.a: ",c.a," d.a: ",d.a);
// print("c.b: ",c.b," d.b: ",d.b);
// print("c.n.x: ",c.n.x," d.n.x: ",d.n.x);
// c = d;
// print("c.a: ",c.a," d.a: ",d.a);
// print("c.b: ",c.b," d.b: ",d.b);
// print("c.n.x: ",c.n.x," d.n.x: ",d.n.x);


// type num {
//     int x;
//     int y;
// }

// num c;
// c.x = 42;
// c.y = 7;

// num* ptr;
// ptr = (&c);

// print("before: ", c.x, " ", c.y);
// print("via ptr: ", (*ptr).x, " ", (*ptr).y);

// (*ptr).x = 100;
// print("after: ", c.x);



// print("A");

// type num {
//     int o;
// }

// num* ptr;
// int* arr;
// arr = malloc(16);
// arr[0] = 10;
// arr[1] = 20;
// arr[2] = 30;
// print(arr[0], " ", arr[1], " ", arr[2]);

// int a = 1 + 2 * 3;
// int b = a * 2 + 1;
// int* p = &a;
// int c = *p;

namespace GDSL {
    map<std::string,g_ptr<Value>> keywords;
    map<char,bool> char_is_split;
    map<uint32_t,bool> state_is_opp;
    map<uint32_t,int> type_precdence;

    map<uint32_t,int> left_binding_power;
    map<uint32_t,int> right_binding_power;

    static bool balance_nodes(list<g_ptr<a_node>>& result) {
        uint32_t state = 0;
        int corrections = 0;
        for (int i = result.size() - 1; i >= 0; i--) {
            g_ptr<a_node> right = result[i];
            if(!right->sub_nodes.empty()) {
                bool changed = true;
                int depth = 0;
                while (changed&&depth<1000) {
                    depth++;
                    changed = balance_nodes(right->sub_nodes);
                }
            }
            if(i==0) continue;
            state=result[i]->type;
            g_ptr<a_node> left = result[i-1];
            if(state_is_opp.getOrDefault(state,false)&&state_is_opp.getOrDefault(left->type,false)) 
            {
                //printnl("LEFT:",type_precdence.get(left->type),":",left->balanced,": "); print_a_node(left); printnl("   "); printnl("RIGHT:",type_precdence.get(state),":",right->balanced,": "); print_a_node(right); print("");
                if(type_precdence.get(left->type)<type_precdence.get(state))
                {
                    #if PRINT_ALL && PRINT_STYLE == 1
                        print("");
                        print_a_node(left); printnl(" <- "); print_a_node(right);
                        print("\n v v v v v v v v");
                    #endif
                    left->sub_nodes << right;
                    if(!left->tokens.empty()&&!left->balanced) {
                        right->tokens.insert(left->tokens.pop(),0);
                    }
                    result.removeAt(i);
                    right->balanced = true;
                    left->balanced = true;
                    #if PRINT_ALL && PRINT_STYLE == 1
                        print_a_node(left); print("\n");
                    #endif
                    corrections++;
                }
            }
        }
        return corrections!=0;
    }

    //As of GDSL 0.1.3 right-associative unary parsing needs explicit parens, i.e if you want to do i=-1, you need i=(-1);
    //Fiqure out how to fix this later
    static void balance_precedence(list<g_ptr<a_node>>& result) {
        bool changed = true;
        int depth = 0;
        while (changed&&depth<1000) {
            depth++;
            #if PRINT_ALL
            print("==BALANCING PASS ",depth,"==");
            #endif
            changed = balance_nodes(result);
        }

        #if PRINT_ALL
        print("Finished balancing");
        int i = 0;
        for (auto& node : result) {
            print_a_node(node, 0, i++);
        }
        #endif
    }

    map<uint32_t, std::function<void(g_ptr<s_node>, g_ptr<s_node>, g_ptr<a_node>)>> scope_link_handlers;
    map<uint32_t, int> scope_precedence;

    void print_scope(g_ptr<s_node> scope, int depth = 0) {
        std::string indent(depth * 2, ' ');
        
        if (depth > 0) {
            print(indent, "{");
        }
        
        for (auto& a_node : scope->a_nodes) {
            print(indent, "  ", TO_STRING(a_node->type));
        
            
            if (a_node->owned_scope) {
                print_scope(a_node->owned_scope, depth + 1);
            }
        }
        
        if (depth > 0) {
            print(indent, "}");
        }
    }

    struct g_value {
    public:
        g_value() {}
        g_value(g_ptr<a_node> _owner) : owner(_owner) {}
        bool explc = false;
        g_ptr<a_node> owner = nullptr;
        bool deferred = false;
    };

    static g_ptr<s_node> parse_scope(list<g_ptr<a_node>> nodes) {
        g_ptr<s_node> root_scope = make<s_node>();
        g_ptr<s_node> current_scope = root_scope;
        list<g_value> stack{g_value()};

        for (int i = 0; i < nodes.size(); ++i) {
            g_ptr<a_node> node = nodes[i];
            g_ptr<a_node> owner_node = (i>0) ? nodes[i - 1] : nullptr;

            int p = scope_precedence.getOrDefault(node->type,0);
            bool on_stack = stack.last().owner ? true : false;
            if(p<=0) {
                if(p<0) {
                    if (current_scope->parent) {
                        current_scope = current_scope->parent;
                    }
                }
                else {
                    current_scope->a_nodes << node; 
                    node->in_scope = current_scope.getPtr();
                    if(on_stack && !stack.last().deferred && !stack.last().explc) {
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                }
            }
            else {
                if(p<10) {
                    current_scope->a_nodes << node;
                    node->in_scope = current_scope.getPtr();
                    owner_node = node;
                }

                if(on_stack) {
                    int stack_precedence = scope_precedence.getOrDefault(stack.last().owner->type,0);
                    if(p >= stack_precedence) {
                        stack.last().deferred = true;
                    }
                }

                if(p == 10) {
                    if(on_stack) {
                        stack.last().explc = true;
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                }
                else {
                    stack << g_value(owner_node);
                }

                g_ptr<s_node> parent_scope = current_scope;
                current_scope = current_scope->spawn_node();
                if (owner_node) {
                    //Deffensive check here
                    try {
                        auto func = scope_link_handlers.get(owner_node->type);
                        func(current_scope,parent_scope,owner_node);
                    }
                    catch(std::exception e) {
                        print("parse_scope::809 missing scope link handler for type: ",TO_STRING(owner_node->type));
                    }
                
                } else {
                    current_scope->scope_type = GET_TYPE(BLOCK);
                }
            }
        }

        #if PRINT_ALL
        print("=== PARSE SCOPE PASS ===");
        print_scope(root_scope);
        #endif
        return root_scope;
    }


    g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
        return make<Value>(reg::new_type(name),size,sub_type);
    }

    size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
        g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
        keywords.put(name,val);
        return val->type;
    }

    template<typename Op>
    static size_t add_function(const std::string& f, Op op) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t call_id = make_keyword(f, 0, uppercase_name);
        state_is_opp.put(call_id, true);
        type_precdence.put(call_id, 20);
        t_functions.put(call_id, [call_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = call_id;
            parse_sub_nodes(ctx, true);
            return ctx.result;
        });
        r_handlers.put(call_id, [call_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = call_id;
            if(ctx.node->left) 
                result->children << resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            if(ctx.node->right)
                result->children << resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            for(auto c : ctx.node->children) {
                result->children << resolve_symbol(c, ctx.scope, ctx.frame);
            }
        });
        exec_handlers.put(call_id, op);

        return call_id;
    }

    // T() {} <- As with function, but we ctx.node->frame contains the bracketed code for further execution.
    template<typename Op>
    static void add_scoped_keyword(const std::string& name, int scope_prec, Op exec_fn)
    {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);

        size_t key_id = reg::new_type(s + "_KEY");
        size_t block_id = reg::new_type(s + "_BLOCK");
        size_t id = make_keyword(name,0,s,0);
        
        a_functions.put(key_id, [id](a_context& ctx) {
            ctx.node = make<a_node>();
            ctx.node->type = id;
            // consume condition if present, e.g. if(...)
        });
        scope_link_handlers.put(id, [block_id](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            new_scope->scope_type = block_id;
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        });
        scope_precedence.put(id, scope_prec);
        t_functions.put(id, [id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> result = t_default_function(ctx);
            result->scope = ctx.node->owned_scope;
            return result;
        });
        r_handlers.put(id, [id](g_ptr<r_node> result, r_context& ctx) {
            result->type = id;
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            result->frame = resolve_symbols(ctx.node->scope);
            if(ctx.node->left) {
                result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            }
            return result;
        });
        exec_handlers.put(id, exec_fn);
    }

    template<typename InputT, typename ResultT, typename Op>
    std::function<g_ptr<r_node>(exec_context& ctx)> make_arithmetic_handler(Op operation, uint32_t return_type) {
        return [operation,return_type](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, 0, -1);
            execute_r_node(ctx.node->right, ctx.frame, 0, -1);
            
            InputT left = ctx.node->left->value->get<InputT>();
            InputT right = ctx.node->right->value->get<InputT>();
            ResultT result = operation(left, right);
            
            ctx.node->value->type = return_type;
            ctx.node->value->set<ResultT>(result);
            return ctx.node;
        };
    }

    size_t add_token(char c, const std::string& f) {
        size_t id = reg::new_type(f);
        tokenizer_functions.put(c,[id,c](tokenizer_context& ctx) {
            ctx.token = make<Token>(id,c);
            ctx.result << ctx.token;
        });
        char_is_split.put(c, true);
        return id;
    }

    // xTx <- Binary operator, ctx.node->right and ctx.node->left are what's on your right and left
    size_t add_binary_operator(char c, const std::string& f,int left_bp, int right_bp,exec_handler operation) {
        size_t id = add_token(c,f);
        left_binding_power.put(id, left_bp);
        right_binding_power.put(id, right_bp);
        r_handlers.put(id, [id](g_ptr<r_node> result, r_context& ctx) {
                result->type = id;
                if(ctx.node->left) 
                    result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
                if(ctx.node->right)
                    result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        });
        exec_handlers.put(id, operation);
        return id;
    }

    size_t add_base_type(const std::string& f, size_t size, a_handler key_a_function) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t id = reg::new_type(uppercase_name);
        size_t key_id = make_keyword(f,size,uppercase_name+"_KEY",id);
        a_functions.put(key_id, key_a_function);
        // a_functions.put(id, [id](a_context& ctx) {
        //     ctx.node = make<a_node>();
        //     ctx.node->type = id;
        //     ctx.node->tokens << ctx.tokens[ctx.index];
        // });
        return id;
    }

    g_ptr<a_node> a_parse_expression(a_context& ctx, int min_bp, g_ptr<a_node> left_node = nullptr) {
        //Prefixual pass
        g_ptr<Token> token = ctx.tokens[ctx.index];
        uint32_t type = token->getType();
        int left_bp = left_binding_power.getOrDefault(type,-1);
        int right_bp = right_binding_power.getOrDefault(type,-1);
        bool has_func = a_functions.hasKey(type);

        print("a_parsing: ",ctx.tokens[ctx.index]->content," [",TO_STRING(ctx.tokens[ctx.index]->getType()),", lbp: ",left_bp,", rbp: ",right_bp,", has_func: ",has_func?"true":"false",", idx: ",ctx.index,"]");
        if(left_node) {
            print("Has a left node:"); print_a_node(left_node);
        }
            

        if(left_bp==-1&&right_bp==-1&&has_func) {
            return nullptr;
        }

        ctx.index++;
        print("     Prefixual, Looking at: ",ctx.tokens[ctx.index]->content," [",TO_STRING(ctx.tokens[ctx.index]->getType()),", idx: ",ctx.index,"]");
        if(!left_node)
            left_node = make<a_node>();
        if(has_func) { //Has a function but no left: direct node build
            ctx.left = left_node;       //Give handler its left context
            ctx.node = make<a_node>();  //Fresh output target
            a_functions.get(type)(ctx);
            left_node = ctx.node; //Read result back
        }
        else if(right_bp!=-1) { //Prefixual unary: recurse with right
            g_ptr<a_node> right_node = a_parse_expression(
                ctx,
                right_bp,
                nullptr
            );
            left_node->type = type;
            if(right_node)
                left_node->sub_nodes << right_node;
        }
        else { //Else atom, like a literal or idenitifer
            if(left_node->type == 0)     // only set type if not already set
                left_node->type = type;
            left_node->tokens << token;
        }

        //Infixual pass
        while(ctx.index < ctx.tokens.length()) {
            g_ptr<Token> op = ctx.tokens[ctx.index];
            int op_left_bp = left_binding_power.getOrDefault(op->getType(), -1);

            if(op_left_bp < min_bp || op_left_bp==-1) break;
            
            
            ctx.index++;
            print("     Infixual, Looking at: ",ctx.tokens[ctx.index]->content," [",TO_STRING(ctx.tokens[ctx.index]->getType()),", idx: ",ctx.index,"]");

            if(a_functions.hasKey(op->getType())) {
                ctx.left = left_node;
                ctx.node = make<a_node>();
                a_functions.get(op->getType())(ctx);
                left_node = ctx.node;
            } else {
                g_ptr<a_node> right_node = a_parse_expression(
                    ctx,
                    right_binding_power.getOrDefault(op->getType(), op_left_bp + 1),
                    nullptr
                );
                
                g_ptr<a_node> node = make<a_node>();
                node->type = op->getType();
                if(left_node)
                    node->sub_nodes << left_node;
                if(right_node)
                    node->sub_nodes << right_node;
                left_node = node;
            }
            
        }
        return left_node;
    };

    void test_module(const std::string& path) {
        a_default_function = [](a_context& ctx) {
            print("Triggered default for: ",ctx.tokens[ctx.index]->content," [",TO_STRING(ctx.tokens[ctx.index]->getType()),"]");
            g_ptr<a_node> expr = a_parse_expression(ctx,0);
            if(expr) {
                ctx.result << expr;
                ctx.index--;
            }
        };

        size_t undefined_id = reg::new_type("UNDEFINED");

        //In essence, the a_stage accumulates tokens which will later be sorted into left/right/child by the t_stage
        //So the default function is just saying 'fold in anything not flagged as an opp'
        //An example would be: string name = "joe"
        //Which tokenizes as: STRING_KEY IDENTIFIER EQUALS STRING_LITERAL
        //The a_stage will turn that into: VAR_DECL[STRING_KEY, IDENTIFIER] ASSIGNMENT[STRING_LITERAL]
        // a_default_function = [](a_context& ctx) {
        //     uint32_t opp = ctx.token->getType();
        //     if(opp!=0) {
        //         //If we're already in an opperation, end the current one, start a new one
        //         if(state_is_opp.getOrDefault(ctx.state,false)) {
        //             ctx.end();
        //         }
        //         ctx.state=opp;
        //         return;
        //     }
        //     ctx.node->tokens << ctx.token;
        // };


        size_t var_decl_id = reg::new_type("VAR_DECL");
        size_t object_id = reg::new_type("OBJECT");
        value_to_string.put(object_id, [](void* data) {
            return std::string("[object @") + std::to_string((size_t)data) + "]";
        });

        size_t type_id = reg::new_type("TYPE");
        size_t type_decl_id = make_keyword("type",8,"TYPE_DECL");
        // a_functions.put(type_decl_id, [type_decl_id](a_context& ctx) {
        //     ctx.state = type_decl_id;
        // });
        // scope_link_handlers.put(type_decl_id, [type_id](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
        //     new_scope->scope_type = type_id;
        //     new_scope->owner = owner_node;
        //     new_scope->type_ref = make<Type>();
        //     owner_node->owned_scope = new_scope.getPtr();
        // });
        // t_functions.put(type_decl_id, [type_decl_id](t_context& ctx) -> g_ptr<t_node> {
        //     ctx.result->type = type_decl_id;
        //     ctx.result->name = ctx.node->tokens.last()->content;
        //     ctx.result->scope = ctx.node->owned_scope;
        //     ctx.node->owned_scope->t_owner = ctx.result;
        //     ctx.result->scope->name = ctx.result->name;
        //     return ctx.result;
        // });
        // //DISCOVERY BELOW
        // r_handlers.put(type_decl_id, [type_decl_id](g_ptr<r_node> result, r_context& ctx) {
        //     result->type = type_decl_id;
        //     result->frame = resolve_symbols(ctx.node->scope);
        // });
        // exec_handlers.put(type_decl_id, [](exec_context& ctx) -> g_ptr<r_node> {
        //     // Probably do nothing for now
        //     return ctx.node;
        // });
        // stream_handlers.put(type_decl_id, [](exec_context& ctx) -> std::function<void()>{
        //    return nullptr;
        // });


        size_t literal_id = reg::new_type("LITERAL");
        r_handlers.put(literal_id, [literal_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = literal_id;
            result->value = ctx.node->value;
        });
        exec_handlers.put(literal_id, [](exec_context& ctx) -> g_ptr<r_node> {
            return ctx.node;
        });

        size_t assignment_id = add_token('=',"ASSIGNMENT");
        left_binding_power.put(assignment_id, 1);
        right_binding_power.put(assignment_id, 0);
        r_handlers.put(assignment_id, [assignment_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = assignment_id;
            result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        });
        exec_handlers.put(assignment_id, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
            execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
            memcpy(ctx.node->left->value->data, ctx.node->right->value->data, ctx.node->right->value->size);
            return ctx.node;
        });

        size_t identifier_id = reg::new_type("IDENTIFIER"); 
        size_t function_id = reg::new_type("FUNCTION");
        size_t method_id = reg::new_type("METHOD");
        size_t func_decl_id = reg::new_type("FUNC_DECL");
        scope_link_handlers.put(var_decl_id, [type_decl_id,function_id,method_id,func_decl_id](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            if(current_scope->scope_type == type_decl_id)
                new_scope->scope_type = method_id;
            else 
                new_scope->scope_type = function_id;
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
            owner_node->type = func_decl_id;
            new_scope->name = owner_node->tokens.last()->content;
        });
        scope_link_handlers.put(identifier_id,scope_link_handlers.get(var_decl_id));
        t_functions.put(func_decl_id, [func_decl_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.root = ctx.node->owned_scope;
            parse_sub_nodes(ctx);
            ctx.result->type = func_decl_id;
            ctx.result->name = ctx.node->tokens.last()->content;
            ctx.result->scope = ctx.node->owned_scope;
            ctx.result->scope->t_owner = ctx.result;
            return ctx.result;
        });
        r_handlers.put(func_decl_id, [func_decl_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = func_decl_id;
            result->name = ctx.node->name;
            result->frame = resolve_symbols(ctx.node->scope);
            for(auto c : ctx.node->children) {
                g_ptr<r_node> child = resolve_symbol(c, ctx.node->scope, result->frame);
                result->children << child;
            }
        });
        exec_handlers.put(func_decl_id, [](exec_context& ctx) -> g_ptr<r_node> {
            //Do nothing
            return ctx.node;
        });

        size_t func_call_id = reg::new_type("FUNC_CALL");
        r_handlers.put(func_call_id, [func_call_id,func_decl_id,identifier_id,assignment_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = func_call_id;
            result->name = ctx.node->name;
            g_ptr<r_node> decl = nullptr;
            find_scope(ctx.scope,[func_decl_id,result,&decl](g_ptr<s_node> scope){
                for(auto t : scope->frame->nodes) {
                    if(t->type==func_decl_id&&t->name==result->name) {
                        decl = t;
                        return true;
                    }
                }
                return false;
            });
            if(decl) {
                result->frame = decl->frame;
                for(int i = 0; i < ctx.node->children.size(); i++) {
                    g_ptr<r_node> arg = resolve_symbol(ctx.node->children[i], ctx.scope, ctx.frame);
                    g_ptr<r_node> param = decl->children[i];
                    // param->type = identifier_id;
                    g_ptr<r_node> assignment = make<r_node>();
                    assignment->type = assignment_id;
                    assignment->left = param;
                    assignment->right = arg;
                    result->children << assignment;
                }
            }
        });
        exec_handlers.put(func_call_id, [](exec_context& ctx) -> g_ptr<r_node> {
            for(auto c : ctx.node->children) {
                execute_r_node(c, ctx.frame, ctx.index, ctx.sub_index);
            }
            ctx.node->frame->return_to = ctx.node;
            execute_sub_frame(ctx.node->frame, ctx.frame);
            return ctx.node;
        });
        
        size_t return_id = make_keyword("return",0,"RETURN");
        a_functions.put(return_id, [return_id](a_context& ctx) {
            ctx.index++; 
            ctx.node = make<a_node>();
            ctx.node->type = return_id;
            g_ptr<a_node> expr = a_parse_expression(ctx, 0, nullptr);
            if(expr) ctx.node->sub_nodes << expr;
            ctx.result << ctx.node;
        });
        // a_functions.put(return_id, [return_id](a_context& ctx) {
        //     ctx.state = return_id;
        //     int next_end = 0;
        //     for(int i = ctx.index+1; i < ctx.tokens.length(); i++) {
        //         if(ctx.tokens[i]->getType() == GET_TYPE(END)) {
        //             next_end = i;
        //             break;
        //         }
        //     }
        //     list<g_ptr<Token>> sub_list;
        //     for(int i = ctx.index+1; i < next_end; i++)
        //         sub_list.push(ctx.tokens[i]);
        //     ctx.node->sub_nodes = parse_tokens(sub_list, true);
        //     ctx.end_lambda();
        //     ctx.state = 0;
        //     ctx.index = next_end-1;
        //     ctx.it = ctx.tokens.begin() + next_end;
        //     ctx.skip_inc = 1;
        // });
        // t_functions.put(return_id, [return_id](t_context& ctx) -> g_ptr<t_node> {
        //     ctx.result->type = return_id;
        //     parse_sub_nodes(ctx, true);
        //     return ctx.result;
        // });
        // r_handlers.put(return_id, [return_id](g_ptr<r_node> result, r_context& ctx) {
        //     result->type = return_id;
        //     result->right = resolve_symbol(ctx.node->children[0], ctx.scope, ctx.frame);
        // });
        // exec_handlers.put(return_id, [](exec_context& ctx) -> g_ptr<r_node> {
        //     execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
        //     g_ptr<Value> ret_val = make<Value>(ctx.node->right->value->type);
        //     ret_val->size = ctx.node->right->value->size;
        //     ret_val->data = malloc(ret_val->size);
        //     memcpy(ret_val->data, ctx.node->right->value->data, ret_val->size);
        //     ctx.frame->return_to->value = ret_val;
        //     return ctx.node;
        // });

        
        t_functions.put(identifier_id, [identifier_id,var_decl_id,type_decl_id,object_id,func_call_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = identifier_id;
            
            std::string type_name = ctx.node->tokens[0]->content;
            node->name = ctx.node->tokens.size() > 1 ? ctx.node->tokens[1]->content : type_name;

            g_ptr<s_node> scope = find_scope(ctx.root,[type_name,type_decl_id](g_ptr<s_node> scope){
                return scope->name == type_name;
            });
            if(scope) {
                if(ctx.node->tokens.length()==1&&!ctx.node->sub_nodes.empty()) { //Like one(3);
                    node->type = func_call_id;
                    parse_sub_nodes(ctx);
                    node->children = ctx.result->children;
                } else { //Like int a;
                    node->type = var_decl_id;
                    node->deferred_identifier = type_name;
                    g_ptr<Value> sub_value = make<Value>(0);
                    sub_value->type = object_id;
                    sub_value->sub_type = object_id;
                    node->value = sub_value;
                    node->scope = ctx.root.getPtr();
                }
            }
            return node;
        });  
        r_handlers.put(identifier_id, [identifier_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = identifier_id;
            result->name = ctx.node->name;
            g_ptr<s_node> scope = find_scope(ctx.scope,[&](g_ptr<s_node> scope_node){
                return scope_node->values.hasKey(ctx.node->name);
            });
            if(scope) {
                result->value = scope->values.get(ctx.node->name);
                result->in_frame = scope->frame;
            } else {
                print("UNRESOVED SCOPE FOR: ",result->name);
            }
        });
        exec_handlers.put(identifier_id, [](exec_context& ctx) -> g_ptr<r_node> {
            return ctx.node;
        });

        left_binding_power.put(var_decl_id,1);
        right_binding_power.put(var_decl_id,0);
        t_functions.put(var_decl_id, [var_decl_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = var_decl_id;
            ctx.result->value = ctx.node->tokens[0]->value;
            ctx.result->name = ctx.node->tokens[1]->content;
            ctx.result->scope = ctx.root.getPtr();
            return ctx.result;
        });
        //DISCOVERY BELOW
        r_handlers.put(var_decl_id, [var_decl_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = var_decl_id;
            result->name = ctx.node->name;
            g_ptr<s_node> scope = find_scope(ctx.scope,[&](g_ptr<s_node> s){
                return s->values.hasKey(ctx.node->name);
            });
            if(scope) {
                result->value = scope->values.get(ctx.node->name);
                result->in_frame = scope->frame;
            }
        });
        exec_handlers.put(var_decl_id, [](exec_context& ctx) -> g_ptr<r_node> {
            ctx.frame->active_memory << ctx.node->value->data;
            return ctx.node;
        });






        a_handler make_var_decl_on_key = [var_decl_id](a_context& ctx) {
            g_ptr<a_node> node = make<a_node>();
            node->type = var_decl_id;
            node->tokens << ctx.tokens[ctx.index]; // just the type token
            ctx.index++;
            
            g_ptr<a_node> expr = a_parse_expression(ctx, 0, node);
            if(expr) node = expr;
            ctx.index--;
            ctx.result << node;
        };

        size_t float_id = add_base_type("float",4,make_var_decl_on_key);
        value_to_string.put(float_id,[](void* data) {
            return std::to_string(*(float*)data);
        });
        negate_value.put(float_id,[](void* data) {
            *(float*)data = -(*(float*)data);
        });
        t_functions.put(float_id, [float_id,literal_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = literal_id;
            node->value->type = float_id;
            node->value->set<float>(std::stoi(ctx.node->tokens[0]->content));
            node->value->size = 4;
            return node;
        }); 

        size_t int_id = add_base_type("int",4,make_var_decl_on_key);
        value_to_string.put(int_id,[](void* data) {
            return std::to_string(*(int*)data);
        });
        negate_value.put(int_id,[](void* data) {
            *(int*)data = -(*(int*)data);
        });
        t_functions.put(int_id, [int_id,literal_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = literal_id;
            node->value->type = int_id;
            node->value->set<int>(std::stoi(ctx.node->tokens[0]->content));
            node->value->size = 4;
            return node;
        }); 

        size_t bool_id = add_base_type("bool",1,make_var_decl_on_key);
        value_to_string.put(bool_id,[](void* data){
            return (*(bool*)data) ? "TRUE" : "FALSE";
        });
        t_functions.put(bool_id, [bool_id,literal_id](t_context& ctx)->g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = literal_id;
            node->value->type = bool_id;
            node->value->set<bool>(ctx.node->tokens[0]->content=="true" ? true : false); 
            node->value->size = 1;
            return node;
        }); 
           
        add_binary_operator('+',"ADD", 4, 5, make_arithmetic_handler<int, int>([](auto a, auto b){return a+b;},int_id));
        add_binary_operator('>',"GREATER_THAN", 4, 5, make_arithmetic_handler<int, bool>([](auto a, auto b){return a>b;},bool_id));

        size_t star_id = add_binary_operator('*',"MULTIPLY", 6, 7, make_arithmetic_handler<int, int>([](auto a, auto b){return a*b;},int_id));
        size_t pointer_id = reg::new_type("POINTER");
        size_t deref_id = reg::new_type("DEREF");
        a_functions.put(star_id, [star_id, deref_id, var_decl_id](a_context& ctx) {
            if(ctx.left && ctx.left->type == var_decl_id) {
                // pointer declaration
                ctx.node = make<a_node>();
                ctx.node->type = var_decl_id;
                ctx.node->tokens << ctx.tokens[ctx.index];
                ctx.node->sub_nodes << ctx.left;
                ctx.index++;
            } else if(ctx.left && ctx.left->type != 0) {
                // multiply — already have left, just need right
                g_ptr<a_node> right = a_parse_expression(ctx, 7, nullptr);
                ctx.node = make<a_node>();
                ctx.node->type = star_id;
                ctx.node->sub_nodes << ctx.left;
                ctx.node->sub_nodes << right;
            } else {
                // dereference — prefix, no left
                g_ptr<a_node> right = a_parse_expression(ctx, 8, nullptr);
                ctx.node = make<a_node>();
                ctx.node->type = deref_id;
                ctx.node->sub_nodes << right;
            }
        });


        size_t amp_id = add_token('&',"AMPERSAND");
        right_binding_power.put(amp_id, 8);
        a_functions.put(amp_id, [amp_id](a_context& ctx) {
            // always prefix, ctx.left should always be null
            // right side is whatever a_parse_expression returns
            g_ptr<a_node> right = a_parse_expression(ctx, 8, nullptr);
            ctx.node = make<a_node>();
            ctx.node->type = amp_id;
            ctx.node->sub_nodes << right;
        });


        t_functions.put(amp_id, [amp_id, identifier_id](t_context& ctx) -> g_ptr<t_node> {
            //Workaround because t_defualt_function and precedence needs more work
            g_ptr<t_node> result = make<t_node>();
            result->type = amp_id;
            g_ptr<a_node> stub = make<a_node>();
            stub->tokens << ctx.node->tokens[0];
            stub->type = identifier_id;
            t_context sub_ctx(result, stub, ctx.root);
            result->left = t_functions.get(identifier_id)(sub_ctx);
            return result;
        });
        size_t addr_of_id = reg::new_type("ADDR_OF");
        r_handlers.put(amp_id, [addr_of_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = addr_of_id;
            result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
        });
        exec_handlers.put(addr_of_id, [pointer_id](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
            ctx.node->value->type = pointer_id;
            ctx.node->value->size = 8;
            ctx.node->value->set<void*>(ctx.node->left->value->data);
            return ctx.node;
        });


        // size_t star_id = add_token('*',"STAR");
        // a_functions.put(star_id, [star_id, var_decl_id](a_context& ctx) {
        //     if(ctx.state == var_decl_id && ctx.node->tokens.size() == 1) {
        //         ctx.state = star_id;
        //         return;
        //     }
        //     a_default_function(ctx);
        // });
        // state_is_opp.put(star_id, true);
        // type_precdence.put(star_id, 3);
        // t_functions.put(star_id, [star_id, var_decl_id, identifier_id](t_context& ctx) -> g_ptr<t_node> {
        //     //Workaround because t_defualt_function and precedence needs more work
        //     if(ctx.node->tokens.size() == 1 && ctx.node->sub_nodes.empty()) {
        //         //*c — unary dereference
        //         g_ptr<t_node> result = make<t_node>();
        //         result->type = star_id;
        //         g_ptr<a_node> stub = make<a_node>();
        //         stub->tokens << ctx.node->tokens[0];
        //         stub->type = identifier_id;
        //         t_context sub_ctx(result, stub, ctx.root);
        //         result->left = t_functions.get(identifier_id)(sub_ctx);
        //         return result;
        //     }
            
        //     return t_default_function(ctx);
        // });
        // r_handlers.put(star_id, [star_id, var_decl_id, pointer_id, deref_id, mul_id](g_ptr<r_node> result, r_context& ctx) {
        //     if(ctx.node->left && ctx.node->left->type == var_decl_id) {
        //         //num* c — pointer declaration, just grab what discover already set up
        //         result->type = pointer_id;
        //         result->name = ctx.node->right->name;
        //         result->left = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        //         result->value = result->left->value;
        //     }
        //     else if(ctx.node->right == nullptr) {
        //         //*c — dereference
        //         result->type = deref_id;
        //         result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
        //     }
        //     else {
        //         //a * b — multiplication
        //         result->type = mul_id;
        //         result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
        //         result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        //     }
        // });
        // exec_handlers.put(mul_id, make_arithmetic_handler<int,int>([](auto a, auto b){return a*b;}, int_id));
        // exec_handlers.put(pointer_id, [](exec_context& ctx) -> g_ptr<r_node> {
        //     ctx.frame->active_memory << ctx.node->value->data;
        //     return ctx.node;
        // });
        // exec_handlers.put(deref_id, [](exec_context& ctx) -> g_ptr<r_node> {
        //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
        //     ctx.node->value->data = *(void**)ctx.node->left->value->data;
        //     ctx.node->value->type = ctx.node->left->value->sub_type;
        //     ctx.node->value->size = ctx.node->left->value->size;
        //     return ctx.node;
        // });

       

        auto resolve_field_size = [object_id,star_id](g_ptr<t_node> field, g_ptr<s_node> root) -> size_t {
            if(field->type == star_id) return 8;
            if(field->value->type == object_id) {
                g_ptr<s_node> type_scope = find_scope_name(field->deferred_identifier, root);
                if(type_scope && type_scope->t_owner) {
                    return type_scope->t_owner->value->size;
                }
                return 0;
            }
            return field->value->size;
        };
        // discover_handlers.put(star_id, [var_decl_id,pointer_id](g_ptr<t_node> node, d_context& ctx) {
        //     if(node->left && node->left->type == var_decl_id) {
        //         g_ptr<Value> ptr_value = make<Value>(pointer_id);
        //         ptr_value->size = 8;
        //         ptr_value->data = malloc(8);
        //         ptr_value->sub_type = node->left->value->type;
        //         node->left->scope->values.put(node->right->name, ptr_value);
        //     }
        // });
        discover_handlers.put(var_decl_id, [object_id,resolve_field_size](g_ptr<t_node> node, d_context& ctx) {
            g_ptr<Value> sub_value = make<Value>(0);
            sub_value->type = node->value->sub_type;
            sub_value->size = resolve_field_size(node,ctx.root);
            sub_value->data = malloc(sub_value->size);
            node->scope->values.put(node->name,sub_value);
        });
        discover_handlers.put(type_decl_id, [var_decl_id,object_id,resolve_field_size](g_ptr<t_node> node, d_context& ctx) {
            if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
            node->scope->name = node->name;
            node->value->size = 0;
            
            for(auto field : node->scope->t_nodes) {
                if(field->type == var_decl_id) {
                    g_ptr<Value> field_value = make<Value>(0);
                    field_value->type = field->value->sub_type;
                    field_value->size = resolve_field_size(field, ctx.root);
                    field_value->address = node->value->size;
                    node->value->size += field_value->size;
                    node->scope->values.put(field->name, field_value);
                }
            }       
        });

        size_t prop_access_id = add_binary_operator('.', "PROP_ACCESS", 8, 9, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
            execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
            ctx.node->value->type = ctx.node->right->value->type;
            ctx.node->value->size = ctx.node->right->value->size;
            ctx.node->value->data = (char*)ctx.node->left->value->data + ctx.node->right->value->address;
            return ctx.node;
        });
        t_functions.set(prop_access_id, [prop_access_id, identifier_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> result = t_default_function(ctx);
            //Workaround because t_defualt_function and precedence needs more work
            if(ctx.node->tokens.size()==1 && ctx.node->sub_nodes.size()==1) {
                std::swap(result->left, result->right);
            }
            return result;
        });

        // size_t index_id = add_binary_operator('[', "INDEX", 8, 9, [](exec_context& ctx) -> g_ptr<r_node> {
        //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
        //     execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
        //     int i = ctx.node->right->value->get<int>();
        //     size_t stride = ctx.node->left->value->size;
        //     ctx.node->value->data = (char*)ctx.node->left->value->data + i * stride;
        //     ctx.node->value->type = ctx.node->left->value->sub_type;
        //     ctx.node->value->size = stride;
        //     return ctx.node;
        // });
        // t_functions.put(index_id, [index_id, identifier_id, int_id, literal_id](t_context& ctx) -> g_ptr<t_node> {
        //     ctx.result->type = index_id;
        //     // left is the array identifier
        //     g_ptr<a_node> left_stub = make<a_node>();
        //     left_stub->tokens << ctx.node->tokens[0];
        //     left_stub->type = identifier_id;
        //     ctx.result->left = parse_a_node(left_stub, ctx.root, nullptr);
        //     // right is the index literal
        //     g_ptr<a_node> right_stub = make<a_node>();
        //     right_stub->tokens << ctx.node->tokens[1];
        //     right_stub->type = int_id;
        //     ctx.result->right = parse_a_node(right_stub, ctx.root, nullptr);
        //     return ctx.result;
        // });
        // size_t rbracket_id = add_token(']',"RBRACKET");
        // a_functions.put(rbracket_id, [](a_context& ctx) {
        //     if(ctx.local && ctx.state != 0) {
        //         ctx.end_lambda();
        //         ctx.state = 0;
        //     } 
        // });
        // add_function("malloc", [pointer_id](exec_context& ctx) -> g_ptr<r_node> {
        //     execute_r_node(ctx.node->children[0], ctx.frame, ctx.index, ctx.sub_index);
        //     int size = ctx.node->children[0]->value->get<int>();
        //     ctx.node->value->set<void*>(malloc(size));
        //     ctx.node->value->type = pointer_id;
        //     ctx.node->value->size = 8;
        //     return ctx.node;
        // });

        size_t end_id = add_token(';',"END");
        a_functions.put(end_id, [end_id](a_context& ctx) {
            // if(ctx.state != 0) {
            //     ctx.end_lambda();
            // } 
            // ctx.state = end_id;
            // ctx.end_lambda();
            // ctx.state = 0;
            // ctx.pos = 0;
        });
        t_functions.put(end_id, [](t_context& ctx) -> g_ptr<t_node> {
            return nullptr; //Do nothing
        });  

        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        reg::new_type("ENTER_PAREN");
        left_binding_power.put(lparen_id, 10);
        a_functions.put(lparen_id, [lparen_id, rparen_id, comma_id, identifier_id](a_context& ctx) {
            g_ptr<a_node> inner = a_parse_expression(ctx, 0, nullptr);
            if(ctx.left && ctx.left->type != 0) {
                ctx.left->sub_nodes << inner;
                while(ctx.tokens[ctx.index]->getType() == comma_id) {
                    ctx.index++; // consume ,
                    g_ptr<a_node> arg = a_parse_expression(ctx, 0, nullptr);
                    if(arg) ctx.left->sub_nodes << arg;
                }
                ctx.index++; // consume )
                ctx.node = ctx.left;
            } else {
                ctx.index++; // consume )
                ctx.node = inner;
            }
        });
        reg::new_type("EXIT_PAREN"); 
        a_functions.put(rparen_id, [](a_context& ctx) {
            //Do nothing and let default bail
        });

        // a_functions.put(comma_id, [comma_id](a_context& ctx) {
        //     if(ctx.local && ctx.state != 0) {
        //         ctx.end_lambda();
        //         ctx.state = comma_id;
        //         ctx.end_lambda();
        //         ctx.state = 0;
        //     }
        //     ctx.pos = 0;
        // });
        // t_functions.put(comma_id, [](t_context& ctx) -> g_ptr<t_node> {
        //     return nullptr; //Do nothing
        // });  

        // size_t lbrace_id = add_token('{',"LBRACE");
        // size_t enter_scope_id = reg::new_type("ENTER_SCOPE"); 
        // a_functions.put(lbrace_id, [enter_scope_id](a_context& ctx) {
        //     if(ctx.state != 0) {
        //         ctx.end_lambda();
        //     }
        //     ctx.state = enter_scope_id;
        //     ctx.end_lambda();
        //     ctx.state = 0;
        //     ctx.pos = 0;
        // });
        // scope_precedence.put(enter_scope_id, 10); 
        // size_t rbrace_id = add_token('}',"RBRACE");
        // size_t exit_scope_id = reg::new_type("EXIT_SCOPE"); 
        // a_functions.put(rbrace_id, [exit_scope_id](a_context& ctx) {
        //     if(ctx.state != 0) {
        //         ctx.end_lambda();
        //     }
        //     ctx.state = exit_scope_id;
        //     ctx.end_lambda();
        //     ctx.state = 0;
        //     ctx.pos = 0;
        // });
        // scope_precedence.put(exit_scope_id, -10);

        char_is_split.put(' ',true);
        size_t in_alpha_id = reg::new_type("IN_ALPHA");
        tokenizer_state_functions.put(in_alpha_id,[](tokenizer_context& ctx) {
            char c = *(ctx.it);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.it;
                g_ptr<Value> value = keywords.getOrDefault(ctx.token->content,fallback_value);
                if(value) {
                    ctx.token->type = value->type;
                    ctx.token->value = value;
                    
                }
                return;
            } else {
                ctx.token->add(c);
            }
        });

        size_t in_digit_id = reg::new_type("IN_DIGIT");
        tokenizer_state_functions.put(in_digit_id,[in_alpha_id,float_id](tokenizer_context& ctx) {
            char c = *(ctx.it);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.it;
                return;
            } else if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
            } else if(c=='.') {
                ctx.token->type = float_id;
            }
            ctx.token->add(c);
        });

        token_handler default_function = [in_alpha_id,in_digit_id,identifier_id,int_id](tokenizer_context& ctx) {
            char c = *(ctx.it);
            if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
                ctx.token = make<Token>(identifier_id,c);
                ctx.result << ctx.token;
            }
            else if(std::isdigit(c)) {
                ctx.state = in_digit_id;
                ctx.token = make<Token>(int_id,c);
                ctx.result << ctx.token;
            } else if(c==' '||c=='\t'||c=='\n') {
                //just skip
            }
            else {
                print("tokenize::default_function missing handling for char: ",c);
            }
        };
        tokenizer_default_function = default_function;

        // size_t string_id = reg::new_type("STRING");
        // size_t string_key_id = make_keyword("string",24,"STRING_KEY",string_id);
        // a_functions.put(string_key_id, [var_decl_id](a_context& ctx) {
        //     if(ctx.state == 0) ctx.state = var_decl_id;
        //     ctx.node->tokens << ctx.token;
        // });
        size_t string_id = add_base_type("string",24,make_var_decl_on_key);
        size_t in_string_id = reg::new_type("IN_STRING_KEY");
        tokenizer_state_functions.put(in_string_id,[](tokenizer_context& ctx) {
            char c = *(ctx.it);
            if(c=='"') {
                ctx.state=0;
            }
            else {
                ctx.token->add(c);
            }
        });
        tokenizer_functions.put('"',[string_id,in_string_id](tokenizer_context& ctx) {
            ctx.state = in_string_id;
            ctx.token = make<Token>(string_id,"");
            ctx.result << ctx.token;
        });
        value_to_string.put(string_id,[](void* data){
            return *(std::string*)data;
        });

        t_default_function = [](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<a_node> node = ctx.node;
            g_ptr<t_node> left = ctx.left;

            g_ptr<t_node> result = make<t_node>();
            result->type = node->type;

            // auto handle_literal = [&result, &ctx](g_ptr<Token> token) -> g_ptr<t_node> {
            //     g_ptr<a_node> stub = make<a_node>();
            //     stub->tokens << token;
            //     t_context sub_ctx(result, stub, ctx.root);
            //     if(!t_functions.hasKey(token->getType())) {
            //         print("t_default_function: no t_function for token type ", TO_STRING(token->getType()));
            //         return nullptr;
            //     }
            //     return t_functions.get(token->getType())(sub_ctx);
            // };

            // auto recurse = [&ctx](g_ptr<a_node> node, g_ptr<t_node> left) -> g_ptr<t_node> {
            //     g_ptr<t_node> sub_result = make<t_node>();
            //     t_context sub_ctx(sub_result,node,ctx.root);
            //     sub_ctx.left = left;
            //     return t_default_function(sub_ctx);
            // };
            
            // if (node->tokens.size() == 2) {
            //     result->left = handle_literal(node->tokens[0]);
            //     result->right = handle_literal(node->tokens[1]);
            // } 
            // else if (node->tokens.size() == 1) {
            //     if(node->sub_nodes.size()==0) {
            //         if(left) {
            //             result->left = left;
            //             result->right = handle_literal(node->tokens[0]);
            //             if(node->in_scope) { //Removes left refrence by taking it's place
            //                 if(node->in_scope->t_nodes.last()==left) {
            //                     node->in_scope->t_nodes.pop();
            //                 }
            //             }
            //         }
            //         else {
            //            if(state_is_opp.getOrDefault(node->type,false)&&t_functions.hasKey(node->type)) { //For opperators like i* or i++
            //                 t_context sub_ctx(result,node,nullptr);
            //                 ctx.left = left;
            //                 result = t_functions.get(node->type)(sub_ctx);
            //             } 
            //             else {
            //                 result = handle_literal(node->tokens[0]);
            //             }
            //         }
            //     }
            //     else if(node->sub_nodes.size()==1) {
            //         result->left = handle_literal(node->tokens[0]);
            //         // result->right = recurse(node->sub_nodes[0],nullptr); //To prevent recursion in unary opperators
            //         // //Was passing result as left, so something else may be broken by this
            //         result->right = parse_a_node(node->sub_nodes[0], ctx.root, nullptr);
            //     }
            //     else if(node->sub_nodes.size()>=2) {
            //         result->left = handle_literal(node->tokens[0]);
            //         g_ptr<t_node> sub = nullptr;
            //         for(auto a : node->sub_nodes) {
            //            sub = recurse(a,sub);
            //             if(a==node->sub_nodes.last()) {
            //                 result->right = sub;
            //             }
            //         }
            //     }
            // }
            // else if(node->tokens.size()==0) {
            //     if(node->sub_nodes.size()==0) {
            //         result->right = nullptr;
            //         result->left = nullptr;
            //     }
            //     else if(node->sub_nodes.size()==1) {
            //         result->left = left;
            //         result->right = recurse(node->sub_nodes[0],nullptr); //By passing nullptr we stop the recursion
            //         if(left&&node->in_scope) { //This removes duplicate left refrences, such as with var_decl + assignment
            //             if(node->in_scope->t_nodes.last()==left) {
            //                 node->in_scope->t_nodes.pop(); //Used to be set last to result, return nullptr
            //             }
            //         } 
            //     }
            //     else if(node->sub_nodes.size()>=2) {
            //         result->left = left;
            //         g_ptr<t_node> sub = nullptr;
            //         for(auto a : node->sub_nodes) {
            //            sub = recurse(a,sub);
            //             if(a==node->sub_nodes.last()) {
            //                 result->right = sub;
            //             }
            //         }
            //         if(left&&node->in_scope) {
            //             if(node->in_scope->t_nodes.last()==left) {
            //                 node->in_scope->t_nodes.pop();
            //             }
            //         }
            //     }
            // }
        
            return result;
        };

        add_function("print",[](exec_context& ctx) -> g_ptr<r_node> {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                execute_r_node(r, ctx.frame, ctx.index,ctx.sub_index);
                toPrint.append(r->value->to_string());
            }
            print(toPrint);
            return ctx.node;
        });

        add_function("layout", [int_id](exec_context& ctx) -> g_ptr<r_node> {
            int mode = 1;
            if(!ctx.node->children.empty()) {
                execute_r_node(ctx.node->children[0],ctx.frame,ctx.index,ctx.sub_index);
                if(ctx.node->children.length()>1) {
                    execute_r_node(ctx.node->children[1],ctx.frame,ctx.index,ctx.sub_index);
                    mode = ctx.node->children[1]->value->get<int>();
                    print(ctx.node->children[0]->in_frame->context->type_to_string(mode));
                }
                else {
                    if(ctx.node->children[0]->value->type==int_id) {
                        mode = ctx.node->children[0]->value->get<int>();
                        print(ctx.frame->context->type_to_string(mode));
                    } else {
                        print(ctx.node->children[0]->in_frame->context->type_to_string(mode));
                    }
                }
            }
            else
                print(ctx.frame->context->type_to_string(mode));
            return ctx.node;
        });

        add_scoped_keyword("if", 2, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
            if(ctx.node->right->value->is_true()) {
                execute_sub_frame(ctx.node->frame, ctx.frame);
            }
            else if(ctx.node->left) {
                execute_sub_frame(ctx.node->left->frame, ctx.frame);
            }
            return ctx.node;
        });

        std::string code = readFile(path);
        list<g_ptr<Token>> tokens = tokenize(code);
        list<g_ptr<a_node>> nodes = parse_tokens(tokens);
        //balance_precedence(nodes);
        g_ptr<s_node> root = parse_scope(nodes);
        parse_nodes(root);
        print("==DISCOVERING SYMBOLS==");
        discover_symbols(root);
        g_ptr<Frame> frame = resolve_symbols(root);
        print("==EXECUTING==");
        execute_r_nodes(frame);
    }
}
using namespace GDSL;

int main() {

    // GDSL::helper_test_module::initialize();
    test_module(root()+"/Projects/Testing/src/test.gld");
    // g_ptr<GDSL::Frame> frame = compile(root()+"/Projects/Testing/src/test.gld");
    // execute_r_nodes(frame);


    // std::vector<bool> b_vec;
    // list<bool> b_list;
    // Log::rig r;
    // r.add_process("clean",[&](int i){
    //     if(i==0) {
    //         b_vec.clear();
    //         b_list.clear();
    //     }
    // });
    // r.add_process("pop_vec",[&](int i){
    //     b_vec.push_back(false);
    // });
    // r.add_process("pop_list",[&](int i){
    //     b_list.push(false);
    // });
    // r.add_process("access_vec",[&](int i){
    //     volatile bool a = b_vec[i];
    // });
    // r.add_process("access_list",[&](int i){
    //     volatile bool a = b_list[i];
    // });

    // //


    // r.add_comparison("pop_vec","pop_list");
    // r.add_comparison("access_vec","access_list");
    // r.run(1000,true,100);

 
    // g_ptr<Type> t = make<Type>();
    // g_ptr<Object> obj = make<Object>();
    // int ITS = 100;
    // int MAX_RANGE = 4;
    // int STRAT = 0;
    // list<std::string> titles;
    // list<int> seq;
    // //std::pow(ITS,5)
    // for(int i=0;i<ITS;i++) {
    //     titles << std::to_string(i);
    //     seq << randi(1,MAX_RANGE);
    // }

    // map<std::string,int> g_map;
    // std::unordered_map<std::string,int> std_map;

    // Log::rig r;
    // r.add_process("clean_maps",[&](int i){
    //     if(i==0) {
    //         g_map.clear();
    //         std_map.clear();
    //     }
    // },1);
    // r.add_process("populate_g_map",[&](int i){
    //     if(STRAT==0||STRAT==1||STRAT==2||STRAT==3) { 
    //         g_map.put(titles[i],i);
    //     } else {
    //         //No strat
    //     }
    // },1);
    // r.add_process("populate_std_map",[&](int i){
    //     if(STRAT==0||STRAT==1||STRAT==2||STRAT==3) { 
    //         std_map.emplace(titles[i],i);
    //     } else {
    //         //No strat
    //     }
    // },1);
    // r.add_process("access_g_map",[&](int i){
    //     if(STRAT==0||STRAT==1||STRAT==2||STRAT==3) { 
    //         volatile int a = g_map.get(titles[i]);
    //     } else {
    //         //No strat
    //     }
    // },1);
    // r.add_process("access_std_map",[&](int i){
    //     if(STRAT==0||STRAT==1||STRAT==2||STRAT==3) { 
    //         volatile int a = std_map.at(titles[i]);
    //     } else {
    //         //No strat
    //     }
    // },1);

    // r.add_process("clean",[&](int i){
    //     if(i==0) {
    //         t = make<Type>();
    //         obj = make<Object>();
    //     }
    // });
    // r.add_process("populate_type",[&](int i){
    //     if(STRAT==0) { //DATA strategy
    //         t->add<int>(titles[i],i);
    //     } else if(STRAT==1) { //ARRAY strategy
    //         t->push<int>(i);
    //     } else if (STRAT==2) { //VARIED strategy
    //         switch(seq[i]) {
    //             case 1: t->add<float>(titles[i],randf(0,100.0f)); break;
    //             case 2: t->add<std::string>(titles[i],titles[i]); break;
    //             case 3: t->add<int>(titles[i],i); break;
    //             case 4: t->add<bool>(titles[i],i%2); break;
    //             default: t->add<int>(titles[i],i); break;
    //         }
    //     } else if (STRAT==3) { //Using the fallback
    //         t->add<std::string>(titles[i],titles[i]);
    //     }
    // });
    // r.add_process("populate_data",[&](int i){
    //     if(STRAT==0||STRAT==1) { //DATA and ARRAY strategy
    //         obj->add<int>(titles[i],i);
    //     } else if (STRAT==2) { //VARIED strategy
    //         switch(seq[i]) {
    //             case 1: obj->add<float>(titles[i],randf(0,100.0f)); break;
    //             case 2: obj->add<std::string>(titles[i],titles[i]); break;
    //             case 3: obj->add<int>(titles[i],i); break;
    //             case 4: obj->add<bool>(titles[i],i%2); break;
    //             default: obj->add<int>(titles[i],i); break;
    //         }
    //     } else if (STRAT==3) { //Using the fallback
    //         obj->add<std::string>(titles[i],titles[i]);
    //     }
    // });
    // r.add_process("access_type",[&](int i){
    //     if(STRAT==0) { //DATA strategy
    //         volatile int a = t->get<int>(titles[i]);
    //     } else if(STRAT==1) { //ARRAY strategy
    //         volatile int a = t->get<int>(i);
    //     } else if (STRAT==2) { //VARIED strategy
    //         switch(seq[i]) {
    //             case 1: { volatile float f = t->get<float>(titles[i]); } break;
    //             case 2: { volatile std::string s = t->get<std::string>(titles[i]); } break;
    //             case 3: { volatile int a = t->get<int>(titles[i]); } break;
    //             case 4: { volatile bool b = t->get<bool>(titles[i]); } break;
    //             default: { volatile int a = t->get<int>(titles[i]); } break;
    //         }
    //     } else if (STRAT==3) { //Using the fallback
    //         volatile std::string s = t->get<std::string>(titles[i]);
    //     }
    // });
    // r.add_process("access_data",[&](int i){
    //     if(STRAT==0||STRAT==1) { //DATA and ARRAY strategy
    //         volatile int a = obj->get<int>(titles[i]);
    //     } else if (STRAT==2) { //VARIED strategy
    //         switch(seq[i]) {
    //             case 1: { volatile float f = obj->get<float>(titles[i]); } break;
    //             case 2: { volatile std::string s = obj->get<std::string>(titles[i]); } break;
    //             case 3: { volatile int a = obj->get<int>(titles[i]); } break;
    //             case 4: { volatile bool b = obj->get<bool>(titles[i]); } break;
    //             default: { volatile int a = obj->get<int>(titles[i]); } break;
    //         }
    //     } else if (STRAT==3) { //Using the fallback
    //         volatile std::string s = obj->get<std::string>(titles[i]);
    //     }
    // });
    // r.add_comparison("populate_type","populate_data");
    // r.add_comparison("access_type","access_data");
    // r.add_comparison("populate_g_map","populate_std_map");
    // r.add_comparison("access_g_map","access_std_map");
    // r.run(1000,true,ITS);


    // for(int i=0;i<5;i++) {
    //     print("ITS: ",(int)std::pow(ITS,i+1));
    //     r.run(1000,false,(int)std::pow(ITS,i+1));
    // }

    // print(obj->data.notes.size());
    // if(STRAT!=1) {
    //     print(t->notes.size());
    // } else {
    //     print(t->row_length(0,4));
    // }

    // int count = 0;
    // map<int,std::string> correct_retrival;
    // for(auto e : t->notes.entrySet()) {
    //     if(e.value.size==24) {
    //         count++;
    //         correct_retrival.put(e.value.sub_index,e.key);
    //     }
    // }
    // print("T contains ",count," strings");
    // for(auto e : correct_retrival.entrySet()) {
    //     print("T at: ",e.key," should be ",e.value);
    //     print(" T at ",e.key," is ",t->get<std::string>(e.value));
    //     print(" ARRAY: T at ",e.key," is ",t->get<std::string>(0,e.key));
    // }

    print("==DONE==");

    // print("Naive sweep");
    // int x = -1000;
    // int y = -1000;
    // for(int a=0;a<2000;a++) {
    //     for(int b=0;b<2000;b++) {
    //         if((x+y==5)&&(x*y==4)) {
    //             print("SUCCESS! X: ",x," Y: ",y);
    //             return 0;
    //         }
    //         y+=1;
    //     }
    //     x+=1;
    //     y = -1000;
    // }
    // print("No valid combinations");

    return 0;
}



//Database like runtime model
    // size_t var_decl_id = reg::new_type("VAR_DECL");
    // size_t object_id = reg::new_type("OBJECT");

    // size_t identifier_id = reg::new_type("IDENTIFIER"); 
    // a_functions.put(identifier_id,[identifier_id](a_context& ctx){
    //     if(ctx.state==0) ctx.state = identifier_id;
    //     ctx.node->tokens << ctx.token;
    // });
    // t_functions.put(identifier_id, [identifier_id,var_decl_id,type_decl_id,object_id](t_context& ctx) -> g_ptr<t_node> {
    //     g_ptr<t_node> node = make<t_node>();
    //     node->name = ctx.node->tokens[0]->content;
    //     node->type = identifier_id;
    //     g_ptr<s_node> type_decl_scope = find_scope(ctx.root,[node,type_decl_id](g_ptr<s_node> scope){
    //         if(!scope->type_ref) 
    //             return false;
    //         return scope->type_ref->type_name == node->name;
    //     });
    //     if(type_decl_scope) {
    //         node->type = var_decl_id;
    //         g_ptr<Value> sub_value = make<Value>(0);
    //         sub_value->type = object_id;
    //         sub_value->sub_type = object_id;
    //         node->value = sub_value;
    //         node->name = ctx.node->tokens[1]->content;
    //         node->scope = type_decl_scope.getPtr();
    //     }
    //     return node;
    // });       
    // r_handlers.put(identifier_id, [identifier_id](g_ptr<r_node> result, r_context& ctx) {
    //     result->type = identifier_id;
    //     result->name = ctx.node->name;
    //     g_ptr<s_node> scope = find_scope(ctx.scope,[&](g_ptr<s_node> scope_node){
    //         return scope_node->has(ctx.node->name+"_value");
    //     });
    //     if(scope) {
    //         result->value = scope->get<g_ptr<Value>>(ctx.node->name+"_value");
    //         if(scope->type_ref->has(ctx.node->name)) {
    //             _note& note = scope->type_ref->get_note(ctx.node->name);
    //             result->index = note.sub_index;
    //             result->value->address = note.index;
    //             result->value->size = note.size;
    //         } 
    //         if(scope->has(ctx.node->name+"_TID")) {
    //             result->value->address = 0; //This is dictated by prop acess somehow
    //             result->index = scope->get<int>(ctx.node->name+"_TID");
    //         }
    //         result->in_frame = scope->frame;
    //     } 
    // });
    // exec_handlers.put(identifier_id, [](exec_context& ctx) -> g_ptr<r_node> {
    //     ctx.node->value->data = ctx.node->in_frame->context->get(ctx.node->value->address,ctx.node->index,ctx.node->value->size);
    //     return ctx.node;
    // });

    // state_is_opp.put(var_decl_id,true);
    // type_precdence.put(var_decl_id,1);
    // t_functions.put(var_decl_id, [var_decl_id](t_context& ctx) -> g_ptr<t_node> {
    //     ctx.result->type = var_decl_id;
    //     ctx.result->value = ctx.node->tokens[0]->value;
    //     ctx.result->name = ctx.node->tokens[1]->content;
    //     ctx.result->scope = ctx.root.getPtr();
    //     return ctx.result;
    // });
    // discover_handlers.put(var_decl_id, [object_id](g_ptr<t_node> node, d_context& ctx) {
    //     g_ptr<Value> sub_value = make<Value>(0);
    //     sub_value->type = node->value->sub_type;
    //     node->scope->add<g_ptr<Value>>(node->name+"_value", sub_value);
    //     if(node->value->type!=object_id) {
    //         node->scope->type_ref->note_value(node->name,node->value->size);
    //     } else {
    //         g_ptr<Object> obj = node->scope->type_ref->create(); //Make a new object
    //         node->scope->add<int>(node->name+"_TID",obj->TID);
    //     }
    // });
    // r_handlers.put(var_decl_id, [var_decl_id](g_ptr<r_node> result, r_context& ctx) {
    //     result->type = var_decl_id;
    //     _note& note = ctx.scope->type_ref->get_note(ctx.node->name);
    //     result->index = note.sub_index;
    //     result->value->address = note.index;
    //     result->value->type = ctx.node->value->type;
    //     result->value->size = note.size;
    //     result->name = ctx.node->name;
    // });
    // exec_handlers.put(var_decl_id, [](exec_context& ctx) -> g_ptr<r_node> {
    //     //Do nothing for now.
    //     return ctx.node;
    // });

    // char_is_split.put('.',true);
    // add_binary_operator('.', "PROP_ACCESS", 8, [](exec_context& ctx) -> g_ptr<r_node> {
    //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
    //     ctx.node->value->type = ctx.node->right->value->type;
    //     ctx.node->value->address = ctx.node->right->value->address;
    //     ctx.node->index = ctx.node->left->index;
    //     ctx.node->value->size = ctx.node->right->value->size;
    //     ctx.node->in_frame = ctx.node->left->in_frame;
    //     // print("L = ",ctx.node->left->name," ADDR: ",ctx.node->left->value->address," IDX: ",ctx.node->left->index);
    //     // print("R = ",ctx.node->right->name," ADDR: ",ctx.node->right->value->address," IDX: ",ctx.node->right->index);
    //     ctx.node->value->data = ctx.node->left->in_frame->context->get(
    //         ctx.node->value->address,  ctx.node->index, ctx.node->value->size);
    //     return ctx.node;
    // });


    // size_t assignment_id = reg::new_type("ASSIGNMENT");
    // char_is_split.put('=',true);
    // tokenizer_functions.put('=',[assignment_id](tokenizer_context& ctx) {
    //     ctx.token = make<Token>(assignment_id,"=");
    //     ctx.result << ctx.token;
    // });
    // state_is_opp.put(assignment_id,true);
    // type_precdence.put(assignment_id,1); 
    // r_handlers.put(assignment_id, [assignment_id](g_ptr<r_node> result, r_context& ctx) {
    //     result->type = assignment_id;
    //     result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
    //     result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
    // });
    // exec_handlers.put(assignment_id, [](exec_context& ctx) -> g_ptr<r_node> {
    //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
    //     execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
    //     size_t target_index = ctx.node->left->index==-1?ctx.index:ctx.node->left->index;
    //     ctx.node->left->in_frame->context->set(
    //         ctx.node->left->value->address,
    //         target_index,
    //         ctx.node->left->value->size,
    //         ctx.node->right->value->data);
    //     return ctx.node;
    // });

//Old test module for GDSL:

    // GDSL::add_keyword("say",[](GDSL::exec_context& ctx){
    //     ctx.frame->replaceScript("talk",[](ScriptContext& scx){
    //         std::string content = scx.get<std::string>("content");
    //         print(content);
    //     });
    //     return ctx.node;
    // });

    // GDSL::add_keyword("shout",[](GDSL::exec_context& ctx){
    //     ctx.frame->replaceScript("talk",[](ScriptContext& scx){
    //         std::string content = scx.get<std::string>("content");
    //         std::transform(content.begin(), content.end(), content.begin(), ::toupper);
    //         print(content);
    //     });
    //     return ctx.node;
    // });

    // GDSL::add_binary_operator(':',"COLON","PARSER",1,[](GDSL::exec_context& ctx){
    //     GDSL::execute_r_node(ctx.node->left,ctx.frame,ctx.index,ctx.sub_index);
    //     GDSL::execute_r_node(ctx.node->right,ctx.frame,ctx.index,ctx.sub_index);
    //     return ctx.node;
    // });

        //reg::new_type("UNDEFINED");
        // reg::new_type("END"); 
        // reg::new_type("T_NOP"); 
        // reg::new_type("UNTYPED"); 
        // reg::new_type("R_NOP");
        // size_t f_type_key_id = reg::new_type("F_TYPE_KEY");//Honestly not sure about these family types
        // size_t f_keyword_id = reg::new_type("F_KEYWORD"); //Keeping them in because it may be useful later
        // reg::new_type("GLOBAL");
        // reg::new_type("BLOCK");
        // reg::new_type("T_BLOCK");  
        // reg::new_type("NOT"); 

        // size_t literal_id = reg::new_type("LITERAL");
        // auto literal_handler = [literal_id](a_context& ctx) {
        //     if(ctx.state == 0) {
        //         ctx.state = literal_id;
        //     }
        //     ctx.node->tokens << ctx.token;
        // };
        // a_functions.put(literal_id, literal_handler);
        // t_functions.put(literal_id, t_literal_handler);
        // size_t t_literal_id = reg::new_type("T_LITERAL");
        // t_literal_handlers.put(literal_id, [t_literal_id](g_ptr<Token> token) -> g_ptr<t_node> {
        //     g_ptr<t_node> node = make<t_node>();
        //     node->type = t_literal_id;
        //     node->value.type = 0;
        //     node->name = token->content;
        //     return node;
        // });
        // size_t r_literal_id = reg::new_type("R_LITERAL");
        // r_handlers.put(t_literal_id, [r_literal_id](g_ptr<r_node> result, r_context& ctx) {
        //     result->type = r_literal_id;
        //     result->name = ctx.node->name;
        //     result->value = std::move(ctx.node->value);
        // });
        // exec_handlers.put(r_literal_id, [](exec_context& ctx) -> g_ptr<r_node> {
        //     ScriptContext scx;
        //     scx.set<std::string>("content",ctx.node->name);
        //     ctx.frame->run("talk",scx);
        //     return ctx.node;
        // });

        // size_t in_command_id = reg::new_type("IN_COMMAND");
        // tokenizer_state_functions.put(in_command_id,[](tokenizer_context& ctx){
        //     char c = *(ctx.it);
        //     if(c==':'||c=='('||c==')') {
        //         ctx.state=0;
        //         if(t_keys.hasKey(ctx.token->content)) {
        //             ctx.token->type_info = t_keys.get(ctx.token->content);
        //         }
        //         --ctx.it;
        //     }
        //     else if(c==' '||c=='\t'||c=='\n') {
        //         if(t_keys.hasKey(ctx.token->content)) {
        //             ctx.token->type_info = t_keys.get(ctx.token->content);
        //         }
        //         ctx.state=0;
        //     }
        //     else {
        //         ctx.token->add(c);
        //     }
        // });
        // size_t in_number_id = reg::new_type("IN_NUMBER"); 
        // tokenizer_state_functions.put(in_number_id,[](tokenizer_context& ctx){
        //     char c = *(ctx.it);
        //     if(c==':'||c=='('||c==')') {
        //         ctx.state=0;
        //         --ctx.it;
        //     }
        //     else if(c==' '||c=='\t'||c=='\n') {
        //         ctx.state=0;
        //     }
        //     else {
        //         ctx.token->add(c);
        //     }
        // });

        // //add_function("COMMAND",[](exec_context& ctx){
        // //     for(auto c : ctx.node->children) {
        // //         execute_r_node(c,ctx.frame,ctx.index,ctx.sub_index);
        // //     }
        // //     return ctx.node;
        // // });

        // size_t int_id = reg::new_type("INT");
        // size_t int_key_id = reg::new_type("INT_KEY");
        // reg_t_key("int", int_key_id, 4, 0); 
        // a_functions.put(int_key_id, type_key_handler);
        // value_to_string.put(int_id,[](void* data){
        //     return std::to_string(*(int*)data);
        // });
        // negate_value.put(int_id,[](void* data){
        //     *(int*)data = -(*(int*)data);
        // });
        // a_functions.put(int_id, literal_handler);
        // type_key_to_type.put(int_key_id, int_id);
        // t_literal_handlers.put(int_id, [t_literal_id, int_id](g_ptr<Token> token) -> g_ptr<t_node> {
        //     g_ptr<t_node> node = make<t_node>();
        //     node->type = t_literal_id;
        //     node->value.type = int_id;
        //     node->value.set<int>(std::stoi(token->content));
        //     return node;
        // });

        // std::function<void(tokenizer_context& ctx)> default_function = [in_command_id,in_number_id,literal_id,int_id](tokenizer_context& ctx) {
        //     char c = *(ctx.it);
        //         if(std::isalpha(c)) {
        //             ctx.state = in_command_id;
        //             ctx.token = make<Token>(literal_id,c);
        //             ctx.result << ctx.token;
        //         }
        //         else if(std::isdigit(c)) {
        //             ctx.state = in_number_id;
        //             ctx.token = make<Token>(int_id,c);
        //             ctx.result << ctx.token;
        //         }
        // };
        // tokenizer_default_function = default_function;


        // size_t lparen_id = add_token('(',"LPAREN");
        // size_t rparen_id = add_token(')',"RPAREN");
        // a_functions.put(lparen_id, [lparen_id,rparen_id](a_context& ctx) {
        //     std::pair<int,int> paren_range = balance_tokens(ctx.tokens, lparen_id, rparen_id, ctx.index-1);
        //     if (paren_range.first < 0 || paren_range.second < 0) {
        //         print("parse_tokens::719 Unmatched parenthesis at ", ctx.index);
        //         return;
        //     }

        //     list<g_ptr<Token>> sub_list;
        //     for(int i=paren_range.first+1;i<paren_range.second;i++) {
        //         sub_list.push(ctx.tokens[i]);
        //     }
        //     ctx.node->sub_nodes = parse_tokens(sub_list,true);
        //     if(ctx.state != 0) {
        //         ctx.end_lambda();     
        //     }
        //     ctx.state = 0;        
        //     ctx.index = paren_range.second-1;
        //     ctx.it = ctx.tokens.begin() + (int)(paren_range.second);
        //     ctx.skip_inc = 1; // Can skip more if needed
        // });
        
        // reg::new_type("EXIT_PAREN"); 
        // a_functions.put(rparen_id, [](a_context& ctx) {
        //     if(ctx.local && ctx.state != 0) {
        //         ctx.end_lambda();
        //         ctx.state = 0;
        //     }
        // });

//Random tests:

    // g_ptr<Type> t = make<Type>();
    // g_ptr<Object> obj = make<Object>();

    // ivec2 test(10,5);

    // print("Ivec2 Test\n---------------");
    // print(" Inserting to Type");
    // t->add<ivec2>("1",test);
    // print(" Inserting to Data");
    // obj->add<ivec2>("1",test);
    // print("Retrival");
    // printnl(" Type test: ");
    // print(t->get<ivec2>("1").to_string());
    // printnl(" Data test: ");
    // print(obj->get<ivec2>("1").to_string());

    // list<ivec2> tl;
    // for(int i=0;i<3;i++) {
    //     tl << ivec2(i*2,i);
    // }

    // print("List Ivec2 Test\n---------------");
    // print(" Inserting to Type");
    // t->add<list<ivec2>>("2",tl);
    // print(" Inserting to Data");
    // obj->add<list<ivec2>>("2",tl);
    // print("Retrival");
    // print(" Type test: ");
    // for(auto a : t->get<list<ivec2>>("2"))
    //     print(a.to_string());
    // print(" Data test: ");
    // for(auto a : obj->get<list<ivec2>>("2"))
    //     print(a.to_string());


// double time_function(int ITERATIONS,std::function<void(int)> process) {
//     auto start = std::chrono::high_resolution_clock::now();
//     for(int i=0;i<ITERATIONS;i++) {
//         process(i);
//     }
//     auto end = std::chrono::high_resolution_clock::now();
//     auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
//     return (double)time.count();
// }


// void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<vec4> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
//     list<list<double>> t_table;

//     for(int c=0;c<f_table.length();c++) {
//         t_table.push(list<double>{});
//         for(int r=0;r<f_table[c].length();r++) {
//             t_table[c].push(0.0);
//         }
//     }

//     for(int m = 0;m<(warm_up?2:1);m++) {
//         int C_ITERATIONS = m==0?1:C_ITS;

//         for(int c=0;c<t_table.length();c++) {
//             for(int r=0;r<t_table[c].length();r++) {
//                 t_table[c][r]=0.0;
//             }
//         }

//         for(int i = 0;i<C_ITERATIONS;i++)
//         {
//             for(int c=0;c<f_table.length();c++) {
//                 for(int r=0;r<f_table[c].length();r++) {
//                     // if(r==0) print("Running: ",s_table[c][r]);
//                     double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
//                     t_table[c][r]+=time;
//                 }
//             }
//         }
//         print("-------------------------");
//         print(m==0 ? "      ==COLD==" : "       ==WARM==");
//         print("-------------------------");
//         for(int c=0;c<t_table.length();c++) {
//             for(int r=0;r<t_table[c].length();r++) {
//                 t_table[c][r]/=C_ITERATIONS;
//                 print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
//             }
//             print("-------------------------");
//         }
//         for(auto v : comps) {
//             double factor = t_table[v.x()][v.y()]/t_table[v.z()][v.w()];
//             std::string sfs;
//             double tolerance = 5.0;
//             if (std::abs(factor - 1.0) < tolerance/100.0) {
//                 sfs = "around the same as ";
//             } else if (factor > 1.0) {
//                 double percentage = (factor - 1.0) * 100.0;
//                 sfs = std::to_string(percentage) + "% slower than ";
//             } else {
//                 double percentage = (1.0/factor - 1.0) * 100.0;
//                 sfs = std::to_string(percentage) + "% faster than ";
//             }
//             print("Factor [",s_table[v.x()][v.y()],"/",s_table[v.z()][v.w()],
//             "]: ",factor," (",s_table[v.x()][v.y()]," is ",sfs,s_table[v.z()][v.w()],")");
//         }
//         print("-------------------------");

//     }
// }

// struct lt {
//     uint8_t tt[100];
//     std::string name = "DEF";
// };

    // list<std::string> names;
    // for(int i=0;i<50;i++) {
    //     names << name::randsgen({
    //         "|, ,"
    //         "Ka|Ke|Ce|Oe|Po|Pa|Lo,"
    //         "|||||||||ck|th|sh|ch|pe|en|on,"
    //         "os|os|si|sa|es|is"
    //     });
    // }
    // int lret = 0;
    // for(auto s : names) {
    //     printnl(s);
    //     if((lret+=s.length())>70) {
    //         lret = 0;
    //         print();
    //     }
    // }
    // print();



    // struct num {
    //     int value = 0;
    // };
    // num nums[50];
    // for(int i=1;i<50;i=(i+1)) {
    //     nums[i].value = (nums[i-1].value+2);
    // }
    // print(nums[20].value);

    // g_ptr<Type> tl;

    // g_ptr<Type> t = make<Type>();
    // t->type_name = "test";

    // // lt l;
    // // l.name = "test_name";
    // // t->push<lt>(l);
    // // print("Name: ",t->get<lt>(0).name,"!");

    // void* bytes = malloc(320);
    // void* target_element = (char*)bytes + (4);
    // int i = 8;
    // lt l;
    // memcpy(target_element,&i,4);
    // t->note_value("bytes",320);
    // t->note_value("other",sizeof(l));
    // t->create();

    // t->set(0,0,320,bytes);

    // l.name = "test_name";
    // t->set(1,0,sizeof(l),&l);

    // print(t->type_to_string(3));

    // print((*(lt*)t->get(1,0,sizeof(l))).name);
    // void* start = t->get(0,0,320);
    // void* element = (char*)start + (4);

    // print(start);
    // print(bytes);
    // print(*(int*)element);

    // lt l;
    // l.name = "test_name";
    // t->push<lt>(l);
    // print("Name: ",t->get<lt>(0).name,"!");
    // //for(int i=0;i<4;i++) t->add_column(4);
    // //for(int i=0;i<13;i++) t->add_rows(4);

    // t->create();
    // t->create();
    // for(int i=0;i<16;i++) {
    //     // t->add_column(4);
    //     if(i<10) t->add<int>(name::randname(),i,randi(0,3));
    //     if(i>6) t->add<int>(std::to_string(i)+"b",i,randi(0,3));
    //     if(i<3) t->add<int>(name::randname(),i,randi(0,3));
    //     if(i>10) t->add<int>(std::to_string(i)+"cd",i,randi(0,3));
    //     if(i>4&&i<8) t->push<int>(i,randi(0,3));
    //     if(i>6&&i<11) t->push<int>(i,randi(0,3));
    //     //t->add_rows(4);
    // }
    // print(t->table_to_string(4,4));



    // void* tt;
    // tt = malloc(sizeof(t));
    // *(g_ptr<Type>*)tt = t;
    // tt = &t;
    // //t->push<int>(10);
    // (*(g_ptr<Type>*)tt)->push<int>(10);
    // print(t->get<int>(0));
    // print((*(g_ptr<Type>*)tt)->get<int>(0));

    // int nums[4] = {1,2,3,4};
    // uintptr_t x = (uintptr_t)&nums[0];
    // print(*(int*)(x+(4*1)));

    // list<list<int>> table;
    // table << list<int>{} << list<int>{};
    // void* addr = &table[0];
    // for(int i=0;i<300;i++)
    //     table[0] << i;
    // print((*(list<int>*)addr).get(8));
    // for(int i=0;i<300;i++) 
    //     table << list<int>{};
    // print((*(list<int>*)addr).get(8));


  // list<list<std::function<void(int)>>> f_table;
    // list<list<std::string>> s_table;
    // list<vec4> comps;
    // int z = 0;
    // f_table << list<std::function<void(int)>>{};
    // s_table << list<std::string>{};
    // g_ptr<Type> type = make<Type>();
    // z = 0;
    // type->add_column(4);
    // s_table[z] << "push_32"; //0
    // f_table[z] << [type](int i){
    //    byte32_t t;
    //    type->push<byte32_t>(t);
    // };
    // s_table[z] << "push_64"; //1
    // f_table[z] << [type](int i){
    //    byte64_t t;
    //    type->push<byte64_t>(t);
    // };
    // s_table[z] << "push_128"; //2
    // f_table[z] << [type](int i){
    //    lt t;
    //    type->push<lt>(t);
    // };
    // s_table[z] << "get_32"; //3
    // f_table[z] << [type](int i){
    //    volatile byte32_t b = type->get<byte32_t>(0,i);
    // };
    // s_table[z] << "get_64"; //4
    // f_table[z] << [type](int i){
    //    volatile byte64_t b = type->get<byte64_t>(0,i);
    // };
    // s_table[z] << "get_128"; //5
    // f_table[z] << [type](int i){
    //     volatile lt b = type->get<lt>(0,i);
    // };
    // list<lt> ltl;
    // s_table[z] << "list:push_128"; //6
    // f_table[z] << [type,&ltl](int i){
    //     lt t;
    //     ltl.push(t);
    // };
    // s_table[z] << "list:get_128"; //7
    // f_table[z] << [type,&ltl](int i){
    //     volatile lt b = ltl.get(i);
    // };
 
    // comps << vec4(0,1 , 0,0);
    // comps << vec4(0,4 , 0,3);
    // comps << vec4(0,2 , 0,1);
    // comps << vec4(0,5 , 0,4);
    // comps << vec4(0,2 , 0,6);
    // comps << vec4(0,5 , 0,7);

    //run_rig(f_table,s_table,comps,true,200,50000);

// list<list<std::function<void(int)>>> f_table;
    // list<list<std::string>> s_table;
    // list<vec4> comps;
    // int z = 0;
    // f_table << list<std::function<void(int)>>{};
    // s_table << list<std::string>{};
    // g_ptr<Type> type = make<Type>();
    // z = 0;
    // type->add_column(4);
    // s_table[z] << "setup"; //0
    // f_table[z] << [type](int i){
    //    type->add_row(0,4);
    // };
    // void* address = &type->byte4_columns[0];
    // s_table[z] << "method-get"; //1
    // f_table[z] << [type,address](int i){
    //    volatile int a = *(int*)Type::get(address,i,4);
    // };
    // s_table[z] << "dir-get"; //2
    // f_table[z] << [type,address](int i){
    //     volatile int a = (int)(*(list<uint32_t>*)address)[i];
    // };

    // z=1;
    // f_table << list<std::function<void(int)>>{};
    // s_table << list<std::string>{};
    // list<int> list;
    // s_table[z] << "list-push"; //0
    // f_table[z] << [&list](int i){
    //    list << i;
    // };
    // s_table[z] << "list-get"; //1
    // f_table[z] << [&list](int i){
    //    volatile int a = list[i];
    // };
   
    // comps << vec4(0,0 , 1,0);
    // comps << vec4(0,1 , 0,2);
    // comps << vec4(0,1 , 1,1);
    // comps << vec4(0,2 , 1,1);

    // run_rig(f_table,s_table,comps,true,3,100);

//Pointer Lunchbox
// auto* int_list = (list<uint32_t>*)&data->byte4_columns[0];
// s_table[z] << "T(A)-d-ptr_get"; //2
// f_table[z] << [int_list](int i){
//     volatile int a = (int)(*int_list)[i];
// };

//Direct Lunchbox
// list<uint32_t>& int_list_d = data->byte4_columns[0];
// s_table[z] << "T(A)-d-dir_get"; //3
// f_table[z] << [&int_list_d](int i){
//     volatile int a = int_list_d[i];
// };

// //g_ptr<s_node> test_script = compile_script("../Projects/Testing/src/golden.gld");

    // reg_b_types();
    // init_t_keys();
    // std::string code = readFile("../Projects/Testing/src/golden.gld");
    // list<g_ptr<Token>> tokens = tokenize(code);
    // reg_a_types();
    // reg_s_types();
    // reg_t_types();
    // init_type_key_to_type();
    // a_function_blob();
    // list<g_ptr<a_node>> nodes = parse_tokens(tokens);
    // balance_precedence(nodes);
    // scope_function_blob();
    // g_ptr<s_node> root = parse_scope(nodes);
    // t_function_blob_top();
    // t_function_blob_bottom();
    // parse_nodes(root);
    // reg_r_types();
    // discover_function_blob();
    // discover_symbols(root);
    // r_function_blob();
    // g_ptr<Frame> frame = resolve_symbols(root);
    // exec_function_blob();
    // execute_r_nodes(frame);

    // // benchmark_performance();
    // // benchmark_bulk_operations();

    // // prime_test();
    // // run_test();

    // print("==DONE==");


    // g_ptr<Type> Person = make<Type>();
    // Person->add_initializer([](g_ptr<Object> object){
    //     object->set<int>("age",42);
    // }); //Explicit intilizar addition

    // (*Person)+([](g_ptr<Object> object){
    //     object->set<int>("age",42);
    // }); //Will be Person +{ * logic here * }

    // auto people = make<group>(); //Will be: group people;
    // auto joe = Person->create(); //Will be: Person joe;
    // *people << joe; //Will be: people << joe;
    // auto man = (*people)[0]; //Will be: man people[0];
    // //print(man->get<int>("age")); //Will be: print(man.age);

// struct Transpiler {
//     map<std::string,std::string> symbols;
//     map<std::string,g_ptr<group>> groups;
//     map<std::string,g_ptr<Type>> types;
//     map<std::string,g_ptr<Object>> vars;

//     void parse_declaration(const std::string& var_name) {
//         // "group players{p1, p2, p3};"
//         size_t brace_pos = var_name.find('{');
//         if (brace_pos != std::string::npos) {
//             list<std::string> init_data = split_str(var_name,',');

//         }
//     }

//     void compile_file(const std::string& path) {
//         std::string code = readFile(path);
//         discover_symbols(code);
//     }

//     void discover_symbols(const std::string& code) {
//         list<std::string> lines = split_str(code,';');
//         clean_lines(lines);
//         // for(auto l : lines) print(l);
//         int func_level = 0;
//         list<char> func_delimiters;
//         std::string func_name = "";
//         list<int> func_name_length;

//         for(int j=0;j<lines.size();j++) {
//             list<std::string> words = split_str(lines[j],' ');
//             if(words[0]=="type") {
//                 if(words.size()>1) {
//                     std::string var_name = func_name+words[1];
//                     std::string unclean_var_name = var_name;
//                     clean_pars(var_name);
//                     if(!symbols.hasKey(var_name)) {
//                         symbols.put(var_name,"type");
//                         g_ptr<Type> new_type = make<Type>();
//                         types.put(var_name,new_type);
//                     }
//                     else {
//                         print("[WARN] Duplicate symbol for 'type' at line: ",j);
//                     }
//                     if(words.size()>2) {
//                         if(words[2].find('{')!=std::string::npos || 
//                             unclean_var_name.find('{')!=std::string::npos) 
//                         {
//                             func_level++;
//                             std::string new_func_name = "_type::"+var_name+"_";
//                             func_name = func_name+new_func_name;
//                             func_delimiters.push('}');
//                             func_name_length.push(new_func_name.length());
//                         }
//                     }
//                 }
//                 else {
//                     print("[WARN] Missing variable name for 'type' at line: ",j);
//                 }
//             }
//             else if(words[0]=="group") {
//                 if(words.size()>1) {
//                     std::string var_name = func_name+words[1];
//                     clean_pars(var_name);
//                     if(!symbols.hasKey(var_name)) {
//                         symbols.put(var_name,"group");
//                         g_ptr<group> new_group = make<group>();
//                         groups.put(var_name,new_group);
//                     }
//                     else {
//                         print("[WARN] Duplicate symbol for 'group' at line: ",j);
//                     }
//                 }
//                 else {
//                     print("[WARN] Missing variable name for 'group' at line: ",j);
//                 }
//             }
//             else {
//                 int dot_pos = lines[j].find('.');
//                 if(dot_pos!=std::string::npos) 
//                 {
//                     std::string var_name = func_name+lines[j].substr(0,dot_pos);
//                     clean_pars(var_name);
//                     if(!symbols.hasKey(var_name)) {
//                         symbols.put(var_name,"var");
//                     }
//                     //Probably going to change this in the future to be a proper symbol based on the type but we have to discover
//                     //type first, thus a pub/sub system
//                     std::string method_name = func_name+lines[j].substr(dot_pos+1,lines[j].length());
//                     clean_pars(method_name);
//                     if(!symbols.hasKey(method_name)) {
//                         symbols.put(method_name,"method");
//                     }
//                 }
//             }

//             if(ends_with_char(words.last(),func_delimiters.last())) {
//                 func_delimiters.pop();
//                 func_level--;
//                 func_name = func_name.substr(func_name_length.pop(),func_name.length());
//             }
//         }

//         for(auto e : symbols.entrySet()) {
//             print("Name: ",e.key,", Type: ",e.value);
//         }
//     }


//     void intilize_lines(const std::string& code) {
//         list<std::string> lines = split_str(code,';');
//         clean_lines(lines);

//         for(int j=0;j<lines.size();j++) {
//             list<std::string> words = split_str(lines[j],' ');
//             std::string start = words[0];
//             clean_pars(start);
//             std::string symbol_type = symbols.get(start);
//             if(symbol_type=="type") {
//                 if(words.size()>1) {
//                     std::string var_name = words[1];
//                     clean_pars(var_name);
//                     if(!vars.hasKey(var_name)) {
//                         g_ptr<Type> type = types.get(start);
//                         vars.put(var_name,type->create());
//                     }
//                 }
//                 else {
//                     print("[WARN] Missing variable name for type defintion at line: ",j);
//                 }
//             }  
//             else if(symbol_type=="group") {

//             }
//             else if(symbol_type=="var") {

//             }
//             else if(symbol_type=="method") {

//             }


//         }
//     }

//     void run() {

//     }

// };




// static std::string prop_access(g_ptr<s_node> scope, g_ptr<a_node> node, int start) {
//     std::string  value = "";
//     g_ptr<Object> sub_object = scope->find_object(node->tokens[start]);
//     std::string sub_var_name = node->tokens[start+1]->content;
//     _note note = sub_object->type_->notes[sub_var_name];

//     switch(note.type) {
//         case LOCAL_OBJECT:
//             print("prop_access::747 No prop_access for objects");
//             break;
//         case LOCAL_INT:
//             value = std::to_string(sub_object->type_->get_int(sub_var_name,sub_object));
//             break;
//         case LOCAL_FLOAT:
//             value = std::to_string(sub_object->type_->get_float(sub_var_name,sub_object));
//             break;
//         case LOCAL_STRING:
//             value = sub_object->type_->get_string(sub_var_name,sub_object);
//             break;
//         default:
//             print("prop_access::759 Undefined prop_access: ",note.type);
//             break;
//     }
//     return value;
// }

// static void assign_prop(g_ptr<s_node> scope,g_ptr<a_node> node) {
//     g_ptr<Object> object = scope->find_object(node->tokens[0]);
//     std::string var_name = node->tokens[1]->content;
//     std::string  value = node->tokens[2]->content; 
//     _note note = object->type_->notes[var_name];

//     if(node->tokens.size()>3) {
//         value = prop_access(scope,node,2);
//     }

//     switch(note.type) {
//         case LOCAL_OBJECT:
//             print("execute_a_node::967 Object property assignment not handled yet!");
//             //object->type_->set_object(object,var_name,);
//             break;
//         case LOCAL_INT:
//             object->type_->set_int(object,var_name,std::stoi(value));
//             break;
//         case LOCAL_FLOAT:
//             object->type_->set_float(object,var_name,std::stof(value));
//             break;
//         case LOCAL_STRING:
//             object->type_->set_string(object,var_name,value);
//             break;
//         default:
//             print("execute_a_node::980 Missing local type for property assignment!");
//             break;
//     }
// }

// static void assignment(g_ptr<s_node> scope,g_ptr<a_node> node) {
//         std::string var_name = node->tokens[0]->content;
//         g_ptr<Token> value = node->tokens[1]; 
//         std::string value_string = value->content;
//         bool is_prop = false;
//         if(node->tokens.size()>2) {
//             is_prop = true;
//             value_string = prop_access(scope,node,1);
//         }

//         symbol_search result = scope->find_symbol(node->tokens[0]);
//         switch(result.type) {
//             case OBJECT:
                
//                 print("execute_a_node::857 Object assignment not yet handled");

//             break;

//             case INT:
//                 if(value->type == INT||is_prop) {
//                     scope->ints.set(var_name, std::stoi(value_string));
//                 } else if(value->type == IDENTIFIER) {
//                     scope->ints.set(var_name, scope->find_symbol(value).intv);
//                 }
//                 break;
//             case FLOAT:
//                 if(value->type == FLOAT||is_prop) {
//                     scope->floats.set(var_name, std::stof(value_string));
//                 } else if(value->type == IDENTIFIER) {
//                     scope->floats.set(var_name, scope->find_symbol(value).floatv);
//                 }
//                 break;
//             case STRING:
//                 if(value->type == STRING||is_prop) {
//                     scope->strings.set(var_name, value_string);
//                 } else if(value->type == IDENTIFIER) {
//                     scope->strings.set(var_name, scope->find_symbol(value).stringv);
//                 }
//                 break;
//             default:
//                 print("execute_a_node::858 missing assignment code for type: ",result.type);
//             break;
//         }
// }


// static void execute_a_node(g_ptr<s_node> scope,g_ptr<a_node> node,ScriptContext& ctx) {
//     g_ptr<Type> type = scope->type_ref;
//     switch(node->type) {
//         case TYPE_DECL:
//         {
//             g_ptr<Type> new_type = make<Type>();
//             std::string type_name = node->tokens.last()->content;;
//             new_type->type_name = type_name;
//             scope->log_identifier(type_name,TYPE,new_type);
//             ctx.flagOn("pending_type");
//             ctx.set<g_ptr<Type>>("the_type",new_type);
//         }
//             break;
//         case VAR_DECL:
//         {
//             t_type var_type = node->tokens[0]->type;
//             std::string var_name = node->tokens[1]->content; 

//             if(type) {
//                 switch(var_type) {
//                     case IDENTIFIER:
//                     {
//                         g_ptr<Type> object_type = scope->find_type(node->tokens[0]);
//                         if(object_type) {
//                             type->new_object_list(var_name);
//                         }
//                         else {
//                             print("execute_a_node::683 Type not found for ",var_name);
//                         }
//                     }
//                         break;
//                     case INT_KEY:
//                         type->new_int_list(var_name);
//                         break;
//                     case FLOAT_KEY:
//                         type->new_float_list(var_name);
//                         break;
//                     case STRING_KEY:
//                         type->new_string_list(var_name);
//                         break;

//                     default: 
//                         print("execute_a_node::692 no field in type for token ",var_name);
//                         break;
//                 }
//             }
//             else {
//                 if(var_type!=IDENTIFIER) {
//                     scope->log_identifier(var_name, var_type);
//                 }
//                 else {
//                     g_ptr<Type> object_type = scope->find_type(node->tokens[0]);
//                     if(object_type) {
//                         scope->log_identifier(var_name, OBJECT,object_type->create());
//                     }
//                     else {
//                         print("execute_a_node::706 Type not found for ",var_name);
//                     }
//                 }
//             }
//         }
//             break;
//         case VAR_DECL_INIT:
//         {
//             t_type var_type = node->tokens[0]->type;
//             std::string var_name = node->tokens[1]->content; 
//             g_ptr<Token> value = node->tokens[2]; 

//             if(type) {
//                 switch(var_type) {
//                     case IDENTIFIER:
//                     {
//                         g_ptr<Type> object_type = scope->find_type(node->tokens[0]);
//                         if(object_type) {
//                             if(value->type==IDENTIFIER)
//                             {
//                                 //Need more defensive programing here but not right now
//                                 type->new_object_list(var_name,scope->find_object(value));
//                             }
//                             else {
//                                 print("execute_a_node::735 Can't intilize an object with a literal ",var_name);
//                             }
//                         }
//                         else {
//                             print("execute_a_node::741 Type not found for ",var_name);
//                         }
//                     }
//                         break;
//                     case INT_KEY:
//                         if(value->type==INT)
//                         {
//                             type->new_int_list(var_name,std::stoi(value->content));
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             type->new_int_list(var_name,scope->find_symbol(value).intv);
//                         }
//                         break;
//                     case FLOAT_KEY:
//                         if(value->type==FLOAT)
//                         {
//                             type->new_float_list(var_name,std::stof(value->content));
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             type->new_float_list(var_name,scope->find_symbol(value).floatv);
//                         }
//                         break;
//                     case STRING_KEY:
//                         if(value->type==STRING)
//                         {
//                             type->new_string_list(var_name,value->content);
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             type->new_string_list(var_name,scope->find_symbol(value).stringv);
//                         }
//                         break;

//                     default: 
//                         print("execute_a_node::748 no field in type for token ",var_name);
//                         break;
//                 }
//             }
//             else {
//                 switch(var_type) {
//                     case IDENTIFIER:
//                     {
//                         g_ptr<Type> object_type = scope->find_type(node->tokens[0]);
//                         if(object_type) {
//                             g_ptr<Object> object = object_type->create();
//                             //This is where constructers and rule of five will come into play
//                             //Can't make a default until we have property acess fiqured out
//                             scope->log_identifier(var_name, OBJECT,object);
//                         }
//                         else {
//                             print("execute_a_node::764 Type not found for ",var_name);
//                         }
//                     }
//                         break;
//                     case INT_KEY:
//                         scope->log_identifier(var_name, INT);
//                         if(value->type==INT)
//                         {
//                             scope->ints.set(var_name,std::stoi(value->content));
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             scope->ints.set(var_name,scope->find_symbol(value).intv);
//                         }
//                         break;
//                     case FLOAT_KEY:
//                         scope->log_identifier(var_name, FLOAT);
//                         if(value->type==FLOAT)
//                         {
//                             scope->floats.set(var_name,std::stof(value->content));
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             scope->floats.set(var_name,scope->find_symbol(value).floatv);
//                         }
//                         break;
//                     case STRING_KEY:
//                         scope->log_identifier(var_name, STRING);
//                         if(value->type==STRING)
//                         {
//                             scope->strings.set(var_name,value->content);
//                         }
//                         else if(value->type==IDENTIFIER) {
//                             scope->strings.set(var_name,scope->find_symbol(value).stringv);
//                         }
//                         break;

//                     default: 
//                         print("execute_a_node::779 No field in type for token ",var_name);
//                         break;
//                 }
//             }
//         }
//             break;
//         case METHOD_DECL:
//         {
//             if(type)
//             {
//                 ctx.flagOn("pending_method");

//                 t_type var_type = node->tokens[0]->type;
//                 std::string var_name = node->tokens[1]->content;

//                 //REPLACE THIS LATER
//                 method_storage.put(type->type_name,map<std::string,g_ptr<s_node>>());
//                 method_storage.get(type->type_name).put(var_name,scope->children[0]);

//                 print("Method decleration attempted: ",node->tokens[0]->content," on ",var_name);
//             }
//             else 
//             {
//                 print("execute_a_node::1004 Method decleration outside of type ");
//             }
//         }
//             break;
//         case PROP_ACCESS:
//             print("execute_a_node::1009 Unused propery acess!");
//             break;
//         case ASSIGNMENT:
//             assignment(scope,node);
//             break;
//         case PROP_ASSIGNMENT:
//             assign_prop(scope,node);
//             break;
        // case METHOD_CALL:
        // {
        //     g_ptr<Object> object = scope->find_object(node->tokens[0]);
        //     std::string var_name = node->tokens[1]->content;
        //     // std::string  value = node->tokens[2]->content; 

        //     print("Method call attempted: ",node->tokens[0]->content," on ",var_name);

        //     g_ptr<s_node> copy = method_storage.get(object->type_->type_name).get(var_name)->clone_scope();
        //     execute_a_node(copy,copy->a_nodes[0],ctx);
        // }
        //     break;
//         case RETURN_CALL:
//         {
//             print("Implment code for return calls later");
//         }
//             break;
//         case PRINT_CALL: 
//         {
//             std::string toPrint = "";
//             for(auto a : node->sub_nodes) {
//                 switch(a->type) {
//                     case LITERAL: 
//                         for(auto t : a->tokens) {
//                             toPrint.append(t->content);
//                         }
//                         break;
//                     case LITERAL_IDENTIFIER: 
//                     {
//                         symbol_search result = scope->find_symbol(a->tokens[0]);
//                         switch(result.type) {
//                             case OBJECT:
//                                 toPrint.append(" [OBJECT MISSING PRINT LOGIC] ");
//                                 break;
//                             case INT:
//                                 toPrint.append(std::to_string(result.intv));
//                                 break;
//                             case FLOAT:
//                                 toPrint.append(std::to_string(result.floatv));
//                                 break;
//                             case STRING:
//                                 toPrint.append(result.stringv);
//                                 break;
//                             case UNDEFINED:
//                                 toPrint.append(" [UNDEFINED] ");
//                                 break;
//                             default:
//                                 toPrint.append(" [TYPE MISSING PRINT LOGIC IN LITERAL_IDENTIFIER] "+std::to_string(result.type));
//                                 break;
//                         }
//                     }
//                         break;
//                     case PROP_ACCESS:
//                     {
//                         g_ptr<Object> object = scope->find_object(a->tokens[0]);
//                         std::string var_name = a->tokens[1]->content;
//                         _note note = object->type_->notes[var_name];
//                         //This is slow and needs to be replaced before being properly used
//                         switch(note.type) {
//                             case LOCAL_OBJECT:
//                                 toPrint.append(" [OBJECT MISSING PRINT LOGIC] ");
//                                 break;
//                             case LOCAL_INT:
//                                 toPrint.append(std::to_string(object->type_->get_int(var_name,object)));
//                                 break;
//                             case LOCAL_FLOAT:
//                                 toPrint.append(std::to_string(object->type_->get_float(var_name,object)));
//                                 break;
//                             case LOCAL_STRING:
//                                 toPrint.append(object->type_->get_string(var_name,object));
//                                 break;
//                             default:
//                                 toPrint.append(" [TYPE MISSING PRINT LOGIC IN PROP_ACESS] ");
//                                 break;
//                         }
//                     }
//                         break;
//                     default: 
//                         toPrint.append(" [ERROR] ");
//                         break;
//                 }
//             }
//             print(toPrint);
//         }
//             break;
//         default:
//             print("execute_a_node::634 Missing execution code for type: ",node->type);
//         break;
//     }
// }


// static void execute_program(g_ptr<s_node> scope) {
//     int child_index = 0;
//     ScriptContext ctx;
//     for (auto a_node : scope->a_nodes) {
//         switch(a_node->type) {
//             case ENTER_SCOPE:
//                 if (child_index < scope->children.size()) {
//                     auto child_scope = scope->children[child_index];
                    
//                     if (ctx.check("pending_type")) {
//                         child_scope->type_ref = ctx.get<g_ptr<Type>>("the_type");
//                         ctx.flagOff("pending_type");
//                     }
                    
//                     if(ctx.check("pending_method")) {
//                         //Do something else here at some point
//                         ctx.flagOff("pending_method");
//                     }
//                     else {
//                         execute_program(child_scope);
//                     }
//                     child_index++;
//                 }
//                 break;
                
//             case EXIT_SCOPE:

//                 break;

//             case ENTER_PAREN:    

//                 break;
//             case EXIT_PAREN:    

//                 break;
            
//             default:
//                 execute_a_node(scope, a_node,ctx);
//                 break;
//         }
//     }
// }


// struct Transpiler {

//    static void discover_symbols(g_ptr<s_node> node) {
//         static map<g_ptr<s_node>,list<g_ptr<Token>>> unparsed;
//         g_ptr<d_list<g_ptr<Token>>> tokens = node->tokens;
//         for(int i=0;i<tokens->size();i++) 
//         {
//             g_ptr<Token> token = tokens->list::get(i);
//             if(token->parsed) continue;
//             g_ptr<Token> left = token->left();
//             g_ptr<Token> right = token->right();
//             switch(token->type) {
//             case IDENTIFIER: 
//                 {
//                     symbol_search search = node->find_symbol(token);
//                     if(search.pointer) {
//                         if(search.type==OBJECT) {
//                             if(auto object_ptr = g_dynamic_pointer_cast<Object>(search.pointer)) {
//                                 if(right)
//                                 {
//                                     if(right->getType()==DOT) {
//                                         print("transpiler::257 Method acess attempted from ",token->content);
//                                     }
//                                     else {
//                                         print("transpiler::260 Opperator ",right->content," attempted by token ",token->content);
//                                     }
//                                 }
//                                 else {
//                                     print("transpiler::263 unused refrence to an object on token: ",token->content);
//                                 }
//                             }
//                         }
//                         else if(search.type==TYPE) {
//                             if(auto type_ptr = g_dynamic_pointer_cast<Type>(search.pointer)) { 
//                                 if(right) {
//                                     if(right->getType()==IDENTIFIER) {
//                                         if(node->objects.hasKey(right->content)) {
//                                             print("transpiler::269 Reused object name in scope on token: ",right->content);
//                                         }
//                                         else {
//                                             right->mark();
//                                             g_ptr<Object> new_object = type_ptr->create();
//                                             node->log_identifier(right->content,OBJECT,new_object);
//                                         }
//                                     }
//                                     else if(right->getType()==DOT) {
//                                         print("transpiler::274 Method acess attempted from ",token->content);
//                                     }
//                                 }
//                             }
//                         }
//                     }
//                     else {
//                         //print("transpiler::284 Unknown identifier refrence on token: ",token->content);
//                     }
//                 }
//                 break;
//             case TYPE_KEY:
//                     if(right&&right->getType()==IDENTIFIER) {
//                         if(node->types.hasKey(right->content)) {
//                             print("transpiler::291 Reused type name in scope on token: ",right->content);
//                         }
//                         else {
//                             right->mark();
//                             g_ptr<Type> new_type = make<Type>();
//                             node->log_identifier(right->content,TYPE,new_type);
//                         }
//                     }
//                     else {
//                         //In the future, loop and move right to collect keywords based upon content like "public"
//                         print("transpiler::294 missing name for type");
//                     }
//                 break;
//             case INT_KEY:
//                 if(right&&right->getType()==IDENTIFIER) {
//                     if(node->ints.hasKey(right->content)) {
//                         print("transpiler::482 Reused type name in scope on token: ",right->content);
//                     }
//                     else {
//                         right->mark();
//                         node->log_identifier(right->content,INT);
//                     }
//                 }
//                 else {
//                     print("transpiler::491 missing name for int");
//                 }
//                 break;
//             case FLOAT_KEY:
//                 if(right&&right->getType()==IDENTIFIER) {
//                     if(node->floats.hasKey(right->content)) {
//                         print("transpiler::497 Reused type name in scope on token: ",right->content);
//                     }
//                     else {
//                         right->mark();
//                         node->log_identifier(right->content,FLOAT);
//                     }
//                 }
//                 else {
//                     print("transpiler::505 missing name for float");
//                 }
//                 break;
//             case STRING_KEY:
//                 if(right&&right->getType()==IDENTIFIER) {
//                     if(node->strings.hasKey(right->content)) {
//                         print("transpiler::511 Reused type name in scope on token: ",right->content);
//                     }
//                     else {
//                         right->mark();
//                         node->log_identifier(right->content,STRING);
//                     }
//                 }
//                 else {
//                     print("transpiler::519 missing name for string");
//                 }
//                 break;
//             case END:
//                 //Not sure what to do with this yet
//                 break;
//             default: 
//                     //print("transpiler::parse_primary No defined behaviour for ",token->content);
//                 break;
//             }
//         }
//         if(!node->children.empty()) {
//             for(auto c : node->children) {
//                 discover_symbols(c);
//             }
//         }
//     }


//     static void parse_print(g_ptr<s_node> node,g_ptr<Token> start) {
//         std::string toPrint = "";
//         int limit = start->index;
//         g_ptr<Token> on = start->right();
//         while(on && on->type!=END) {
//             if(on->type==IDENTIFIER)
//             {
//                 symbol_search search = node->find_symbol(on);
//                 switch(search.type) {
//                     case OBJECT:
//                         //Printing the direct name for now, in the future allow opperator overloading of print_key
//                         toPrint.append(on->content);
//                     break;
//                     case INT:
//                         toPrint.append(std::to_string(search.intv));
//                     break;
//                     case FLOAT:
//                         toPrint.append(std::to_string(search.floatv));
//                     break;
//                     case STRING:
//                         toPrint.append(search.stringv);
//                     break;
//                     default: 
//                         toPrint.append(on->content);
//                     break;
//                 }
//             }
//             else if(on->type==INT) {
//                 toPrint.append(std::to_string(node->ints.get(on->content)));
//             }
//             else if(on->type==FLOAT) {
//                 toPrint.append(std::to_string(node->floats.get(on->content)));
//             }
//             else {
//                 toPrint.append(on->content);
//             }
//             toPrint.append(" ");
//             limit++;
//             on = on->right();
//         }
//         print(toPrint);
//     }

//    static void parse_symbols(g_ptr<s_node> node) {
//         list<g_ptr<Token>> toRemove;
//         g_ptr<d_list<g_ptr<Token>>> tokens = node->tokens;
//         list<g_ptr<Token>> prints;
//         auto it = tokens->begin();
//         while(it!=tokens->end()) {
//             g_ptr<Token> token = *it;
//             if(token->parsed) {
//                 ++it;
//                 continue;
//             }
//             g_ptr<Token> left = token->left();
//             g_ptr<Token> right = token->right();
//             switch(token->type) {
//             case PRINT_KEY:
//             {
//                prints << token;
//             }
//                 break;
//             case EQUALS:
//             {
//                 if(!left||!right) break;
//                 if(left->getType()==IDENTIFIER) {
//                     symbol_search left_search = node->find_symbol(left);
//                     if(right->getType()==IDENTIFIER) {
//                         symbol_search right_search = node->find_symbol(right);
//                         switch(right_search.type) {
//                             case STRING:
//                                 node->strings.set(left_search.key,right_search.stringv);
//                             break;
//                             default:

//                             break;
//                         }
//                     }
//                     else if(right->type==STRING) {
//                         node->strings.set(left_search.key,right->content);
//                     }
//                 }
//             }
//                 break;
//             case PLUS:
//             {
//                 if(!left||!right) break;

//                 switch(left->getType()) {
//                     case INT:
//                         if(right->getType()==INT) {
//                             int val = std::stoi(left->content) + std::stoi(right->content);
//                             left->content = std::to_string(val);
//                         }
//                     break;
//                     default:

//                     break;
//                 }

//                 // symbol_search left_search = node->find_symbol(left);
//                 // symbol_search right_search = node->find_symbol(right);
//                 // switch(left_search.type) {
//                 //     case INT:
//                 //         if(right_search.type==INT) {
//                 //             int val = left_search.intv + right_search.intv;
//                 //             node->ints.set(left_search.key,val);
//                 //         }
//                 //     break;
//                 // }
//             }
//                 break;
//             case END:
//                 if(!prints.empty()) parse_print(node,prints.pop());
//                 break;
//             default: 

//                 break;
//             }
//             ++it;
//         }
//         if(!node->children.empty()) {
//             for(auto c : node->children) {
//                 parse_symbols(c);
//             }
//         }
//     }
// };







// static bool balance_nodes(list<g_ptr<a_node>>& result) {
//     a_type state = UNTYPED;
//     int corrections = 0;
//     for (int i = result.size() - 1; i >= 0; i--) {
//         g_ptr<a_node> right = result[i];
//         if(!right->sub_nodes.empty()) {
//             balance_nodes(right->sub_nodes);
//         }
//         if(i==0) continue;
//         //print("Checking index:", i, " against ", i-1);
//         state=result[i]->type;
//         g_ptr<a_node> left = result[i-1];
//         // print("LEFT: ",a_type_string(left->type));
//         // print("RIGHT: ",a_type_string(right->type));
//         if(a_is_opp(state)&&a_is_opp(left->type)) 
//         {
//             if(type_precdence(left->type)<=type_precdence(state))
//             {
//                 left->sub_nodes << right;
//                 right->tokens.insert(left->tokens.pop(),0);
//                 result.removeAt(i);
//                 corrections++;
//             }
//         }
//     }

//     return corrections!=0;
// }

// static void balance_precedence(list<g_ptr<a_node>>& result) {
//     bool changed = true;
//     int depth = 0;
//     while (changed&&depth<10) {
//         depth++;
//         // print("==BALANCING PASS ",depth,"==");
//         changed = balance_nodes(result);
//     }

//     //print("Finished balancing");
//     int i = 0;
//     // for (auto& node : result) {
//     //     print("On: ",i);
//     //     print_a_node(node, 0, i++);
//     // }
// }


//Single pass approach
// static void balance_precedence(list<g_ptr<a_node>>& result);

// static g_ptr<a_node> balance_node(list<g_ptr<a_node>>& result, int& current_index, int min_precedence) {
//     if (current_index >= result.size()) return nullptr;
    
//     g_ptr<a_node> left = result[current_index++];
//     if (!left) return nullptr;
    
//     while (current_index < result.size()) {
//         g_ptr<a_node> op_node = result[current_index];
//         if (!op_node || !a_is_opp(op_node->type)) break;
        
//         int precedence = type_precdence(op_node->type);
//         if (precedence <= min_precedence) break;
        
//         current_index++;
        
//         g_ptr<a_node> right = balance_node(result, current_index, precedence+1);
//         if (!right) break;
        
//         g_ptr<a_node> binary_op = make<a_node>();
//         binary_op->type = op_node->type;
//         binary_op->sub_nodes << left;
//         binary_op->sub_nodes << right;
        
//         left = binary_op;
//     }
    
//     return left;
// }
                
// static void balance_precedence(list<g_ptr<a_node>>& result) {
//     int current_index = 0;
//     print("====Balancing precedence===");
//     while (current_index < result.size()) {
//         g_ptr<a_node> expr = balance_node(result,current_index,0);
//         if (!expr) break;
//     }

//     int i = 0;
//     for (auto& node : result) {
//         print_a_node(node, 0, i++);
//     }
// }



//Benchmark

// static void run_simple_precedence_benchmark() {
//     print("=== SIMPLE PRECEDENCE BENCHMARK ===");
    
//     // Test 1: Your iterative correction approach
    
//     std::string code = readFile("../Projects/Testing/src/golden.gld");
//     list<g_ptr<Token>> sub_tokens = tokenize(code);
//     Tokens tokens = make<TokensM>();
//     tokens->pushAll(sub_tokens);
//     list<g_ptr<a_node>> nodes = parse_tokens(tokens);

//     print("Testing iterative correction approach...");
//     auto start1 = std::chrono::high_resolution_clock::now();

//     balance_precedence(nodes);
    
//     auto end1 = std::chrono::high_resolution_clock::now();
//     auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    
//     // Test 2: New single-pass precedence approach
    
//     std::string code2 = readFile("../Projects/Testing/src/golden.gld");
//     list<g_ptr<Token>> sub_tokens2 = tokenize(code2);
//     Tokens tokens2 = make<TokensM>();
//     tokens2->pushAll(sub_tokens2);
//     list<g_ptr<a_node>> nodes2 = parse_tokens(tokens2);
    
//     print("Testing single-pass precedence approach...");
//     auto start2 = std::chrono::high_resolution_clock::now();

//     balance_nodesO(nodes2);
    
//     auto end2 = std::chrono::high_resolution_clock::now();
//     auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    
//     // Results
//     print("=== RESULTS ===");
//     print("Iterative correction: ", duration1.count(), " microseconds");
//     print("Single-pass approach: ", duration2.count(), " microseconds");
    
//     if (duration1.count() < duration2.count()) {
//         double speedup = (double)duration2.count() / duration1.count();
//         print("Iterative correction is ", speedup, "x faster");
//     } else {
//         double speedup = (double)duration1.count() / duration2.count();
//         print("Single-pass approach is ", speedup, "x faster");
//     }
// }








// Version 0.0.5 t_node execution

// static void t_variable_assignment(g_ptr<t_node> left, g_ptr<t_node> right,g_ptr<s_node> scope) {
//     // std::string name = left->name;
//     // if(scope->scope_type==TYPE_DEF) {
//     //     switch(right->value.type) {
//     //         case OBJECT: scope->type_ref->set_default<g_ptr<q_object>>(name,right->value.objectv); break;
//     //         case STRING: scope->type_ref->set_default<std::string>(name,right->value.stringv); break;
//     //         case INT: scope->type_ref->set_default<int>(name,right->value.intv); break;
//     //         case FLOAT: scope->type_ref->set_default<float>(name,right->value.floatv); break;
//     //         case BOOL:  break; //ADD handeling later
//     //         case CHAR:  break;
//     //         default:
//     //             print("t_variable_assignment::1236 Missing assignment for b_type: ",right->value.type);
//     //         break;
//     //     }
//     // }
//     // else {
//     // if(left->left && left->left->value.objectv!=nullptr) {
//     //          g_ptr<Object> object = left->left->value.objectv;
//     //          name = left->right->name;
//     //         _note note = object->type_->notes.get(name);
//     //         switch(note.type)
//     //         {
//     //         case LOCAL_OBJECT: object->type_->set<g_ptr<q_object>>(name,object->ID,right->value.objectv); break;
//     //         case LOCAL_INT: object->type_->set<int>(name,object->ID,right->value.intv); break;
//     //         case LOCAL_FLOAT: object->type_->set<float>(name,object->ID,right->value.floatv); break;
//     //         case LOCAL_STRING: object->type_->set<std::string>(name,object->ID,right->value.stringv); break;
//     //         default: break;
//     //         }
//     // }
//     // else 
//     // {
//     //     switch(right->value.type) {
//     //         case OBJECT: scope->set_symbol(scope->find_symbol_ref(name),right->value.objectv); break;
//     //         case STRING: scope->set_symbol(scope->find_symbol_ref(name),right->value.stringv); break;
//     //         case INT: scope->set_symbol(scope->find_symbol_ref(name),right->value.intv); break;
//     //         case FLOAT: scope->set_symbol(scope->find_symbol_ref(name),right->value.floatv); break;
//     //         case BOOL: scope->set_symbol(scope->find_symbol_ref(name),right->value.boolv); break;
//     //         case CHAR: scope->set_symbol(scope->find_symbol_ref(name),right->value.charv); break;
//     //         default:
//     //             print("t_variable_assignment::1236 Missing assignment for b_type: ",right->value.type);
//     //         break;
//     //     }
//     //     //}
//     // }
//     // }
// }

// map<std::string,map<std::string,g_ptr<s_node>>> type_methods;


// template<typename Op>
// void execute_binary_operation(g_ptr<t_node> node, Op operation, b_type result_type,g_ptr<s_node> scope);

// void execute_nodes(g_ptr<s_node> root);

// static void execute_t_variable_decleration(g_ptr<t_node> node,g_ptr<s_node> scope) {
//     // if(scope->scope_type==TYPE_DEF) {
//     //     switch(node->value.type) {
//     //         case OBJECT: scope->type_ref->new_list<g_ptr<q_object>>(node->name); break;
//     //         case INT: scope->type_ref->new_list<int>(node->name); break;
//     //         case FLOAT: scope->type_ref->new_list<float>(node->name); break;
//     //         case STRING: scope->type_ref->new_list<std::string>(node->name); break;
//     //         default: break; 
//     //     }
//     // }
//     // else {
//     //     if(node->value.type == OBJECT) {
//     //         g_ptr<Object> new_object = scope->get_symbol_ref(node->value.stringv).typev->create();
//     //         node->value.objectv = new_object;
//     //     }
//     //     scope->symbols.put(node->name,node->value);
//     // }
// }

// static void execute_t_node(g_ptr<t_node> node,g_ptr<s_node> scope)
// {
//     // switch(node->type) {
//     //     case T_VAR_DECL:
//     //         execute_t_variable_decleration(node,scope);
//     //         break;
//     //     case T_TYPE_DECL:
//     //         if(scope->scope_type==TYPE_DEF) {
//     //             //Inheretence would happen here by forming a type tree
//     //         }
//     //         else {
//     //             g_ptr<Type> type = make<Type>();
//     //             type->type_name = node->name;
//     //             node->value.type = TYPE;
//     //             node->value.typev = type;
//     //             scope->symbols.put(node->name,node->value);
//     //             node->scope->type_ref = type;
//     //             execute_nodes(node->scope);
//     //         }
//     //         break;
//     //     case T_METHOD_DECL:
//     //     {
//     //         if(scope->scope_type==TYPE_DEF) {
//     //             g_ptr<Type> type = scope->type_ref;
//     //             if(!type_methods.hasKey(type->type_name)) {
//     //                 type_methods.put(type->type_name,map<std::string,g_ptr<s_node>>());
//     //             }
//     //             type_methods.get(type->type_name).put(node->name,node->scope);

//     //         }
//     //         else {
//     //             //I think method declerations only happen in a type_def so...
//     //             //This might be reduntent!
//     //         }
//     //     }
//     //         break;
//     //     case T_METHOD_CALL:
//     //     {
//     //         //This is **super** messy and really needs to be improved and changed!
//     //         execute_t_node(node->left,scope);
//     //         g_ptr<Object> object = node->left->value.objectv;
//     //         g_ptr<s_node> exec_scope = clone_scope(type_methods.get(object->type_->type_name).get(node->right->name));

//     //         g_ptr<t_node> decl = exec_scope->t_owner;
//     //         if(!decl->children.empty()) {
//     //             if(decl->children.size()==node->children.size()) {
//     //                 for(int i=0;i<decl->children.size();i++) {
//     //                     execute_t_node(node->children[i], scope);
//     //                     g_ptr<t_node> param_decl = decl->children[i];
//     //                     g_ptr<t_node> param_value = node->children[i];
//     //                     execute_t_variable_decleration(param_decl,exec_scope);
//     //                     t_variable_assignment(param_decl,param_value,exec_scope);
//     //                 }
//     //             }
//     //             else {
//     //                 print("execute_t_node::1489 Unexcectped argument count in method call");
//     //             }
//     //         }

//     //         execute_nodes(exec_scope);
//     //         if(exec_scope->return_val.type!=UNDEFINED) {
//     //             node->value = exec_scope->return_val;
//     //         }
//     //     }
//     //         break;
//     //     case T_RETURN:
//     //         execute_t_node(node->right,scope);
//     //         scope->return_val = node->right->value;
//     //         break;
//     //     case T_ASSIGN:
//     //     //Might cause double symbol defintion where executing a T_VAR_DECL is creating another instance of the variable
//     //     //Could be using up more memory than we need
//     //         if(node->left->type==T_IDENTIFIER||node->left->type==T_PROP_ACCESS) {
//     //         execute_t_node(node->left,scope);
//     //         }
//     //         execute_t_node(node->right,scope); 
//     //         t_variable_assignment(node->left,node->right,scope);
//     //         break;
//     //     case T_ADD:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a + b; }, INT,scope);
//     //         break;
//     //     case T_SUBTRACT:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a - b; }, INT,scope);
//     //         break;
//     //     case T_MULTIPLY:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a * b; }, INT,scope);
//     //         break;
//     //     case T_DIVIDE:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a / b; }, INT,scope);
//     //         break;
//     //     case T_LESS_THAN:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a < b; }, BOOL,scope);
//     //         break;
//     //     case T_GREATER_THAN:
//     //         execute_binary_operation(node, [](auto a, auto b){ return a > b; }, BOOL,scope);
//     //         break;
//     //     case T_PROP_ACCESS:
//     //     {
//     //         execute_t_node(node->left,scope);
//     //         g_ptr<Object> object = node->left->value.objectv;
//     //         node->left->value.objectv = object;
//     //         _note note = object->type_->notes.get(node->right->name);
//     //         // switch(note.type)
//     //         // {
//     //         // case LOCAL_OBJECT: node->value.objectv = g_dynamic_pointer_cast<Object>(object->type_->get<g_ptr<q_object>>(node->right->name,object->ID)); node->value.type=OBJECT; break;
//     //         // case LOCAL_INT: node->value.intv = object->type_->get<int>(node->right->name,object->ID); node->value.type=INT; break;
//     //         // case LOCAL_FLOAT: node->value.floatv = object->type_->get<float>(node->right->name,object->ID); node->value.type=FLOAT; break;
//     //         // case LOCAL_STRING: node->value.stringv = object->type_->get<std::string>(node->right->name,object->ID); node->value.type=STRING; break;
//     //         // default: break;
//     //         // }
//     //     }
//     //         break;
//     //     case T_LITERAL:
//     //         //Nothing here as they're already literal!
//     //         break;
//     //     case T_IDENTIFIER:
//     //         if(scope->scope_type==TYPE_DEF) {

//     //         }
//     //         else {
//     //             node->value = scope->get_symbol_ref(node->name);
//     //         }
//     //         break;
//     //     case T_IF: 
//     //         execute_t_node(node->right,scope); 
//     //         if (node->right->value.boolv) {
//     //             execute_nodes(node->scope);
//     //         } else if(node->left) {
//     //             if(node->left->scope) {
//     //                 execute_nodes(node->left->scope);
//     //             }
//     //             else {
//     //                 //This is where an if else is handled
//     //             }
//     //         }
//     //         break;
//     //     case T_PRINT:
//     //     {
//     //             std::string toPrint = "";
//     //             for(auto t : node->children) {
//     //                 execute_t_node(t,scope);
//     //                 toPrint.append(t->value.to_string());
//     //             }
//     //             print(toPrint);
//     //     }
//     //         break;
//     //     default:
//     //         print("Missing parsing code for t_node type: ",node->type);
//     //         break;
//     // }
// }


// template<typename Op>
// void execute_binary_operation(g_ptr<t_node> node, Op operation, b_type result_type,g_ptr<s_node> scope) {
//     // execute_t_node(node->left,scope);
//     // execute_t_node(node->right,scope);
    
//     // symbol& left_val = node->left->value;
//     // symbol& right_val = node->right->value;
//     // node->value.type = result_type;
    
//     // if(left_val.type == INT && right_val.type == INT) {
//     //     auto result = operation(left_val.intv, right_val.intv);
//     //     if(result_type == BOOL) node->value.boolv = result;
//     //     else node->value.intv = result;
//     // }
//     // else if(left_val.type == FLOAT || right_val.type == FLOAT) {
//     //     float left_f = (left_val.type == FLOAT) ? left_val.floatv : left_val.intv;
//     //     float right_f = (right_val.type == FLOAT) ? right_val.floatv : right_val.intv;
//     //     auto result = operation(left_f, right_f);
//     //     if(result_type == BOOL) node->value.boolv = result;
//     //     else node->value.floatv = result;
//     // }
//     // else {
//     //     print("Type error in binary operation");
//     // }
// }

// void execute_nodes(g_ptr<s_node> root) {    
//     for(auto t_node : root->t_nodes) {
//         execute_t_node(t_node,root);
//     }
// }