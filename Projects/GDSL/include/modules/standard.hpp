#include<Golden.hpp>


//For all the core lanqauge stuff, always goes first
namespace base_module {
    static void initialize() {
        reg::new_type("UNDEFINED");
        reg::new_type("T_NOP"); 
        reg::new_type("UNTYPED"); 
        reg::new_type("R_NOP");
        reg::new_type("F_TYPE_KEY");//Honestly not sure about these family types
        reg::new_type("F_KEYWORD"); //Keeping them in because it may be useful later

        reg::new_type("GLOBAL");
        reg::new_type("BLOCK");
        reg::new_type("T_BLOCK");  
        
        reg::new_type("NOT"); 
    }
}

namespace paren_module {
    static void initialize() {
        size_t end_id = reg::new_type("END"); 
        a_functions.put(end_id, [end_id](a_context& ctx) {
            if(ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
            } 
            ctx.state = end_id;
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
            ctx.pos = 0;
        });
        t_functions.put(end_id, [](t_context& ctx) -> g_ptr<t_node>{
            return nullptr; //Do nothing
        });  

        size_t comma_id = reg::new_type("COMMA");
        a_functions.put(comma_id, [comma_id](a_context& ctx) {
            if(ctx.local && ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
                ctx.state = comma_id;
                ctx.end_lambda();
                ctx.state = GET_TYPE(UNTYPED);
            }
            ctx.pos = 0;
        });
        t_functions.put(comma_id, [](t_context& ctx) -> g_ptr<t_node>{
            return nullptr; //Do nothing
        });  

        size_t lbrace_id = reg::new_type("LBRACE");
        size_t enter_scope_id = reg::new_type("ENTER_SCOPE"); 
        a_functions.put(lbrace_id, [enter_scope_id](a_context& ctx) {
            if(ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
            }

            //Could be added to the Return as well and add implicit scoping for methods
            if(ctx.result.last()->type==GET_TYPE(VAR_DECL)) {
                ctx.result.last()->type = GET_TYPE(METHOD_DECL);
            }

            ctx.state = enter_scope_id;
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
            ctx.pos = 0;
        });
        scope_precedence.put(enter_scope_id, 10); 
    
        size_t rbrace_id = reg::new_type("RBRACE"); 
        size_t exit_scope_id = reg::new_type("EXIT_SCOPE"); 
        a_functions.put(rbrace_id, [exit_scope_id](a_context& ctx) {
            if(ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
            }
            ctx.state = exit_scope_id;
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
            ctx.pos = 0;
        });
        scope_precedence.put(exit_scope_id, -10);
        
        size_t lparen_id = reg::new_type("LPAREN");
        reg::new_type("ENTER_PAREN");
        a_functions.put(lparen_id, [](a_context& ctx) {
            auto paren_range = balance_tokens(ctx.tokens, GET_TYPE(LPAREN), GET_TYPE(RPAREN), ctx.index-1);
            if (paren_range.x() < 0 || paren_range.y() < 0) {
                print("parse_tokens::719 Unmatched parenthesis at ", ctx.index);
                return;
            }
    
            if (ctx.state == GET_TYPE(PROP_ACCESS)) {
                ctx.state = GET_TYPE(METHOD_CALL);
            }// else if (ctx.pos >= 2 && ctx.state == GET_TYPE(VAR_DECL)) {
            //     ctx.state = GET_TYPE(METHOD_DECL);
            // } 
    
            list<g_ptr<Token>> sub_list;
            for(int i=paren_range.x()+1;i<paren_range.y();i++) {
                sub_list.push(ctx.tokens[i]);
            }
            ctx.node->sub_nodes = parse_tokens(sub_list,true);
            if(ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();     
            }
            ctx.state = GET_TYPE(UNTYPED);        
            ctx.index = paren_range.y()-1;
            ctx.it = ctx.tokens.begin() + (int)(paren_range.y());
            ctx.skip_inc = 1; // Can skip more if needed
        });
     

        size_t rparen_id = reg::new_type("RPAREN"); 
        reg::new_type("EXIT_PAREN"); 
        a_functions.put(rparen_id, [](a_context& ctx) {
            if(ctx.local && ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
                ctx.state = GET_TYPE(UNTYPED);
            }
        });

        size_t brackets_id = reg::new_type("BRACKETS"); 
        size_t indexing_id = reg::new_type("INDEXING"); 
        size_t lbracket_id = reg::new_type("LBRACKET"); 
        a_functions.put(lbracket_id, [brackets_id,indexing_id](a_context& ctx) {
            auto bracket_range = balance_tokens(ctx.tokens, GET_TYPE(LBRACKET), GET_TYPE(RBRACKET), ctx.index-1);
            if (bracket_range.x() < 0 || bracket_range.y() < 0) {
                print("parse_tokens::128 Unmatched brackets at ", ctx.index);
                return;
            }
    
            // if (ctx.state == GET_TYPE(PROP_ACCESS)) {
            //     ctx.state = GET_TYPE(METHOD_CALL);
            // }// else if (ctx.pos >= 2 && ctx.state == GET_TYPE(VAR_DECL)) {
            //     ctx.state = GET_TYPE(METHOD_DECL);
            // } 
    
            list<g_ptr<Token>> sub_list;
            for(int i=bracket_range.x()+1;i<bracket_range.y();i++) {
                sub_list.push(ctx.tokens[i]);
            }

            if(ctx.state == GET_TYPE(LITERAL_IDENTIFIER)) {
                ctx.end_lambda();   
                ctx.node->tokens << ctx.result.pop()->tokens.pop();   
            }   
            else if(ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();     
            }
            ctx.state = indexing_id;
            ctx.node->sub_nodes = parse_tokens(sub_list,true);
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);        
            ctx.index = bracket_range.y()-1;
            ctx.it = ctx.tokens.begin() + (int)(bracket_range.y());
            ctx.skip_inc = 1; // Can skip more if needed
        });
        
        size_t rbracket_id = reg::new_type("RBRACKET"); 
        a_functions.put(rbracket_id, [](a_context& ctx) {
            if(ctx.local && ctx.state != GET_TYPE(UNTYPED)) {
                ctx.end_lambda();
                ctx.state = GET_TYPE(UNTYPED);
            }
        });

        state_is_opp.put(indexing_id,true);
        token_to_opp.put(brackets_id,indexing_id);
        type_precdence.put(indexing_id,20);
        size_t t_indexing_id = reg::new_type("T_INDEXING"); 
        t_opp_conversion.put(indexing_id, t_indexing_id);
        t_functions.put(indexing_id, [t_indexing_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_indexing_id;
            if(ctx.node->sub_nodes.length()>1) {
                print("indexing::171 the brackets/indexing opperator only accepts one opperand right now");
            }
            // ctx.result->left = t_parse_expression(ctx.node,ctx.left);
            ctx.result->left = t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]) ;
            ctx.result->right = t_parse_expression(ctx.node->sub_nodes[0],ctx.left);
            return ctx.result;
        });
        size_t r_indexing_id = reg::new_type("R_INDEXING"); 
        r_handlers.put(t_indexing_id,[r_indexing_id](g_ptr<r_node>& result, r_context& ctx) {
            result->type = r_indexing_id;
            if(ctx.node->left) {
                result->left = resolve_symbol(ctx.node->left,ctx.scope,ctx.frame);
            }
            if(ctx.node->right) {
                result->right = resolve_symbol(ctx.node->right,ctx.scope,ctx.frame);
            }
        });
        exec_handlers.put(r_indexing_id, [](exec_context& ctx) -> g_ptr<r_node> { 
            if(ctx.node->left) {
                execute_r_node(ctx.node->left,ctx.frame,ctx.index);
                if(ctx.node->left->resolved_type) {
                    //add opperator overloading for indexing
                }
                else if(ctx.node->right) {
                    execute_r_node(ctx.node->right,ctx.frame,ctx.index);
                    int index = ctx.node->left->in_scope->notes.get(ctx.node->left->name).index;
                    int sub_index = *(int*)ctx.node->right->value.data;
                    if(sub_index==-1) {
                        print(ctx.node->left->in_scope->table_to_string(ctx.node->left->value.size));
                    }
                    else {
                        ctx.node->index = sub_index;
                        ctx.node->value = std::move(ctx.node->left->value);
                        ctx.node->value.data = ctx.node->left->in_scope->get(index,sub_index,4);
                    }
                }
              
             }
            return ctx.node;
        });
    }
}

namespace control_module {
    static void initialize() {

        reg::new_type("IF_BLOCK");  
        reg::new_type("WHILE_LOOP"); 
        reg::new_type("FOR_LOOP");

        size_t if_key_id = reg::new_type("IF_KEY"); 
        reg_t_key("if", if_key_id, 0, GET_TYPE(F_KEYWORD));
        size_t if_decl_id = reg::new_type("IF_DECL"); 
        a_functions.put(if_key_id, [if_decl_id](a_context& ctx) {
            ctx.state = if_decl_id;
        });
        size_t t_if_id = reg::new_type("T_IF");
        t_opp_conversion.put(if_decl_id, t_if_id); 
        size_t r_if_id = reg::new_type("R_IF");  
        auto if_handler = [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            new_scope->scope_type = GET_TYPE(IF_BLOCK);
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        };
        scope_link_handlers.put(if_decl_id, if_handler);
        scope_precedence.put(if_decl_id, 2);
        t_functions.put(if_decl_id, [](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> result = t_parse_expression(ctx.node, nullptr);
            result->scope = ctx.node->owned_scope;
            return result;
        });
        r_handlers.put(t_if_id, [r_if_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_if_id;
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            result->frame = resolve_symbols(ctx.node->scope);
            if(ctx.node->left) {
                result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            }
        });
        exec_handlers.put(r_if_id, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            if(ctx.node->right->value.is_true()) {
                execute_r_nodes(ctx.node->frame);
            }
            else if(ctx.node->left) {
                execute_r_nodes(ctx.node->left->frame);
            }
            return ctx.node;
        });
        
        size_t else_key_id = reg::new_type("ELSE_KEY");
        reg_t_key("else", else_key_id, 0, GET_TYPE(F_KEYWORD)); 
        size_t else_decl_id = reg::new_type("ELSE_DECL"); 
        size_t t_else_id = reg::new_type("T_ELSE");
        t_opp_conversion.put(else_decl_id, t_else_id); 
        size_t r_else_id = reg::new_type("R_ELSE");
        a_functions.put(else_key_id, [if_decl_id,else_decl_id](a_context& ctx) {
            if(ctx.state == if_decl_id) {
                ctx.end_lambda();
            }
            ctx.state = else_decl_id;
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
        });
        scope_link_handlers.put(else_decl_id, if_handler);
        scope_precedence.put(else_decl_id, 1);
        t_functions.put(else_decl_id, [t_else_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_else_id;
            ctx.result->scope = ctx.node->owned_scope;
            ctx.left->left = ctx.result;
            return nullptr;
        });
        r_handlers.put(t_else_id, [r_else_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_else_id;
            result->frame = resolve_symbols(ctx.node->scope);
        });
    

        size_t while_key_id = reg::new_type("WHILE_KEY"); 
        reg_t_key("while", while_key_id, 0, GET_TYPE(F_KEYWORD)); 
        size_t while_decl_id = reg::new_type("WHILE_DECL"); 
        size_t t_while_id = reg::new_type("T_WHILE");
        t_opp_conversion.put(while_decl_id, t_while_id);
        size_t r_while_id = reg::new_type("R_WHILE"); 
        a_functions.put(while_key_id, [while_decl_id](a_context& ctx) {
            ctx.state = while_decl_id;
        });
        auto loop_handler = [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            new_scope->scope_type = GET_TYPE(WHILE_LOOP);
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        };
        scope_link_handlers.put(while_decl_id, loop_handler);
        scope_precedence.put(while_decl_id, 3); 
        t_functions.put(while_decl_id, [](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> result = t_parse_expression(ctx.node, nullptr);
            result->scope = ctx.node->owned_scope;
            if(ctx.left && ctx.left->type == GET_TYPE(T_DO)) {
                result->left = ctx.left;
                result->scope = ctx.left->scope;
                ctx.root->t_nodes.erase(ctx.left);
            }
            return result;
        });
        r_handlers.put(t_while_id, [r_while_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_while_id;
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            result->frame = resolve_symbols(ctx.node->scope);
            if(ctx.node->left) {
                result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
                result->left->frame = result->frame;
            }
        });
        exec_handlers.put(r_while_id, [](exec_context& ctx) -> g_ptr<r_node> {
            ctx.node->frame->resurrect();
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            while(ctx.node->right->value.is_true()) {
                execute_r_node(ctx.node->right, ctx.frame, ctx.index);
                if (!ctx.node->right->value.is_true()) break; 
                execute_r_nodes(ctx.node->frame);
            }
            return ctx.node;
        });

        size_t do_key_id = reg::new_type("DO_KEY");
        reg_t_key("do", do_key_id, 0, GET_TYPE(F_KEYWORD));
        size_t do_decl_id = reg::new_type("DO_DECL");
        size_t t_do_id = reg::new_type("T_DO");
        size_t r_do_id = reg::new_type("R_DO");
        a_functions.put(do_key_id, [do_decl_id](a_context& ctx) {
            ctx.state = do_decl_id;
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
        });  
        scope_link_handlers.put(do_decl_id, loop_handler);
        scope_precedence.put(do_decl_id, 2);
        t_functions.put(do_decl_id, [t_do_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_do_id;
            ctx.result->scope = ctx.node->owned_scope;
            return ctx.result;
        });
        r_handlers.put(t_do_id, [r_do_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_do_id;
        });
    

        size_t break_key_id = reg::new_type("BREAK_KEY"); 
        reg_t_key("break", break_key_id, 0, GET_TYPE(F_KEYWORD)); 
        size_t break_call_id = reg::new_type("BREAK_CALL"); 
        size_t t_break_id = reg::new_type("T_BREAK");
        t_opp_conversion.put(break_call_id, t_break_id); 
        size_t r_break_id = reg::new_type("R_BREAK");
        a_functions.put(break_key_id, [break_call_id](a_context& ctx) {
            ctx.state = break_call_id;
        });
        t_functions.put(break_call_id, [t_break_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_break_id;
            return ctx.result;
        });
        r_handlers.put(t_break_id, [r_break_id](g_ptr<r_node> result, r_context& ctx) {
            g_ptr<s_node> on_scope = ctx.scope;
            g_ptr<Frame> loop_frame = nullptr;
            while(!loop_frame) {
                if(on_scope->scope_type == GET_TYPE(WHILE_LOOP) || on_scope->scope_type == GET_TYPE(FOR_LOOP)) {
                    if(on_scope->frame) {
                        loop_frame = on_scope->frame;
                    }
                }
                if(loop_frame) break;
                if(on_scope->parent) {
                    on_scope = on_scope->parent;
                } else break;
            }
            result->frame = loop_frame;
            result->type = r_break_id;
        });
        exec_handlers.put(r_break_id, [](exec_context& ctx) -> g_ptr<r_node> {
            if(ctx.node->frame)
                ctx.node->frame->stop();
            return ctx.node;
        });    
    }

}

namespace data_module {
    static void initialize() {
        size_t list_key_id = reg::new_type("LIST_KEY"); 
        reg_t_key("list", list_key_id, 8, GET_TYPE(F_TYPE_KEY)); 
        size_t list_decl_id = reg::new_type("LIST_DECL"); 
        a_functions.put(list_key_id, type_key_handler);
        size_t list_id = reg::new_type("LIST");
        value_to_string.put(list_id,[](void* data) -> std::string{
            if(*(g_ptr<Type>*)data) {
                return "list of length "+std::to_string((*(g_ptr<Type>*)data)->array.length());
            }
            else {
                return "null list";
            }
        });
        //a_functions.put(list_id, literal_handler);
        type_key_to_type.put(list_key_id, list_id);
        t_literal_handlers.put(list_id, [list_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = GET_TYPE(T_LITERAL);
            node->value.type = list_id;
            node->value.set<int>(4);
            return node;
        });
       
    }
}

namespace type_module {
    static void initialize() {
        size_t method_id = reg::new_type("METHOD");
        size_t function_id = reg::new_type("FUNCTION");
        size_t method_key_id = reg::new_type("METHOD_KEY");
        size_t method_call_id = reg::new_type("METHOD_CALL"); 
        size_t t_method_call_id = reg::new_type("T_METHOD_CALL"); 
        size_t r_method_call_id =reg::new_type("R_METHOD_CALL"); 
        t_opp_conversion.put(method_call_id, t_method_call_id); 
        t_functions.put(method_call_id, [](t_context& ctx) -> g_ptr<t_node> {
            ctx.result = t_parse_expression(ctx.node, ctx.left);
            if(!ctx.node->sub_nodes.empty()) {
                g_ptr<t_node> last = nullptr;
                for(auto c : ctx.node->sub_nodes) {
                    g_ptr<t_node> sub = parse_a_node(c, ctx.root, last);
                    if(sub) {
                        last = sub;
                        ctx.result->children << last;
                    } else {
                        last = nullptr;
                    }
                }
            }
            return ctx.result;
        });
        r_handlers.put(t_method_call_id, [r_method_call_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_method_call_id;
            g_ptr<r_node> info = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            if(info->resolved_type) {
                result->slot = info->slot;
                if(ctx.frame->slots.length() <= result->slot) ctx.frame->slots << 0;
                result->resolved_type = info->resolved_type;
                result->name = ctx.node->right->name;
                g_ptr<s_node> type_decl = find_type_decl(result->resolved_type, ctx.scope);
                if(type_decl) {
                    g_ptr<r_node> method_decl = type_decl->method_map.get(result->name);
                    result->frame = method_decl->frame;
                    
                    for(int i = 0; i < ctx.node->children.size(); i++) {
                        g_ptr<r_node> arg = resolve_symbol(ctx.node->children[i], ctx.scope, ctx.frame);
                        g_ptr<r_node> param = method_decl->children[i];
                        param->type = GET_TYPE(R_IDENTIFIER);
                        g_ptr<r_node> assignment = make<r_node>();
                        assignment->type = GET_TYPE(R_ASSIGNMENT);
                        assignment->left = param;
                        assignment->right = arg;
                        result->children << assignment;
                    }
                }
            } 
        });
        exec_handlers.put(r_method_call_id, [](exec_context& ctx) -> g_ptr<r_node> {
            g_ptr<Object> context = ctx.node->frame->context->create();
        
            // Execute pre-wired parameter assignments
            for(auto assignment : ctx.node->children) {
                if(assignment->left->value.type == GET_TYPE(OBJECT)) {
                    ctx.node->frame->slots[assignment->left->slot] = ctx.frame->slots[assignment->right->slot];
                }
                else {
                    execute_r_node(assignment, ctx.node->frame, context->ID);
                }
            }
        
            execute_r_nodes(ctx.node->frame, context);
            if(ctx.node->frame->return_val.type != GET_TYPE(UNDEFINED)) {
                ctx.node->value = std::move(ctx.node->frame->return_val);
            }
            ctx.node->frame->return_val.type = GET_TYPE(UNDEFINED);
            return ctx.node;
        });
        stream_handlers.put(r_method_call_id, [](exec_context& ctx) -> std::function<void()>{
            g_ptr<Frame> frame = ctx.node->frame;
            std::function<void()> func = [frame](){
                g_ptr<Object> context = frame->context->create();
                execute_stream(frame);
            };
            return func;
        });
        
 
        size_t type_id = reg::new_type("TYPE"); 
        size_t type_key_id =  reg::new_type("TYPE_KEY"); 
        reg_t_key("type", type_key_id, 8, GET_TYPE(F_TYPE_KEY)); 
        size_t type_decl_id =  reg::new_type("TYPE_DECL"); 
        size_t type_def_id =  reg::new_type("TYPE_DEF");
        size_t t_type_decl_id = reg::new_type("T_TYPE_DECL");
        size_t r_type_decl_id =reg::new_type("R_TYPE_DECL");
        a_functions.put(type_key_id, [type_decl_id](a_context& ctx) {
            ctx.state = type_decl_id;
        });
        scope_link_handlers.put(type_decl_id, [type_def_id](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            new_scope->scope_type = type_def_id;
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        });
        t_functions.put(type_decl_id, [t_type_decl_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_type_decl_id;
            ctx.result->name = ctx.node->tokens.last()->content;
            ctx.result->scope = ctx.node->owned_scope;
            return ctx.result;
        });
        discover_handlers.put(t_type_decl_id, [](g_ptr<t_node> node, d_context& ctx) {
            if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
            node->scope->type_ref->type_name = node->name;
        });
        r_handlers.put(t_type_decl_id, [r_type_decl_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_type_decl_id;
            result->frame = resolve_symbols(ctx.node->scope);
        });
        exec_handlers.put(r_type_decl_id, [](exec_context& ctx) -> g_ptr<r_node> {
            // Probably do nothing for now
            return ctx.node;
        });
        stream_handlers.put(r_type_decl_id, [](exec_context& ctx) -> std::function<void()>{
           return nullptr;
        });


        size_t method_decl_id = reg::new_type("METHOD_DECL");
        size_t t_method_decl_id = reg::new_type("T_METHOD_DECL");  
        size_t r_method_decl_id =reg::new_type("R_METHOD_DECL");
        scope_link_handlers.put(method_decl_id, [type_def_id,method_id,function_id] (g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
            if(current_scope->scope_type == type_def_id)
                new_scope->scope_type = method_id;
            else 
                new_scope->scope_type = function_id;
            new_scope->owner = owner_node;
            owner_node->owned_scope = new_scope.getPtr();
        });
        t_functions.put(method_decl_id, [t_method_decl_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_method_decl_id;
            ctx.result->name = ctx.node->tokens.last()->content;
            ctx.result->scope = ctx.node->owned_scope;
            ctx.result->scope->t_owner = ctx.result;
            if(!ctx.node->sub_nodes.empty()) {
                g_ptr<t_node> last = nullptr;
                for(auto c : ctx.node->sub_nodes) {
                    g_ptr<t_node> sub = parse_a_node(c, ctx.root, last);
                    if(sub) {
                        last = sub;
                        ctx.result->children << last;
                    } else {
                        last = nullptr;
                    }
                }
            }
            return ctx.result;
        });
        discover_handlers.put(t_method_decl_id, [](g_ptr<t_node> node, d_context& ctx) {
            if(node->scope) {
                if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
                node->scope->type_ref->type_name = node->name;
                
                for(auto c : node->children) {
                    if(c->type == GET_TYPE(T_VAR_DECL)) {
                        d_context tctx(node->scope,ctx.idx);
                        discover_symbol(c,tctx);
                    }
                }
            }
        });
        r_handlers.put(t_method_decl_id, [r_method_decl_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_method_decl_id;
            result->name = ctx.node->name;
            result->frame = resolve_symbols(ctx.node->scope);
            for(auto c : ctx.node->children) {
                g_ptr<r_node> child = resolve_symbol(c, ctx.node->scope, result->frame);
                resolve_identifier(c, child, ctx.node->scope, result->frame);
                result->children << child;
            }
            ctx.scope->method_map.put(result->name, result);
        });

        size_t return_key_id = reg::new_type("RETURN_KEY");
        size_t return_call_id = reg::new_type("RETURN_CALL"); 
        reg_t_key("return", return_key_id, 0, GET_TYPE(F_KEYWORD)); 
        size_t t_return_id = reg::new_type("T_RETURN"); 
        size_t r_return_id =reg::new_type("R_RETURN");
        a_functions.put(return_key_id, [return_call_id](a_context& ctx) {
            ctx.state = return_call_id;
        });
        t_opp_conversion.put(return_call_id, t_return_id);
        t_functions.put(return_call_id, [](t_context& ctx) -> g_ptr<t_node> {
            return t_parse_expression(ctx.node, ctx.result);
        });
        r_handlers.put(t_return_id, [method_id,function_id,r_return_id] (g_ptr<r_node> result, r_context& ctx) {
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
            g_ptr<s_node> on_scope = ctx.scope;
            g_ptr<Frame> method_frame = nullptr;
            while(!method_frame) {
                if(on_scope->scope_type == method_id || on_scope->scope_type == function_id) {
                    if(on_scope->frame) {
                        method_frame = on_scope->frame;
                    }
                }
                if(method_frame) break;
                if(on_scope->parent) {
                    on_scope = on_scope->parent;
                } else break;
            }
            result->frame = method_frame;
            result->type = r_return_id;
        });
        exec_handlers.put(r_return_id, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            if(ctx.node->frame)
                ctx.node->frame->return_val = std::move(ctx.node->right->value);
            return ctx.node;
        });
        stream_handlers.put(r_return_id, [](exec_context& ctx) -> std::function<void()>{
            return nullptr;
         });
    }
    
}

namespace property_module {
    static void initialize() {
        size_t equals_id = reg::new_type("EQUALS"); 
        size_t dot_id = reg::new_type("DOT");

        size_t prop_access_id = reg::new_type("PROP_ACCESS"); 
        state_is_opp.put(prop_access_id,true); 
        token_to_opp.put(dot_id,prop_access_id);
        type_precdence.put(prop_access_id,8);
        size_t t_prop_access_id = reg::new_type("T_PROP_ACCESS");
        t_opp_conversion.put(prop_access_id, t_prop_access_id);
        size_t r_prop_access_id = reg::new_type("R_PROP_ACCESS");
        t_functions.put(prop_access_id, [t_prop_access_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_prop_access_id;
            ctx.result->left = t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]);
            ctx.result->right = t_literal_handlers.get(ctx.node->tokens[1]->getType())(ctx.node->tokens[1]);
            return ctx.result;
        });
        r_handlers.put(t_prop_access_id, [r_prop_access_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_prop_access_id;
            g_ptr<r_node> info = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            if(info->resolved_type) {
                result->slot = info->slot;
                if(ctx.frame->slots.length() <= result->slot) ctx.frame->slots << 0;
                result->resolved_type = info->resolved_type;
                result->value.address = result->resolved_type->adress_column(ctx.node->right->name);
                result->value.size = result->resolved_type->notes.get(ctx.node->right->name).size;
                result->value.type = info->value.type;
                result->name = ctx.node->left->name;
                result->frame = info->frame;
    
                for(auto c : ctx.scope->children) {
                    if(c->type_ref && c->type_ref == result->resolved_type) {
                        result->value.type = c->o_type_map.get(ctx.node->right->name);
                        break;
                    }
                }
    
                g_ptr<s_node> search_scope = ctx.scope;
                while(result->value.type == GET_TYPE(OBJECT)) {
                    if(search_scope->type_ref && search_scope->type_ref == result->resolved_type) {
                        result->value.type = search_scope->o_type_map.get(ctx.node->right->name);
                        break;
                    }
                    for(auto c : search_scope->children) {
                        if(c->type_ref && c->type_ref == result->resolved_type) {
                            result->value.type = c->o_type_map.get(ctx.node->right->name);
                            break;
                        }
                    }
                    if(result->value.type != GET_TYPE(OBJECT)) break;
                    if(search_scope->parent) {
                        search_scope = search_scope->parent;
                    } else break;
                }
            }
        });
        exec_handlers.put(r_prop_access_id, [](exec_context& ctx) -> g_ptr<r_node> {
            if(ctx.node->frame) {
                ctx.node->value.data = Type::get(ctx.node->value.address, 
                    ctx.node->frame->slots.get(ctx.node->slot), 
                    ctx.node->value.size);
                // if(!ctx.node->value.data) print("Data failed");
                // else print("Data success");
                // print(ctx.node->value.address, "-", 
                //     ctx.node->frame->slots.get(ctx.node->slot, "execute_r_node::prop_access"), 
                //     " -> ", ctx.node->value.to_string());
            }
            else print("execute_r_node::1970 No frame in node for prop access");
            return ctx.node;
        });
        stream_handlers.put(r_prop_access_id, [](exec_context& ctx) -> std::function<void()>{ 
            void** l_data = &ctx.node->value.data; 
            //Also: t_value* result_location = &ctx.node->value;
            void* l_address = ctx.node->value.address;
            size_t slot = ctx.node->frame->slots.get(ctx.node->slot);
            size_t size = ctx.node->value.size;      
            std::function<void()> func = [l_data,l_address,slot,size](){
                *l_data = Type::get(l_address,slot,size);
            };
            return func;
        });

        size_t assignment_id = reg::new_type("ASSIGNMENT");
        state_is_opp.put(assignment_id,true);
        token_to_opp.put(equals_id,assignment_id);
        type_precdence.put(assignment_id,1); 
        size_t t_assignment_id = reg::new_type("T_ASSIGN"); 
        t_opp_conversion.put(assignment_id, t_assignment_id);
        size_t r_assignment_id = reg::new_type("R_ASSIGNMENT");
        r_handlers.put(t_assignment_id, [r_assignment_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_assignment_id;
            result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        });
        exec_handlers.put(r_assignment_id, [r_prop_access_id](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, ctx.index);
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);

            size_t target_index = (ctx.node->left->type == r_prop_access_id) ? 
                ctx.frame->slots.get(ctx.node->left->slot) : (ctx.node->left->index==-1?ctx.index:ctx.node->left->index);
            Type::set(ctx.node->left->value.address, ctx.node->right->value.data, 
                                   target_index, ctx.node->right->value.size);

            return ctx.node;
        });
        stream_handlers.put(r_assignment_id, [](exec_context& ctx) -> std::function<void()>{
            execute_r_node(ctx.node->left, ctx.frame, ctx.index);
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            
            size_t target_index = (ctx.node->left->type == GET_TYPE(R_IDENTIFIER)) ? 
                ctx.index : ctx.frame->slots.get(ctx.node->left->slot);
            
            void* l_addr = ctx.node->left->value.address;
                if(!l_addr) print("r_assignment::stream_handler: l_addr not resolved");
            void** r_data = &ctx.node->right->value.data;
                if(!r_data) print("r_assignment::stream_handler: r_data not resolved");
            size_t r_size = ctx.node->right->value.size;
            //Can turn this into a switch statment returning functions so we bake size in here to gain about 27% more performance
            //by not using Type's API
            //(*(list<uint32_t>*)l_addr)[target_index] = *(uint32_t*)r_data;
            //Though in testing it doesn't make that much of a difference
            std::function<void()> func = [l_addr,r_data,target_index,r_size]() {
                Type::set(l_addr, *r_data, target_index, r_size);
            };
            return func;
        });
    }
}

namespace opperator_module {
    static void initialize() {
    //Consider instead a central registry system, wherein we encode information like split charchteristics and family in one struct
    char_is_split.put('+',true); char_is_split.put('-',true); char_is_split.put('/',true); char_is_split.put('%',true);
    char_is_split.put('(',true); char_is_split.put(')',true); char_is_split.put(',',true); char_is_split.put('=',true);
    char_is_split.put('>',true); char_is_split.put('<',true); char_is_split.put('[',true); char_is_split.put(']',true);
    char_is_split.put('*',true); char_is_split.put('@',true);
   
    

    size_t plus_id = reg::new_type("PLUS"); 
    size_t minus_id = reg::new_type("MINUS"); 
    size_t star_id = reg::new_type("STAR"); 
    size_t slash_id = reg::new_type("SLASH"); 
    size_t langle_id = reg::new_type("LANGLE"); 
    size_t rangle_id = reg::new_type("RANGLE"); 
    size_t at_symbol_id = reg::new_type("AT_SYMBOL"); 

    reg::new_type("PLUS_EQUALS");
    reg::new_type("EQUALS_EQUALS");
    reg::new_type("NOT_EQUALS");
    reg::new_type("PLUS_PLUS"); 
    reg::new_type("MINUS_MINUS");
    reg::new_type("MINUS_EQUALS");
    reg::new_type("SLASH_EQUALS");
    reg::new_type("STAR_EQUALS");

    reg::new_type("IS_EQUALS"); 

    auto binary_op_handler = [](uint32_t r_type) {
        return [r_type](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_type;
            if(ctx.node->left) 
                result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            if(ctx.node->right)
                result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        };
    };
    auto arithmetic_handler = [](auto operation, uint32_t result_type) {
        return [operation, result_type](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_operation(ctx.node, operation, result_type, ctx.frame);
            return ctx.node;
        };
    };

    size_t add_id = reg::new_type("ADD");
    state_is_opp.put(add_id,true);
    token_to_opp.put(plus_id,add_id);
    type_precdence.put(add_id,2); 
    size_t t_add_id = reg::new_type("T_ADD"); 
    t_opp_conversion.put(add_id, t_add_id); 
    size_t r_add_id = reg::new_type("R_ADD"); 
    r_handlers.put(t_add_id, binary_op_handler(r_add_id));
    exec_handlers.put(r_add_id, arithmetic_handler([](auto a, auto b){ return a+b; }, GET_TYPE(INT)));

    size_t subtract_id = reg::new_type("SUBTRACT"); 
    state_is_opp.put(subtract_id,true);
    token_to_opp.put(minus_id,subtract_id);
    type_precdence.put(subtract_id,2); 
    size_t t_subtract_id = reg::new_type("T_SUBTRACT"); 
    t_opp_conversion.put(subtract_id, t_subtract_id);
    size_t t_dash_id = reg::new_type("T_DASH"); 
    t_functions.put(subtract_id, [t_subtract_id,t_dash_id](t_context& ctx) -> g_ptr<t_node> {
        if(!ctx.root)
        {
            g_ptr<t_node> left = t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]);
            if(left->type==GET_TYPE(T_LITERAL)) {
                left->value.negate();
                ctx.result = left;
            } else {
                ctx.result->type = t_dash_id;
                ctx.result->left = left;
            }
        } else {
            ctx.result = t_parse_expression(ctx.node,ctx.left);
        }
        return ctx.result;
    });
    size_t r_subtract_id = reg::new_type("R_SUBTRACT");
    r_handlers.put(t_subtract_id, binary_op_handler(r_subtract_id));
    exec_handlers.put(r_subtract_id, arithmetic_handler([](auto a, auto b){ return a-b; }, GET_TYPE(INT)));

    size_t r_dash_id = reg::new_type("R_DASH");
    r_handlers.put(t_dash_id,[r_dash_id](g_ptr<r_node>& result, r_context& ctx) {
        result->type = r_dash_id;
        if(ctx.node->left) {
            result->left = resolve_symbol(ctx.node->left,ctx.scope,ctx.frame);
        }
    });
    exec_handlers.put(r_dash_id, [](exec_context& ctx) -> g_ptr<r_node> { 
        if(ctx.node->left) {
            execute_r_node(ctx.node->left,ctx.frame,ctx.index);
            if(ctx.node->left->resolved_type) {
                //add opperator overloading for dash
            }
            else {
                ctx.node->left->value.negate();
                ctx.node->value = std::move(ctx.node->left->value);
            }
          
         }
        return ctx.node;
    });


    size_t at_id = reg::new_type("AT"); 
    state_is_opp.put(at_id,true);
    token_to_opp.put(at_symbol_id,at_id); 
    type_precdence.put(at_id,4); //Unsure
    size_t t_at_id = reg::new_type("T_AT");
    t_opp_conversion.put(at_id, t_at_id);
    size_t r_at_id = reg::new_type("R_AT"); 
    r_handlers.put(t_at_id, binary_op_handler(r_at_id));
    exec_handlers.put(r_at_id, [](exec_context& ctx) -> g_ptr<r_node> {
        execute_r_node(ctx.node->left,ctx.frame,ctx.index);
        execute_r_node(ctx.node->right,ctx.frame,ctx.index);
        int size = *(int*)ctx.node->left->value.data;
        uint64_t addr = (uint64_t)ctx.node->right->value.data;
        for(int i=0;i<32;i++)
        print(*(int*)(addr+i));
       //type_key_to_type.get(ctx.node->left->type)
        print(value_to_string.get(ctx.node->left->value.type)(ctx.node->right->value.data));
        return ctx.node;
    });


    size_t multiply_id = reg::new_type("MULTIPLY"); 
    state_is_opp.put(multiply_id,true);
    token_to_opp.put(star_id,multiply_id); 
    type_precdence.put(multiply_id,4);
    size_t t_multiply_id = reg::new_type("T_MULTIPLY");
    t_opp_conversion.put(multiply_id, t_multiply_id);
    size_t r_multiply_id = reg::new_type("R_MULTIPLY"); 
    r_handlers.put(t_multiply_id, binary_op_handler(r_multiply_id));
    exec_handlers.put(r_multiply_id, arithmetic_handler([](auto a, auto b){ return a*b; }, GET_TYPE(INT)));

    size_t t_ptr_id = reg::new_type("T_POINTER"); 
    t_functions.put(multiply_id, [t_multiply_id,t_ptr_id](t_context& ctx) -> g_ptr<t_node> {
        if(!ctx.root)
        {
            g_ptr<t_node> result = make<t_node>();
            result->type = t_ptr_id;
            result->left = t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]);
            ctx.result = result;
        } else {
            ctx.result = t_parse_expression(ctx.node,ctx.left);
        }
        return ctx.result;
    });
    size_t r_ptr_id = reg::new_type("R_POINTER");
    r_handlers.put(t_ptr_id,[r_ptr_id](g_ptr<r_node> result, r_context& ctx) {
        result->type = r_ptr_id;
        if(ctx.node->left) {
            result->left = resolve_symbol(ctx.node->left,ctx.scope,ctx.frame);
        }
    });
    exec_handlers.put(r_ptr_id, [](exec_context& ctx) -> g_ptr<r_node> {
        if(ctx.node->left) {
           execute_r_node(ctx.node->left,ctx.frame,ctx.index);
           ctx.node->value.type = GET_TYPE(U64);
           ctx.node->value.size = 8;
           if(ctx.node->left->resolved_type) {
            //add opperator overloading
            ctx.node->value.data = &ctx.node->left->resolved_type->objects.get(ctx.frame->slots.get(ctx.node->left->slot));
           }
           else {
            ctx.node->value.data = Type::get(ctx.node->left->value.address, ctx.index, ctx.node->left->value.size);
           }
           //ctx.node->left->value.address;
        }
        return ctx.node;
    });


    size_t divide_id = reg::new_type("DIVIDE");
    state_is_opp.put(divide_id,true);
    token_to_opp.put(slash_id,divide_id);
    type_precdence.put(divide_id,4);
    size_t t_divide_id = reg::new_type("T_DIVIDE");
    t_opp_conversion.put(divide_id, t_divide_id); 
    size_t r_divide_id = reg::new_type("R_DIVIDE"); 
    r_handlers.put(t_divide_id, binary_op_handler(r_divide_id));
    exec_handlers.put(r_divide_id, arithmetic_handler([](auto a, auto b){ return a/b; }, GET_TYPE(INT)));

    size_t greater_than_id = reg::new_type("GREATER_THAN");
    state_is_opp.put(greater_than_id,true);
    token_to_opp.put(rangle_id,greater_than_id);
    type_precdence.put(greater_than_id,2);
    size_t t_greater_than = reg::new_type("T_GREATER_THAN");
    t_opp_conversion.put(greater_than_id, t_greater_than);
    size_t r_greater_than_id = reg::new_type("R_GREATER_THAN");
    r_handlers.put(t_greater_than, binary_op_handler(r_greater_than_id)); 
    exec_handlers.put(r_greater_than_id, arithmetic_handler([](auto a, auto b){ return a>b; }, GET_TYPE(BOOL)));

    size_t less_than_id = reg::new_type("LESS_THAN");
    state_is_opp.put(less_than_id,true);
    token_to_opp.put(langle_id,less_than_id); 
    type_precdence.put(less_than_id,2); 
    size_t t_less_than_id = reg::new_type("T_LESS_THAN");
    t_opp_conversion.put(less_than_id, t_less_than_id);
    size_t r_less_than_id = reg::new_type("R_LESS_THAN");
    r_handlers.put(t_less_than_id, binary_op_handler(r_less_than_id));
    exec_handlers.put(r_less_than_id, arithmetic_handler([](auto a, auto b){ return a<b; }, GET_TYPE(BOOL)));

    }
}

namespace variables_module {
    static void initialize() {
        size_t var_decl_id = reg::new_type("VAR_DECL");
        state_is_opp.put(var_decl_id,true);
        type_precdence.put(var_decl_id,1);
        size_t t_var_decl_id = reg::new_type("T_VAR_DECL");
        size_t object_id = reg::new_type("OBJECT");
        t_functions.put(var_decl_id, [t_var_decl_id,object_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type=t_var_decl_id;
            uint32_t type = ctx.node->tokens[0]->getType();
            if(type==GET_TYPE(IDENTIFIER)) { //For declaring an object
                ctx.result->value.type = object_id; 
                ctx.result->deferred_identifier = ctx.node->tokens[0]->content; //For later resolution
            }
            else { //For declaring a literal
                ctx.result->value.type = type_key_to_type.get(ctx.node->tokens[0]->type_info.type);
                ctx.result->value.size = ctx.node->tokens[0]->type_info.size;
            }
            ctx.result->name = ctx.node->tokens[1]->content;
            if(!ctx.node->sub_nodes.empty()) {
                g_ptr<t_node> last = nullptr;
                for(auto c : ctx.node->sub_nodes) {
                    g_ptr<t_node> sub = parse_a_node(c, ctx.root, last);
                    if(sub) {
                        last = sub;
                        ctx.result->children << last;
                    } else {
                        last = nullptr;
                    }
                }
            }
            return ctx.result;
        });
        //This is immensely important, all objects are created here.
        discover_handlers.put(t_var_decl_id, [](g_ptr<t_node> node, d_context& ctx) {
             if(node->value.type==GET_TYPE(OBJECT)) {
                //Can add short circuiting and error handeling here
                g_ptr<s_node> on_scope = ctx.root;
                g_ptr<Type> type = nullptr;
                //This scope walking for types needs to be inspected further, potential cause of problems.
                for(auto c : ctx.root->children) {
                    if(c->type_ref && c->type_ref->type_name == node->deferred_identifier) {
                        type = c->type_ref;
                    }
                }
                while(!type) {
                    if(on_scope->type_ref) {
                        if(on_scope->type_ref->type_name == node->deferred_identifier) {
                            type = on_scope->type_ref;
                        }
                    }
                    if(type) break;
                    if(on_scope->parent) {
                        on_scope = on_scope->parent;
                    }
                    else break;
                }
                if(type) {
                    ctx.root->type_map.put(node->name,type);
                }
                ctx.root->slot_map.put(node->name,ctx.root->slot_map.size()); //Linking slot
                ctx.root->type_ref->note_value(node->name,sizeof(size_t)); //Adding local variable
                //Need size map entry here too?
            }
            else {
                ctx.root->type_ref->note_value(node->name,node->value.size); //Adding local variable slot
                ctx.root->size_map.put(node->name,node->value.size); //Adding size for future access in resolution
                ctx.root->o_type_map.put(node->name,node->value.type); //Adding type for future access in resolution
            }
        });    
        value_to_string.put(object_id,[](void* data){
            return "OBJECT";
        });
        size_t r_var_decl_id = reg::new_type("R_VAR_DECL");
        r_handlers.put(t_var_decl_id, [r_var_decl_id,object_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_var_decl_id;
            result->value.type = ctx.node->value.type;
            result->value.size = ctx.node->value.size;
            result->name = ctx.node->name;
            void* address = ctx.scope->type_ref->adress_column(ctx.node->name);
            if(address) {
                result->value.address = address;
            } else print("r_var_decl::r_handler No address found");
            if(ctx.node->value.type == object_id) {
                g_ptr<r_node> info = make<r_node>();
                resolve_identifier(ctx.node, info, ctx.scope, ctx.frame);
                if(info->resolved_type) {
                    result->slot = info->slot;
                    if(ctx.frame->slots.length() <= result->slot) ctx.frame->slots << 0;
                    result->resolved_type = info->resolved_type;
                    result->name = info->name;
                    result->value.type = object_id;
                }
            } else if(!ctx.node->children.empty()) { //If we're a list or non-object constructer
                for(auto c : ctx.node->children) {
                    result->children.push(resolve_symbol(c,ctx.scope,ctx.frame));
                }
            }
        });
        exec_handlers.put(r_var_decl_id, [object_id](exec_context& ctx) -> g_ptr<r_node> {
            if(!ctx.node->children.empty()) {
                execute_r_node(ctx.node->children[0],ctx.frame,ctx.index);
                int size = *(int*)ctx.node->children[0]->right->value.data;
                int index = ctx.frame->context->notes.get(ctx.node->name).index;
                while(ctx.frame->context->row_length(index,4)<size) {
                    ctx.frame->context->add_row(index,4);
                }
            }
            else if(ctx.node->value.type == object_id) {
                g_ptr<Object> obj = ctx.node->resolved_type->create();
                ctx.frame->slots[ctx.node->slot] = obj->ID;
            }
            else if(ctx.node->value.type == GET_TYPE(LIST)) {
                g_ptr<Type> array = make<Type>();
                ctx.node->value.set<g_ptr<Type>>(array);
            }
            return ctx.node;
        });
        stream_handlers.put(r_var_decl_id, [object_id](exec_context& ctx) -> std::function<void()>{
            if(ctx.node->value.type == object_id) {
                g_ptr<Type> resolved_type = ctx.node->resolved_type;
                size_t slot = ctx.node->slot;
                g_ptr<Frame> frame = ctx.frame;
                g_ptr<Object> obj = resolved_type->create();
                frame->slots[slot] = obj->ID;
                std::function<void()> func = [resolved_type,frame,slot](){
                   //Run constructor
                };
                return func;
            }
            else return nullptr;
        });

        
        size_t identifier_id = reg::new_type("IDENTIFIER");
        size_t literal_identifier_id = reg::new_type("LITERAL_IDENTIFIER");
        // state_is_opp.put(literal_identifier_id,true); Not a good idea
        // type_precdence.put(literal_identifier_id,4);
        a_functions.put(identifier_id, [literal_identifier_id,var_decl_id](a_context& ctx) {
            if(ctx.state == GET_TYPE(UNTYPED)) {
                ctx.state = literal_identifier_id;
            }
            else if(ctx.state == literal_identifier_id) {
                ctx.state = var_decl_id;
            }
            ctx.node->tokens << ctx.token;
        });
        size_t t_identifier_id = reg::new_type("T_IDENTIFIER");
        t_literal_handlers.put(identifier_id, [t_identifier_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = t_identifier_id;
            node->value.type = GET_TYPE(UNDEFINED);
            node->name = token->content;
            return node;
        });
        t_functions.put(literal_identifier_id,[t_var_decl_id,object_id](t_context& ctx) -> g_ptr<t_node> {
            g_ptr<t_node> node = t_literal_handler(ctx);
            if(!ctx.node->sub_nodes.empty()) {
                g_ptr<t_node> last = nullptr;
                for(auto c : ctx.node->sub_nodes) {
                    g_ptr<t_node> sub = parse_a_node(c, ctx.root, last);
                    if(sub) {
                        last = sub;
                        node->children << last;
                    } else {
                        last = nullptr;
                    }
                }
            }
            return node;
        });
       
        size_t r_identifier_id = reg::new_type("R_IDENTIFIER"); 
        r_handlers.put(t_identifier_id, [r_identifier_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_identifier_id;
            resolve_identifier(ctx.node, result, ctx.scope, ctx.frame);
            if(!ctx.node->children.empty()) { //For array access or opperator usage
                for(auto c : ctx.node->children) {
                    result->children.push(resolve_symbol(c,ctx.scope,ctx.frame));
                }
            }
        });
        exec_handlers.put(r_identifier_id, [object_id](exec_context& ctx) -> g_ptr<r_node> {
            if(ctx.node->value.type!=object_id) //So we don't perform set opperations on objects
                ctx.node->value.data = Type::get(ctx.node->value.address, ctx.index, ctx.node->value.size);

            return ctx.node;
        });


        
    }

}

namespace literals_module {
    static void initialize() {
        size_t literal_id = reg::new_type("LITERAL");
        size_t t_literal_id = reg::new_type("T_LITERAL");
        t_functions.put(literal_id, t_literal_handler);
        size_t r_literal_id = reg::new_type("R_LITERAL");
        r_handlers.put(t_literal_id, [r_literal_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_literal_id;
            result->value = std::move(ctx.node->value);
        });
        exec_handlers.put(r_literal_id, [](exec_context& ctx) -> g_ptr<r_node> {
            // Do nothing
            return ctx.node;
        });

        size_t int_key_id = reg::new_type("INT_KEY");
        reg_t_key("int", int_key_id, 4, GET_TYPE(F_TYPE_KEY)); 
        a_functions.put(int_key_id, type_key_handler);
        size_t int_id = reg::new_type("INT");
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

        size_t u64_key_id = reg::new_type("U64_KEY");
        reg_t_key("u64", u64_key_id, 8, GET_TYPE(F_TYPE_KEY));
        a_functions.put(u64_key_id, type_key_handler);
        size_t u64_id = reg::new_type("U64");
        value_to_string.put(u64_id,[](void* data){
            return std::to_string(*(uint64_t*)data);
        });
        negate_value.put(u64_id,[](void* data){
            *(uint64_t*)data = -(*(uint64_t*)data);
        });
        a_functions.put(u64_id, literal_handler);
        type_key_to_type.put(u64_key_id, u64_id);
        t_literal_handlers.put(u64_id, [t_literal_id, u64_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = t_literal_id;
            node->value.type = u64_id;
            node->value.set<uint64_t>(std::stol(token->content));
            return node;
        });
        
        size_t double_key_id = reg::new_type("DOUBLE_KEY");
        reg_t_key("double", double_key_id, 8, GET_TYPE(F_TYPE_KEY));
        a_functions.put(double_key_id, type_key_handler);
        size_t double_id = reg::new_type("DOUBLE");
        value_to_string.put(double_id,[](void* data){
            return std::to_string(*(double*)data);
        });
        negate_value.put(double_id,[](void* data){
            *(double*)data = -(*(double*)data);
        });
        a_functions.put(double_id, literal_handler);
        type_key_to_type.put(double_key_id, double_id);
        t_literal_handlers.put(double_id, [t_literal_id, double_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = t_literal_id;
            node->value.type = double_id;
            node->value.set<double>(std::stod(token->content));
            return node;
        });

        size_t float_key_id = reg::new_type("FLOAT_KEY");
        reg_t_key("float", float_key_id, 4, GET_TYPE(F_TYPE_KEY));
        a_functions.put(float_key_id, type_key_handler);
        size_t float_id = reg::new_type("FLOAT");
        value_to_string.put(float_id,[](void* data){
            return std::to_string(*(float*)data);
        });
        negate_value.put(float_id,[](void* data){
            *(float*)data = -(*(float*)data);
        });
        a_functions.put(float_id, literal_handler);
        type_key_to_type.put(float_key_id, float_id);
        t_literal_handlers.put(float_id, [t_literal_id, float_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = t_literal_id;
            node->value.type = float_id;
            node->value.set<float>(std::stof(token->content));
            return node;
        });

        
        
        size_t string_key_id = reg::new_type("STRING_KEY");
        reg_t_key("string", string_key_id, 24, GET_TYPE(F_TYPE_KEY)); 
        a_functions.put(string_key_id, type_key_handler);
        size_t string_id = reg::new_type("STRING");
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

        size_t char_key_id = reg::new_type("CHAR_KEY");
        reg_t_key("char", char_key_id, 1, GET_TYPE(F_TYPE_KEY)); 
        a_functions.put(char_key_id, type_key_handler);
        size_t char_id = reg::new_type("CHAR");
        value_to_string.put(char_id,[](void* data){
            return std::string(1, *(char*)data);
        });
        a_functions.put(char_id, literal_handler);
        type_key_to_type.put(char_key_id, char_id);
        
        size_t bool_key_id = reg::new_type("BOOL_KEY");
        reg_t_key("bool", bool_key_id, 1, GET_TYPE(F_TYPE_KEY));
        a_functions.put(bool_key_id, type_key_handler);
        size_t bool_id = reg::new_type("BOOL");
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

        size_t void_key_id = reg::new_type("VOID_KEY");
        reg_t_key("void", void_key_id, 0, GET_TYPE(F_KEYWORD)); 
        a_functions.put(void_key_id, type_key_handler);  
        size_t void_id = reg::new_type("VOID");
        type_key_to_type.put(void_key_id, void_id);        
    }
}

namespace functions_module {
    static void initialize() {
        size_t print_key_id = reg::new_type("PRINT_KEY");  
        size_t print_call_id = reg::new_type("PRINT_CALL");
        reg_t_key("print", print_key_id, 0, GET_TYPE(F_KEYWORD)); 
        a_functions.put(print_key_id, [print_call_id](a_context& ctx) {
            ctx.state = print_call_id;
        });
        size_t t_print_id = reg::new_type("T_PRINT");
        t_functions.put(print_call_id, [t_print_id](t_context& ctx) -> g_ptr<t_node> {
            ctx.result->type = t_print_id;
            g_ptr<t_node> last = nullptr;
            for(auto a : ctx.node->sub_nodes) {
                g_ptr<t_node> sub = parse_a_node(a, ctx.root, last);
                if(sub) {
                    if(last && sub->left == last) {
                        ctx.result->children.erase(last);
                    }
                    last = sub;
                    ctx.result->children << last;
                } else {
                    last = nullptr;
                }
            }
            return ctx.result;
        });
        size_t r_print_call_id = reg::new_type("R_PRINT_CALL"); 
        r_handlers.put(t_print_id, [r_print_call_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_print_call_id;
            for(auto c : ctx.node->children) {
                g_ptr<r_node> sub = resolve_symbol(c, ctx.scope, ctx.frame);
                result->children << sub;
            }
        });
        exec_handlers.put(r_print_call_id, [](exec_context& ctx) -> g_ptr<r_node> {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                execute_r_node(r, ctx.frame, ctx.index);
                toPrint.append(r->value.to_string());
            }
            print(toPrint);
            return ctx.node;
        });
        stream_handlers.put(r_print_call_id, [](exec_context& ctx) -> std::function<void()>{
            list<uint32_t> types;
            list<void**> datas; //Not sure if this is nessecary
            for(auto r : ctx.node->children) {
                execute_r_node(r, ctx.frame, ctx.index);
                types << r->value.type;
                datas << &r->value.data;
            }
            std::function<void()> func = [types,datas](){
                std::string toPrint = "";
                for(int i=0;i<types.length();i++) {
                    toPrint.append(data_to_string(types[i],*datas[i]));
                }
                print(toPrint);
            };
            return func;
        });
    }

}
