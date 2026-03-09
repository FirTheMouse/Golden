#include <chrono>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#include<core/type.hpp>
#include<util/strings.hpp>

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

// print(1,2,3);
// return 30;



// global wubless struct make_wub {

// };


// inline wubfull fold_wub(const string marker, int b) {
    
// };

// make_wub wubmaker;
// fold_wub("edgar.wub",42);
// print((*x).y," or ",(*a).b);




// wubless type num {
//     int a;

//     const int addOne(wubless int val, int o) {
//         val = val+1;
//         return val;
//     }
// }

// num c;
// int a;
// c.a = c.addOne(1);
// print(c.a);



namespace GDSL {
    map<std::string,g_ptr<Value>> keywords;
    map<std::string,uint32_t> tokenized_keywords;

    void a_pass_resolve_keywords(list<g_ptr<Node>>& nodes) {
        for(g_ptr<Node>& node : nodes) {
            a_pass_resolve_keywords(node->children);
            g_ptr<Value> value = keywords.getOrDefault(node->name,fallback_value);
            if(value!=fallback_value) {
                if(!node->value)
                    node->value = make<Value>(0);
                
                node->value->copy(value);
            }
        }
    };

    map<char,bool> char_is_split;
    map<uint32_t,bool> discard_types;

    map<uint32_t,int> left_binding_power;
    map<uint32_t,int> right_binding_power;

    map<uint32_t, std::function<void(g_ptr<Node>, g_ptr<Node>, g_ptr<Node>)>> scope_link_handlers;
    map<uint32_t, int> scope_precedence;



    void print_scope(g_ptr<Node> scope, int depth = 0) {
        std::string indent(depth * 2, ' ');
        
        if (depth > 0) {
            log(indent, "{");
        }
        
        for (auto& subnode : scope->children) {
            log(indent, "  ", TO_STRING(subnode->type));
        
            
            if(!subnode->scopes.empty()) {
                for(auto subscope : subnode->scopes) {
                    print_scope(subscope, depth + 1);
                }
            }
        }
        
        if (depth > 0) {
            log(indent, "}");
        }
    }

    struct g_value {
    public:
        g_value() {}
        g_value(g_ptr<Node> _owner) : owner(_owner) {}
        bool explc = false;
        g_ptr<Node> owner = nullptr;
        bool deferred = false;
    };

    static g_ptr<Node> parse_scope(list<g_ptr<Node>> nodes) {
        g_ptr<Node> root_scope = make<Node>();
        root_scope->frame = make<Frame>();
        root_scope->name = "GLOBAL";
        g_ptr<Node> current_scope = root_scope;
        list<g_value> stack{g_value()};

        #if PRINT_ALL
        newline("Parse scope pass");
        #endif

        for (int i = 0; i < nodes.size(); ++i) {
            g_ptr<Node> node = nodes[i];
            g_ptr<Node> owner_node = (i>0) ? nodes[i - 1] : nullptr;

            int p = scope_precedence.getOrDefault(node->type,0);
            bool on_stack = stack.last().owner ? true : false;
            if(p<=0) {
                if(p<0) {
                    if (current_scope->parent) {
                        current_scope = current_scope->parent;
                    }
                }
                else {
                    current_scope->children << node; 
                    node->place_in_scope(current_scope.getPtr());
                    if(on_stack && !stack.last().deferred && !stack.last().explc) {
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                }
            }
            else {
                if(p<10) {
                    current_scope->children << node;
                    node->place_in_scope(current_scope.getPtr());
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

                g_ptr<Node> parent_scope = current_scope;
                current_scope = current_scope->spawn_sub_scope();
                current_scope->frame = make<Frame>();
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
                    current_scope->type = 0; //Suppoused to be GET_TYPE(BLOCK), doesn't matter, don't care
                }
            }
        }

        #if PRINT_ALL
        //print_scope(root_scope);
        endline();
        #endif
        return root_scope;
    }


    g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
        return make<Value>(sub_type,size,reg::new_type(name));
    }

    size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
        g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
        keywords.put(name,val);
        return val->sub_type;
    }

    size_t make_tokenized_keyword(const std::string& f) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t id = reg::new_type(uppercase_name);
        tokenized_keywords.put(f,id);
        return id;
    }

    // T() {} <- As with function, but we ctx.node->frame contains the bracketed code for further execution.
    template<typename Op>
    static size_t add_scoped_keyword(const std::string& name, int scope_prec, Op exec_fn)
    {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        size_t id = make_tokenized_keyword(name);
        scope_precedence.put(id, scope_prec);
        scope_link_handlers.put(id, [](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
            if(owner_node->scope()) { //To handle implicit scoping
                owner_node->in_scope->scopes.erase(owner_node->scopes.take(0));
            } 
            new_scope->owner = owner_node.getPtr();
            owner_node->scopes << new_scope.getPtr();
            for(auto c : owner_node->children) {
                c->place_in_scope(new_scope.getPtr());
            }
            new_scope->name = owner_node->name;
        });
        t_functions.put(id, [](Context& ctx){});
        exec_handlers.put(id, exec_fn);
        return id;
    }

    template<typename InputT, typename ResultT, typename Op>
    Handler make_arithmetic_handler(Op operation, uint32_t return_type) {
        return [operation,return_type](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            execute_r_node(ctx.node->right, ctx.frame);
            
            InputT left = ctx.node->left->value->get<InputT>();
            InputT right = ctx.node->right->value->get<InputT>();
            ResultT result = operation(left, right);
            
            ctx.node->value->type = return_type;
            ctx.node->value->set<ResultT>(result);
        };
    }

    size_t add_token(char c, const std::string& f) {
        size_t id = reg::new_type(f);
        tokenizer_functions.put(c,[id,c](Context& ctx) {
            ctx.node = make<Node>(id,c);
            ctx.result->push(ctx.node);
        });
        char_is_split.put(c, true);
        return id;
    }

    struct BinaryOpIds {
        BinaryOpIds() {}
        BinaryOpIds(size_t _op, size_t _decl, size_t _unary) : op_id(_op), decl_id(_decl), unary_id(_unary) {}

        size_t op_id;
        size_t decl_id;
        size_t unary_id;
    };
    
    size_t var_decl_id = 0;

    // xTx <- Binary operator, ctx.node->right and ctx.node->left are what's on your right and left
    BinaryOpIds add_binary_operator(char c, const std::string& f,int left_bp, int right_bp) {
        size_t id = add_token(c,f);
        left_binding_power.put(id, left_bp);
        right_binding_power.put(id, right_bp);

        size_t decl_id = reg::new_type(f+"_DECL");
        size_t unary_id = reg::new_type(f+"_UNARY");
    

        t_functions.put(id,[decl_id,unary_id](Context& ctx){
        auto& children = ctx.node->children;
            parse_sub_nodes(ctx); //Note to self: this will cause names to be thought of as undefined and potentially get entered into HM, we should probably only resolve left or right if it has children.
            if(children.length() == 2) {
                g_ptr<Node> type_term = children[0];
                g_ptr<Node> id_term = children[1];
                
                if(type_term->type==var_decl_id) {
                    ctx.node->type = decl_id;
                    ctx.node->value = type_term->value;
                    ctx.node->name = id_term->name;
                    ctx.node->value->sub_type = 0;
                    ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                    ctx.node->children.clear();
                }
            } else if(children.length() == 1) {
                g_ptr<Node> type_term = children[0];

                ctx.node->type = unary_id;
                ctx.node->value->copy(type_term->value);
            }
        });
        
        return BinaryOpIds(id,decl_id,unary_id);
    }


    g_ptr<Value> make_qual_value(const std::string& f, size_t size = 0) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t id = reg::new_type(uppercase_name);
        g_ptr<Value> val = make<Value>(id,size,id);
        return val;
    }

    g_ptr<Value> make_type(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_qual_value(f,size);
        t_prefix_functions.put(val->type,[](Context& ctx){
            ctx.value->sub_type = ctx.qual->sub_type;
            ctx.value->type = ctx.qual->type;
            ctx.value->size = ctx.qual->size;
            if(ctx.qual->type_scope)
                ctx.value->type_scope = ctx.qual->type_scope;
        });
        return  val;
    }

    size_t qual_id = 0;
    size_t add_qualifier(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_qual_value(f,size);
        keywords.put(f,val);
        return val->type;
    }
        
    size_t add_type(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_type(f,size);
        keywords.put(f,val);
        return val->type;
    }

    template<typename Op>
    static size_t add_function(const std::string& f, Op op, uint32_t return_type = 0) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        g_ptr<Value> val = make_value(uppercase_name,0,return_type);
        keywords.put(f,val);
        size_t call_id = val->sub_type;

        exec_handlers.put(call_id, op);
        return call_id;
    }

    size_t end_id = 0;
    g_ptr<Node> a_parse_expression(Context& ctx, int min_bp, g_ptr<Node> left_node = nullptr) {
        //Prefixual pass
        if(ctx.index>=ctx.nodes.length()) return nullptr;
        // while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() == end_id) {
        //     ctx.index++;
        // }
        // if(ctx.index>=ctx.nodes.length()) return nullptr;
        g_ptr<Node> token = ctx.nodes[ctx.index];

        newline("a_parse_expression_pass: "+token->info());

        uint32_t type = token->getType();
        int left_bp = left_binding_power.getOrDefault(type,-1);
        int right_bp = right_binding_power.getOrDefault(type,-1);
        bool has_func = a_functions.hasKey(type);

        // log("Starting at index ",ctx.index,", token: ",token->info()," (left_bp: ",left_bp,", right_bp: ",right_bp,", has_func: ",has_func?"yes":"no",")");
        // if(left_node)
        //     log("Left_node: ",left_node->info());
        // if(ctx.left) 
        //     log("ctx.left: ",ctx.left->info());
        // if(ctx.node) 
        //     log("ctx.node: ",ctx.node->info());

        ctx.index++;
        // if(ctx.index<ctx.nodes.length())
        //     log("Looking at index ",ctx.index,", token: ",ctx.nodes[ctx.index]->info());

        if(!left_node)
            left_node = make<Node>();

        if(has_func) { //Has a function but no left: direct node build
            ctx.left = token;
            ctx.node = make<Node>();  //Fresh output target
            //newline("Running a_function for "+token->info());
            a_functions.get(type)(ctx);
            // if(ctx.node) 
            //     log("Function returned:\n",ctx.node->to_string(1));
            //endline();
            left_node = ctx.node; //Read result back
        }
        else if(right_bp!=-1) { //Prefixual unary: recurse with right
            g_ptr<Node> right_node = a_parse_expression(
                ctx,
                right_bp,
                nullptr
            );
            left_node->type = type;
            if(right_node)
                left_node->children << right_node;
        }
        else { //Else atom, like a literal or idenitifer
            left_node = token;
            ctx.node = left_node;
        }

        //Infixual pass
        //newline("Running infix");
        while(ctx.index < ctx.nodes.length()) {
            g_ptr<Node> op = ctx.nodes[ctx.index];
            int op_left_bp = left_binding_power.getOrDefault(op->getType(), -1);

            //log("In infix at index ",ctx.index,", op: ",op->info()," (left_bp: ",op_left_bp,")");

            if(op_left_bp < min_bp || op_left_bp==-1) {
                break;
            }
            
            ctx.index++;

            //if(ctx.index<ctx.nodes.length()) log("Looking at index ",ctx.index,", token: ",ctx.nodes[ctx.index]->info());

            if(a_functions.hasKey(op->getType())) {
                if(left_node->type!=0)
                    ctx.left = left_node;
                ctx.node = make<Node>();
                //newline("Running function in infix from: "+op->info());
                a_functions.get(op->getType())(ctx);
                // if(ctx.node) 
                //     log("Infix function returned:\n",ctx.node->to_string(1));
               // endline();
                left_node = ctx.node;
            } else {
                g_ptr<Node> right_node = a_parse_expression(
                    ctx,
                    right_binding_power.getOrDefault(op->getType(), op_left_bp + 1),
                    nullptr
                );
                
                g_ptr<Node> node = make<Node>();
                node->type = op->getType();
                if(left_node)
                    node->children << left_node;
                if(right_node) {
                    if(!discard_types.hasKey(right_node->type))
                        node->children << right_node;
                }
                left_node = node;
                
            }
        }
        //log("Ended infix loop at index ",ctx.index,ctx.index<ctx.nodes.length()?", token: "+ctx.nodes[ctx.index]->info():" OVERSHOT INDEX!!");
        //endline();
        if(left_node) {
            ctx.node = left_node;
            //log("Returning left node:\n",left_node->to_string(4));
        }
        endline();
        return left_node;
    }

    size_t identifier_id = 0;
    void register_bracket_pair(size_t open_id, size_t close_id) {   
        a_functions.put(open_id, [open_id, close_id](Context& ctx) {
            list<g_ptr<Node>>* main_result = ctx.result;
            g_ptr<Node> result_node = nullptr;
            if(ctx.left && ctx.left->type != 0 && ctx.left->type != open_id) {
                result_node = ctx.left;
                if(!ctx.left->children.empty())
                    ctx.left = ctx.left->children.last();
                ctx.result = &ctx.left->children;
            } else {
                result_node = make<Node>();
                ctx.result = &result_node->children;
            }
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != close_id) {
                g_ptr<Node> inner = a_parse_expression(ctx, 0);
                if(inner && !discard_types.getOrDefault(inner->getType(), false))
                    ctx.result->push(inner);
            }
            ctx.result = main_result;
            if(result_node && discard_types.getOrDefault(result_node->getType(), false) && !result_node->children.empty()) {
                g_ptr<Node> to_promote = result_node->children.take(0);
                to_promote->children << result_node->children;
                result_node = to_promote;
            }
            ctx.node = result_node;
            ctx.index++;

            while(ctx.index < ctx.nodes.length() && 
            ctx.nodes[ctx.index]->getType() == identifier_id) {
            ctx.node->children << ctx.nodes[ctx.index];
            ctx.index++;
            }
        });
    }


    void register_other_kind_bracket_pair(size_t open_id, size_t close_id) {   
        a_functions.put(open_id, [open_id, close_id](Context& ctx) {
            list<g_ptr<Node>>* main_result = ctx.result;
            g_ptr<Node> result_node = nullptr;
            if(ctx.left && ctx.left->type != 0) {
                result_node = ctx.left;
                if(!ctx.left->children.empty())
                    ctx.left = ctx.left->children.last();
                ctx.result = &ctx.left->children;
            } else {
                result_node = make<Node>();
                ctx.result = &result_node->children;
            }
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != close_id) {
                g_ptr<Node> inner = a_parse_expression(ctx, 0);
                if(inner && !discard_types.getOrDefault(inner->getType(), false))
                    ctx.result->push(inner);
            }
            ctx.result = main_result;
            ctx.node = result_node;
            ctx.index++;
        });
    }

    void test_module(const std::string& path) {
        span = make<Log::Span>();

        a_parse_function = [](Context& ctx) {
            g_ptr<Node> expr = a_parse_expression(ctx, 0);
            if(expr && !discard_types.getOrDefault(expr->getType(),false)) {
                ctx.result->push(expr);
                //ctx.index--;
            }
        };

        size_t undefined_id = reg::new_type("UNDEFINED");
        qual_id = reg::new_type("QUALIFIER");
        discard_types.put(undefined_id,true);
        size_t literal_id = reg::new_type("LITERAL");
        exec_handlers.put(literal_id, [](Context& ctx){});
        end_id = add_token(';',"END");
        discard_types.put(end_id,true);

        var_decl_id = reg::new_type("VAR_DECL");
        size_t object_id = reg::new_type("OBJECT");
        value_to_string.put(object_id, [](void* data) {
            return std::string("[object @") + std::to_string((size_t)data) + "]";
        });

        identifier_id = reg::new_type("IDENTIFIER");
        a_functions.put(identifier_id,[](Context& ctx){
            if(ctx.index>=ctx.nodes.length()) {
                log("Hey! Index overun by ",ctx.left->info());
                return;
            }

            if(ctx.left) {
                //log("Left looks like: ",ctx.left->info());
                if(ctx.nodes[ctx.index]->getType()==identifier_id) {
                    ctx.node = ctx.left;
                    while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() == identifier_id) {
                        ctx.node->children << ctx.nodes[ctx.index];
                        ctx.index++;
                    }
                    //log("Folded into the left, forming:\n",ctx.left->to_string(1));
                    //a_parse_expression(ctx,0,nullptr);
                } else {
                    //log("Not an identifier, so no folding");
                    ctx.node = ctx.left;
                }
                //log("Ending on: ",ctx.nodes[ctx.index]->info());
            } else {
                //log("Nothing to my left");
            }
        });
        size_t function_id = reg::new_type("FUNCTION");
        size_t method_id = reg::new_type("METHOD");
        size_t func_decl_id = reg::new_type("FUNC_DECL");


        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        discard_types.put(lparen_id,true);
        discard_types.put(rparen_id,true);
        discard_types.put(comma_id,true);
        left_binding_power.put(lparen_id, 10);
        right_binding_power.put(lparen_id, 0);
        register_bracket_pair(lparen_id,rparen_id);
      

        size_t float_id = add_type("float",4);
        value_to_string.put(float_id,[](void* data) {
            return std::to_string(*(float*)data);
        });
        negate_value.put(float_id,[](void* data) {
            *(float*)data = -(*(float*)data);
        });
        t_functions.put(float_id, [float_id,literal_id](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(float_id,4);
            value->set<float>(std::stoi(ctx.node->name));
            ctx.node->value = value;
        }); 

        size_t int_id = add_type("int",4);
        value_to_string.put(int_id,[](void* data) {
            return std::to_string(*(int*)data);
        });
        negate_value.put(int_id,[](void* data) {
            *(int*)data = -(*(int*)data);
        });
        t_functions.put(int_id, [int_id,literal_id](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(int_id,4);
            value->set<int>(std::stoi(ctx.node->name));
            ctx.node->value = value;
        }); 

        size_t bool_id = add_type("bool",1);
        value_to_string.put(bool_id,[](void* data){
            return (*(bool*)data) ? "TRUE" : "FALSE";
        });
        t_functions.put(bool_id, [bool_id,literal_id](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(bool_id,1);
            value->set<bool>(ctx.node->name=="true" ? true : false); 
            ctx.node->value = value;
        }); 

        size_t string_id = add_type("string",24);
        size_t in_string_id = reg::new_type("IN_STRING_KEY");
        tokenizer_state_functions.put(in_string_id,[](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(c=='"') {
                ctx.state=0;
            }
            else {
                ctx.node->name += c;
            }
        });
        tokenizer_functions.put('"',[string_id,in_string_id](Context& ctx) {
            ctx.state = in_string_id;
            ctx.node = make<Node>();
            ctx.node->type = string_id;
            ctx.node->name = "";
            ctx.result->push(ctx.node);
        });
        value_to_string.put(string_id,[](void* data){
            return *(std::string*)data;
        });
        t_functions.put(string_id, [string_id,literal_id](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(string_id,24);
            value->set<std::string>(ctx.node->name);
            ctx.node->value = value;
        }); 
           
        BinaryOpIds plus_ids = add_binary_operator('+',"PLUS", 4, 5);
        exec_handlers.put(plus_ids.op_id,make_arithmetic_handler<int, int>([](auto a, auto b){return a+b;},int_id));

        BinaryOpIds dash_ids = add_binary_operator('-',"DASH", 4, 5);
        exec_handlers.put(dash_ids.op_id,make_arithmetic_handler<int, int>([](auto a, auto b){return a-b;},int_id));

        BinaryOpIds rangle_ids = add_binary_operator('>',"RANGLE", 2, 3);
        exec_handlers.put(rangle_ids.op_id,make_arithmetic_handler<int, bool>([](auto a, auto b){return a>b;},bool_id));

        BinaryOpIds langle_ids = add_binary_operator('<',"LANGLE", 2, 3);
        exec_handlers.put(langle_ids.op_id,make_arithmetic_handler<int, bool>([](auto a, auto b){return a<b;},bool_id));

        register_other_kind_bracket_pair(langle_ids.op_id,rangle_ids.op_id);

        std::function<void(list<g_ptr<Node>>&,bool)> t_pass_register_generics = [rangle_ids,langle_ids,&t_pass_register_generics](list<g_ptr<Node>>& nodes, bool in_generic){
            size_t rangle = rangle_ids.op_id;
            size_t langle = langle_ids.op_id;

            list<int> wishlist;
            int start = -1;
            for(int i=0;i<nodes.length();i++) {


                if(!nodes[i]->children.empty()) {
                    t_pass_register_generics(nodes[i]->children, nodes[i]->type==langle);
                    // // After recursing, if last child is a RANGLE, extract it
                    // if(nodes[i]->children.last()->type == rangle) {
                    //     g_ptr<Node> extracted = nodes[i]->children.pop();
                    //     nodes.insert(extracted, i+1);
                    // }
                }

                if(start!=-1) {
                    if(nodes[i]->type==rangle) {
                        bool already_stitched = !nodes[i]->children.empty() && 
                                                nodes[i]->children[0]->type == langle;
                        if(already_stitched) {
                            wishlist << i; // treat as middle param, it's a complete nested generic
                        } else {
                            // This is our closer
                            g_ptr<Node> rangle_node = nodes[i];
                            for(int n=wishlist.length()-1;n>=0;n--) {
                                nodes[start]->children << nodes.take(wishlist[n]);
                            }
                            while(rangle_node->children.length()>1) {
                                nodes[start]->children << rangle_node->children.take(0);
                            }
                            rangle_node->children.insert(nodes.take(start),0);
                            start = -1;
                            wishlist.clear();
                        }
                    } else {
                        wishlist << i;
                    }
                } else if(nodes[i]->type==langle) {
                    start = i;
                } 
            }
        };
        
        BinaryOpIds equals_ids = add_binary_operator('=', "ASSIGNMENT", 1, 0);
        t_functions.set(equals_ids.op_id, [](Context& ctx){parse_sub_nodes(ctx);}); //So we don't turn things into declerations
        exec_handlers.put(equals_ids.op_id,[](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            execute_r_node(ctx.node->right, ctx.frame);

            //print("Assinging from:\n",ctx.node->right->to_string(1),"\nto\n",ctx.node->left->to_string(1));
            memcpy(ctx.node->left->value->data, ctx.node->right->value->data, ctx.node->right->value->size);
            //print("Assignment finished, value is: ",ctx.node->left->value->info());
        });

        size_t func_call_id = reg::new_type("FUNC_CALL");
        r_handlers.put(func_call_id, [equals_ids](Context& ctx) {
            if(ctx.node->scope()) {
                for(int i = 0; i < ctx.node->children.size(); i++) {
                    g_ptr<Node> arg = resolve_symbol(ctx.node->children[i], ctx.root, ctx.frame);
                    g_ptr<Node> param = ctx.node->scope()->owner->children[i];
                    g_ptr<Node> assignment = make<Node>();
                    assignment->type = equals_ids.op_id;
                    assignment->children << param;
                    assignment->children << arg;
                    assignment->populate_lr();
                    ctx.node->children[i] = assignment;
                }
            }
        });
        exec_handlers.put(func_call_id, [](Context& ctx) {
            for(auto c : ctx.node->children) {
                execute_r_node(c, ctx.frame);
            }
            ctx.node->frame->return_to = ctx.node;
            execute_r_nodes(ctx.node->frame);
        });


        BinaryOpIds star_ids = add_binary_operator('*',"STAR", 6, 7);
        exec_handlers.put(star_ids.op_id,make_arithmetic_handler<int, int>([](auto a, auto b){return a*b;},int_id));
        BinaryOpIds amp_ids = add_binary_operator('&',"AMPERSAND",-1,8);

        discover_handlers.put(star_ids.decl_id, [](Context& ctx) {
            ctx.node->value->size = 8;
        });

        exec_handlers.put(star_ids.decl_id, [](Context& ctx) {
            if(!ctx.node->value->data) {
                ctx.node->value->data = malloc(8);
            }
            ctx.frame->active_memory << ctx.node->value->data;
        });
        
        exec_handlers.put(star_ids.unary_id, [](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            ctx.node->value->data = *(void**)ctx.node->left->value->data;
            ctx.node->value->type = ctx.node->left->value->type;
            ctx.node->value->size = ctx.node->left->value->size;
        });
        
        exec_handlers.put(amp_ids.unary_id, [](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            ctx.node->value->type = ctx.node->left->value->type;
            ctx.node->value->size = 8;
            ctx.node->value->set<void*>(ctx.node->left->value->data);
        });


        discover_handlers.put(star_ids.unary_id, [](Context& ctx) {
            discover_symbol(ctx.node->left,ctx.root);
            ctx.node->value->copy(ctx.node->left->value);
        });

        BinaryOpIds dot_ids = add_binary_operator('.', "PROP_ACCESS", 8, 9);
        t_functions.set(dot_ids.op_id, [amp_ids,func_call_id](Context& ctx) {
            g_ptr<Node> left = ctx.node->children[0];
            g_ptr<Node> right = ctx.node->children[1];
            parse_a_node(left, ctx.root);
            // log("Giving at:\n",left->to_string(1),"\nto\n",right->to_string(1));

            if(left->value->type_scope) {
                right->in_scope = left->value->type_scope;
            } else  {
                //log(red("NO SCOPE SEEN!"));
            }

            if(right) {
                parse_a_node(right, ctx.root);

                //The &this that gets passed to object calls.
                //we inject it fully formed here because right won't parse it's children again, it already has (has no scope)
                if(right->type == func_call_id) {
                    g_ptr<Node> amp = make<Node>();
                    amp->type = amp_ids.unary_id;
                    amp->value->copy(left->value);
                    amp->children << left;
                    amp->left = left;
                    right->children.insert(amp, 0);
                }

                ctx.node->value->copy(right->value);
            }
        });
        exec_handlers.put(dot_ids.op_id,[func_call_id](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            execute_r_node(ctx.node->right, ctx.frame);
            ctx.node->value->type = ctx.node->right->value->type;
            ctx.node->value->size = ctx.node->right->value->size;
            if(ctx.node->right->type == func_call_id) {
                ctx.node->value->data = ctx.node->right->value->data;
            } else {
                ctx.node->value->data = (char*)ctx.node->left->value->data + ctx.node->right->value->address;
            }
        });

        //-> for sugar
        r_handlers.put(dash_ids.op_id, [dash_ids, rangle_ids, star_ids, dot_ids](Context& ctx) {
            if(ctx.node->right && ctx.node->right->type == rangle_ids.unary_id) {
                ctx.node->type = dot_ids.op_id;
                g_ptr<Node> star = make<Node>();
                star->type = star_ids.unary_id;
                star->children << ctx.node->left;
                star->left = ctx.node->left;
                ctx.node->left = star;
                ctx.node->right = ctx.node->right->left;
                print("Assembled node: ",ctx.node->to_string());
            } else {
                resolve_sub_nodes(ctx);
            }
        });
        
        size_t return_id = make_tokenized_keyword("return");
        register_other_kind_bracket_pair(return_id,end_id);
        exec_handlers.put(return_id, [](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            g_ptr<Value> ret_val = make<Value>(ctx.node->left->value->type);
            ret_val->size = ctx.node->left->value->size;
            ret_val->data = malloc(ret_val->size);
            memcpy(ret_val->data, ctx.node->left->value->data, ret_val->size);
            ctx.frame->return_to->value = ret_val;
            ctx.frame->stop();
        });

        size_t break_id = make_tokenized_keyword("break");
        exec_handlers.put(break_id, [](Context& ctx) {
            ctx.frame->stop();
        });


        size_t lbracket_id = add_token('[', "LBRACKET");
        size_t rbracket_id = add_token(']', "RBRACKET");
        left_binding_power.put(lbracket_id, 10);
        a_functions.put(lbracket_id, [lbracket_id, rbracket_id](Context& ctx) {
            g_ptr<Node> result_node = make<Node>();
            result_node->type = lbracket_id;
            
            if(ctx.left && ctx.left->type != 0) {
                result_node->children << ctx.left;
            }
            
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != rbracket_id) {
                g_ptr<Node> expr = a_parse_expression(ctx, 0);
                if(expr) result_node->children << expr;
            }
            ctx.index++; // consume ']'
            
            ctx.node = result_node;
        });
        t_functions.put(lbracket_id, [lbracket_id](Context& ctx) {
            parse_sub_nodes(ctx);
            auto& children = ctx.node->children;
            
            if(children[0]->type == var_decl_id) {
                //Declaration: int a[3]
                ctx.node->type = var_decl_id;
                ctx.node->name = children[0]->name;
                ctx.node->value = children[0]->value;
                ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                ctx.node->value->sub_type = 0;
                g_ptr<Node> size_expr = children[1];
                ctx.node->children.clear();
                ctx.node->children << size_expr;
            } else {
                //Just an access case
            }
        });
        exec_handlers.put(lbracket_id, [lbracket_id](Context& ctx) {
            execute_r_node(ctx.node->children[0], ctx.frame); // base
            execute_r_node(ctx.node->children[1], ctx.frame); // index
            
            int i = ctx.node->children[1]->value->get<int>();
            size_t element_size = ctx.node->children[0]->value->size;
            
            ctx.node->value->type = ctx.node->children[0]->value->type;
            ctx.node->value->size = element_size;
            ctx.node->value->type_scope = ctx.node->children[0]->value->type_scope;
            ctx.node->value->data = (char*)ctx.node->children[0]->value->data + i * element_size;
        });

        r_handlers.put(var_decl_id, [](Context& ctx) {
            //Do nothing
        });
        exec_handlers.put(var_decl_id, [](Context& ctx) {
            if(!ctx.node->value->data) {
                size_t alloc_size = ctx.node->value->size;
                if(alloc_size == 0 && ctx.node->value->type_scope) {
                    alloc_size = ctx.node->value->type_scope->owner->value->size;
                    ctx.node->value->size = alloc_size;
                }
                if(!ctx.node->children.empty()) { //Arrays
                    execute_r_node(ctx.node->children[0], ctx.frame);
                    int count = ctx.node->children[0]->value->get<int>();
                    alloc_size *= count;
                }
                ctx.node->value->data = malloc(alloc_size);
            }
            ctx.frame->active_memory << ctx.node->value->data;
        });

        size_t type_decl_id = reg::new_type("TYPE_DECL");
        discover_handlers.put(type_decl_id,[star_ids](Context& ctx){
            g_ptr<Node> node  = ctx.node;
            for(auto child : node->scope()->children) {
                child->value->address = node->value->size;
                if(child->type == var_decl_id) {
                    if(child->value->type_scope) {
                        node->value->size+=child->value->type_scope->owner->value->size; //Note to self: too indirect, should make this path more direct at some point
                    } else {
                        node->value->size+=child->value->size;
                    }
                } else if(child->type == star_ids.decl_id) {
                    node->value->size+=8;
                }
            }
        });
        r_handlers.put(type_decl_id, [type_decl_id](Context& ctx) {
            ctx.node->frame = ctx.node->scope()->frame;
        });
        exec_handlers.put(type_decl_id, [](Context& ctx){});
        r_handlers.put(func_decl_id, [func_decl_id](Context& ctx) {
            for(auto c : ctx.node->children) {
                g_ptr<Node> child = resolve_symbol(c, ctx.node->scope(),  ctx.node->frame);
            }
        });
        exec_handlers.put(func_decl_id, [](Context& ctx){});


        size_t method_scope_id = reg::new_type("METHOD_SCOPE");
        size_t type_scope_id = reg::new_type("TYPE_SCOPE");

        
        scope_link_handlers.put(identifier_id,[](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
            new_scope->owner = owner_node.getPtr();
            owner_node->scopes << new_scope.getPtr();
            for(auto c : owner_node->children) {
                c->place_in_scope(new_scope.getPtr());
            }
            new_scope->name = owner_node->name;
        });

        std::function<void(g_ptr<Node>)> inject_this_param = [star_ids](g_ptr<Node> node) {
            g_ptr<Node> star = make<Node>();
            star->type = star_ids.op_id;
            star->place_in_scope(node->scope().getPtr());

            g_ptr<Node> type_term = make<Node>();
            type_term->type = identifier_id;
            type_term->name = node->in_scope->owner->name;
            type_term->place_in_scope(node->scope().getPtr());
            star->children << type_term;

            g_ptr<Node> id_term = make<Node>();
            id_term->type = identifier_id;
            id_term->name = "this";
            id_term->place_in_scope(node->scope().getPtr());
            star->children << id_term;

            node->children.insert(star, 0);
        };
        std::function<void(g_ptr<Node>)>  inject_member_access = [dot_ids,star_ids](g_ptr<Node> node) {
            g_ptr<Node> prop = make<Node>();
            prop->type = dot_ids.op_id;
            prop->place_in_scope(node->in_scope);
            
            g_ptr<Node> star = make<Node>();
            star->type = star_ids.op_id;
            star->place_in_scope(node->in_scope);
            
            g_ptr<Node> this_id = make<Node>();
            this_id->type = identifier_id;
            this_id->name = "this";
            this_id->place_in_scope(node->in_scope);
            star->children << this_id;
            
            g_ptr<Node> member_id = make<Node>();
            member_id->type = identifier_id;
            member_id->name = node->name;
            member_id->place_in_scope(node->in_scope);
            
            prop->children << star;
            prop->children << member_id;
            //We parse this node because it won't resolve again like something in a type scope would
            parse_a_node(prop, node->in_scope);
            node->copy(prop);
        };


        t_functions.put(identifier_id, [type_decl_id,object_id,func_call_id,func_decl_id,method_scope_id,type_scope_id,&inject_this_param,&inject_member_access](Context& ctx) {
            //log("Parsing an idenitifer:\n",ctx.node->to_string(1));
            g_ptr<Node> node = ctx.node;
            g_ptr<Value> decl_value = make<Value>();
            
            bool had_a_value = (bool)(node->value);
            bool had_a_scope = (bool)(node->scope());
            bool found_a_value = node->find_value_in_scope();
            bool is_qualifier = node->value_is_valid();

            int root_idx = -1;
            if(node->value_is_valid() && node->value->sub_type != 0) {
                //log("I am a qualifier");
                decl_value->quals << node->value;
                for(int i = 0; i < node->children.length(); i++) {
                    g_ptr<Node> c = node->children[i];
                    c->find_value_in_scope();
                    if(c->value_is_valid()) {
                        decl_value->quals << c->value;
                    } else {
                        root_idx = i;
                        break;
                    }
                }
                if(root_idx!=-1) {
                    g_ptr<Node> root = node->children[root_idx];
                    node->name = root->name;
                    for(int i = root_idx+1; i < node->children.length(); i++) {
                        g_ptr<Node> c = node->children[i];
                        c->find_value_in_scope();
                        if(c->value_is_valid()) {
                            node->quals << c->value;
                        } 
                    }
                    node->children = node->children.take(root_idx)->children;
                }
            } else {
                //log("no valid value, I am the root");
            }

            if(!node->scope()) { //Defer, the r_stage will do this later for scoped nodes
                parse_sub_nodes(ctx);
            }

            if(keywords.hasKey(node->name)) {
                node->type = node->value->sub_type;
                return;
            }

            node->value = decl_value;
            parse_quals(ctx, decl_value);

            bool has_scope = node->scope() != nullptr;
            bool has_type_scope = node->value->type_scope != nullptr;
            bool has_sub_type = node->value->sub_type != 0;
            
            if(has_scope) {
                node->scope()->owner = node.getPtr();
                node->scope()->name = node->name;
                if(has_sub_type) {
                    node->type = func_decl_id;
                    node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
                    node->value->type_scope = node->scope().getPtr();
                    node->value = node->in_scope->distribute_value(node->name,node->value);
                    node->value->sub_type = 0;

                    if(node->in_scope->type==type_scope_id) {
                        node->scope()->type = method_scope_id;
                    }

                    //The 'this' field passed to a func_decl with members
                    if(node->in_scope->value_table.hasKey(node->in_scope->name)) {
                        inject_this_param(node);
                    }
                } else {
                    node->type = type_decl_id;
                    node->value->copy(make_type(node->name,0));
                    node->value->type_scope = node->scope().getPtr();
                    node->value = node->in_scope->distribute_value(node->name,node->value);

                    node->scope()->type = type_scope_id;
                }
            } else {
                has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node->type = var_decl_id;
                    if(node->in_scope->type==type_scope_id) {
                        node->in_scope->value_table.put(node->name, decl_value);
                    } else {
                        node->value = node->in_scope->distribute_value(node->name, decl_value);
                    }
                    node->value->sub_type = 0;
                } else if(has_scope) {
                    node->type = func_call_id;
                    node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(node->value->type_scope)
                        node->scopes[0] = node->value->type_scope; //Swap to the type scope
                } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
                    node->find_value_in_scope();
                } else {
                    if(node->in_scope->value_table.hasKey("this")) {
                        inject_member_access(node);
                    } else {
                        //HM Tracing goes here.
                        //Attatch a qual with handlers for it
                        //Borrow checker too, maybe not here though.
                    }
                }
            }

            // log("Returning:\n",node->to_string(1));
        });

        exec_handlers.put(identifier_id, [](Context& ctx){});

        size_t lbrace_id = add_token('{', "LBRACE");
        scope_precedence.put(lbrace_id, 10);
        size_t rbrace_id = add_token('}', "RBRACE");
        scope_precedence.put(rbrace_id, -10);

        char_is_split.put(' ',true);
        size_t in_alpha_id = reg::new_type("IN_ALPHA");
        tokenizer_state_functions.put(in_alpha_id,[](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.index;
                ctx.node->type = tokenized_keywords.getOrDefault(ctx.node->name,ctx.node->type);
                return;
            } else {
                ctx.node->name += c;
            }
        });

        size_t in_digit_id = reg::new_type("IN_DIGIT");
        tokenizer_state_functions.put(in_digit_id,[in_alpha_id,float_id](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.index;
                return;
            } else if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
            } else if(c=='.') {
                ctx.node->type = float_id;
            }
            ctx.node->name += c;
        });

        tokenizer_default_function =[in_alpha_id,in_digit_id,int_id](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
                ctx.node = make<Node>(identifier_id,c);
                ctx.result->push(ctx.node);
            }
            else if(std::isdigit(c)) {
                ctx.state = in_digit_id;
                ctx.node = make<Node>(int_id,c);
                ctx.result->push(ctx.node);
            } else if(c==' '||c=='\t'||c=='\n') {
                //just skip
            }
            else {
                print("tokenize::default_function missing handling for char: ",c);
            }
        };

        t_default_function = [](Context& ctx) {
            print("TRIGGERED T DEFAULT");
            parse_sub_nodes(ctx);
        };

        r_default_function = [](Context& ctx) {
            resolve_sub_nodes(ctx);
        };

        add_function("print",[](Context& ctx) {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                execute_r_node(r, ctx.frame);
                toPrint.append(r->value->to_string());
            }
            print(toPrint);
        });

        size_t if_id = add_scoped_keyword("if", 2, [](Context& ctx) {
            execute_r_node(ctx.node->left, ctx.frame);
            if(ctx.node->left->value->is_true()) {
                execute_r_nodes(ctx.node->frame);
            }
            else if(ctx.node->right) {
                execute_r_nodes(ctx.node->right->frame);
            }
        });
        size_t else_id = add_scoped_keyword("else", 1, [](Context& ctx){
            //This doesn't ever really execute
        });
        r_handlers.put(else_id, [if_id](Context& ctx) {
            int my_id = ctx.root->children.find(ctx.node);
            if(my_id>0) {
                g_ptr<Node> left = ctx.root->children[my_id-1];
                if(left->type==if_id) {
                    left->children << ctx.root->children.take(my_id);
                    left->right = ctx.node;
                }
            }
            ctx.node = nullptr;
        });

        size_t while_id = add_scoped_keyword("while", 2, [](Context& ctx) {
            ctx.node->frame->resurrect();
            execute_r_node(ctx.node->left, ctx.frame);
            while(ctx.node->left->value->is_true()) {
                execute_r_nodes(ctx.node->frame);
                ctx.node->frame->resurrect();
                execute_r_node(ctx.node->left, ctx.frame);
            }
        });
        


        add_qualifier("wubless");
        add_qualifier("wubfull");
        add_qualifier("const");
        add_qualifier("static");
        add_qualifier("inline");
        add_qualifier("struct");
        add_qualifier("type");

        std::function<void(g_ptr<Node>)> print_scopes = [&print_scopes](g_ptr<Node> root) {
            for(auto t : root->scopes) {
                print(t->to_string());
            }
            for(auto child_scope : root->scopes) {
                print_scopes(child_scope);
            }
        };


        span->print_on_line_end = false; //While things aren't crashing
       // span->log_everything = true; //While things are crashing


        std::string code = readFile(path);
        Log::Line timer; timer.start();
        print("TOKENIZE");
        list<g_ptr<Node>> tokens = tokenize(code);
        print("A STAGE");
        list<g_ptr<Node>> nodes = parse_tokens(tokens);
        a_pass_resolve_keywords(nodes);
        print("==Keyword resolved nodes==");
        for(auto a : nodes) {
            print(a->to_string(1));
        }

        // t_pass_register_generics(nodes,false);
        // print("==After stiching==");
        // for(auto a : nodes) {
        //     print(a->to_string(1));
        // }

        print("S STAGE");
        g_ptr<Node> root = parse_scope(nodes);

        //print(root->to_string(0,0,true));

        print("T STAGE");
        parse_nodes(root);

        //print_nodes_from_scopes(root);

        // print("==LOG==");
        // span->print_all();

        //log(root->to_string(0,0,true));


        newline("Discovering symbols");
        print("D STAGE");
        discover_symbols(root);
        endline();
        print("R STAGE");
        g_ptr<Frame> frame = resolve_symbols(root);

        std::string final_time = ftime(timer.end());

        //log(root->to_string(0,0,true));
        print("==LOG==");
        span->print_all();

        print_scopes(root);

        timer.start();

        print("==EXECUTING==");
        newline("Executing");
        execute_r_nodes(frame);
        endline();
        print("Exec time: ",ftime(timer.end()));
        print("Final time: ",final_time);

        


        // span->print_all();

    }
}
using namespace GDSL;

int main() {

    // GDSL::helper_test_module::initialize();
    test_module(root()+"/Projects/Testing/src/test.gld");
    // g_ptr<GDSL::Frame> frame = compile(root()+"/Projects/Testing/src/test.gld");
    // execute_r_nodes(frame);


    // int t[3][2] = {{2,3},{3,2},{10,8}};
    // for(int i=0;i<3;i++) {
    //     list<g_ptr<Node>> tokens = tokenize(
    //         "int a[0];"
    //         "int b[1][2];"
    //         "b[0];"
    //         "a[1][2];"
    //     );
    //     left_binding_power.set(bracket_ids.op_id,t[i][0]);
    //     right_binding_power.set(bracket_ids.op_id,t[i][1]);

    //     left_binding_power.set(lbracket_ids.op_id,t[i][0]);
    //     right_binding_power.set(lbracket_ids.op_id,t[i][1]);
    //     list<g_ptr<Node>> nodes = parse_tokens(tokens);
    //     print(i,": ",t[i][0],"|",t[i][1]);
    //     for(auto n : nodes) {
    //         print(n->to_string(1));
    //     }
    // }


    // add_function("layout", [int_id](Context& ctx) {
    //     int mode = 1;
    //     if(!ctx.node->children.empty()) {
    //         execute_r_node(ctx.node->children[0],ctx.frame);
    //         if(ctx.node->children.length()>1) {
    //             execute_r_node(ctx.node->children[1],ctx.frame);
    //             mode = ctx.node->children[1]->value->get<int>();
    //             print(ctx.node->children[0]->frame->context->type_to_string(mode));
    //         }
    //         else {
    //             if(ctx.node->children[0]->value->type==int_id) {
    //                 mode = ctx.node->children[0]->value->get<int>();
    //                 print(ctx.frame->context->type_to_string(mode));
    //             } else {
    //                 print(ctx.node->children[0]->frame->context->type_to_string(mode));
    //             }
    //         }
    //     }
    //     else
    //         print(ctx.frame->context->type_to_string(mode));
    // });

        // discover_handlers.put(star_id, [var_decl_id,pointer_id](g_ptr<Node> node, Context& ctx) {
        //     if(node->left && node->left->type == var_decl_id) {
        //         g_ptr<Value> ptr_value = make<Value>(pointer_id);
        //         ptr_value->size = 8;
        //         ptr_value->data = malloc(8);
        //         ptr_value->sub_type = node->left->value->type;
        //         node->left->scope->values.put(node->right->name, ptr_value);
        //     }
        // });
        // size_t star_id = add_token('*',"STAR");
        // a_functions.put(star_id, [star_id, var_decl_id](Context& ctx) {
        //     if(ctx.state == var_decl_id && ctx.node->tokens.size() == 1) {
        //         ctx.state = star_id;
        //         return;
        //     }
        //     a_default_function(ctx);
        // });
        // state_is_opp.put(star_id, true);
        // type_precdence.put(star_id, 3);
        // t_functions.put(star_id, [star_id, var_decl_id, identifier_id](Context& ctx) -> g_ptr<Node> {
        //     //Workaround because t_defualt_function and precedence needs more work
        //     if(ctx.node->tokens.size() == 1 && ctx.node->sub_nodes.empty()) {
        //         //*c — unary dereference
        //         g_ptr<Node> result = make<t_node>();
        //         result->type = star_id;
        //         g_ptr<Node> stub = make<a_node>();
        //         stub->tokens << ctx.node->tokens[0];
        //         stub->type = identifier_id;
        //         Context sub_ctx(result, stub, ctx.root);
        //         result->left = t_functions.get(identifier_id)(sub_ctx);
        //         return result;
        //     }
            
        //     return t_default_function(ctx);
        // });
        // r_handlers.put(star_id, [star_id, var_decl_id, pointer_id, deref_id, mul_id](g_ptr<Node> result, Context& ctx) {
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
        // exec_handlers.put(pointer_id, [](Context& ctx) -> g_ptr<Node> {
        //     ctx.frame->active_memory << ctx.node->value->data;
        //     return ctx.node;
        // });
        // exec_handlers.put(deref_id, [](Context& ctx) -> g_ptr<Node> {
        //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
        //     ctx.node->value->data = *(void**)ctx.node->left->value->data;
        //     ctx.node->value->type = ctx.node->left->value->sub_type;
        //     ctx.node->value->size = ctx.node->left->value->size;
        //     return ctx.node;
        // });

           // size_t index_id = add_binary_operator('[', "INDEX", 8, 9, [](Context& ctx) -> g_ptr<Node> {
        //     execute_r_node(ctx.node->left, ctx.frame, ctx.index, ctx.sub_index);
        //     execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
        //     int i = ctx.node->right->value->get<int>();
        //     size_t stride = ctx.node->left->value->size;
        //     ctx.node->value->data = (char*)ctx.node->left->value->data + i * stride;
        //     ctx.node->value->type = ctx.node->left->value->sub_type;
        //     ctx.node->value->size = stride;
        //     return ctx.node;
        // });
        // t_functions.put(index_id, [index_id, identifier_id, int_id, literal_id](Context& ctx) -> g_ptr<Node> {
        //     ctx.result->type = index_id;
        //     // left is the array identifier
        //     g_ptr<Node> left_stub = make<a_node>();
        //     left_stub->tokens << ctx.node->tokens[0];
        //     left_stub->type = identifier_id;
        //     ctx.result->left = parse_a_node(left_stub, ctx.root, nullptr);
        //     // right is the index literal
        //     g_ptr<Node> right_stub = make<a_node>();
        //     right_stub->tokens << ctx.node->tokens[1];
        //     right_stub->type = int_id;
        //     ctx.result->right = parse_a_node(right_stub, ctx.root, nullptr);
        //     return ctx.result;
        // });
        // size_t rbracket_id = add_token(']',"RBRACKET");
        // a_functions.put(rbracket_id, [](Context& ctx) {
        //     if(ctx.local && ctx.state != 0) {
        //         ctx.end_lambda();
        //         ctx.state = 0;
        //     } 
        // });
        // add_function("malloc", [pointer_id](Context& ctx) -> g_ptr<Node> {
        //     execute_r_node(ctx.node->children[0], ctx.frame, ctx.index, ctx.sub_index);
        //     int size = ctx.node->children[0]->value->get<int>();
        //     ctx.node->value->set<void*>(malloc(size));
        //     ctx.node->value->type = pointer_id;
        //     ctx.node->value->size = 8;
        //     return ctx.node;
        // });


    // struct Context {
    //     uint64_t padding = 18;
    //     uint64_t padding_2 = 45;
    //     uint64_t padding_3 = 19;
    //     uint64_t padding_4 = 10000;
    //     int index = 0;
    // };

    // map<uint32_t, std::function<void(Context&)>> std_fn_map;
    // map<uint32_t, void(*)(Context&)> raw_ptr_map;
    
    // // Register 1000 noise types first to stress the map
    // for(int i = 0; i < 1000; i++) {
    //     reg::new_type(sgen::randsgen(sgen::RANDOM));
    // }
    
    // // Now register test handlers with realistic capture sizes
    // uint32_t id_a = reg::new_type("TEST_A");
    // uint32_t id_b = reg::new_type("TEST_B");
    // uint32_t id_c = reg::new_type("TEST_C");
    
    // // Realistic capture: 3 IDs, like your actual handlers
    // std_fn_map.put(id_a, [id_a, id_b, id_c](Context& ctx) {
    //     ctx.index++;
    // });
    
    // // Raw ptr can't capture, so IDs have to live elsewhere
    // // This simulates the GET_TYPE() alternative
    // raw_ptr_map.put(id_a, [](Context& ctx) {
    //     uint32_t a = GET_TYPE(TEST_A);
    //     uint32_t b = GET_TYPE(TEST_B); 
    //     uint32_t c = GET_TYPE(TEST_C);
    //     ctx.index++;
    // });
    
    // Context test_ctx; // dummy context
    
    // Log::rig r;
    // r.add_process("std_fn_3capture_dispatch", [&](int i) {
    //     std_fn_map.get(id_a)(test_ctx);
    // });
    // r.add_process("raw_ptr_gettype_dispatch", [&](int i) {
    //     raw_ptr_map.get(id_a)(test_ctx);
    // });
    
    // r.add_comparison("std_fn_3capture_dispatch", "raw_ptr_gettype_dispatch");
    // r.run(1000, true, 100);

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