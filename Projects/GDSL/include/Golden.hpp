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

static std::string fallback = "[undefined]";

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

static map<std::string,t_info> t_keys;

static void reg_t_key(std::string name,const t_info& info) {
    t_keys.put(name,info);
}

static void reg_t_key(std::string name,uint32_t enum_key,size_t size, uint32_t family_key) {
    reg_t_key(name,t_info(enum_key,size,family_key));
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

map<uint32_t,std::function<std::string(void*)>> value_to_string;



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
        
        try {
            return value_to_string.get(type)(data);
        }
        catch(std::exception e) {
            return "[missing value_to_string for type "+TO_STRING(type)+"]";
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

//This goes in the compiler because it's to do with internals, the modules are trusting this handeling
auto literal_handler = [](a_context& ctx) {
    if(ctx.state == GET_TYPE(UNTYPED)) {
        ctx.state = GET_TYPE(LITERAL);
    }
    ctx.node->tokens << ctx.token;
};

auto type_key_handler = [](a_context& ctx) {
    if(ctx.state == GET_TYPE(UNTYPED)) {
        ctx.state = GET_TYPE(VAR_DECL);
    }
    ctx.node->tokens << ctx.token;
};

static list<g_ptr<a_node>> parse_tokens(list<g_ptr<Token>> tokens,bool local = false) {
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
//Assignment like: i=i+1 is currently broken under this, use parens instead
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

/// @brief  This method is not yet complete! Just like the other precdence, it is in need of refinment and 
/// edge case strengthining in 0.0.9. 
/// No noted edge cases in light testing
/// If you don't see a scope in the print out, check link owners, chances are there's no case for linking them meaning it can't be found
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
                //print("On ",a_type_string(node->type));
                if(on_stack && !stack.last().deferred && !stack.last().explc) {
                    //print("Popping ",a_type_string(node->type));
                    if (current_scope->parent) {
                        current_scope = current_scope->parent;
                    }
                }
            }
        }
        else {
            if(p<10) {
                current_scope->a_nodes << node;
                owner_node = node;
            }

            if(on_stack) {
                int stack_precedence = scope_precedence.getOrDefault(stack.last().owner->type,0);
                // print(a_type_string(owner_node->type),": ",p);
                // print(a_type_string(stack.last().owner->type),": ",stack_precedence);
                    if(p >= stack_precedence) {
                        stack.last().deferred = true;
                    } else {
                        //This should do something 
                        //stack.pop();
                        // if (current_scope->parent) {
                        //     current_scope = current_scope->parent;
                        // } //Is this needed or not?
                    }
            }

            if(p == 10) {
                if(on_stack) {
                    stack.last().explc = true;
                    //print(a_type_string(stack.last().owner->type));
                    if (current_scope->parent) {
                        current_scope = current_scope->parent;
                    }
                }
            }
            else {
                //print("Pushing ",a_type_string(owner_node->type));
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

    print("=== PARSE SCOPE PASS ===");
    print_scope(root_scope);
    return root_scope;
}

struct t_context {
    t_context(g_ptr<t_node>& _result,g_ptr<a_node> _node,g_ptr<s_node> _root) : 
    result(_result), node(_node), root(_root) {}

    g_ptr<t_node>& result;
    g_ptr<a_node> node;
    g_ptr<s_node> root;
    g_ptr<t_node> left = nullptr;
};

map<uint32_t, uint32_t> t_opp_conversion;
//I *strongly* dislike how this is being used, and very much intened to clean it up
map<uint32_t, std::function<g_ptr<t_node>(g_ptr<Token>)>> t_literal_handlers;
auto t_literal_handler = [](t_context& ctx) -> g_ptr<t_node> {
    //I hate this, it's unsafe and verbose, this needs to change
    return t_literal_handlers.get(ctx.node->tokens[0]->getType())(ctx.node->tokens[0]);
};
map<uint32_t, uint32_t> type_key_to_type;
map<uint32_t,std::function<g_ptr<t_node>(t_context& ctx)>> t_functions;

static void t_variable_decleration(g_ptr<t_node> result,g_ptr<a_node> node) {
    result->type=GET_TYPE(T_VAR_DECL);
    uint32_t type = node->tokens[0]->getType();
    if(type==GET_TYPE(IDENTIFIER)) {
        result->value.type = GET_TYPE(OBJECT); 
        result->deferred_identifier = node->tokens[0]->content; //For later resolution
    }
    else
    {
        result->value.type = type_key_to_type.get(node->tokens[0]->type_info.type);
        result->value.size = node->tokens[0]->type_info.size;
    }
    
    result->name = node->tokens[1]->content;
}

static g_ptr<t_node> t_parse_expression(g_ptr<a_node> node,g_ptr<t_node> left=nullptr) {
    g_ptr<t_node> result = make<t_node>();
    result->type = t_opp_conversion.get(node->type);
    
    if (node->tokens.size() == 2) {
        // if(node->sub_nodes.size()>0) {
        //     print("t_parse_expression::972 Warning, odd subnode count on a_node!");
        // }
        result->left = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
        result->right = t_literal_handlers.get(node->tokens[1]->getType())(node->tokens[1]);
    } 
    else if (node->tokens.size() == 1) {
        if(node->sub_nodes.size()==0) {
            //result->left = left; // <- This creates recursion if we have a single value like (x), but is critical for 7+8+9
            // result->right = t_make_literal(node->tokens[0]);
            //result = t_make_literal(node->tokens[0]); // <- This fixes the recursion bug but breaks chained left expressions

            //The potential solution
            if(left) {
                result->left = left;
                result->right = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
            }
            else
                result = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
        }
        else if(node->sub_nodes.size()==1) {
            result->left = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
            result->right = t_parse_expression(node->sub_nodes[0],result);
        }
        else if(node->sub_nodes.size()>=2) {
            result->left = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
            g_ptr<t_node> sub = nullptr;
            for(auto a : node->sub_nodes) {
               sub = t_parse_expression(a,sub);
                if(a==node->sub_nodes.last()) {
                    result->right = sub;
                }
            }
        }
    }
    else if(node->tokens.size()==0) {
        if(node->sub_nodes.size()==0) {
            result->right = nullptr;
            result->left = nullptr;
        }
        else if(node->sub_nodes.size()==1) {
            result->left = left;
            result->right = t_parse_expression(node->sub_nodes[0],nullptr); //By passing nullptr we stop the recursion
        }
        else if(node->sub_nodes.size()>=2) {
            result->left = left;
            g_ptr<t_node> sub = nullptr;
            for(auto a : node->sub_nodes) {
               sub = t_parse_expression(a,sub);
                if(a==node->sub_nodes.last()) {
                    result->right = sub;
                }
            }
        }
    }

    return result;
}

static g_ptr<t_node> parse_a_node(g_ptr<a_node> node,g_ptr<s_node> root,g_ptr<t_node> left = nullptr)
{
    g_ptr<t_node> result = make<t_node>();
    t_context ctx(result,node,root);
    ctx.left = left;
    if(t_functions.hasKey(node->type)) {
        auto func = t_functions.get(node->type);
        result = func(ctx);
    }
    else {
        if(state_is_opp.getOrDefault(node->type,false)) {
            result = t_parse_expression(node,left);
        }
        else {
            print("parse_a_node::940 missing parsing code for a_node type: ",TO_STRING(node->type)); 
        }
    }
    return result;
}

void print_t_node(const g_ptr<t_node>& node, int depth = 0, int index = 0) {
    std::string indent(depth * 2, ' '); 
    
    // Print node info
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type));
    
    // Print value if it exists
    if (node->value.type != GET_TYPE(UNDEFINED)) {
        print(indent, "  Value: ", node->value.to_string(), " (type: ", node->value.type, ")");
    }
    
    // Print name if it exists
    if (!node->name.empty()) {
        print(indent, "  Name: ", node->name);
    }
    
    // Print left operand
    if (node->left) {
        print(indent, "  Left:");
        print_t_node(node->left, depth + 1, 0);
    } else {
        print(indent, "  Left: nullptr");
    }
    
    // Print right operand  
    if (node->right) {
        print(indent, "  Right:");
        print_t_node(node->right, depth + 1, 0);
    } else {
        print(indent, "  Right: nullptr");
    }
    
    // Print children if any
    if (!node->children.empty()) {
        print(indent, "  Children: ", node->children.size());
        int child_index = 0;
        for (auto& child : node->children) {
            print_t_node(child, depth + 1, child_index++);
        }
    }
 }

static void parse_nodes(g_ptr<s_node> root) {
    print("==PARSE NODES PASS==");
    g_ptr<t_node> result = nullptr;
    for(int i = 0; i < root->a_nodes.size(); i++) {
        auto node = root->a_nodes[i];
        auto left = (i > 0) ? root->a_nodes[i-1] : nullptr;
        auto right = (i < root->a_nodes.size()-1) ? root->a_nodes[i+1] : nullptr;

        g_ptr<t_node> sub = parse_a_node(node,root,result);
        if(sub) {
        result = sub;
        root->t_nodes << result;
        }
    }

    for(auto t : root->t_nodes) {
        print_t_node(t,0,0);
    }
    
    for(auto child_scope : root->children) {
        parse_nodes(child_scope);
    }
}


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

//Fix this walking pattern later with a designated entry point so we're not processing parallel scope
static void discover_var_decleration(g_ptr<t_node> node, g_ptr<s_node> root,size_t& idx) 
{
    if(node->value.type==GET_TYPE(OBJECT)) {
        //Can add short circuiting and error handeling here
        g_ptr<s_node> on_scope = root;
        g_ptr<Type> type = nullptr;
        //This scope walking for types needs to be inspected further, potential cause of problems.
        for(auto c : root->children) {
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
            root->type_map.put(node->name,type);
        }
        root->slot_map.put(node->name,root->slot_map.size());
        root->type_ref->note_value(sizeof(size_t), node->name); 
        //Need size map entry here too?
    }
    else {
        root->type_ref->note_value(node->value.size,node->name);
        root->size_map.put(node->name,node->value.size);
        root->o_type_map.put(node->name,node->value.type);
    }
}

static g_ptr<s_node> discover_type_decl(g_ptr<t_node> node, g_ptr<s_node> root) {
    g_ptr<s_node> on_scope = root;
    g_ptr<Type> type = nullptr;
    for(auto c : root->children) {
        if(c->type_ref && c->type_ref->type_name == node->deferred_identifier) {
            return c;
        }
    }
    while(!type) {
        if(on_scope->type_ref) {
            if(on_scope->type_ref->type_name == node->deferred_identifier) {
                type = on_scope->type_ref;
            }
        }
        if(on_scope->parent) {
            on_scope = on_scope->parent;
        }
        else break;
    }
    return on_scope;
}

struct d_context {
    g_ptr<s_node> root;
    size_t& idx;
    
    d_context(g_ptr<s_node> _root, size_t& _idx) : root(_root), idx(_idx) {}
};
map<uint32_t, std::function<void(g_ptr<t_node>, d_context&)>> discover_handlers;

static void discover_symbols(g_ptr<s_node> root) {
    if(!root->type_ref) {
        root->type_ref = make<Type>();
    }
    size_t idx = 0;
    d_context ctx(root, idx);
    
    for(int i = 0; i < root->t_nodes.size(); i++) {
        auto node = root->t_nodes[i];
        if(discover_handlers.hasKey(node->type)) {
            auto func = discover_handlers.get(node->type);
            func(node, ctx);
        }
        else {
            //Default goes here
        }
    }
    
    for(auto child_scope : root->children) {
        discover_symbols(child_scope);
    }
}

static g_ptr<Frame> resolve_symbols(g_ptr<s_node> root);

static g_ptr<s_node> find_type_decl(g_ptr<Type> type, g_ptr<s_node> root) {
    g_ptr<s_node> on_scope = root;
    for(auto c : root->children) {
        if(c->type_ref && c->type_ref == type) {
            return c;
        }
    }
    while(on_scope->parent) {
        if(on_scope->type_ref) {
            if(on_scope->type_ref  == type) {
                return on_scope;
            }
        }
        on_scope = on_scope->parent;
    }
    return nullptr;
}

static void resolve_identifier(g_ptr<t_node> node,g_ptr<r_node> result,g_ptr<s_node> scope,g_ptr<Frame> frame) {
    void* ptr = nullptr;
    g_ptr<s_node> on_scope = scope;
    while(!ptr) {
        ptr = on_scope->type_ref->adress_column(node->name);
        if(ptr) break;
        if(on_scope->parent) {
            on_scope = on_scope->parent;
        }
        else break;
    }
    if(ptr) {
        result->name = node->name;
        if(on_scope->slot_map.hasKey(node->name)) {
            if(on_scope->frame) {
                if(on_scope->frame->slots.length()<=result->slot) on_scope->frame->slots << 0;
                result->frame = on_scope->frame;
            }
            else
                print("resolve_identifier::1621 no frame for scope!");
            result->slot = on_scope->slot_map.get(node->name);
            result->resolved_type = on_scope->type_map.get(node->name);
            result->value.type = GET_TYPE(OBJECT);
        }
        result->value.address = ptr;
        if(on_scope->size_map.hasKey(node->name)) {
            result->value.size = on_scope->size_map.get(node->name);
            result->value.type = on_scope->o_type_map.get(node->name);
        }
    }
    else print("resolve_symbol::1418 No address found for identifier ",node->name);
}

struct r_context {
    g_ptr<t_node> node;
    g_ptr<s_node> scope;
    g_ptr<Frame> frame;
    
    r_context(g_ptr<t_node> _node, g_ptr<s_node> _scope,g_ptr<Frame> _frame) : node(_node), scope(_scope), frame(_frame) {}
};
map<uint32_t, std::function<void(g_ptr<r_node>, r_context&)>> r_handlers;

static g_ptr<r_node> resolve_symbol(g_ptr<t_node> node,g_ptr<s_node> scope,g_ptr<Frame> frame) {
    g_ptr<r_node> result = make<r_node>();
    r_context ctx(node,scope,frame);
    try {
        r_handlers.get(node->type)(result,ctx);
    }
    catch(std::exception e) {
        print("resolve_symbol::1454 Missing case for t_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return result;
}

void print_r_node(const g_ptr<r_node>& node, int depth = 0, int index = 0) {
    std::string indent(depth * 2, ' '); 
    
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type));

    if (node->value.type != GET_TYPE(UNDEFINED)) {
        print(indent, "  Value: ", node->value.to_string(), " (type: ", node->value.type, ")");
    }
    
    if (!node->name.empty()) {
        print(indent, "  Name: ", node->name);
    }
    
    if (node->left) {
        print(indent, "  Left:");
        print_r_node(node->left, depth + 1, 0);
    } 
    
    if (node->right) {
        print(indent, "  Right:");
        print_r_node(node->right, depth + 1, 0);
    } 
    
    if (!node->children.empty()) {
        print(indent, "  Children: ", node->children.size());
        int child_index = 0;
        for (auto& child : node->children) {
            print_r_node(child, depth + 1, child_index++);
        }
    }
 }

static g_ptr<Frame> resolve_symbols(g_ptr<s_node> root) {
    print("==RESOLVE SYMBOLS PASS==");
    g_ptr<Frame> frame = make<Frame>();
    frame->type = root->scope_type;
    frame->context = root->type_ref; //This should've been filled in during discovery
    root->frame = frame;

    for(int i = 0; i < root->t_nodes.size(); i++) {
        auto node = root->t_nodes[i];
        g_ptr<r_node> rnode = resolve_symbol(node,root,frame);
        if(rnode) frame->nodes << rnode;
    }

    print("==RESOLVED SYMBOLS==");
    for(auto r : frame->nodes) {
        print_r_node(r);
    }
    
    // for(auto child_scope : root->children) {
    //     resolve_symbols(child_scope);
    // }
    return frame;
}   


static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context = nullptr);
static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index);

template<typename Op>
void execute_r_operation(g_ptr<r_node> node, Op operation,uint32_t result_type, g_ptr<Frame> frame) {
    if(node->left)
        execute_r_node(node->left,frame,0);
    if(node->right)
        execute_r_node(node->right,frame,0);
    
    t_value& left_val = node->left->value;
    t_value& right_val = node->right->value;
    node->value.type = result_type;
    
    if(left_val.type == GET_TYPE(INT) && right_val.type == GET_TYPE(INT)) {
        auto result = operation(*(int*)left_val.data, *(int*)right_val.data);
        if(result_type == GET_TYPE(BOOL)) node->value.set<bool>(result);
        else node->value.set<int>(result);
    }
    else if(left_val.type == GET_TYPE(FLOAT) || right_val.type == GET_TYPE(FLOAT)) {
        float left_f = (left_val.type == GET_TYPE(FLOAT)) ? *(float*)left_val.data : *(int*)left_val.data;
        float right_f = (right_val.type == GET_TYPE(FLOAT)) ? *(float*)right_val.data : *(int*)right_val.data;
        auto result = operation(left_f, right_f);
        if(result_type == GET_TYPE(BOOL)) node->value.set<bool>(result);
        else if(result_type == GET_TYPE(FLOAT)) node->value.set<float>(result);
        else node->value.set<int>(result);
    }
    else {
        print("execute_r_operation::1860 Type error in binary operation");
    }
}

struct exec_context {
    g_ptr<r_node> node;
    g_ptr<Frame> frame;
    size_t index;
    
    exec_context(g_ptr<r_node> _node, g_ptr<Frame> _frame, size_t _index) 
        : node(_node), frame(_frame), index(_index) {}
};

map<uint32_t, std::function<g_ptr<r_node>(exec_context&)>> exec_handlers;

static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index) {
    exec_context ctx(node,frame,index);
    try {
        exec_handlers.get(node->type)(ctx);
    }
    catch(std::exception e) {
        print("execute_r_node::1554 Missing case for r_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return node;
}

static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context) {
    if(!context) {
        context = frame->context->create();
    }
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame,context->ID);
        if(frame->return_val.type != GET_TYPE(UNDEFINED)) {
            break;
        }
        else if(!frame->isActive()) {
            break;
        }
    }
    frame->context->recycle(context);
}   

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
