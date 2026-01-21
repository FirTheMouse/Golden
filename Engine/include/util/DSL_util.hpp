#pragma once

#include<rendering/scene.hpp>
#include<core/GDSL.hpp>

namespace GDSL 
{
    namespace helper_tokenizer_module 
    {
        static void initialize(char end_char = ';') {

        }
    }

    size_t add_token(char c,const std::string& token_name) {
        size_t t_id = reg::new_type(token_name);
        tokenizer_functions.put(c,[t_id,c](tokenizer_context& ctx){
            ctx.token = make<Token>(t_id,c);
            ctx.result << ctx.token;
        });
        return t_id;
    }

    static std::pair<size_t,size_t> reg_keyword(const std::string& keyword_name) {
        size_t key_id = reg::new_type(keyword_name+"_KEY");  
        size_t call_id = reg::new_type(keyword_name+"_CALL");

        std::string lowercase_name  = keyword_name;
        std::transform(lowercase_name.begin(),lowercase_name.end(), lowercase_name.begin(), ::tolower);
        //REGISTER LOWERCASE NAME AS A KEYWORD
        token_to_opp.put(key_id, call_id);
        state_is_opp.put(call_id, true);
        type_precdence.put(call_id, 20);

        return {key_id,call_id};
    }

    static void add_keyword(const std::string& keyword_name,exec_handler handler) {
        auto key_and_call = reg_keyword(keyword_name);
        size_t key_id = key_and_call.first;
        size_t call_id = key_and_call.second;

        size_t t_id = reg::new_type("T_"+keyword_name);
        t_opp_conversion.put(call_id, t_id); 
        t_functions.put(call_id, [t_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_id;
            return ctx.result;
        });
        size_t r_id = reg::new_type("R_"+keyword_name); 
        r_handlers.put(t_id, [r_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_id;
        });
        exec_handlers.put(r_id,handler);
    }

    template<typename T>
    T get_arg(exec_context& ctx, int index) {
        if(index >= ctx.node->children.length()) {
            print("GDSL::get_arg missing argument at index: ", index);
            return T{};
        }
        execute_r_node(ctx.node->children[index], ctx.frame, ctx.index, ctx.sub_index);
        return ctx.node->children[index]->value.get<T>();
    }

    // T() <- Function, your data appears in ctx.node->children[]
    template<typename Op>
    static size_t add_function(const std::string& f, Op op) {
        auto key_and_call = reg_keyword(f);
        size_t key_id = key_and_call.first;
        size_t call_id = key_and_call.second;

        size_t t_id = reg::new_type("T_"+f);
        t_opp_conversion.put(call_id, t_id); 
        t_functions.put(call_id, [t_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_id;
            parse_sub_nodes(ctx,true);
            return ctx.result;
        });
        size_t r_id = reg::new_type("R_"+f); 
        r_handlers.put(t_id, [r_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_id;
            if(ctx.node->left) 
                result->children << resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            if(ctx.node->right)
                result->children << resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            for(auto c : ctx.node->children) {
                g_ptr<r_node> sub = resolve_symbol(c, ctx.scope, ctx.frame);
                result->children << sub;
            }
        });
        exec_handlers.put(r_id,op);

        return key_id;
    }

    // T() {} <- As with function, but we ctx.node->frame contains the bracketed code for further execution.
    template<typename Op>
    static void add_scoped_keyword(const std::string& name, int scope_prec, Op exec_fn)
    {
        size_t key_id = reg::new_type(name + "_KEY");
        size_t decl_id = reg::new_type(name + "_DECL");
        size_t block_id = reg::new_type(name + "_BLOCK");
        
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        //REGISTER T_KEY
        
        // A-stage: set state to declaration
        a_functions.put(key_id, [decl_id](a_context& ctx) {
            ctx.state = decl_id;
        });
        
        // Scope linking
        scope_link_handlers.put(decl_id, [block_id](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            new_scope->scope_type = block_id;
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        });
        scope_precedence.put(decl_id, scope_prec);
        
        // T-stage: parse expression and attach scope
        size_t t_id = reg::new_type("T_" + name);
        t_opp_conversion.put(decl_id, t_id);
        t_functions.put(decl_id, [t_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> result = t_parse_expression(ctx.node, nullptr);
            result->scope = ctx.node->owned_scope;
            return result;
        });
        
        // R-stage: resolve condition and create frame
        size_t r_id = reg::new_type("R_" + name);
        r_handlers.put(t_id, [r_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_id;
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            result->frame = resolve_symbols(ctx.node->scope);
            if(ctx.node->left) {
                result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            }
            return result;
        });
        
        // Execution: user-defined
        exec_handlers.put(r_id, exec_fn);
    }


    template<typename InputT, typename ResultT, typename Op>
    std::function<g_ptr<r_node>(exec_context& ctx)> make_arithmetic_handler(Op operation, uint32_t return_type) {
        return [operation,return_type](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, 0, -1);
            execute_r_node(ctx.node->right, ctx.frame, 0, -1);
            
            InputT left = ctx.node->left->value.get<InputT>();
            InputT right = ctx.node->right->value.get<InputT>();
            ResultT result = operation(left, right);
            
            ctx.node->value.type = return_type;
            ctx.node->value.set<ResultT>(result);
            return ctx.node;
        };
    }

    // xTx <- Binary operator, ctx.node->right and ctx.node->left are what's on your right and left
    void add_binary_operator(char c,const std::string& token_name, const std::string& f,int precedence,exec_handler operation) {
        size_t a_id = add_token(c,token_name);
        size_t o_id = reg::new_type(f);
        state_is_opp.put(o_id, true);
        token_to_opp.put(a_id, o_id);
        type_precdence.put(o_id, precedence);
        size_t t_id = reg::new_type("T_"+f);
        t_opp_conversion.put(o_id, t_id);
        size_t r_id = reg::new_type("R_"+f);
        r_handlers.put(t_id, [r_id](g_ptr<r_node> result, r_context& ctx){
                result->type = r_id;
                if(ctx.node->left) 
                    result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
                if(ctx.node->right)
                    result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        });
        exec_handlers.put(r_id, operation);
    }

    // xT & Tx <- Unary opperator, ctx.node->left or right contains what you need.

    namespace helper_test_module 
    {
        static void initialize() {

            char_is_split.put('+',true); char_is_split.put('-',true); char_is_split.put('/',true); char_is_split.put('%',true);
            char_is_split.put('(',true); char_is_split.put(')',true); char_is_split.put(',',true); char_is_split.put('=',true);
            char_is_split.put('>',true); char_is_split.put('<',true); char_is_split.put('[',true); char_is_split.put(']',true);
            char_is_split.put('*',true); char_is_split.put('@',true);

            reg::new_type("UNDEFINED");

            reg::new_type("T_NOP"); 
            reg::new_type("UNTYPED"); 
            reg::new_type("R_NOP");
            size_t f_type_key_id = reg::new_type("F_TYPE_KEY");//Honestly not sure about these family types
            size_t f_keyword_id = reg::new_type("F_KEYWORD"); //Keeping them in because it may be useful later
    
            reg::new_type("GLOBAL");
            reg::new_type("BLOCK");
            reg::new_type("T_BLOCK");  
            
            reg::new_type("NOT"); 
    
            // === LITERALS ===
            size_t literal_id = reg::new_type("LITERAL");
            auto literal_handler = [literal_id](a_context& ctx) {
                if(ctx.state == 0) {
                    ctx.state = literal_id;
                }
                ctx.node->tokens << ctx.token;
            };
            t_functions.put(literal_id, t_literal_handler);
            size_t t_literal_id = reg::new_type("T_LITERAL");
            size_t r_literal_id = reg::new_type("R_LITERAL");
            r_handlers.put(t_literal_id, [r_literal_id](g_ptr<r_node> result, r_context& ctx) {
                result->type = r_literal_id;
                result->value = std::move(ctx.node->value);
            });
            exec_handlers.put(r_literal_id, [](exec_context& ctx) -> g_ptr<r_node> {
                return ctx.node;
            });
        
            size_t end_id = reg::new_type("END"); 
            a_functions.put(end_id, [end_id](a_context& ctx) {
                if(ctx.state != 0) {
                    ctx.end_lambda();
                } 
                ctx.state = end_id;
                ctx.end_lambda();
                ctx.state = 0;
                ctx.pos = 0;
            });
            t_functions.put(end_id, [](t_context& ctx) -> g_ptr<t_node>{
                return nullptr; //Do nothing
            });  
    
            size_t comma_id = reg::new_type("COMMA");
            tokenizer_functions.put(',', [comma_id](tokenizer_context& ctx){
                ctx.token = make<Token>(comma_id, ",");
                ctx.result << ctx.token;
            });
            a_functions.put(comma_id, [comma_id](a_context& ctx) {
                if(ctx.local && ctx.state != 0) {
                    ctx.end_lambda();
                    ctx.state = comma_id;
                    ctx.end_lambda();
                    ctx.state = 0;
                }
                ctx.pos = 0;
            });
            t_functions.put(comma_id, [](t_context& ctx) -> g_ptr<t_node>{
                return nullptr; //Do nothing
            });  
    
            size_t lbrace_id = reg::new_type("LBRACE");
            size_t enter_scope_id = reg::new_type("ENTER_SCOPE"); 
            a_functions.put(lbrace_id, [enter_scope_id](a_context& ctx) {
                if(ctx.state != 0) {
                    ctx.end_lambda();
                }
    
                //Could be added to the Return as well and add implicit scoping for methods
                // if(ctx.result.last()->type==GET_TYPE(VAR_DECL)) {
                //     ctx.result.last()->type = GET_TYPE(METHOD_DECL);
                // }
    
                ctx.state = enter_scope_id;
                ctx.end_lambda();
                ctx.state = 0;
                ctx.pos = 0;
            });
            scope_precedence.put(enter_scope_id, 10); 
        
            size_t rbrace_id = reg::new_type("RBRACE"); 
            size_t exit_scope_id = reg::new_type("EXIT_SCOPE"); 
            a_functions.put(rbrace_id, [exit_scope_id](a_context& ctx) {
                if(ctx.state != 0) {
                    ctx.end_lambda();
                }
                ctx.state = exit_scope_id;
                ctx.end_lambda();
                ctx.state = 0;
                ctx.pos = 0;
            });
            scope_precedence.put(exit_scope_id, -10);
            
            size_t lparen_id = reg::new_type("LPAREN");
            tokenizer_functions.put('(', [lparen_id](tokenizer_context& ctx){
                ctx.token = make<Token>(lparen_id, "(");
                ctx.result << ctx.token;
            });
    
            size_t rparen_id = reg::new_type("RPAREN");
            tokenizer_functions.put(')', [rparen_id](tokenizer_context& ctx){
                ctx.token = make<Token>(rparen_id, ")");
                ctx.result << ctx.token;
            });
            reg::new_type("ENTER_PAREN");
            a_functions.put(lparen_id, [lparen_id,rparen_id](a_context& ctx) {
                std::pair<int,int> paren_range = balance_tokens(ctx.tokens, lparen_id, rparen_id, ctx.index-1);
                if (paren_range.first < 0 || paren_range.second < 0) {
                    print("parse_tokens::719 Unmatched parenthesis at ", ctx.index);
                    return;
                }
        
                list<g_ptr<Token>> sub_list;
                for(int i=paren_range.first+1;i<paren_range.second;i++) {
                    sub_list.push(ctx.tokens[i]);
                }
                ctx.node->sub_nodes = parse_tokens(sub_list,true);
                if(ctx.state != 0) {
                    ctx.end_lambda();     
                }
                ctx.state = 0;        
                ctx.index = paren_range.second-1;
                ctx.it = ctx.tokens.begin() + (int)(paren_range.second);
                ctx.skip_inc = 1; // Can skip more if needed
            });
         
            reg::new_type("EXIT_PAREN"); 
            a_functions.put(rparen_id, [](a_context& ctx) {
                if(ctx.local && ctx.state != 0) {
                    ctx.end_lambda();
                    ctx.state = 0;
                }
            });


            size_t float_id = reg::new_type("FLOAT");
        
            size_t int_id = reg::new_type("INT");
            size_t int_key_id = reg::new_type("INT_KEY");
            //REGISTER INT AS A KEYWORD!
            a_functions.put(int_key_id, type_key_handler);
            value_to_string.put(int_id,[](void* data){
                return std::to_string(*(int*)data);
            });
            negate_value.put(int_id,[](void* data){
                *(int*)data = -(*(int*)data);
            });
            a_functions.put(int_id, literal_handler);
            type_key_to_type.put(int_key_id, int_id);
            t_literal_handlers.put(int_id, [t_literal_id, int_id](g_ptr<Token> token) -> g_ptr<t_node> {
                g_ptr<t_node> node = make<t_node>();
                node->type = t_literal_id;
                node->value.type = int_id;
                node->value.set<int>(std::stoi(token->content));
                return node;
            });

            size_t bool_id = reg::new_type("BOOL");
            size_t bool_key_id = reg::new_type("BOOL_KEY");
            //REGISTER BOOL AS A KEYWORD!
            a_functions.put(bool_key_id, type_key_handler);
            value_to_string.put(bool_id,[](void* data){
                return (*(bool*)data) ? "TRUE" : "FALSE";
            });
            a_functions.put(bool_id, literal_handler);
            type_key_to_type.put(bool_key_id, bool_id);
            t_literal_handlers.put(bool_id, [t_literal_id, bool_id](g_ptr<Token> token) -> g_ptr<t_node> {
                g_ptr<t_node> node = make<t_node>();
                node->type = t_literal_id;
                node->value.type = bool_id;
                //This means the fallback is false if they do TRUE might want to change it later
                node->value.set<bool>(token->content=="true" ? true : false); 
                return node;
            });
               

            add_binary_operator('+',"PLUS","ADD", 2, make_arithmetic_handler<int, int>([](auto a, auto b){return a+b;},int_id));
            add_binary_operator('>',"RANGLE","GREATER_THAN", 2, make_arithmetic_handler<int, bool>([](auto a, auto b){return a>b;},bool_id));

            size_t string_id = reg::new_type("STRING");
            size_t string_key_id = reg::new_type("STRING_KEY");
            size_t in_string_id = reg::new_type("IN_STRING_KEY");
            tokenizer_state_functions.put(in_string_id,[](tokenizer_context& ctx){
                char c = *(ctx.it);
                if(c=='"') {
                    ctx.state=0;
                }
                else {
                    ctx.token->add(c);
                }
            });
            tokenizer_functions.put('"',[string_id,in_string_id](tokenizer_context& ctx){
                ctx.state = in_string_id;
                ctx.token = make<Token>(string_id,"");
                ctx.result << ctx.token;
            });
            //ADD STRING KEYWORD
            a_functions.put(string_key_id, type_key_handler);
            value_to_string.put(string_id,[](void* data){
                return *(std::string*)data;
            });
            a_functions.put(string_id, literal_handler);
            type_key_to_type.put(string_key_id, string_id);
            t_literal_handlers.put(string_id, [t_literal_id, string_id](g_ptr<Token> token) -> g_ptr<t_node> {
                g_ptr<t_node> node = make<t_node>();
                node->type = t_literal_id;
                node->value.type = string_id;
                node->value.set<std::string>(token->content);
                node->value.size = 24;
                return node;
            });
            

            add_function("print",[](exec_context& ctx) -> g_ptr<r_node> {
                std::string toPrint = "";
                for(auto r : ctx.node->children) {
                    execute_r_node(r, ctx.frame, ctx.index,ctx.sub_index);
                    toPrint.append(r->value.to_string());
                }
                print(toPrint);
                return ctx.node;
            });

            add_scoped_keyword("if", 2, [](exec_context& ctx) -> g_ptr<r_node> {
                execute_r_node(ctx.node->right, ctx.frame, ctx.index, ctx.sub_index);
                if(ctx.node->right->value.is_true()) {
                    execute_sub_frame(ctx.node->frame, ctx.frame);
                }
                else if(ctx.node->left) {
                    execute_sub_frame(ctx.node->left->frame, ctx.frame);
                }
                return ctx.node;
            });

            char end_char = ';';

            size_t in_identifier_id = reg::new_type("IN_IDENTIFIER"); 
            tokenizer_state_functions.put(in_identifier_id,[end_char](tokenizer_context& ctx){
                char c = *(ctx.it);
                if(c==end_char||c=='.'||char_is_split.getOrDefault(c,false)) {
                    ctx.state=0;
                    --ctx.it;
                }
                else if(c==' '||c=='\t'||c=='\n') {
                    ctx.state=0;
                }
                else {
                    ctx.token->add(c);
                }
            });

            size_t in_number_id = reg::new_type("IN_NUMBER"); 
            size_t identifier_id = reg::new_type("IDENTIFIER");
            
            tokenizer_state_functions.put(in_number_id,[end_char,float_id,identifier_id,in_identifier_id](tokenizer_context& ctx){
                char c = *(ctx.it);
                if(c==end_char||char_is_split.getOrDefault(c,false)) {
                    ctx.state=0;
                    --ctx.it;
                }
                else if(c==' '||c=='\t'||c=='\n') {
                    ctx.state=0;
                }
                else if(c=='.') {
                    if(ctx.token->dotted) {
                        ctx.state=in_identifier_id;
                        ctx.token->setType(identifier_id);
                        ctx.token->add(c);
                    }
                    else {
                        ctx.token->setType(float_id);
                        ctx.token->dotted = true;
                        ctx.token->add(c);
                    }
                }
                else {
                    ctx.token->add(c);
                }
            });

            std::function<void(tokenizer_context& ctx)> default_function = [in_identifier_id,in_number_id,int_id,identifier_id](tokenizer_context& ctx) {
                char c = *(ctx.it);
                    if(std::isalpha(c)||c=='_') {
                        ctx.state = in_identifier_id;
                        ctx.token = make<Token>(identifier_id,c);
                        ctx.result << ctx.token;
                    }
                    else if(std::isdigit(c)) {
                        ctx.state = in_number_id;
                        ctx.token = make<Token>(int_id,c);
                        ctx.result << ctx.token;
                    }
            };
            tokenizer_default_function = default_function;

            a_default_function = [](a_context& ctx){
                uint32_t opp = token_to_opp.getOrDefault(ctx.token->getType(),(unsigned int)0);
                if(opp!=0) {
                    //If we're already in an opperation, end the current one, start a new one
                    if(state_is_opp.getOrDefault(ctx.state,false)) {
                        ctx.end();
                        ctx.state=opp;
                    }
                    else {
                        ctx.state=opp;
                    }
                }
                else
                print("parse_tokens::653 missing case for type ",TO_STRING(ctx.token->getType()));
            };

            tokenizer_functions.put(end_char,[end_char,end_id](tokenizer_context& ctx){
                    ctx.token = make<Token>(end_id,end_char);
                    ctx.result << ctx.token;
            });
        }
    }
}