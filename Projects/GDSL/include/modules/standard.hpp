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
    }
}

namespace control_module {
    static void initialize() {
        reg::new_type("IF_KEY");
        reg::new_type("ELSE_KEY");
        reg::new_type("RETURN_KEY");
        reg::new_type("WHILE_KEY"); 
        reg::new_type("BREAK_KEY"); 
        reg::new_type("DO_KEY");
    }

}


namespace property_module {
    static void initialize() {
        size_t equals_id = reg::new_type("EQUALS"); 
        size_t dot_id = reg::new_type("DOT");

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
        exec_handlers.put(r_assignment_id, [](exec_context& ctx) -> g_ptr<r_node> {
            execute_r_node(ctx.node->left, ctx.frame, ctx.index);
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            
            size_t target_index = (ctx.node->left->type == GET_TYPE(R_IDENTIFIER)) ? 
                ctx.index : ctx.frame->slots.get(ctx.node->left->slot);
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

        size_t prop_access_id = reg::new_type("PROP_ACCESS"); 
        state_is_opp.put(prop_access_id,true); 
        token_to_opp.put(dot_id,prop_access_id);
        type_precdence.put(prop_access_id,3);
        size_t t_prop_access_id = reg::new_type("T_PROP_ACCESS");
        t_opp_conversion.put(prop_access_id, t_prop_access_id);
        size_t r_prop_access_id = reg::new_type("R_PROP_ACCESS");
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
    }
}

namespace opperator_module {
    static void initialize() {
    //Consider instead a central registry system, wherein we encode information like split charchteristics and family in one struct
    char_is_split.put('+',true); char_is_split.put('-',true); char_is_split.put('/',true); char_is_split.put('%',true);
    char_is_split.put('(',true); char_is_split.put(')',true); char_is_split.put(',',true); char_is_split.put('=',true);
    char_is_split.put('>',true); char_is_split.put('<',true); char_is_split.put('[',true); char_is_split.put(']',true);
   
    

    size_t plus_id = reg::new_type("PLUS"); 
    size_t minus_id = reg::new_type("MINUS"); 
    size_t star_id = reg::new_type("STAR"); 
    size_t slash_id = reg::new_type("SLASH"); 
    size_t langle_id = reg::new_type("LANGLE"); 
    size_t rangle_id = reg::new_type("RANGLE"); 

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
    type_precdence.put(add_id,1); 
    size_t t_add_id = reg::new_type("T_ADD"); 
    t_opp_conversion.put(add_id, t_add_id); 
    size_t r_add_id = reg::new_type("R_ADD"); 
    r_handlers.put(t_add_id, binary_op_handler(r_add_id));
    exec_handlers.put(r_add_id, arithmetic_handler([](auto a, auto b){ return a+b; }, GET_TYPE(INT)));

    size_t subtract_id = reg::new_type("SUBTRACT"); 
    state_is_opp.put(subtract_id,true);
    token_to_opp.put(minus_id,subtract_id);
    type_precdence.put(subtract_id,1); 
    size_t t_subtract_id = reg::new_type("T_SUBTRACT"); 
    t_opp_conversion.put(subtract_id, t_subtract_id);
    size_t r_subtract_id = reg::new_type("R_SUBTRACT");
    r_handlers.put(t_subtract_id, binary_op_handler(r_subtract_id));
    exec_handlers.put(r_subtract_id, arithmetic_handler([](auto a, auto b){ return a-b; }, GET_TYPE(INT)));

    size_t multiply_id = reg::new_type("MULTIPLY"); 
    state_is_opp.put(multiply_id,true);
    token_to_opp.put(star_id,multiply_id); 
    type_precdence.put(multiply_id,2);
    size_t t_multiply_id = reg::new_type("T_MULTIPLY");
    t_opp_conversion.put(multiply_id, t_multiply_id);
    size_t r_multiply_id = reg::new_type("R_MULTIPLY"); 
    r_handlers.put(t_multiply_id, binary_op_handler(r_multiply_id));
    exec_handlers.put(r_multiply_id, arithmetic_handler([](auto a, auto b){ return a*b; }, GET_TYPE(INT)));

    size_t divide_id = reg::new_type("DIVIDE");
    state_is_opp.put(divide_id,true);
    token_to_opp.put(slash_id,divide_id);
    type_precdence.put(divide_id,2);
    size_t t_divide_id = reg::new_type("T_DIVIDE");
    t_opp_conversion.put(divide_id, t_divide_id); 
    size_t r_divide_id = reg::new_type("R_DIVIDE"); 
    r_handlers.put(t_divide_id, binary_op_handler(r_divide_id));
    exec_handlers.put(r_divide_id, arithmetic_handler([](auto a, auto b){ return a/b; }, GET_TYPE(INT)));

    size_t greater_than_id = reg::new_type("GREATER_THAN");
    state_is_opp.put(greater_than_id,true);
    token_to_opp.put(rangle_id,greater_than_id);
    type_precdence.put(greater_than_id,1);
    size_t t_greater_than = reg::new_type("T_GREATER_THAN");
    t_opp_conversion.put(greater_than_id, t_greater_than);
    size_t r_greater_than_id = reg::new_type("R_GREATER_THAN");
    r_handlers.put(t_greater_than, binary_op_handler(r_greater_than_id)); 
    exec_handlers.put(r_greater_than_id, arithmetic_handler([](auto a, auto b){ return a>b; }, GET_TYPE(BOOL)));

    size_t less_than_id = reg::new_type("LESS_THAN");
    state_is_opp.put(less_than_id,true);
    token_to_opp.put(langle_id,less_than_id); 
    type_precdence.put(less_than_id,1); 
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
        type_precdence.put(var_decl_id,1);
        size_t t_var_decl_id = reg::new_type("T_VAR_DECL");
        t_functions.put(var_decl_id, [](t_context& ctx) -> g_ptr<t_node> {
            t_variable_decleration(ctx.result, ctx.node);
            return ctx.result;
        });
        discover_handlers.put(t_var_decl_id, [](g_ptr<t_node> node, d_context& ctx) {
            discover_var_decleration(node, ctx.root, ctx.idx);
        });    
        size_t object_id = reg::new_type("OBJECT");
        value_to_string.put(object_id,[](void* data){
            return "OBJECT";
        });
        size_t r_var_decl_id = reg::new_type("R_VAR_DECL");
        r_handlers.put(t_var_decl_id, [r_var_decl_id,object_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_var_decl_id;
            result->value.type = ctx.node->value.type;
            result->value.size = ctx.node->value.size;
            result->name = ctx.node->name;
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
            }
        });
        exec_handlers.put(r_var_decl_id, [object_id](exec_context& ctx) -> g_ptr<r_node> {
            if(ctx.node->value.type == object_id) {
                g_ptr<Object> obj = ctx.node->resolved_type->create();
                ctx.frame->slots[ctx.node->slot] = obj->ID;
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
        t_functions.put(literal_identifier_id, t_literal_handler);
        size_t r_identifier_id = reg::new_type("R_IDENTIFIER"); 
        r_handlers.put(t_identifier_id, [r_identifier_id](g_ptr<r_node> result, r_context& ctx) {
            result->type = r_identifier_id;
            resolve_identifier(ctx.node, result, ctx.scope, ctx.frame);
        });
        exec_handlers.put(r_identifier_id, [](exec_context& ctx) -> g_ptr<r_node> {
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
        a_functions.put(int_id, literal_handler);
        type_key_to_type.put(int_key_id, int_id);
        t_literal_handlers.put(int_id, [t_literal_id, int_id](g_ptr<Token> token) -> g_ptr<t_node> {
            g_ptr<t_node> node = make<t_node>();
            node->type = t_literal_id;
            node->value.type = int_id;
            node->value.set<int>(std::stoi(token->content));
            return node;
        });
        
        size_t float_key_id = reg::new_type("FLOAT_KEY");
        reg_t_key("float", float_key_id, 4, GET_TYPE(F_TYPE_KEY));
        a_functions.put(float_key_id, type_key_handler);
        size_t float_id = reg::new_type("FLOAT");
        value_to_string.put(float_id,[](void* data){
            return std::to_string(*(float*)data);
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

static void reg_b_types() {
   
    reg::new_type("TYPE"); reg::new_type("TYPE_KEY"); 
    reg::new_type("LBRACE"); reg::new_type("RBRACE"); 
    reg::new_type("LPAREN"); reg::new_type("RPAREN"); 
    reg::new_type("LBRACKET"); reg::new_type("RBRACKET"); 
    reg::new_type("END");   reg::new_type("METHOD_KEY");
    reg::new_type("PRINT_KEY");  reg::new_type("NOT"); reg::new_type("COMMA");
   
}
static void init_t_keys() {
    reg_t_key("type", GET_TYPE(TYPE_KEY), 8, GET_TYPE(F_TYPE_KEY)); 
    reg_t_key("print", GET_TYPE(PRINT_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("return", GET_TYPE(RETURN_KEY), 0, GET_TYPE(F_KEYWORD)); 
     reg_t_key("if", GET_TYPE(IF_KEY), 0, GET_TYPE(F_KEYWORD)); 
    reg_t_key("else", GET_TYPE(ELSE_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("break", GET_TYPE(BREAK_KEY), 0, GET_TYPE(F_KEYWORD)); 
    reg_t_key("while", GET_TYPE(WHILE_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("do", GET_TYPE(DO_KEY), 0, GET_TYPE(F_KEYWORD));
 }
static void reg_a_types() {
    reg::new_type("VAR_DECL_INIT"); reg::new_type("METHOD_CALL"); reg::new_type("METHOD_DECL");
    reg::new_type("TYPE_DECL");  reg::new_type("ENTER_SCOPE"); reg::new_type("EXIT_SCOPE");
    reg::new_type("PRINT_CALL"); reg::new_type("ENTER_PAREN"); reg::new_type("EXIT_PAREN");  
    reg::new_type("RETURN_CALL"); reg::new_type("ARGUMENT_GROUP"); reg::new_type("IF_DECL"); reg::new_type("ELSE_DECL"); 
    reg::new_type("BREAK_CALL"); reg::new_type("WHILE_DECL"); reg::new_type("DO_DECL");
 }
static void reg_s_types() { 
    reg::new_type("GLOBAL"); reg::new_type("TYPE_DEF"); reg::new_type("METHOD"); reg::new_type("FUNCTION");
    reg::new_type("IF_BLOCK"); reg::new_type("BLOCK"); reg::new_type("WHILE_LOOP"); reg::new_type("FOR_LOOP");
}
static void reg_t_types() {
    reg::new_type("T_METHOD_CALL"); reg::new_type("T_RETURN"); reg::new_type("T_METHOD_DECL"); reg::new_type("T_IF");
    reg::new_type("T_ELSE"); reg::new_type("T_BLOCK");  reg::new_type("T_PRINT"); reg::new_type("T_TYPE_DECL");
    reg::new_type("T_WHILE"); reg::new_type("T_BREAK"); reg::new_type("T_DO");
 }
static void reg_r_types() {
    reg::new_type("R_IF");   reg::new_type("R_METHOD_CALL");
     reg::new_type("R_TYPE_DECL"); reg::new_type("R_METHOD_DECL"); 
    reg::new_type("R_PRINT_CALL"); reg::new_type("R_RETURN"); reg::new_type("R_ELSE");  reg::new_type("R_BREAK");
    reg::new_type("R_WHILE"); reg::new_type("R_DO");
 }

static void a_function_blob() {
    a_functions.put(GET_TYPE(END), [](a_context& ctx) {
        if(ctx.state != GET_TYPE(UNTYPED)) {
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
        }
        ctx.pos = 0;
    });

    a_functions.put(GET_TYPE(LBRACE), [](a_context& ctx) {
        if(ctx.state != GET_TYPE(UNTYPED)) {
            ctx.end_lambda();
        }
        ctx.state = GET_TYPE(ENTER_SCOPE);
        ctx.end_lambda();
        ctx.state = GET_TYPE(UNTYPED);
        ctx.pos = 0;
    });

    a_functions.put(GET_TYPE(RBRACE), [](a_context& ctx) {
        if(ctx.state != GET_TYPE(UNTYPED)) {
            ctx.end_lambda();
        }
        ctx.state = GET_TYPE(EXIT_SCOPE);
        ctx.end_lambda();
        ctx.state = GET_TYPE(UNTYPED);
        ctx.pos = 0;
    });

    a_functions.put(GET_TYPE(LPAREN), [](a_context& ctx) {
        auto paren_range = balance_tokens(ctx.tokens, GET_TYPE(LPAREN), GET_TYPE(RPAREN), ctx.index-1);
        if (paren_range.x() < 0 || paren_range.y() < 0) {
            print("parse_tokens::719 Unmatched parenthesis at ", ctx.index);
            return;
        }

        if (ctx.state == GET_TYPE(PROP_ACCESS)) {
            ctx.state = GET_TYPE(METHOD_CALL);
        } else if (ctx.pos >= 2 && ctx.state == GET_TYPE(VAR_DECL)) {
            ctx.state = GET_TYPE(METHOD_DECL);
        } 

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

    a_functions.put(GET_TYPE(TYPE_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(TYPE_DECL);
    });

    a_functions.put(GET_TYPE(PRINT_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(PRINT_CALL);
    });

    a_functions.put(GET_TYPE(RETURN_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(RETURN_CALL);
    });

    a_functions.put(GET_TYPE(BREAK_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(BREAK_CALL);
    });

    a_functions.put(GET_TYPE(IF_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(IF_DECL);
    });

    a_functions.put(GET_TYPE(WHILE_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(WHILE_DECL);
    });

    a_functions.put(GET_TYPE(ELSE_KEY), [](a_context& ctx) {
        if(ctx.state == GET_TYPE(IF_DECL)) {
            ctx.end_lambda();
        }
        ctx.state = GET_TYPE(ELSE_DECL);
        ctx.end_lambda();
        ctx.state = GET_TYPE(UNTYPED);
    });

    a_functions.put(GET_TYPE(DO_KEY), [](a_context& ctx) {
        ctx.state = GET_TYPE(DO_DECL);
        ctx.end_lambda();
        ctx.state = GET_TYPE(UNTYPED);
    });

    a_functions.put(GET_TYPE(COMMA), [](a_context& ctx) {
        if(ctx.local && ctx.state != GET_TYPE(UNTYPED)) {
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
        }
    });

    a_functions.put(GET_TYPE(RPAREN), [](a_context& ctx) {
        if(ctx.local && ctx.state != GET_TYPE(UNTYPED)) {
            ctx.end_lambda();
            ctx.state = GET_TYPE(UNTYPED);
        }
    });
}

static void scope_function_blob() {
    scope_link_handlers.put(GET_TYPE(TYPE_DECL), [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
        new_scope->scope_type = GET_TYPE(TYPE_DEF);
        new_scope->owner = owner_node;
        owner_node->owned_scope = new_scope.getPtr();
    });
    
    scope_link_handlers.put(GET_TYPE(METHOD_DECL), [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
        if(current_scope->scope_type == GET_TYPE(TYPE_DEF))
            new_scope->scope_type = GET_TYPE(METHOD);
        else 
            new_scope->scope_type = GET_TYPE(FUNCTION);
        new_scope->owner = owner_node;
        owner_node->owned_scope = new_scope.getPtr();
    });
    
    auto if_handler = [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
        new_scope->scope_type = GET_TYPE(IF_BLOCK);
        new_scope->owner = owner_node;
        owner_node->owned_scope = new_scope.getPtr();
    };
    scope_link_handlers.put(GET_TYPE(IF_DECL), if_handler);
    scope_link_handlers.put(GET_TYPE(ELSE_DECL), if_handler);
    
    auto loop_handler = [](g_ptr<s_node> new_scope, g_ptr<s_node> current_scope, g_ptr<a_node> owner_node) {
        new_scope->scope_type = GET_TYPE(WHILE_LOOP);
        new_scope->owner = owner_node;
        owner_node->owned_scope = new_scope.getPtr();
    };
    scope_link_handlers.put(GET_TYPE(WHILE_DECL), loop_handler);
    scope_link_handlers.put(GET_TYPE(DO_DECL), loop_handler);

    scope_precedence.put(GET_TYPE(ENTER_SCOPE), 10); scope_precedence.put(GET_TYPE(EXIT_SCOPE), -10);
    scope_precedence.put(GET_TYPE(WHILE_DECL), 3); scope_precedence.put(GET_TYPE(DO_DECL), 2);
    scope_precedence.put(GET_TYPE(IF_DECL), 2); scope_precedence.put(GET_TYPE(ELSE_DECL), 1);
}

static void t_function_blob_top() {

     t_opp_conversion.put(GET_TYPE(IF_DECL), GET_TYPE(T_IF)); t_opp_conversion.put(GET_TYPE(ELSE_DECL), GET_TYPE(T_ELSE));
    t_opp_conversion.put(GET_TYPE(WHILE_DECL), GET_TYPE(T_WHILE)); t_opp_conversion.put(GET_TYPE(BREAK_CALL), GET_TYPE(T_BREAK)); 
    t_opp_conversion.put(GET_TYPE(METHOD_CALL), GET_TYPE(T_METHOD_CALL)); t_opp_conversion.put(GET_TYPE(RETURN_CALL), GET_TYPE(T_RETURN));
 }

static void t_function_blob_bottom() {

    t_functions.put(GET_TYPE(TYPE_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_TYPE_DECL);
        ctx.result->name = ctx.node->tokens.last()->content;
        ctx.result->scope = ctx.node->owned_scope;
        return ctx.result;
    });

    t_functions.put(GET_TYPE(METHOD_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_METHOD_DECL);
        ctx.result->name = ctx.node->tokens.last()->content;
        ctx.result->scope = ctx.node->owned_scope;
        ctx.result->scope->t_owner = ctx.result;
        if(!ctx.node->sub_nodes.empty()) {
            for(auto c : ctx.node->sub_nodes) {
                ctx.result->children << parse_a_node(c, ctx.root);
            }
        }
        return ctx.result;
    });

    t_functions.put(GET_TYPE(METHOD_CALL), [](t_context& ctx) -> g_ptr<t_node> {
        g_ptr<t_node> result = t_parse_expression(ctx.node, ctx.left);
        if(!ctx.node->sub_nodes.empty()) {
            for(auto c : ctx.node->sub_nodes) {
                result->children << parse_a_node(c, ctx.root);
            }
        }
        return result;
    });

    t_functions.put(GET_TYPE(RETURN_CALL), [](t_context& ctx) -> g_ptr<t_node> {
        return t_parse_expression(ctx.node, ctx.result);
    });

    t_functions.put(GET_TYPE(BREAK_CALL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_BREAK);
        return ctx.result;
    });


    t_functions.put(GET_TYPE(PROP_ACCESS), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_PROP_ACCESS);
        ctx.result->left = t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]);
        ctx.result->right = t_literal_handlers.get(ctx.node->tokens[1]->getType())(ctx.node->tokens[1]);
        return ctx.result;
    });

    t_functions.put(GET_TYPE(IF_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        g_ptr<t_node> result = t_parse_expression(ctx.node, nullptr);
        result->scope = ctx.node->owned_scope;
        return result;
    });

    t_functions.put(GET_TYPE(ELSE_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_ELSE);
        ctx.result->scope = ctx.node->owned_scope;
        ctx.left->left = ctx.result;
        return nullptr;
    });

    t_functions.put(GET_TYPE(WHILE_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        g_ptr<t_node> result = t_parse_expression(ctx.node, nullptr);
        result->scope = ctx.node->owned_scope;
        if(ctx.left && ctx.left->type == GET_TYPE(T_DO)) {
            result->left = ctx.left;
            result->scope = ctx.left->scope;
            ctx.root->t_nodes.erase(ctx.left);
        }
        return result;
    });

    t_functions.put(GET_TYPE(DO_DECL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_DO);
        ctx.result->scope = ctx.node->owned_scope;
        return ctx.result;
    });

    t_functions.put(GET_TYPE(PRINT_CALL), [](t_context& ctx) -> g_ptr<t_node> {
        ctx.result->type = GET_TYPE(T_PRINT);
        g_ptr<t_node> sub = nullptr;
        for(auto a : ctx.node->sub_nodes) {
            g_ptr<t_node> n_sub = parse_a_node(a, ctx.root, sub);
            if(sub && n_sub->left == sub) {
                ctx.result->children.erase(sub);
            }
            sub = n_sub;
            ctx.result->children << sub;
        }
        return ctx.result;
    });
}

static void discover_function_blob() {
    discover_handlers.put(GET_TYPE(T_TYPE_DECL), [](g_ptr<t_node> node, d_context& ctx) {
        if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
        node->scope->type_ref->type_name = node->name;
    });

    discover_handlers.put(GET_TYPE(T_METHOD_DECL), [](g_ptr<t_node> node, d_context& ctx) {
        if(node->scope) {
            if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
            node->scope->type_ref->type_name = node->name;
            
            for(auto c : node->children) {
                if(c->type == GET_TYPE(T_VAR_DECL))
                    discover_var_decleration(c, node->scope, ctx.idx);
            }
        }
    });
}
static void r_function_blob() {
   
    r_handlers.put(GET_TYPE(T_METHOD_CALL), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_METHOD_CALL);
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

   

    r_handlers.put(GET_TYPE(T_IF), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_IF);
        result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        result->frame = resolve_symbols(ctx.node->scope);
        if(ctx.node->left) {
            result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
        }
    });

    r_handlers.put(GET_TYPE(T_ELSE), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_ELSE);
        result->frame = resolve_symbols(ctx.node->scope);
    });

    r_handlers.put(GET_TYPE(T_WHILE), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_WHILE);
        result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        result->frame = resolve_symbols(ctx.node->scope);
        if(ctx.node->left) {
            result->left = resolve_symbol(ctx.node->left, ctx.scope, ctx.frame);
            result->left->frame = result->frame;
        }
    });

    r_handlers.put(GET_TYPE(T_DO), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_DO);
    });

    r_handlers.put(GET_TYPE(T_TYPE_DECL), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_TYPE_DECL);
        result->frame = resolve_symbols(ctx.node->scope);
    });

    r_handlers.put(GET_TYPE(T_METHOD_DECL), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_METHOD_DECL);
        result->name = ctx.node->name;
        result->frame = resolve_symbols(ctx.node->scope);
        for(auto c : ctx.node->children) {
            g_ptr<r_node> child = resolve_symbol(c, ctx.node->scope, result->frame);
            resolve_identifier(c, child, ctx.node->scope, result->frame);
            result->children << child;
        }
        ctx.scope->method_map.put(result->name, result);
    });

    r_handlers.put(GET_TYPE(T_RETURN), [](g_ptr<r_node> result, r_context& ctx) {
        result->right = resolve_symbol(ctx.node->right, ctx.scope, ctx.frame);
        g_ptr<s_node> on_scope = ctx.scope;
        g_ptr<Frame> method_frame = nullptr;
        while(!method_frame) {
            if(on_scope->scope_type == GET_TYPE(METHOD) || on_scope->scope_type == GET_TYPE(FUNCTION)) {
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
        result->type = GET_TYPE(R_RETURN);
    });

    r_handlers.put(GET_TYPE(T_BREAK), [](g_ptr<r_node> result, r_context& ctx) {
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
        result->type = GET_TYPE(R_BREAK);
    });

    

    r_handlers.put(GET_TYPE(T_PRINT), [](g_ptr<r_node> result, r_context& ctx) {
        result->type = GET_TYPE(R_PRINT_CALL);
        for(auto c : ctx.node->children) {
            g_ptr<r_node> sub = resolve_symbol(c, ctx.scope, ctx.frame);
            result->children << sub;
        }
    });
}

static void exec_function_blob() {
    
    exec_handlers.put(GET_TYPE(R_IF), [](exec_context& ctx) -> g_ptr<r_node> {
        execute_r_node(ctx.node->right, ctx.frame, ctx.index);
        if(ctx.node->right->value.is_true()) {
            execute_r_nodes(ctx.node->frame);
        }
        else if(ctx.node->left) {
            execute_r_nodes(ctx.node->left->frame);
        }
        return ctx.node;
    });

    exec_handlers.put(GET_TYPE(R_PRINT_CALL), [](exec_context& ctx) -> g_ptr<r_node> {
        std::string toPrint = "";
        for(auto r : ctx.node->children) {
            execute_r_node(r, ctx.frame, ctx.index);
            toPrint.append(r->value.to_string());
        }
        print(toPrint);
        return ctx.node;
    });
    stream_handlers.put(GET_TYPE(R_PRINT_CALL), [](exec_context& ctx) -> std::function<void()>{
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

    exec_handlers.put(GET_TYPE(R_TYPE_DECL), [](exec_context& ctx) -> g_ptr<r_node> {
        // Probably do nothing for now
        // execute_r_nodes(ctx.node->frame);
        return ctx.node;
    });
    stream_handlers.put(GET_TYPE(R_TYPE_DECL), [](exec_context& ctx) -> std::function<void()>{
       return nullptr;
    });

    exec_handlers.put(GET_TYPE(R_METHOD_CALL), [](exec_context& ctx) -> g_ptr<r_node> {
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

    exec_handlers.put(GET_TYPE(R_RETURN), [](exec_context& ctx) -> g_ptr<r_node> {
        execute_r_node(ctx.node->right, ctx.frame, ctx.index);
        if(ctx.node->frame)
            ctx.node->frame->return_val = std::move(ctx.node->right->value);
        return ctx.node;
    });

    exec_handlers.put(GET_TYPE(R_BREAK), [](exec_context& ctx) -> g_ptr<r_node> {
        if(ctx.node->frame)
            ctx.node->frame->stop();
        return ctx.node;
    });

    exec_handlers.put(GET_TYPE(R_WHILE), [](exec_context& ctx) -> g_ptr<r_node> {
        ctx.node->frame->resurrect();
        execute_r_node(ctx.node->right, ctx.frame, ctx.index);
        while(ctx.node->right->value.is_true()) {
            execute_r_node(ctx.node->right, ctx.frame, ctx.index);
            if (!ctx.node->right->value.is_true()) break; 
            execute_r_nodes(ctx.node->frame);
        }
        return ctx.node;
    });
}