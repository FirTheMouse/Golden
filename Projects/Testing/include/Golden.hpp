#pragma once

#include <util/util.hpp>
#include<core/type.hpp>
#include<util/group.hpp>
#include<util/d_list.hpp>



constexpr uint32_t hashString(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

uint32_t hashString(const std::string& str) {
    return hashString(str.c_str());
}

#define TYPE_ID(name) hashString(#name)

class reg {
    static inline map<uint32_t,uint32_t> hash_to_enum;
    static inline map<uint32_t,std::string> enum_to_string;
    static inline uint32_t next_id = 0; //Consider making this atomic if the maps become thread safe
public:
    static uint32_t new_type(const std::string& name) {
        //Put logic here to warn if there's duplicate keys
        uint32_t hash = hashString(name.c_str());
        uint32_t enum_id = next_id++;
        
        hash_to_enum.put(hash,enum_id);
        enum_to_string.put(enum_id,name);
        return enum_id;
    }

    static uint32_t ID(uint32_t hash) {
        if(!hash_to_enum.hasKey(hash)) { //Horrible for performance, replace this later but keep it for security now
            print("reg::40 Warning, key not found for enum ",hash,"!");
            return 0;
        }
        else return hash_to_enum.get(hash);
    }

    static std::string name(uint32_t ID) {
        if(!enum_to_string.hasKey(ID)) { //Horrible for performance, replace this later but keep it for security now
            print("reg::50 Warning, key not found for enum ",ID,"!");
            return "[null]";
        }
        else return enum_to_string.get(ID);
    }

    static void printRegistry() {
        print("-- registry print --");
        for (const auto& pair : enum_to_string.entrySet()) {
            print(pair.key," -> ",pair.value);
        }
    }
};


//For debug
//#define GET_TYPE(name) (print("Looking up: " #name), reg::ID(TYPE_ID(name)))
#define GET_TYPE(name) reg::ID(TYPE_ID(name))
#define TO_STRING(ID) reg::name(ID)
#define IS_TYPE(node, type_name) ((node)->type == GET_TYPE(type_name))

list<std::function<void()>> tokenize_functions;

// class h_node : public Object {
//     public:
//         h_node() {}
//         h_node(uint32_t _type,std::string _content) : type(_type), content(_content) {}
//         uint32_t type;
//         std::string content;
// };

// static void prime_test() {
//     print("Registiering");
//     uint32_t if_id = g_register::registerType("TEST");
//     uint32_t other_id = g_register::registerType("TESTA");
// }

// static void run_test() {
//     print("Making");
//     g_ptr<h_node> node = make<h_node>(GET_TYPE(TESTA),"test2");
//     print("Testing");
//     print("Is type: ",IS_TYPE(node,TESTA));
//     print("To string: ",TO_STRING(node->type));
// }



static std::string fallback = "[undefined]";

static void reg_b_types() {
    reg::new_type("UNDEFINED");
    reg::new_type("OBJECT"); reg::new_type("TYPE"); reg::new_type("LITERAL"); reg::new_type("IDENTIFIER"); reg::new_type("DOT");
    reg::new_type("LBRACE"); reg::new_type("RBRACE"); reg::new_type("LPAREN"); reg::new_type("RPAREN"); reg::new_type("LBRACKET");
    reg::new_type("RBRACKET"); reg::new_type("BOOL"); reg::new_type("END"); reg::new_type("TYPE_KEY"); reg::new_type("IF_KEY");
    reg::new_type("ELSE_KEY"); reg::new_type("INT_KEY"); reg::new_type("FLOAT_KEY"); reg::new_type("STRING_KEY"); reg::new_type("METHOD_KEY");
    reg::new_type("RETURN_KEY"); reg::new_type("VOID_KEY"); reg::new_type("INT"); reg::new_type("FLOAT"); reg::new_type("CHAR");
    reg::new_type("STRING"); reg::new_type("PRINT_KEY"); reg::new_type("EQUALS"); reg::new_type("PLUS"); reg::new_type("PLUS_EQUALS");
    reg::new_type("EQUALS_EQUALS"); reg::new_type("NOT_EQUALS"); reg::new_type("PLUS_PLUS"); reg::new_type("MINUS"); reg::new_type("MINUS_MINUS");
    reg::new_type("MINUS_EQUALS"); reg::new_type("LANGLE"); reg::new_type("RANGLE"); reg::new_type("NOT"); reg::new_type("COMMA");
    reg::new_type("CHAR_KEY"); reg::new_type("BOOL_KEY"); reg::new_type("STAR"); reg::new_type("SLASH"); reg::new_type("STAR_EQUALS");
    reg::new_type("SLASH_EQUALS"); reg::new_type("WHILE_KEY"); reg::new_type("BREAK_KEY"); reg::new_type("DO_KEY");

    reg::new_type("F_TYPE_KEY");  reg::new_type("F_KEYWORD");

 }
struct t_info {
    t_info(uint32_t _type, size_t _size, uint32_t _family) : type(_type), size(_size), family(_family) {}
    t_info(uint32_t _type, size_t _size) : type(_type), size(_size) {}
    t_info() {}
    uint32_t type = 0;
    size_t size = 0;
    uint32_t family = 0;
};

class Token : public Object {
    public:
        Token(uint32_t _type,const std::string& _content) 
        : content(_content) {
            type_info = t_info(_type,0,GET_TYPE(LITERAL));
        }
        ~Token() {}
    
        int index = -1; 
        bool parsed = false;
        t_info type_info;
        std::string content;    
        uint32_t getType() {return type_info.type;}
        uint32_t getFamily() {return type_info.family;}

        void setType(uint32_t type) {type_info.type = type;}
        void setFamily(uint32_t type) {type_info.family = type;}
        void mark() {parsed = true;}

        void add(const std::string& _content) {
           content.append(_content);
        }
        void add(char c) {
            content += c;
         }
};

static bool token_is_split(char c) {
    return (c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='('||c==')'||c==','||c=='='||c=='>'||c=='<');
}

// static b_type key_to_type(b_type key) {
//     switch(key) {
//         case TYPE_KEY: return TYPE;
//         case INT_KEY: return INT;
//         case FLOAT_KEY: return FLOAT;
//         case STRING_KEY: return STRING;
//         case BOOL_KEY: return BOOL;
//         case CHAR_KEY: return CHAR;
//         default: return UNDEFINED;
//     }
// }

// static size_t key_to_size(uint32_t key) {
//     switch(key) {
//         case TYPE_KEY: return 8;
//         case INT_KEY: return 4;
//         case FLOAT_KEY: return 4;
//         case STRING_KEY: return 32;
//         case BOOL_KEY: return 1;
//         case CHAR_KEY: return 1;
//         default: return UNDEFINED;
//     }
// }

static map<std::string,t_info> t_keys;

static void reg_t_key(std::string name,const t_info& info) {
    t_keys.put(name,info);
}

static void reg_t_key(std::string name,uint32_t enum_key,size_t size, uint32_t family_key) {
    reg_t_key(name,t_info(enum_key,size,family_key));
}

static void init_t_keys() {
    reg_t_key("type", GET_TYPE(TYPE_KEY), 8, GET_TYPE(F_TYPE_KEY)); reg_t_key("int", GET_TYPE(INT_KEY), 4, GET_TYPE(F_TYPE_KEY)); 
    reg_t_key("float", GET_TYPE(FLOAT_KEY), 4, GET_TYPE(F_TYPE_KEY)); reg_t_key("string", GET_TYPE(STRING_KEY), 32, GET_TYPE(F_TYPE_KEY)); 
    reg_t_key("bool", GET_TYPE(BOOL_KEY), 1, GET_TYPE(F_TYPE_KEY)); reg_t_key("char", GET_TYPE(CHAR_KEY), 1, GET_TYPE(F_TYPE_KEY)); 
    reg_t_key("print", GET_TYPE(PRINT_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("return", GET_TYPE(RETURN_KEY), 0, GET_TYPE(F_KEYWORD)); 
    reg_t_key("void", GET_TYPE(VOID_KEY), 0, GET_TYPE(F_KEYWORD));  reg_t_key("if", GET_TYPE(IF_KEY), 0, GET_TYPE(F_KEYWORD)); 
    reg_t_key("else", GET_TYPE(ELSE_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("break", GET_TYPE(BREAK_KEY), 0, GET_TYPE(F_KEYWORD)); 
    reg_t_key("while", GET_TYPE(WHILE_KEY), 0, GET_TYPE(F_KEYWORD)); reg_t_key("do", GET_TYPE(DO_KEY), 0, GET_TYPE(F_KEYWORD));
 }


static list<g_ptr<Token>> tokenize(const std::string& code,char end_char = ';') {
    auto it = code.begin();
    list<g_ptr<Token>> result;
    g_ptr<Token> token = nullptr;

    enum State {
       OPEN, IN_STRING, IN_NUMBER, IN_IDENTIFIER
    };

    State state = OPEN;
    t_info fallback;
    auto check_type = [&](){                
        t_info result = t_keys.getOrDefault(token->content, fallback);
        if(result.type != GET_TYPE(UNDEFINED)) {
            token->type_info = result;
        }
    };


    while (it != code.end()) {
        char c = *it;
        if(state==IN_STRING) {
            if(c=='"') {
                state=OPEN;
                token->type_info.size = 32;
            }
            else {
                token->add(c);
            }
            ++it;
        }
        else if(state==IN_IDENTIFIER) {
            if(c==end_char||c=='.'||token_is_split(c)) {
                state=OPEN;
                check_type();
                continue;
            }
            else if(c==' '||c=='\t'||c=='\n') {
                check_type();
                state=OPEN;
            }
            else {
                token->add(c);
            }
            ++it;
        }
        else if(state==IN_NUMBER) {
            if(c==end_char||token_is_split(c)) {
                state=OPEN;
                continue;
            }
            else if(c==' '||c=='\t'||c=='\n') {
                state=OPEN;
            }
            else if(c=='.') {
                if(token->check("dotted")) {
                    state=IN_IDENTIFIER;
                    token->setType(GET_TYPE(IDENTIFIER));
                    token->type_info.size = 0;
                    token->add(c);
                }
                else {
                    token->setType(GET_TYPE(FLOAT));
                    token->type_info.size = 4;
                    token->flagOn("dotted");
                    token->add(c);
                }
            }
            else {
                token->add(c);
            }
            ++it;
        }
        else {
            switch (c) {
                case ' ': case '\t': case '\n':
                    break;
                case '.':
                    token = make<Token>(GET_TYPE(DOT),".");
                    result << token;
                    break;
                case ',':
                    token = make<Token>(GET_TYPE(COMMA),",");
                    result << token;
                    break;
                case '"':
                    state = IN_STRING;
                    token = make<Token>(GET_TYPE(STRING),"");
                    result << token;
                    break;
                case '+':
                    if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(PLUS_EQUALS),"+=");
                        result << token;
                        ++it; //Double move so we skip
                    }
                    else if(*(it+1)=='+') {
                        token = make<Token>(GET_TYPE(PLUS_PLUS),"++");
                        result << token;
                        ++it;
                    }
                    else {
                        token = make<Token>(GET_TYPE(PLUS),"+");
                        result << token;
                    }
                    break;
                case '-':
                    if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(MINUS_EQUALS),"-=");
                        result << token;
                        ++it; 
                    }
                    else if(*(it+1)=='-') {
                        token = make<Token>(GET_TYPE(MINUS_MINUS),"--");
                        result << token;
                        ++it;
                    }
                    else {
                        token = make<Token>(GET_TYPE(MINUS),"-");
                        result << token;
                    }
                    break;
                case '*':
                    if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(STAR_EQUALS),"*=");
                        result << token;
                        ++it;
                    }
                    else {
                        token = make<Token>(GET_TYPE(STAR),"*");
                        result << token;
                    }
                    break;
                case '/':
                    if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(SLASH_EQUALS),"/=");
                        result << token;
                        ++it;
                    }
                    else {
                        token = make<Token>(GET_TYPE(SLASH),"/");
                        result << token;
                    }
                    break;
                case '=':
                    if(*(it+1)=='+') {
                        token = make<Token>(GET_TYPE(PLUS_EQUALS),"=+");
                        result << token;
                        ++it; 
                    }
                    else if(*(it+1)=='-') {
                        token = make<Token>(GET_TYPE(MINUS_EQUALS),"=-");
                        result << token;
                        ++it;
                    }
                    else if(*(it+1)=='*') {
                        token = make<Token>(GET_TYPE(STAR_EQUALS),"=*");
                        result << token;
                        ++it;
                    }
                    else if(*(it+1)=='/') {
                        token = make<Token>(GET_TYPE(SLASH_EQUALS),"=/");
                        result << token;
                        ++it;
                    }
                    else if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(EQUALS_EQUALS),"==");
                        result << token;
                        ++it;
                    }
                    else {
                        token = make<Token>(GET_TYPE(EQUALS),"=");
                        result << token;
                    }
                    break;
                case '!':
                    if(*(it+1)=='=') {
                        token = make<Token>(GET_TYPE(NOT_EQUALS),"!=");
                        result << token;
                        ++it; 
                    }
                    else {
                        token = make<Token>(GET_TYPE(NOT),"!");
                        result << token;
                    }
                    break;
                case '<':
                        token = make<Token>(GET_TYPE(LANGLE),"<");
                        result << token;
                    break;
                case '>':
                        token = make<Token>(GET_TYPE(RANGLE),">");
                        result << token;
                    break;
                case '{':
                        token = make<Token>(GET_TYPE(LBRACE),"{");
                        result << token;
                    break;
                case '}':
                    token = make<Token>(GET_TYPE(RBRACE),"}");
                    result << token;
                    break;
                case '(':
                    token = make<Token>(GET_TYPE(LPAREN),"(");
                    result << token;
                    break;
                case ')':
                    token = make<Token>(GET_TYPE(RPAREN),")");
                    result << token;
                    break;
                default:
                    if(std::isalpha(c)||c=='_') {
                        state = IN_IDENTIFIER;
                        token = make<Token>(GET_TYPE(IDENTIFIER),"");
                        token->add(c);
                        result << token;
                    }
                    else if(std::isdigit(c)) {
                        state = IN_NUMBER;
                        token = make<Token>(GET_TYPE(INT),"");
                        token->add(c);
                        result << token;
                    }
                    break;
            }
            if(c==end_char) {
                token = make<Token>(GET_TYPE(END),"");
                token->add(c);
                result << token;
            }
            ++it;
        }
    }

    for(auto t : result) {
        if(t->getType()) {
            print(TO_STRING(t->getType()));
        }
    }
    return result;
}

static void reg_a_types() {
    reg::new_type("UNTYPED"); reg::new_type("VAR_DECL"); reg::new_type("VAR_DECL_INIT"); reg::new_type("METHOD_CALL"); reg::new_type("METHOD_DECL");
    reg::new_type("TYPE_DECL"); reg::new_type("PROP_ACCESS"); reg::new_type("ASSIGNMENT"); reg::new_type("ENTER_SCOPE"); reg::new_type("EXIT_SCOPE");
    reg::new_type("PRINT_CALL"); reg::new_type("ENTER_PAREN"); reg::new_type("EXIT_PAREN"); reg::new_type("LITERAL"); reg::new_type("LITERAL_IDENTIFIER");
    reg::new_type("RETURN_CALL"); reg::new_type("ARGUMENT_GROUP"); reg::new_type("IF_DECL"); reg::new_type("ELSE_DECL"); reg::new_type("ADD");
    reg::new_type("SUBTRACT"); reg::new_type("MULTIPLY"); reg::new_type("DIVIDE"); reg::new_type("IS_EQUALS"); reg::new_type("GREATER_THAN");
    reg::new_type("LESS_THAN"); reg::new_type("BREAK_CALL"); reg::new_type("WHILE_DECL"); reg::new_type("DO_DECL");
 }

class s_node;

class a_node : public Object {
public:
    a_node() {}
    ~a_node() {}

    list<g_ptr<Token>> tokens;
    uint32_t type = GET_TYPE(UNTYPED);

    list<g_ptr<a_node>> sub_nodes;
    s_node* owned_scope = nullptr;
};

static void reg_s_types() { 
    reg::new_type("GLOBAL"); reg::new_type("TYPE_DEF"); reg::new_type("METHOD"); reg::new_type("FUNCTION");
    reg::new_type("IF_BLOCK"); reg::new_type("BLOCK"); reg::new_type("WHILE_LOOP"); reg::new_type("FOR_LOOP");
}


static void reg_t_types() {
    reg::new_type("T_NOP"); reg::new_type("T_LITERAL"); reg::new_type("T_IDENTIFIER"); reg::new_type("T_VAR_DECL"); reg::new_type("T_ASSIGN");
    reg::new_type("T_METHOD_CALL"); reg::new_type("T_RETURN"); reg::new_type("T_PROP_ACCESS"); reg::new_type("T_METHOD_DECL"); reg::new_type("T_IF");
    reg::new_type("T_ELSE"); reg::new_type("T_BLOCK"); reg::new_type("T_ADD"); reg::new_type("T_SUBTRACT"); reg::new_type("T_MULTIPLY");
    reg::new_type("T_DIVIDE"); reg::new_type("T_PRINT"); reg::new_type("T_GREATER_THAN"); reg::new_type("T_LESS_THAN"); reg::new_type("T_TYPE_DECL");
    reg::new_type("T_WHILE"); reg::new_type("T_BREAK"); reg::new_type("T_DO");
 }

/// @brief BIG WARNING! These leak right now and we need to add cleanup for data in the future!
struct t_value {
    uint32_t type = GET_TYPE(UNDEFINED);
    void* data;
    void* address;
    size_t size;

    template<typename T>
    T get() { return *(T*)data; }
    
    template<typename T>
    void set(const T& value) { 
        if (!data) {
            data = malloc(sizeof(T));
            size = sizeof(T);
        }
        *(T*)data = value; 
    }

    std::string to_string() {
        if (!data) return "[null]";
        
        if(IS_TYPE(this,INT)) {
            return std::to_string(*(int*)data);
        }
        else if(IS_TYPE(this,FLOAT)) {
            return std::to_string(*(float*)data);
        }
        else if(IS_TYPE(this,CHAR)) {
            return std::string(1, *(char*)data);
        }
        else if(IS_TYPE(this,BOOL)) {
            return (*(bool*)data) ? "TRUE" : "FALSE";
        }
        else if(IS_TYPE(this,OBJECT)) {
            return "OBJECT";
        }
        else {
            return "[unkown_type]";
        }
    }

    bool is_true() {
        if (IS_TYPE(this,BOOL) && data) {
            return *(bool*)data; 
        }
        return false;
    }
};


class r_node;
class Frame;

/// @brief WARNING! Value will cause memory leaks on deconstruction in the current version
class t_node : public Object {
public:
    uint32_t type = GET_TYPE(T_NOP);
    g_ptr<t_node> left;
    g_ptr<t_node> right;
    list<g_ptr<t_node>> children;
    list<g_ptr<t_node>> arguments;
    t_value value;
    std::string name;
    std::string deferred_identifier;
    s_node* scope = nullptr;
};

class s_node : public Object {
public:
    s_node() {

    }
    ~s_node() {
       children.destroy();

    }

    uint32_t scope_type = GET_TYPE(GLOBAL);
    list<g_ptr<Token>> tokens;

    g_ptr<s_node> parent;
    g_ptr<Type> type_ref = nullptr;
    g_ptr<Frame> frame = nullptr;

    g_ptr<a_node> owner;
    g_ptr<t_node> t_owner;
    list<g_ptr<a_node>> a_nodes;
    list<g_ptr<t_node>> t_nodes;
    list<g_ptr<s_node>> children;

    map<std::string,size_t> slot_map;
    map<std::string,g_ptr<Type>> type_map;
    map<std::string,size_t> size_map;
    map<std::string,uint32_t> o_type_map;
    map<std::string,g_ptr<r_node>> method_map;

    void addToken(g_ptr<Token> token) {
        token->index = tokens.length();
        tokens.push(token);
    }
    void addToken(uint32_t _type, const std::string& _content) {
        g_ptr<Token> token = make<Token>(_type,_content);
        addToken(token);
    }

    g_ptr<s_node> spawn_node() {
        g_ptr<s_node> new_node = make<s_node>();
        new_node->parent = this;
        children << new_node;
        return new_node; 
    }
};

void print_a_node(const g_ptr<a_node>& node, int depth = 0, int index = 0) {
    std::string indent(depth * 2, ' '); 
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type), " Subs: ", node->sub_nodes.size());

    if (!node->tokens.empty()) {
        print(indent, "  Tokens:");
        for (auto& t : node->tokens) {
            print(indent, "    ", t->content, " (", t->getType(), ")");
        }
    }

    if (!node->sub_nodes.empty()) {
        print(indent, "  Sub-nodes:");
        int sub_index = 0;
        for (auto& sub : node->sub_nodes) {
            print_a_node(sub, depth + 1, sub_index++);
        }
    }
}

static vec2 balance_tokens(list<g_ptr<Token>> tokens, uint32_t a, uint32_t b,int start = 0) {
    vec2 result(-1,-1);
    int depth = 0;
    for(int i=start;i<tokens.length();i++) {
        g_ptr<Token> t = tokens[i];
        if(t->getType()==a) {
            if(depth==0) {
                result.setX(i);
            }
            depth++;
        }
        else if(t->getType()==b) {
            depth--;
            if(depth<=0) {
                result.setY(i+1);
                break;
            }
        }
    }
    return result;
}

struct a_context {
    uint32_t& state;
    g_ptr<Token> token;
    g_ptr<a_node>& node;
    list<g_ptr<a_node>>& result;
    bool& no_add;
    std::function<void()> end_lambda;
    list<g_ptr<Token>>& tokens;
    int& index;
    g_ptr<Token>*& it;
    int& pos;
    bool local;
    int skip_inc = 0;
    
    a_context(uint32_t& state, g_ptr<Token> token, g_ptr<a_node>& node, 
              list<g_ptr<a_node>>& result, bool& no_add, std::function<void()> end_lambda,
              list<g_ptr<Token>>& tokens, int& index, g_ptr<Token>*& it, int& pos, bool local)
        : state(state), token(token), node(node), result(result), no_add(no_add),
          end_lambda(end_lambda), tokens(tokens), index(index), it(it), pos(pos), local(local) {}
    
    void end() { end_lambda(); }
};

using a_handler = std::function<void(a_context& ctx)>;
map<uint32_t,a_handler> a_functions;
map<uint32_t,bool> state_is_opp;
map<uint32_t,uint32_t> token_to_opp;
map<uint32_t,int> type_precdence;

static list<g_ptr<a_node>> parse_tokens(list<g_ptr<Token>> tokens,bool local = false);

// Temporary function for testing, these are to be broken out into their own units
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

    a_functions.put(GET_TYPE(IDENTIFIER), [](a_context& ctx) {
        if(ctx.state == GET_TYPE(UNTYPED)) {
            ctx.state = GET_TYPE(LITERAL_IDENTIFIER);
        }
        else if(ctx.state == GET_TYPE(LITERAL_IDENTIFIER)) {
            ctx.state = GET_TYPE(VAR_DECL);
        }
        ctx.node->tokens << ctx.token;
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

    // Multi-token handlers
    auto type_key_handler = [](a_context& ctx) {
        if(ctx.state == GET_TYPE(UNTYPED)) {
            ctx.state = GET_TYPE(VAR_DECL);
        }
        ctx.node->tokens << ctx.token;
    };
    
    a_functions.put(GET_TYPE(STRING_KEY), type_key_handler);
    a_functions.put(GET_TYPE(INT_KEY), type_key_handler);
    a_functions.put(GET_TYPE(FLOAT_KEY), type_key_handler);
    a_functions.put(GET_TYPE(BOOL_KEY), type_key_handler);
    a_functions.put(GET_TYPE(CHAR_KEY), type_key_handler);
    a_functions.put(GET_TYPE(VOID_KEY), type_key_handler);

    auto literal_handler = [](a_context& ctx) {
        if(ctx.state == GET_TYPE(UNTYPED)) {
            ctx.state = GET_TYPE(LITERAL);
        }
        ctx.node->tokens << ctx.token;
    };
    
    a_functions.put(GET_TYPE(STRING), literal_handler);
    a_functions.put(GET_TYPE(INT), literal_handler);
    a_functions.put(GET_TYPE(FLOAT), literal_handler);
    a_functions.put(GET_TYPE(BOOL), literal_handler);
    a_functions.put(GET_TYPE(CHAR), literal_handler);

    state_is_opp.put(GET_TYPE(ADD),true); state_is_opp.put(GET_TYPE(SUBTRACT),true); state_is_opp.put(GET_TYPE(MULTIPLY),true);
    state_is_opp.put(GET_TYPE(DIVIDE),true); state_is_opp.put(GET_TYPE(GREATER_THAN),true); state_is_opp.put(GET_TYPE(LESS_THAN),true);
    state_is_opp.put(GET_TYPE(ASSIGNMENT),true); state_is_opp.put(GET_TYPE(PROP_ACCESS),true); 

    token_to_opp.put(GET_TYPE(PLUS),GET_TYPE(ADD)); token_to_opp.put(GET_TYPE(MINUS),GET_TYPE(SUBTRACT)); token_to_opp.put(GET_TYPE(STAR),GET_TYPE(MULTIPLY)); 
    token_to_opp.put(GET_TYPE(SLASH),GET_TYPE(DIVIDE)); token_to_opp.put(GET_TYPE(LANGLE),GET_TYPE(LESS_THAN)); token_to_opp.put(GET_TYPE(RANGLE),GET_TYPE(GREATER_THAN)); 
    token_to_opp.put(GET_TYPE(EQUALS),GET_TYPE(ASSIGNMENT)); token_to_opp.put(GET_TYPE(DOT),GET_TYPE(PROP_ACCESS));
    
    type_precdence.put(GET_TYPE(ADD),1); type_precdence.put(GET_TYPE(SUBTRACT),1); type_precdence.put(GET_TYPE(MULTIPLY),2); type_precdence.put(GET_TYPE(DIVIDE),2);
    type_precdence.put(GET_TYPE(LESS_THAN),1); type_precdence.put(GET_TYPE(GREATER_THAN),1); type_precdence.put(GET_TYPE(ASSIGNMENT),1); type_precdence.put(GET_TYPE(VAR_DECL),1);
    type_precdence.put(GET_TYPE(PROP_ACCESS),3);
}

static list<g_ptr<a_node>> parse_tokens(list<g_ptr<Token>> tokens,bool local) {
        print("==PARSE TOKENS PASS==");
        list<g_ptr<a_node>> result;
        uint32_t state = GET_TYPE(UNTYPED);
        g_ptr<a_node> node = make<a_node>();
        bool no_add = false;

        auto end = [&](){
            node->type = state;
            if(no_add) 
                no_add = false;
            else
                result << node;
            node = make<a_node>();
        };

        auto it = tokens.begin();
        int pos = 1;
        int index = -1;
        a_context ctx(state, nullptr, node, result, no_add, end, tokens, index, it, pos, local);
        while(it!=tokens.end()) {
            if(!*it) break;
            index++;
            if(ctx.skip_inc>0) {
                ctx.skip_inc--;
                continue;
            }
            g_ptr<Token> token = *it;
            ctx.token = token;
            // print("State: ",a_type_string(state));
            // print("Reading: ",token->content);
            if(a_functions.hasKey(token->getType())) {
                auto func = a_functions.get(token->getType());
                func(ctx);
                if(ctx.skip_inc>0) {
                    ctx.skip_inc--;
                    continue;
                }
            }
            else {
                if(token_to_opp.getOrDefault(token->getType(),GET_TYPE(UNTYPED))!=GET_TYPE(UNTYPED)) {
                    if(state==GET_TYPE(LITERAL)||state==GET_TYPE(LITERAL_IDENTIFIER)||state==GET_TYPE(UNTYPED)) {
                        state=token_to_opp.get(token->getType());
                    }
                    else if(state_is_opp.getOrDefault(state,false)||state==GET_TYPE(VAR_DECL)||state==GET_TYPE(METHOD_CALL)||state==GET_TYPE(PROP_ACCESS)) {
                        end();
                        state=token_to_opp.get(token->getType());
                        if(state==GET_TYPE(PROP_ACCESS)||state==GET_TYPE(METHOD_CALL)) {
                            if(type_precdence.get(result.last()->type) < type_precdence.get(state)) {
                                result.last()->sub_nodes << node;
                                node->tokens << result.last()->tokens.pop();
                                no_add = true;
                            }
                        }
                    }
                }
            }
            ++it;
            ++pos;
        }

        if(!local)
        {
            int i = 0;
            for (auto& node : result) {
                print_a_node(node, 0, i++);
            }
        }

        return result;
}


static bool balance_nodes(list<g_ptr<a_node>>& result) {
    uint32_t state = GET_TYPE(UNTYPED);
    int corrections = 0;
    for (int i = result.size() - 1; i >= 0; i--) {
        g_ptr<a_node> right = result[i];
        if(!right->sub_nodes.empty()) {
            balance_nodes(right->sub_nodes);
        }
        if(i==0) continue;
        //print("Checking index:", i, " against ", i-1);
        state=result[i]->type;
        g_ptr<a_node> left = result[i-1];
        // print("LEFT: ",a_type_string(left->type));
        // print("RIGHT: ",a_type_string(right->type));
        if(state_is_opp.getOrDefault(state,false)&&state_is_opp.getOrDefault(left->type,false)) 
        {
            if(type_precdence.get(left->type)<type_precdence.get(state))
            {
                left->sub_nodes << right;
                right->tokens.insert(left->tokens.pop(),0);
                result.removeAt(i);
                corrections++;
            }
        }
    }

    return corrections!=0;
}

//Need seperate handeling for associativity in the future for proper precdence 
static void balance_precedence(list<g_ptr<a_node>>& result) {
    bool changed = true;
    int depth = 0;
    while (changed&&depth<10) {
        depth++;
        // print("==BALANCING PASS ",depth,"==");
        changed = balance_nodes(result);
    }

    print("Finished balancing");
    int i = 0;
    for (auto& node : result) {
        print_a_node(node, 0, i++);
    }
}


// static void link_scope_owner(g_ptr<a_node> owner_node, g_ptr<s_node> current_scope, g_ptr<s_node> new_scope) {
//     // Only link owner <-> owned_scope for nodes that semantically *own* a scope.
//     // I plan to use this later to distinguish declarations from definitions.
//     switch (owner_node->type) {
//         case TYPE_DECL:
//             new_scope->scope_type = TYPE_DEF;

//             new_scope->owner = owner_node;
//             owner_node->owned_scope = new_scope.getPtr();
//             break;
//         case METHOD_DECL:
//         if(current_scope->scope_type == TYPE_DEF)
//             new_scope->scope_type = METHOD;
//         else 
//             new_scope->scope_type = FUNCTION;
        
//             new_scope->owner = owner_node;
//             owner_node->owned_scope = new_scope.getPtr();
//             break;
//         case IF_DECL: case ELSE_DECL:
//             new_scope->scope_type = IF_BLOCK;

//             new_scope->owner = owner_node;
//             owner_node->owned_scope = new_scope.getPtr();
//             break;
//         case WHILE_DECL: case DO_DECL:
//             new_scope->scope_type = WHILE_LOOP;

//             new_scope->owner = owner_node;
//             owner_node->owned_scope = new_scope.getPtr();
//             break;
//         default:
//             new_scope->scope_type = BLOCK;
//             break;
//     }
// }

// void print_scope(g_ptr<s_node> scope, int depth = 0) {
//     std::string indent(depth * 2, ' ');
    
//     if (depth > 0) {
//         print(indent, "{");
//     }
    
//     for (auto& a_node : scope->a_nodes) {
//         print(indent, "  ", a_type_string(a_node->type));
    
        
//         if (a_node->owned_scope) {
//             print_scope(a_node->owned_scope, depth + 1);
//         }
//     }
    
//     if (depth > 0) {
//         print(indent, "}");
//     }
// }

// struct g_value {
// public:
//     g_value() {}
//     g_value(g_ptr<a_node> _owner) : owner(_owner) {}
//     bool explc = false;
//     g_ptr<a_node> owner = nullptr;
//     bool deferred = false;
// };

// static int scope_precedence(g_ptr<a_node> node) {
//     switch(node->type) {
//         case ENTER_SCOPE: return 10;
//         case EXIT_SCOPE: return -10;
//         case WHILE_DECL: return 3; //No idea what the precdence of this should be
//         case DO_DECL: return 2; //No idea what the precdence of this should be
//         case IF_DECL: return 2;
//         case ELSE_DECL: return 1;
//         default: return 0;
//     }
// }

// /// @brief  This method is not yet complete! Just like the other precdence, it is in need of refinment and 
// /// edge case strengthining in 0.0.9. 
// /// No noted edge cases in light testing
// /// If you don't see a scope in the print out, check link owners, chances are there's no case for linking them meaning it can't be found
// static g_ptr<s_node> parse_scope(list<g_ptr<a_node>> nodes) {
//     g_ptr<s_node> root_scope = make<s_node>();
//     g_ptr<s_node> current_scope = root_scope;

//     list<g_value> stack{g_value()};

//     for (int i = 0; i < nodes.size(); ++i) {
//         g_ptr<a_node> node = nodes[i];
//         g_ptr<a_node> owner_node = (i>0) ? nodes[i - 1] : nullptr;

//         int p = scope_precedence(node);
//         bool on_stack = stack.last().owner ? true : false;
//         if(p<=0) {
//             if(p<0) {
//                 if (current_scope->parent) {
//                     current_scope = current_scope->parent;
//                 }
//             }
//             else {
//                 current_scope->a_nodes << node; 
//                 //print("On ",a_type_string(node->type));
//                 if(on_stack && !stack.last().deferred && !stack.last().explc) {
//                     //print("Popping ",a_type_string(node->type));
//                     if (current_scope->parent) {
//                         current_scope = current_scope->parent;
//                     }
//                 }
//             }
//         }
//         else {
//             if(p<10) {
//                 current_scope->a_nodes << node;
//                 owner_node = node;
//             }

//             if(on_stack) {
//                 int stack_precedence = scope_precedence(stack.last().owner);
//                 // print(a_type_string(owner_node->type),": ",p);
//                 // print(a_type_string(stack.last().owner->type),": ",stack_precedence);
//                     if(p >= stack_precedence) {
//                         stack.last().deferred = true;
//                     } else {
//                         //This should do something 
//                         //stack.pop();
//                         // if (current_scope->parent) {
//                         //     current_scope = current_scope->parent;
//                         // } //Is this needed or not?
//                     }
//             }

//             if(p == 10) {
//                 if(on_stack) {
//                     stack.last().explc = true;
//                     //print(a_type_string(stack.last().owner->type));
//                     if (current_scope->parent) {
//                         current_scope = current_scope->parent;
//                     }
//                 }
//             }
//             else {
//                 //print("Pushing ",a_type_string(owner_node->type));
//                 stack << g_value(owner_node);
//             }

//             g_ptr<s_node> parent_scope = current_scope;
//             current_scope = current_scope->spawn_node();
//             if (owner_node) {
//                 link_scope_owner(owner_node,parent_scope,current_scope);
//             } else {
//                 current_scope->scope_type = BLOCK;
//             }
//         }
//     }

//     print("=== PARSE SCOPE PASS ===");
//     print_scope(root_scope);
//     return root_scope;
// }


// static void t_variable_decleration(g_ptr<t_node> result,g_ptr<a_node> node) {
//     result->type=T_VAR_DECL;
//     b_type type = node->tokens[0]->getType();
//     switch(type) {
//         case IDENTIFIER:
//             result->value.type = OBJECT; 
//             result->deferred_identifier = node->tokens[0]->content; //For later resolution
//             break;
//         default: 
//             result->value.type = key_to_type(type); 
//             result->value.size = key_to_size(type);
//         break;
//     }
//     result->name = node->tokens[1]->content;
// }

// g_ptr<t_node> t_make_literal(g_ptr<Token> token) {
//     g_ptr<t_node> node = make<t_node>();
//     node->type = T_LITERAL;
//     switch(token->getType()) {
//         case IDENTIFIER:
//             node->type = T_IDENTIFIER;
//             node->value.type = UNDEFINED;
//             node->name = token->content;
//             break;
//         case INT:
//             node->value.type = INT;
//             node->value.set<int>(std::stoi(token->content));
//             break;
//         case FLOAT:
//             node->value.type = FLOAT;
//             node->value.set<float>(std::stof(token->content));
//             break;
//         case STRING:
//             node->value.type = STRING;
//             node->value.set<std::string>(token->content);
//             node->value.size = 32;
//             break;
//         case BOOL:
//             node->value.type = BOOL;
//             node->value.set<bool>(token->content=="true" ? true : false);
//             break;
//             //Add characher handeling?
//         default: 
        
//             break;
//     }
//     return node;
// }

// static t_type t_opp_conversion(a_type b) {
//     switch(b) {
//     case ADD: return T_ADD;
//     case SUBTRACT: return T_SUBTRACT;
//     case MULTIPLY: return T_MULTIPLY;
//     case DIVIDE: return T_DIVIDE;
//     case ASSIGNMENT: return T_ASSIGN;
//     case LESS_THAN: return T_LESS_THAN;
//     case GREATER_THAN: return T_GREATER_THAN;
//     case IF_DECL: return T_IF;
//     case ELSE_DECL: return T_ELSE;
//     case WHILE_DECL: return T_WHILE;
//     case BREAK_CALL: return T_BREAK;
//     case PROP_ACCESS: return T_PROP_ACCESS;
//     case METHOD_CALL: return T_METHOD_CALL;
//     case RETURN_CALL: return T_RETURN;
//     default: return T_NOP;
//     }
// }

// // Changes occured to the handeling of Sub:0 Tokens:1, which may have broken normal expression parsing
// static g_ptr<t_node> t_parse_expression(g_ptr<a_node> node,g_ptr<t_node> left=nullptr) {
//     g_ptr<t_node> result = make<t_node>();
//     result->type = t_opp_conversion(node->type);
    
//     if (node->tokens.size() == 2) {
//         if(node->sub_nodes.size()>0) {
//             print("t_parse_expression::972 Warning, odd subnode count on a_node!");
//         }
//         result->left = t_make_literal(node->tokens[0]);
//         result->right = t_make_literal(node->tokens[1]);
//     } 
//     else if (node->tokens.size() == 1) {
//         if(node->sub_nodes.size()==0) {
//             //result->left = left; // <- This creates recursion if we have a single value like (x), but is critical for 7+8+9
//             // result->right = t_make_literal(node->tokens[0]);
//             //result = t_make_literal(node->tokens[0]); // <- This fixes the recursion bug but breaks chained left expressions

//             //The potential solution
//             if(left) {
//                 result->left = left;
//                 result->right = t_make_literal(node->tokens[0]);
//             }
//             else
//                 result = t_make_literal(node->tokens[0]);
//         }
//         else if(node->sub_nodes.size()==1) {
//             result->left = t_make_literal(node->tokens[0]);
//             result->right = t_parse_expression(node->sub_nodes[0],result);
//         }
//         else if(node->sub_nodes.size()>=2) {
//             result->left = t_make_literal(node->tokens[0]);
//             g_ptr<t_node> sub = nullptr;
//             for(auto a : node->sub_nodes) {
//                sub = t_parse_expression(a,sub);
//                 if(a==node->sub_nodes.last()) {
//                     result->right = sub;
//                 }
//             }
//         }
//     }
//     else if(node->tokens.size()==0) {
//         if(node->sub_nodes.size()==0) {
//             result->right = nullptr;
//             result->left = nullptr;
//         }
//         else if(node->sub_nodes.size()==1) {
//             result->left = left;
//             result->right = t_parse_expression(node->sub_nodes[0],nullptr); //By passing nullptr we stop the recursion
//         }
//         else if(node->sub_nodes.size()>=2) {
//             result->left = left;
//             g_ptr<t_node> sub = nullptr;
//             for(auto a : node->sub_nodes) {
//                sub = t_parse_expression(a,sub);
//                 if(a==node->sub_nodes.last()) {
//                     result->right = sub;
//                 }
//             }
//         }
//     }

//     return result;
// }


// // UNHANDLED PROBLEMS
// // - Precdence can't be trusted as there's not associtivity


// static g_ptr<t_node> parse_a_node(g_ptr<a_node> node,g_ptr<s_node> root,g_ptr<t_node> left = nullptr)
// {
//     g_ptr<t_node> result = make<t_node>();
//     switch(node->type) {
//         case VAR_DECL:
//             t_variable_decleration(result,node);
//             break;
//         case TYPE_DECL:
//             result->type = T_TYPE_DECL;
//             result->name = node->tokens.last()->content;
//             result->scope = node->owned_scope;
//             break;
//         case METHOD_DECL:
//             result->type = T_METHOD_DECL;
//             result->name = node->tokens.last()->content;
//             result->scope = node->owned_scope;
//             result->scope->t_owner = result;
//             if(!node->sub_nodes.empty()) {
//                 for(auto c : node->sub_nodes) {
//                     result->children << parse_a_node(c,root);
//                 }
//             }
//             break;
//         case METHOD_CALL:
//             result = t_parse_expression(node,left);
//             if(!node->sub_nodes.empty()) {
//                 for(auto c : node->sub_nodes) {
//                     result->children << parse_a_node(c,root);
//                 }
//             }
//             break;
//         case RETURN_CALL:
//             result = t_parse_expression(node,result);
//             break;
//         case BREAK_CALL:
//             result->type = T_BREAK;
//             break;
//         case LITERAL: case LITERAL_IDENTIFIER:
//             result = t_make_literal(node->tokens[0]);
//             break;
//         case PROP_ACCESS:
//             result->type = T_PROP_ACCESS;
//             result->left = t_make_literal(node->tokens[0]);
//             result->right = t_make_literal(node->tokens[1]);
//             break;
//         case IF_DECL:
//             result = t_parse_expression(node,nullptr);
//             result->scope = node->owned_scope;
//             break;
//         case ELSE_DECL:
//             result->type = T_ELSE;
//             result->scope = node->owned_scope;
//             left->left = result;
//             return nullptr;
//         case WHILE_DECL:
//             result = t_parse_expression(node,nullptr);
//             result->scope = node->owned_scope;
//             if(left->type == T_DO) {
//                 result->left = left;
//                 result->scope = left->scope;
//                 root->t_nodes.erase(left);
//             }
//             break;
//         case DO_DECL:
//             result->type = T_DO;
//             result->scope = node->owned_scope;
//             break;
//         case PRINT_CALL:
//         {
//             result->type=T_PRINT;
//             g_ptr<t_node> sub = nullptr;
//             for(auto a : node->sub_nodes) {
//                 g_ptr<t_node> n_sub = parse_a_node(a,root,sub);
//                 if(sub&&n_sub->left==sub) {
//                     result->children.erase(sub);
//                 }
//                 sub = n_sub;
//                 result->children << sub;
//             }
//         }
//             break;
//         default:
//             if(a_is_opp(node->type)) {
//                 result = t_parse_expression(node,left);
//             }
//             else {
//             print("Missing parsing code for a_node type: ",node->type); 
//             }
//             break;
//     }

//     return result;
// }

// void print_t_node(const g_ptr<t_node>& node, int depth = 0, int index = 0) {
//     std::string indent(depth * 2, ' '); 
    
//     // Print node info
//     print(indent, "Node #", index, " Type: ", t_type_string(node->type));
    
//     // Print value if it exists
//     if (node->value.type != UNDEFINED) {
//         print(indent, "  Value: ", node->value.to_string(), " (type: ", node->value.type, ")");
//     }
    
//     // Print name if it exists
//     if (!node->name.empty()) {
//         print(indent, "  Name: ", node->name);
//     }
    
//     // Print left operand
//     if (node->left) {
//         print(indent, "  Left:");
//         print_t_node(node->left, depth + 1, 0);
//     } else {
//         print(indent, "  Left: nullptr");
//     }
    
//     // Print right operand  
//     if (node->right) {
//         print(indent, "  Right:");
//         print_t_node(node->right, depth + 1, 0);
//     } else {
//         print(indent, "  Right: nullptr");
//     }
    
//     // Print children if any
//     if (!node->children.empty()) {
//         print(indent, "  Children: ", node->children.size());
//         int child_index = 0;
//         for (auto& child : node->children) {
//             print_t_node(child, depth + 1, child_index++);
//         }
//     }
//  }

// static void parse_nodes(g_ptr<s_node> root) {
//     print("==PARSE NODES PASS==");
//     g_ptr<t_node> result = nullptr;
//     for(int i = 0; i < root->a_nodes.size(); i++) {
//         auto node = root->a_nodes[i];
//         auto left = (i > 0) ? root->a_nodes[i-1] : nullptr;
//         auto right = (i < root->a_nodes.size()-1) ? root->a_nodes[i+1] : nullptr;

//         g_ptr<t_node> sub = parse_a_node(node,root,result);
//         if(sub) {
//         result = sub;
//         root->t_nodes << result;
//         }
//     }

//     for(auto t : root->t_nodes) {
//         print_t_node(t,0,0);
//     }
    
//     for(auto child_scope : root->children) {
//         parse_nodes(child_scope);
//     }
// }

// enum r_type {
//     R_NOP,R_IF,R_VAR_DECL,R_ASSIGNMENT,R_METHOD_CALL,R_PROP_ACCESS,R_TYPE_DECL,
//     R_METHOD_DECL, R_IDENTIFIER, R_LITERAL, R_PRINT_CALL, R_RETURN, R_ELSE,
//     R_ADD, R_SUBTRACT, R_MULTIPLY, R_DIVIDE, R_GREATER_THAN, R_LESS_THAN, R_BREAK,
//     R_WHILE, R_DO
//  };


// static std::string r_type_string(r_type type) {
//     switch(type) {
//         case R_IF: return "R_IF";
//         case R_DO: return "R_DO";
//         case R_NOP: return "R_NOP";
//         case R_ADD: return "R_ADD";
//         case R_ELSE: return "R_ELSE";
//         case R_WHILE: return "R_WHILE";
//         case R_BREAK: return "R_BREAK";
//         case R_RETURN: return "R_RETURN";
//         case R_LITERAL: return "R_LITERAL";
//         case R_VAR_DECL: return "R_VAR_DECL";
//         case R_TYPE_DECL: return "R_TYPE_DECL";
//         case R_LESS_THAN: return "R_LESS_THAN";
//         case R_ASSIGNMENT: return "R_ASSIGNMENT";
//         case R_METHOD_CALL: return "R_METHOD_CALL";
//         case R_PROP_ACCESS: return "R_PROP_ACCESS";
//         case R_METHOD_DECL: return "R_METHOD_DECL";
//         case R_GREATER_THAN: return "R_GREATER_THAN";
//         case R_IDENTIFIER: return "R_IDENTIFIER";
//         case R_PRINT_CALL: return "R_PRINT_CALL";
//         case R_SUBTRACT: return "R_SUBTRACT";
//         case R_MULTIPLY: return "R_MULTIPLY";
//         case R_DIVIDE: return "R_DIVIDE";
//         default: return "UNKNOWN_R_TYPE";
//     }
// }

class Frame : public Object {
    public:
    uint32_t type = GET_TYPE(GLOBAL);
    g_ptr<Type> context;
    list<g_ptr<r_node>> nodes;
    list<size_t> slots;
    t_value return_val;
};

class r_node : public Object {
    public:
    uint32_t type = GET_TYPE(R_NOP);
    g_ptr<r_node> left;
    g_ptr<r_node> right;
    list<g_ptr<r_node>> children;
    t_value value;
    g_ptr<Type> resolved_type = nullptr;
    size_t slot = 0;
    std::string name;
    g_ptr<Frame> frame;
};

// //Fix this walking pattern later with a designated entry point so we're not processing parallel scope
// static void discover_var_decleration(g_ptr<t_node> node, g_ptr<s_node> root,size_t& idx) 
// {
//     if(node->value.type==OBJECT) {
//         //Can add short circuiting and error handeling here
//         g_ptr<s_node> on_scope = root;
//         g_ptr<Type> type = nullptr;
//         //This scope walking for types needs to be inspected further, potential cause of problems.
//         for(auto c : root->children) {
//             if(c->type_ref && c->type_ref->type_name == node->deferred_identifier) {
//                 type = c->type_ref;
//             }
//         }
//         while(!type) {
//             if(on_scope->type_ref) {
//                 if(on_scope->type_ref->type_name == node->deferred_identifier) {
//                     type = on_scope->type_ref;
//                 }
//             }
//             if(type) break;
//             if(on_scope->parent) {
//                 on_scope = on_scope->parent;
//             }
//             else break;
//         }
//         if(type) {
//             root->type_map.put(node->name,type);
//         }
//         root->slot_map.put(node->name,root->slot_map.size());
//         root->type_ref->note_value(sizeof(size_t), node->name); 
//         //Need size map entry here too?
//     }
//     else {
//         root->type_ref->note_value(node->value.size,node->name);
//         root->size_map.put(node->name,node->value.size);
//         root->o_type_map.put(node->name,node->value.type);
//     }
// }

// static g_ptr<s_node> discover_type_decl(g_ptr<t_node> node, g_ptr<s_node> root) {
//     g_ptr<s_node> on_scope = root;
//     g_ptr<Type> type = nullptr;
//     for(auto c : root->children) {
//         if(c->type_ref && c->type_ref->type_name == node->deferred_identifier) {
//             return c;
//         }
//     }
//     while(!type) {
//         if(on_scope->type_ref) {
//             if(on_scope->type_ref->type_name == node->deferred_identifier) {
//                 type = on_scope->type_ref;
//             }
//         }
//         if(on_scope->parent) {
//             on_scope = on_scope->parent;
//         }
//         else break;
//     }
//     return on_scope;
// }

// static void discover_symbols(g_ptr<s_node> root) {
//    // print("==DISCOVER SYMBOLS PASS==");
//     if(!root->type_ref) {
//         root->type_ref = make<Type>();
//     }
//     size_t idx = 0;
//     for(int i = 0; i < root->t_nodes.size(); i++) {
//         auto node = root->t_nodes[i];
//         switch(node->type) {
//             case T_TYPE_DECL: 
//             {
//                 if(!node->scope->type_ref)  node->scope->type_ref = make<Type>();
//                 node->scope->type_ref->type_name = node->name;
//             }
//             break;
//             case T_METHOD_DECL: 
//             {
//                 if(node->scope)
//                 {
//                     if(!node->scope->type_ref) node->scope->type_ref = make<Type>();
//                     node->scope->type_ref->type_name = node->name;
//                     //May want to make this method recursive in the future
//                     //Discovering arguments
//                     for(auto c : node->children) {
//                         if(c->type==T_VAR_DECL)
//                             discover_var_decleration(c,node->scope,idx);
//                     }
//                 }
//             }
//             break;
//             case T_VAR_DECL:
//                 discover_var_decleration(node,root,idx);
//             break;
//             default: break;
//         }
//     }
//     for(auto child_scope : root->children) {
//         discover_symbols(child_scope);
//     }
// }  

// static g_ptr<Frame> resolve_symbols(g_ptr<s_node> root);


// static g_ptr<s_node> find_type_decl(g_ptr<Type> type, g_ptr<s_node> root) {
//     g_ptr<s_node> on_scope = root;
//     for(auto c : root->children) {
//         if(c->type_ref && c->type_ref == type) {
//             return c;
//         }
//     }
//     while(on_scope->parent) {
//         if(on_scope->type_ref) {
//             if(on_scope->type_ref  == type) {
//                 return on_scope;
//             }
//         }
//         on_scope = on_scope->parent;
//     }
//     return nullptr;
// }

// static void resolve_identifier(g_ptr<t_node> node,g_ptr<r_node> result,g_ptr<s_node> scope,g_ptr<Frame> frame) {
//     void* ptr = nullptr;
//     g_ptr<s_node> on_scope = scope;
//     while(!ptr) {
//         ptr = on_scope->type_ref->adress_column(node->name);
//         if(ptr) break;
//         if(on_scope->parent) {
//             on_scope = on_scope->parent;
//         }
//         else break;
//     }
//     if(ptr) {
//         result->name = node->name;
//         if(on_scope->slot_map.hasKey(node->name)) {
//             if(on_scope->frame) {
//                 if(on_scope->frame->slots.length()<=result->slot) on_scope->frame->slots << 0;
//                 result->frame = on_scope->frame;
//             }
//             else
//                 print("resolve_identifier::1621 no frame for scope!");
//             result->slot = on_scope->slot_map.get(node->name);
//             result->resolved_type = on_scope->type_map.get(node->name);
//             result->value.type = OBJECT;
//         }
//         result->value.address = ptr;
//         if(on_scope->size_map.hasKey(node->name)) {
//             result->value.size = on_scope->size_map.get(node->name);
//             result->value.type = on_scope->o_type_map.get(node->name);
//         }
//     }
//     else print("resolve_symbol::1418 No address found for identifier ",node->name);
// }

// static g_ptr<r_node> resolve_symbol(g_ptr<t_node> node,g_ptr<s_node> scope,g_ptr<Frame> frame) {
//     g_ptr<r_node> result = make<r_node>();
//     switch(node->type) {
//         case T_IDENTIFIER:
//         {
//             result->type = R_IDENTIFIER;
//             resolve_identifier(node,result,scope,frame);
//         }
//         break;
//         case T_LITERAL:
//         {
//             result->type = R_LITERAL;
//             result->value = std::move(node->value);
//         }
//         break;
//         case T_VAR_DECL:
//         result->type = R_VAR_DECL;
//         result->value.type = node->value.type;
//         result->value.size = node->value.size;
//         result->name = node->name;
//         if(node->value.type==OBJECT) {
//             g_ptr<r_node> info = make<r_node>();
//             resolve_identifier(node,info,scope,frame);
//             if(info->resolved_type) {
//                 result->slot = info->slot;
//                 if(frame->slots.length()<=result->slot) frame->slots << 0;
//                 result->resolved_type = info->resolved_type;
//                 result->name = info->name; //The name of what to construct (possibly)
//                 result->value.type = OBJECT;
                
//             } 
//             //Create a constructor call
//         } 
//         break;
//         case T_METHOD_CALL:
//         {
//             result->type = R_METHOD_CALL;
//             g_ptr<r_node> info = resolve_symbol(node->left,scope,frame);
//             if(info->resolved_type) {
//                 result->slot = info->slot;
//                 if(frame->slots.length()<=result->slot) frame->slots << 0;
//                 result->resolved_type = info->resolved_type;
//                 result->name = node->right->name; //The method to call
//                 g_ptr<s_node> type_decl = find_type_decl(result->resolved_type,scope);
//                 //This feels very hacky
//                 if(type_decl) {
//                     g_ptr<r_node> method_decl = type_decl->method_map.get(result->name);
//                     result->frame = method_decl->frame;
                    
//                     // Pre-resolve argument→parameter connections
//                     for(int i = 0; i < node->children.size(); i++) {
//                         g_ptr<r_node> arg = resolve_symbol(node->children[i], scope, frame);
//                         g_ptr<r_node> param = method_decl->children[i];  // The parameter declaration
//                         // print(param->name," : ",param->value.type," S: ",param->slot);
//                         // print(arg->name," : ",arg->value.type," S: ",arg->slot);
//                         // if(param->value.address) print("Has Address");
//                         // print(param->slot); //The slot is always 0!!!
//                         param->type = R_IDENTIFIER;
//                         g_ptr<r_node> assignment = make<r_node>();
//                         assignment->type = R_ASSIGNMENT;
//                         assignment->left = param;
//                         assignment->right = arg;
                        
//                         result->children << assignment;
//                     }
//                 }
//             }
//         }
//         break;
//         case T_PROP_ACCESS: 
//         {
//             result->type = R_PROP_ACCESS;
//             g_ptr<r_node> info = resolve_symbol(node->left,scope,frame); // Will link to a t_identifier of type Object
//             if(info->resolved_type) {
//                 result->slot = info->slot;
//                 if(frame->slots.length()<=result->slot) frame->slots << 0;
//                 result->resolved_type = info->resolved_type;
//                 result->value.address = result->resolved_type->adress_column(node->right->name);
//                 result->value.size = result->resolved_type->notes.get(node->right->name).size;
//                 result->value.type = info->value.type;
//                 result->name = node->left->name;
//                 result->frame = info->frame;

//                 for(auto c : scope->children) {
//                     if(c->type_ref && c->type_ref == result->resolved_type) {
//                         result->value.type = c->o_type_map.get(node->right->name);
//                         break;
//                     }
//                 }

//                 g_ptr<s_node> search_scope = scope;
//                 while(result->value.type==OBJECT) {
//                     if(search_scope->type_ref && search_scope->type_ref == result->resolved_type) {
//                         result->value.type = search_scope->o_type_map.get(node->right->name);
//                         break; 
//                     }
//                     if(result->value.type!=OBJECT) break;
//                     if(search_scope->parent)
//                     {
//                     search_scope = search_scope->parent; // Walk up
//                     }
//                     else break;
//                 }
//             }
//         }
//         break;
//         case T_ASSIGN: 
//         {
//             result->type = R_ASSIGNMENT;

//             result->left = resolve_symbol(node->left,scope,frame);
//             result->right = resolve_symbol(node->right,scope,frame);
//         }
//         break;
//         case T_IF:
//             result->type = R_IF;
//             result->right = resolve_symbol(node->right,scope,frame); //This may need changes
//             result->frame = resolve_symbols(node->scope);
//             if(node->left) {
//                 result->left = resolve_symbol(node->left,scope,frame);
//             }
//         break;
//         case T_ELSE:
//             result->type = R_ELSE;
//             result->frame = resolve_symbols(node->scope);
//         break;
//         case T_WHILE:
//             result->type = R_WHILE;
//             result->right = resolve_symbol(node->right,scope,frame);
//             result->frame = resolve_symbols(node->scope);
//             if(node->left) {
//                 result->left = resolve_symbol(node->left,scope,frame);
//                 result->left->frame = result->frame; //They share a frame
//             }
//         break;
//         case T_DO:
//             result->type = R_DO;
//         break;
//         case T_TYPE_DECL:
//         {
//             result->type = R_TYPE_DECL;
//             result->frame = resolve_symbols(node->scope);
//         }
//         break;
//         case T_METHOD_DECL:
//         {
//             result->type = R_METHOD_DECL;
//             result->name = node->name;
//             result->frame = resolve_symbols(node->scope);
//             for(auto c : node->children) {
//                 g_ptr<r_node> child = resolve_symbol(c,node->scope,result->frame);
//                 resolve_identifier(c,child,node->scope,result->frame); //This should be done on R_VAR_DECL
//                 result->children << child;
//             } //The arguments
//             scope->method_map.put(result->name,result);
//             //Can read the return type here by indicating the value type
//             //Need to store that in the t_node
//         }
//         break;
//         case T_RETURN:
//         {
//             result->right = resolve_symbol(node->right,scope,frame);
//             g_ptr<s_node> on_scope = scope;
//             g_ptr<Frame> method_frame = nullptr;
//             while(!method_frame) {
//                 if(on_scope->scope_type == METHOD || on_scope->scope_type == FUNCTION) {
//                     if(on_scope->frame) {
//                         method_frame = on_scope->frame;
//                     }
//                 }
//                 if(method_frame) break;
//                 if(on_scope->parent) {
//                     on_scope = on_scope->parent;
//                 }
//                 else break;
//             }
//             result->frame = method_frame;
//             result->type = R_RETURN;
//         }
//         break;
//         case T_BREAK:
//         {
//             g_ptr<s_node> on_scope = scope;
//             g_ptr<Frame> loop_frame = nullptr;
//             while(!loop_frame) {
//                 if(on_scope->scope_type == WHILE_LOOP || on_scope->scope_type == FOR_LOOP) {
//                     if(on_scope->frame) {
//                         loop_frame = on_scope->frame;
//                     }
//                 }
//                 if(loop_frame) break;
//                 if(on_scope->parent) {
//                     on_scope = on_scope->parent;
//                 }
//                 else break;
//             }
//             result->frame = loop_frame;
//             result->type = R_BREAK;
//         }
//         break;
//         case T_ADD:
//             result->type = R_ADD;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//         case T_SUBTRACT:
//             result->type = R_SUBTRACT;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//         case T_MULTIPLY:
//             result->type = R_MULTIPLY;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//         case T_DIVIDE:
//             result->type = R_DIVIDE;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//         case T_GREATER_THAN:
//             result->type = R_GREATER_THAN;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//             case T_LESS_THAN:
//             result->type = R_LESS_THAN;
//             if(node->left) 
//                 result->left = resolve_symbol(node->left,scope,frame);
//             if(node->right)
//                 result->right = resolve_symbol(node->right,scope,frame);
//         break;
//         case T_PRINT: 
//         {
//             result->type = R_PRINT_CALL;
//             for(auto c : node->children) {
//                 g_ptr<r_node> sub = resolve_symbol(c,scope,frame);
//                 result->children << sub;
//             }
//         }
//         break;
//         default: 
//             print("resolve_symbol::1454 Missing case for t_type: ",t_type_string(node->type));
//             return nullptr;
//         break;
//     }
//     return result;
// }


// void print_r_node(const g_ptr<r_node>& node, int depth = 0, int index = 0) {
//     std::string indent(depth * 2, ' '); 
    
//     print(indent, "Node #", index, " Type: ", r_type_string(node->type));

//     if (node->value.type != UNDEFINED) {
//         print(indent, "  Value: ", node->value.to_string(), " (type: ", node->value.type, ")");
//     }
    
//     if (!node->name.empty()) {
//         print(indent, "  Name: ", node->name);
//     }
    
//     if (node->left) {
//         print(indent, "  Left:");
//         print_r_node(node->left, depth + 1, 0);
//     } 
    
//     if (node->right) {
//         print(indent, "  Right:");
//         print_r_node(node->right, depth + 1, 0);
//     } 
    
//     if (!node->children.empty()) {
//         print(indent, "  Children: ", node->children.size());
//         int child_index = 0;
//         for (auto& child : node->children) {
//             print_r_node(child, depth + 1, child_index++);
//         }
//     }
//  }

// static g_ptr<Frame> resolve_symbols(g_ptr<s_node> root) {
//     print("==RESOLVE SYMBOLS PASS==");
//     g_ptr<Frame> frame = make<Frame>();
//     frame->type = root->scope_type;
//     frame->context = root->type_ref; //This should've been filled in during discovery
//     root->frame = frame;

//     for(int i = 0; i < root->t_nodes.size(); i++) {
//         auto node = root->t_nodes[i];
//         g_ptr<r_node> rnode = resolve_symbol(node,root,frame);
//         if(rnode) frame->nodes << rnode;
//     }

//     print("==RESOLVED SYMBOLS==");
//     for(auto r : frame->nodes) {
//         print_r_node(r);
//     }
    
//     // for(auto child_scope : root->children) {
//     //     resolve_symbols(child_scope);
//     // }
//     return frame;
// }   

// static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context = nullptr);
// static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index);

// template<typename Op>
// void execute_r_operation(g_ptr<r_node> node, Op operation,b_type result_type, g_ptr<Frame> frame) {
//     if(node->left)
//         execute_r_node(node->left,frame,0);
//     if(node->right)
//         execute_r_node(node->right,frame,0);
    
//     t_value& left_val = node->left->value;
//     t_value& right_val = node->right->value;
//     node->value.type = result_type;
    
//     if(left_val.type == INT && right_val.type == INT) {
//         auto result = operation(*(int*)left_val.data, *(int*)right_val.data);
//         if(result_type == BOOL) node->value.set<bool>(result);
//         else node->value.set<int>(result);
//     }
//     else if(left_val.type == FLOAT || right_val.type == FLOAT) {
//         float left_f = (left_val.type == FLOAT) ? *(float*)left_val.data : *(int*)left_val.data;
//         float right_f = (right_val.type == FLOAT) ? *(float*)right_val.data : *(int*)right_val.data;
//         auto result = operation(left_f, right_f);
//         if(result_type == BOOL) node->value.set<bool>(result);
//         else if(result_type == FLOAT) node->value.set<float>(result);
//         else node->value.set<int>(result);
//     }
//     else {
//         print("execute_r_operation::1860 Type error in binary operation");
//     }
// }

// static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index) {
//     switch(node->type) {
//         case R_VAR_DECL:
//         if(node->value.type==OBJECT) {
//             g_ptr<Object> obj = node->resolved_type->create();
//             frame->slots[node->slot] = obj->ID;
//             //And do construction here
//         }
//         break;
//         case R_IDENTIFIER: 
//             node->value.data = frame->context->get(node->value.address,index,node->value.size);
//             //print(node->name,": ",node->value.address,"-",index," -> ",node->value.to_string());
//         break;
//         case R_PROP_ACCESS:
//             //print(node->name," Slot: ",node->slot);
//             if(node->frame)
//             {
//                 node->value.data = frame->context->get(node->value.address,node->frame->slots.get(node->slot,"execute_r_node::prop_access"),node->value.size);
//                 print(node->value.address,"-",node->frame->slots.get(node->slot,"execute_r_node::prop_access")," -> ",node->value.to_string());
//             }
//             else print("execute_r_node::1970 No frame in node for prop access");
//         break;
//         case R_LITERAL:
//             //Do nothing
//         break;
//         case R_TYPE_DECL:
//             //Probbaly do nothing for now
//             //execute_r_nodes(node->frame);
//         break;
//         case R_METHOD_CALL:
//         {
//             g_ptr<Object> context = node->frame->context->create();
    
//             // Execute pre-wired parameter assignments
//             for(auto assignment : node->children) {
//                 if(assignment->left->value.type == OBJECT) {
//                     node->frame->slots[assignment->left->slot] = frame->slots[assignment->right->slot];
//                 }
//                 else
//                 {
//                     execute_r_node(assignment, node->frame, context->ID);
//                 }
                
//             }
        
//             execute_r_nodes(node->frame,context);
//             if(node->frame->return_val.type != UNDEFINED) {
//                 node->value = std::move(node->frame->return_val);
//             }
//             node->frame->return_val.type = UNDEFINED;
//         }
//         break;
//         case R_ASSIGNMENT:
//         {
//             execute_r_node(node->left, frame, index);
//             execute_r_node(node->right, frame, index);
            
//             size_t target_index = (node->left->type == R_IDENTIFIER) ? 
//                 index : frame->slots.get(node->left->slot,"execute_r_node::assignment");

//            // print(node->left->value.address,"-",target_index," <- ",node->right->value.to_string());
            
//             frame->context->set(node->left->value.address, node->right->value.data, 
//                                target_index, node->right->value.size);
//         }
//         break;
//         case R_RETURN:
//             execute_r_node(node->right, frame, index);
//             if(node->frame)
//                 node->frame->return_val = std::move(node->right->value);
//         break;
//         case R_BREAK:
//             if(node->frame)
//                 node->frame->stop();
//         break;
//         case R_IF:
//             execute_r_node(node->right,frame,index);
//             if(node->right->value.is_true()) {
//                 execute_r_nodes(node->frame);
//             }
//             else if(node->left) {
//                 execute_r_nodes(node->left->frame);
//             }
//         break;
//         case R_WHILE:
//         {
//             node->frame->resurrect();
//             execute_r_node(node->right,frame,index);
//             while(node->right->value.is_true()) {
//                 execute_r_node(node->right,frame,index);
//                 if (!node->right->value.is_true()) break; 
//                 execute_r_nodes(node->frame);
//             }
//         }
//         break;
//         case R_ADD:
//             execute_r_operation(node, [](auto a, auto b){ return a+b; },INT,frame);
//         break;
//         case R_SUBTRACT:
//             execute_r_operation(node, [](auto a, auto b){ return a-b; },INT,frame);
//         break;
//         case R_MULTIPLY:
//             execute_r_operation(node, [](auto a, auto b){ return a*b; },INT,frame);
//         break;
//         case R_DIVIDE:
//             execute_r_operation(node, [](auto a, auto b){ return a/b; },INT,frame);
//         break;
//         case R_GREATER_THAN:
//             execute_r_operation(node, [](auto a, auto b){ return a>b; },BOOL,frame);
//         break;
//         case R_LESS_THAN:
//             execute_r_operation(node, [](auto a, auto b){ return a<b; },BOOL,frame);
//         break;
//         case R_PRINT_CALL: 
//         {
//             std::string toPrint = "";
//             for(auto r : node->children) {
//                 execute_r_node(r,frame,index);
//                 toPrint.append(r->value.to_string());
//             }
//             print(toPrint);
//         }
//         break;
//         default: 
//             print("execute_r_node::1554 Missing case for r_type: ",r_type_string(node->type));
//             return nullptr;
//         break;
//     }
//     return node;
// }

// static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context) {
//     if(!context) {
//         context = frame->context->create();
//     }
//     for(int i = 0; i < frame->nodes.size(); i++) {
//         auto node = frame->nodes[i];
//         execute_r_node(node,frame,context->ID);
//         if(frame->return_val.type != UNDEFINED) {
//             break;
//         }
//         else if(!frame->isActive()) {
//             break;
//         }
//     }
//     frame->context->recycle(context);
// }   

// g_ptr<s_node> compile_script(const std::string& file) {
//     //"../Projects/Testing/src/golden.gld"
//     std::string code = readFile(file);
//     list<g_ptr<Token>> tokens = tokenize(code);
//     list<g_ptr<a_node>> nodes = parse_tokens(tokens);
//     balance_precedence(nodes);
//     g_ptr<s_node> root = parse_scope(nodes);
//     parse_nodes(root);
//     discover_symbols(root);
//     g_ptr<Frame> frame = resolve_symbols(root);
//     print("==RUNNING==");
//     execute_r_nodes(frame);
//     return root;
// }