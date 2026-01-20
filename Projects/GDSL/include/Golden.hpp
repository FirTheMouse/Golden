#pragma once

#include<util/util.hpp>
#include<core/type.hpp>

//Controls for the compiler printing, for debugging
#define PRINT_ALL 0
#define PRINT_STYLE 1
//Very important when adding modules, but will add overhead on execution, causes the reg class
//to check if a hash exists before using it
#define CHECK_REG 0


//The GDSL compiler has undergone plenty of changes over time, its current version is 0.1.9, with 0.2.0 planned to be the
//addition of proper arrays and for loops that can take advantage of the memory model to test bulk object opperations.
//Orginally, this started as a one pass string-based parser, similar to my other early domain specific languages. 
//I was making GDSL (lit. Golden Domain Specific Language) for GUIDE (Golden User Interface Development Envrionment) as
//a way to store the behaviour of dynamic gui elements with just a string. 
//While writting the code, I had an idea to eliminate the synactic overhead of the engine API
//i.e: GDSL was going to be a transpiler to C++, just a way to get rid of 'g_ptr<T>'.
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
//This was the original API sketch I had for how I wanted GDSL's syntax to look, at this point I had decided
//this would be a dsl for Golden instead of just a script interpreter for GUIDE.
//By version 0.0.5 (which is archived in the giant storage dump in the main.cpp of testing) I had moved towards a multi-pass system
//which is more recognizable as GDSL's compiler, with the pipeline: token -> a_node -> t_node/s_node. This was after about 4 days
//of development. At this time, I started working on the Type as well, a big pain point with this version was adding types, which each 
//required explicit handeling such as int_list, float_list, and switch cases to detect them all. 
//At this time, Type was based on the architecture of Scene, with the idea being we create a struct of arrays/ECS and use that as the memory model.
//To make Type type agnostic, I came up with the idea to store the bytes directly, and bucket the arrays by size instead of by type. 
//I eventually implmented this on day 6, and got it working in a very basic state with a clunky API, though in testing it matched a 
//handmade struct of arrays. I actually made the classes now in Logger/rig (my profiler) such as time_function for this. 
//I used the new Type in version 0.0.6, and realized the next major pain point would be the switch cases. I didn't like the idea
//of having to go through a bunch of methods to add and maintain cases, it felt like something that would lead to points of failure
//and development pain later on. I also *really* didn't want to spend hours refractoring all the switch cases I had accumulated.
//I avoided this for 3 days, and eventually, I had the spike of motivation to just bite the bullet and get it done.
//It only took 5 hours to actaully fiqure out how to make it modular with the registry and maps. 
//After that, it was days of one to two hours of just sorting and converting, I would work on it while doing other things
//like during a long car drive or while watching a show. Eventually, I had everything aranged into the new module system.
//I did some more work to get GDSL to a functioning state, up to 0.1.0, then I had to go to college and deal with moving in and such.
//A couple weeks later, I did further work, all of which should be visible on GitHub now.
//At one point, while working on something else, I had the idea that maybe I could turn the GDSL compiler into a way to make
//dsls quickly for various projects, instead of my quick string-based parsers I've made a couple times, yet I haven't tried this yet.



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
        #if CHECK_REG
        if(!hash_to_enum.hasKey(hash)) {
            print("reg::40 Warning, key not found for enum ",hash,"!");
            return 0;
        }
        #endif 
        return hash_to_enum.get(hash);
    }

    static std::string name(uint32_t ID) {
        #if CHECK_REG
        if(!enum_to_string.hasKey(ID)) {
            print("reg::50 Warning, key not found for enum ",ID,"!");
            return "[null]";
        } 
        #endif
        return enum_to_string.get(ID);
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

//t_info contains all the key information for a token (t stage), the family and such is a bit unnesecary
//right now, but I keep it around because I've got no idea when we'll need it. 
struct t_info {
    t_info(uint32_t _type, size_t _size, uint32_t _family) : type(_type), size(_size), family(_family) {}
    t_info(uint32_t _type, size_t _size) : type(_type), size(_size) {}
    t_info() {}
    uint32_t type = 0;
    size_t size = 0;
    uint32_t family = 0;
};

            //Many classes inherity from Object for the smart pointer, g_ptr, and because I made use of dynamic properties in prototyping
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

// static bool token_is_split(char c) {
//     return (c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='('||c==')'||c==','||c=='='||c=='>'||c=='<'||c=='['||c==']');
// }

map<char,bool> char_is_split;

static map<std::string,t_info> t_keys;

static void reg_t_key(std::string name,const t_info& info) {
    t_keys.put(name,info);
}

static void reg_t_key(std::string name,uint32_t enum_key,size_t size, uint32_t family_key) {
    reg_t_key(name,t_info(enum_key,size,family_key));
}

//Consider adding this in the future, if it's ever really needed
//map<char,std::function<void()>> tokenizer_functions;

//Basic recursive descent tokenizer, this is probably the least fancy part of the entire compiler
static list<g_ptr<Token>> tokenize(const std::string& code,char end_char = ';') {
    auto it = code.begin();
    list<g_ptr<Token>> result;
    g_ptr<Token> token = nullptr;

    enum State {
       OPEN, IN_STRING, IN_NUMBER, IN_IDENTIFIER, IN_COMMENT
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
                token->type_info.size = 24;
            }
            else {
                token->add(c);
            }
            ++it;
        } else if(state==IN_COMMENT) {
            if(c=='\n') {
                state=OPEN;
            }
            ++it;
        }
        else if(state==IN_IDENTIFIER) {
            if(c==end_char||c=='.'||char_is_split.hasKey(c)) {
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
            if(c==end_char||char_is_split.hasKey(c)) {
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
                case '@':
                    token = make<Token>(GET_TYPE(AT_SYMBOL),"@");
                    result << token;
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
                case '[':
                        token = make<Token>(GET_TYPE(LBRACKET),"[");
                        result << token;
                break;
                case ']':
                        token = make<Token>(GET_TYPE(RBRACKET),"]");
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
                case '#':
                    state = IN_COMMENT;
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

    #if PRINT_ALL
    for(auto t : result) {
        if(t->getType()) {
            print(TO_STRING(t->getType()));
        }
    }
    #endif
    return result;
}

//Forward delcaring s_node
class s_node;

//The a_node is used for the 'a stage', it contains groupings of tokens and their scopes.
//Most a_functions are very basic state transforms for the recursive descent parser, this is more a cleanup stage after tokenization
class a_node : public Object {
public:
    a_node() {}
    ~a_node() {}

    list<g_ptr<Token>> tokens;
    uint32_t type = GET_TYPE(UNTYPED);

    list<g_ptr<a_node>> sub_nodes;
    s_node* owned_scope = nullptr;
    s_node* in_scope = nullptr;
    bool balanced = false;
};

//Used for debugging, and for printing 
map<uint32_t,std::function<std::string(void*)>> value_to_string;
map<uint32_t, std::function<void(void*)>> negate_value;

static std::string data_to_string(uint32_t type,void* data) {    
    try {
        return value_to_string.get(type)(data);
    }
    catch(std::exception e) {
        return "[missing value_to_string for type "+TO_STRING(type)+"]";
    }
}


/// @brief BIG WARNING! These leak right now and we need to add cleanup for data in the future!
struct t_value {

    ~t_value() {
        //Can't do any of this because it would cause a double free
        //I should really just make t_value a g_ptr...
        //free(data);
    }
    uint32_t type = GET_TYPE(UNDEFINED);
    void* data;
    int address = -1;
    size_t size;
    int sub_size = -1;

    template<typename T>
    T get() { return *(T*)data; }
    
    template<typename T>
    void set(const T& value) { 
        if (!data) {
            data = malloc(sizeof(T));
            size = sizeof(T);
        }
        new(data) T(value);
    }

    std::string to_string() {
        if (!data) {
            if(type==GET_TYPE(OBJECT)) {
                return "OBJ";
            }
            else {
                return "[null]";
            }
        }
        
        try {
            return value_to_string.get(type)(data);
        }
        catch(std::exception e) {
            return "[missing value_to_string for type "+TO_STRING(type)+"]";
        }
    }

    void negate() {
        if(data) {
            try {
                negate_value.get(type)(data);
            }
            catch(std::exception e) {
                print("t_value::450 missing negate_value handler for ",TO_STRING(type));
            }
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

//S_node means 'scene node' it's used as the main store for meaning and is where a lot of the linking and transformation happens
//before the 'r stage'.
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

    map<std::string,int> slot_map;
    map<std::string,g_ptr<Type>> type_map;
    map<std::string,size_t> size_map;
    map<std::string,size_t> total_size_map;
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
#if PRINT_STYLE == 0
    std::string indent(depth * 2, ' '); 
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type), " Subs: ", node->sub_nodes.size());

    if (!node->tokens.empty()) {
        print(indent, "  Tokens:");
        for (auto& t : node->tokens) {
            print(indent, "    ", t->content, " (", TO_STRING(t->getType()), ")");
        }
    }

    if (!node->sub_nodes.empty()) {
        print(indent, "  Sub-nodes:");
        int sub_index = 0;
        for (auto& sub : node->sub_nodes) {
            print_a_node(sub, depth + 1, sub_index++);
        }
    }
#endif
#if PRINT_STYLE == 1
    std::string indent(depth * 2, ' '); 
    if(node->type==GET_TYPE(END)) {
        print(indent,"END");
    }
    else {
        printnl(indent,TO_STRING(node->type));
        if (!node->tokens.empty()) {
            printnl("[");
            for (auto& t : node->tokens) {
                printnl(t->content,t==node->tokens.last()?"]":", ");
            }
        }

        if (!node->sub_nodes.empty()) {
            for (auto& sub : node->sub_nodes) {
                print(""); print_a_node(sub,depth+1);
            }
        }
    }
#endif
}

static std::pair<int,int> balance_tokens(list<g_ptr<Token>> tokens, uint32_t a, uint32_t b,int start = 0) {
    std::pair<int,int> result(-1,-1);
    int depth = 0;
    for(int i=start;i<tokens.length();i++) {
        g_ptr<Token> t = tokens[i];
        if(t->getType()==a) {
            if(depth==0) {
                result.first = i;
            }
            depth++;
        }
        else if(t->getType()==b) {
            depth--;
            if(depth<=0) {
                result.second = (i+1);
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
        #if PRINT_ALL
        print("==PARSE TOKENS PASS==");
        #endif
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
                uint32_t opp = token_to_opp.getOrDefault(token->getType(),GET_TYPE(UNTYPED));
                if(opp!=GET_TYPE(UNTYPED)) {
                    //If we're already in an opperation, end the current one, start a new one
                    if(state_is_opp.getOrDefault(state,false)) {
                        end();
                        state=opp;
                    }
                    else {
                        state=opp;
                    }
                }
                else
                print("parse_tokens::653 missing case for type ",TO_STRING(token->getType()));
            }
            ++it;
            ++pos;
        }

        #if PRINT_ALL
        if(!local)
        {
            int i = 0;
            for (auto& node : result) {
                print_a_node(node, 0, i++);
            }
        }
        #endif

        return result;
}


static bool balance_nodes(list<g_ptr<a_node>>& result) {
    uint32_t state = GET_TYPE(UNTYPED);
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
                node->in_scope = current_scope.getPtr();
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
                node->in_scope = current_scope.getPtr();
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

    #if PRINT_ALL
    print("=== PARSE SCOPE PASS ===");
    print_scope(root_scope);
    #endif
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

//t_parse_expression is one of my favourite methods, it handles bassicly all experssion parsing for the 't stage'
//In just 80 lines of code (as of 0.1.9), its a very clean example of how I think code should be structured: 
//find the core relationship rather than the emergent property.
static g_ptr<t_node> t_parse_expression(g_ptr<a_node> node,g_ptr<t_node> left=nullptr) {
    g_ptr<t_node> result = make<t_node>();
    result->type = t_opp_conversion.getOrDefault(node->type,GET_TYPE(UNDEFINED));
    
    if (node->tokens.size() == 2) {
        result->left = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
        result->right = t_literal_handlers.get(node->tokens[1]->getType())(node->tokens[1]);
    } 
    else if (node->tokens.size() == 1) {
        if(node->sub_nodes.size()==0) {
            if(left) {
                result->left = left;
                result->right = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
                if(node->in_scope) { //Removes left refrence by taking it's place
                    if(node->in_scope->t_nodes.last()==left) {
                        node->in_scope->t_nodes.pop();
                    }
                }
            }
            else {
               if(state_is_opp.getOrDefault(node->type,false)&&t_functions.hasKey(node->type)) { //For opperators like i* or i++
                    t_context ctx(result,node,nullptr);
                    ctx.left = left;
                    result = t_functions.get(node->type)(ctx);
                } 
                else {
                    result = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
                }
            }
        }
        else if(node->sub_nodes.size()==1) {
            result->left = t_literal_handlers.get(node->tokens[0]->getType())(node->tokens[0]);
            result->right = t_parse_expression(node->sub_nodes[0],nullptr); //To prevent recursion in unary opperators
            //Was passing result as left, so something else may be broken by this
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
            if(left&&node->in_scope) { //This removes duplicate left refrences, such as with var_decl + assignment
                if(node->in_scope->t_nodes.last()==left) {
                    node->in_scope->t_nodes.pop(); //Used to be set last to result, return nullptr
                    // node->in_scope->t_nodes.last() = result;
                    // return nullptr;
                }
            } 
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
            if(left&&node->in_scope) {
                if(node->in_scope->t_nodes.last()==left) {
                    node->in_scope->t_nodes.pop();
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
            if(!result) {
                print("NULL RETURNED: ",TO_STRING(node->type));
            }
        }
        else {
            print("parse_a_node::940 missing parsing code for a_node type: ",TO_STRING(node->type)); 
        }
    }
    return result;
}

static void parse_sub_nodes(t_context& ctx, bool deduplicate = false) {
    if(!ctx.node->sub_nodes.empty()) {
        g_ptr<t_node> last = nullptr;
        for(auto c : ctx.node->sub_nodes) {
            g_ptr<t_node> sub = parse_a_node(c, ctx.root, last);
            if(sub) {
                if(deduplicate && last && sub->left == last) {
                    ctx.result->children.erase(last);
                }
                last = sub;
                ctx.result->children << last;
            } else {
                last = nullptr;
            }
        }
    }
}

void print_t_node(const g_ptr<t_node>& node, int depth = 0, int index = 0) {
    std::string indent(depth * 2, ' '); 
    
    // Print node info
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type));
    
    // Print value if it exists
    if (node->value.type != GET_TYPE(UNDEFINED)) {
        print(indent, "  Value: ", node->value.to_string(), " (type: ", TO_STRING(node->value.type), ")");
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
    #if PRINT_ALL
    print("==PARSE NODES PASS==");
    #endif
    g_ptr<t_node> last = nullptr;
    for(int i = 0; i < root->a_nodes.size(); i++) {
        auto node = root->a_nodes[i];
        g_ptr<t_node> sub = parse_a_node(node,root,last);
        last = sub;
        if(sub) {
            root->t_nodes << last;
        }
    }

    #if PRINT_ALL
    for(auto t : root->t_nodes) {
        print_t_node(t,0,0);
    }
    #endif
    
    for(auto child_scope : root->children) {
        parse_nodes(child_scope);
    }
}


class Frame : public Object {
    public:
    Frame() {
        slots << list<size_t>();
    }
    uint32_t type = GET_TYPE(GLOBAL);
    g_ptr<Type> context;
    list<g_ptr<r_node>> nodes;
    list<list<size_t>> slots;
    g_ptr<r_node> return_to = nullptr;
    list<g_ptr<Object>> active_objects;
    list<void*> active_memory;
    list<std::function<void()>> stored_functions;
};

class r_node : public Object {
    public:
    uint32_t type = GET_TYPE(R_NOP);
    g_ptr<r_node> left;
    g_ptr<r_node> right;
    list<g_ptr<r_node>> children;
    t_value value;
    g_ptr<Type> in_scope = nullptr;
    g_ptr<Frame> in_frame = nullptr;
    int slot = -1;
    int index = -1;
    std::string name;
    g_ptr<Frame> frame;
};

struct d_context {
    g_ptr<s_node> root;
    size_t& idx;
    
    d_context(g_ptr<s_node> _root, size_t& _idx) : root(_root), idx(_idx) {}
};
map<uint32_t, std::function<void(g_ptr<t_node>, d_context&)>> discover_handlers;

//The discovery stage is here because of how Type works, we need to know the memory layout
//before it gets populated, if we use address refrences instead of indexes in the Type.
//For streaming and C++ competitive performance we need this. Though this stage can also be used for other things

static void discover_symbol(g_ptr<t_node> node,d_context& ctx) {
    if(discover_handlers.hasKey(node->type)) {
        auto func = discover_handlers.get(node->type);
        func(node, ctx);
    }
    else {
        if(node->left) {
            discover_symbol(node->left,ctx);
        }
        if(node->right) {
            discover_symbol(node->right,ctx);
        }
        for(auto c : node->children) {
            discover_symbol(c,ctx);
        }
    }
}

static void discover_symbols(g_ptr<s_node> root) {
    if(!root->type_ref) {
        root->type_ref = make<Type>();
    }
    size_t idx = 0;
    d_context ctx(root, idx);
    
    for(int i = 0; i < root->t_nodes.size(); i++) {
        auto node = root->t_nodes[i];
        discover_symbol(node,ctx);
    }
    
    for(auto child_scope : root->children) {
        for(auto e : root->slot_map.entrySet()) {
            child_scope->slot_map.put(e.key,e.value);
        }
        discover_symbols(child_scope);
    }
}

static g_ptr<Frame> resolve_symbols(g_ptr<s_node> root);

static g_ptr<s_node> find_scope(g_ptr<s_node> start, std::function<bool(g_ptr<s_node>)> check) {
    g_ptr<s_node> on_scope = start;
    for(auto c : start->children) {
        if(check(c)) {
            return c;
        }
    }
    while(true) {
        if(on_scope->type_ref) {
            if(check(on_scope)) {
               return on_scope;
            }
        }
        for(auto c : on_scope->children) {
            if(c==start||c==on_scope) continue;
            if(check(c)) {
               return c;
            }
        }
        if(on_scope->parent) {
            on_scope = on_scope->parent;
        }
        else 
            break;
    }
    return nullptr;
}

static g_ptr<s_node> find_type_ref(const std::string& match,g_ptr<s_node> start) {
    return find_scope(start,[match](g_ptr<s_node> c){
        if(!c->type_ref) return false;
        return c->type_ref->type_name == match;
    });
}

static void resolve_identifier(g_ptr<t_node> node,g_ptr<r_node> result,g_ptr<s_node> scope,g_ptr<Frame> frame = nullptr) {
    int index = -1;
    g_ptr<s_node> on_scope = find_scope(scope,[node](g_ptr<s_node> c){
        return (c->type_ref->get_note(node->name).index!=-1 || c->type_map.hasKey(node->name) || c->type_ref->type_name == node->name);
    });

    if(!on_scope) {
        print("resolve_symbol::1318 Unable to resolve an identifier: ",node->name);
        return;
    }

    if(on_scope->type_map.hasKey(node->name)) index = 0;
    else index = on_scope->type_ref->get_note(node->name).index;
    if(index>-1) {
        result->name = node->name;
        if(on_scope->type_map.hasKey(node->name)) {
            if(on_scope->frame) {
                //if(on_scope->frame->slots[0].length()<=result->slot) on_scope->frame->slots[0] << 0;
                result->frame = on_scope->frame; //Why did I make 2 pointers to the frame?
                result->in_frame = on_scope->frame; //Check later to see if this is actaully used!
            }
            else {
                print("resolve_identifier::1621 no frame for scope!");
            }
            //Assign -2 as the default slot for identifiers if they aren't in the slot map
            result->slot = on_scope->slot_map.getOrDefault(node->name,-2);
            //Forgot what all the in_scope, in_frame, frame, are meant to do, find that and comment to remind!
            result->in_scope = on_scope->type_map.get(node->name);
            result->value.type = GET_TYPE(OBJECT);
        } 
        else
            result->value.address = index;

        if(on_scope->size_map.hasKey(node->name)) {
            result->value.size = on_scope->size_map.get(node->name);
            result->value.type = on_scope->o_type_map.get(node->name);
            result->in_scope = on_scope->type_ref;
            result->in_frame = on_scope->frame;
            if(on_scope->total_size_map.hasKey(node->name)) {
                result->value.sub_size = on_scope->total_size_map.get(node->name);
            }
        }

        // if(node->deferred_identifier!="") {
        //     std::string o_name = node->name; //For cleanup if we ever reuse the nodes
        //     std::string o_def_id = node->deferred_identifier;
        //     node->name = node->deferred_identifier;
        //     node->deferred_identifier = "";
        //     g_ptr<r_node> sub = make<r_node>();
        //     resolve_identifier(node,sub,scope);
        //     result->value = std::move(sub->value);
        //     node->name = o_name;
        //     node->deferred_identifier = o_def_id;
        // }
    }
    else { //This is the foundation for using type keys in debug
        // on_scope = find_type_ref(node->name,scope); //Unnesecary because I added this check to the intial scope search check
        if(on_scope) {
            result->name = node->name;
            result->value.address = -1;
            result->value.size = 0;
            result->in_scope = on_scope->type_ref;
            //Using TYPE
            // result->value.type = GET_TYPE(TYPE); //Yes this is a token type, I'm using it as a placeholder for keys
            // result->type = GET_TYPE(TYPE);

            //Trying as a flexible OBJECT instead
            result->in_frame = on_scope->frame;
            result->frame = on_scope->frame;
            result->value.type = GET_TYPE(OBJECT);

        }
        else
            print("resolve_symbol::1418 No address found for identifier ",node->name);
    }
}

struct r_context {
    g_ptr<t_node> node;
    g_ptr<s_node> scope;
    g_ptr<Frame> frame;
    
    r_context(g_ptr<t_node> _node, g_ptr<s_node> _scope,g_ptr<Frame> _frame) : node(_node), scope(_scope), frame(_frame) {}
};
map<uint32_t, std::function<void(g_ptr<r_node>&, r_context&)>> r_handlers;

static g_ptr<r_node> resolve_symbol(g_ptr<t_node> node,g_ptr<s_node> scope,g_ptr<Frame> frame,g_ptr<r_node> result = nullptr) {
    if(!result) result = make<r_node>();
    r_context ctx(node,scope,frame);
    try {
        result->in_scope = scope->type_ref;
        result->in_frame = scope->frame;
        r_handlers.get(node->type)(result,ctx);
    }
    catch(std::exception e) {
        if(!r_handlers.hasKey(node->type)) print("resolve_symbol::1222 Missing case for t_type: ",TO_STRING(node->type));
        else  print("resolve_symbol::1223 crash on t_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return result;
}

void print_r_node(const g_ptr<r_node>& node, int depth = 0, int index = 0) {
    std::string indent(depth * 2, ' '); 
    
    print(indent, "Node #", index, " Type: ", TO_STRING(node->type));

    if (node->value.type != GET_TYPE(UNDEFINED)) {
        print(indent, "  Value: ", node->value.to_string(), " (type: ", TO_STRING(node->value.type), ")");
    }
    
    if (!node->name.empty()) {
        print(indent, "  Name: ", node->name);
    }

    if(node->in_scope) {
        print(indent,"  Scope: ", node->in_scope->type_name);
    }

    if(node->slot!=-1) {
        print(indent,"  Slot: ", node->slot);
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
    #if PRINT_ALL
    print("==RESOLVE SYMBOLS PASS==");
    #endif
    g_ptr<Frame> frame = make<Frame>();
    frame->type = root->scope_type;
    frame->context = root->type_ref; //This should've been filled in during discovery
    root->frame = frame;
    if(frame->context->type_name=="bullets") frame->context->type_name = TO_STRING(root->scope_type);

    for(int i = 0; i < root->t_nodes.size(); i++) {
        auto node = root->t_nodes[i];
        g_ptr<r_node> rnode = resolve_symbol(node,root,frame);
        if(rnode) frame->nodes << rnode;
    }

    #if PRINT_ALL
    print("==RESOLVED SYMBOLS: ",frame->context->type_name,"==");
    for(auto r : frame->nodes) {
        print_r_node(r);
    }
    #endif
    
    for(auto child_scope : root->children) {
        resolve_symbols(child_scope);
        // g_ptr<Frame> child_frame = resolve_symbols(child_scope);
        // child_frame->slots << frame->slots;
    }
    return frame;
}   


static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context,int sub_index);
static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index,int sub_index);

template<typename Op>
void execute_r_operation(g_ptr<r_node> node, Op operation,uint32_t result_type, g_ptr<Frame> frame) {
    if(node->left)
        execute_r_node(node->left,frame,0,-1);
    if(node->right)
        execute_r_node(node->right,frame,0,-1);
    
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
    int sub_index = -1;
    
    exec_context(g_ptr<r_node> _node, g_ptr<Frame> _frame, size_t _index,int _sub_index) 
        : node(_node), frame(_frame), index(_index), sub_index(_sub_index) {}
};

map<uint32_t, std::function<g_ptr<r_node>(exec_context&)>> exec_handlers;

static g_ptr<r_node> execute_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index, int sub_index) {
    exec_context ctx(node,frame,index,sub_index);
    try {
        exec_handlers.get(node->type)(ctx);
    }
    catch(std::exception e) {
        if(!exec_handlers.hasKey(node->type)) print("execute_r_node::1343 Missing case for r_type: ",TO_STRING(node->type));
        else  print("execute_r_node::1343 crash on r_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return node;
}


static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context,int sub_index) {
    if(!context) { 
        //This is bassicly just to push rows and mark them as usable or not to a thread
        //Want to replace this later with something more efficent, or, make more use of context
        context = frame->context->create();
    }

    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame,context->ID,sub_index);
        if(!frame->isActive()) {
            break;
        }
    }
    if(!frame->isActive())
        frame->resurrect();

    frame->context->recycle(context);
    for(int i=frame->active_objects.length()-1;i>=0;i--) {
        frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    }
    for(int i=frame->active_memory.length()-1;i>=0;i--) {
        free(frame->active_memory[i]);
    }
}   

static void execute_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context = nullptr) {
    execute_r_nodes(frame,context,-1);
}

static void execute_constructor(g_ptr<Frame> frame,int index) {
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame,index,-1);
        if(!frame->isActive()) {
            break;
        }
    }
}

//Maintains slot cohesion when executing sub frames, such as a lower "if" or components of a loop
static void execute_sub_frame(g_ptr<Frame> sub_frame, g_ptr<Frame> frame,g_ptr<Object> context = nullptr) {
    for(int i=0;i<frame->slots.length();i++) {
        sub_frame->slots[i] = frame->slots[i];
    }
    execute_r_nodes(sub_frame,context);
    for(int i=0;i<frame->slots.length();i++) {
        frame->slots[i] = sub_frame->slots[i];
    }
}

map<uint32_t, std::function<std::function<void()>(exec_context&)>> stream_handlers;

static void stream_r_node(g_ptr<r_node> node,g_ptr<Frame> frame,size_t index) {
    exec_context ctx(node,frame,index,-1);
    try {
        std::function<void()> func = stream_handlers.get(node->type)(ctx);
        //frame->context->push(&func,32,1);
        if(func) {
            frame->stored_functions << func;
        }
    }
    catch(std::exception e) {
        if(!stream_handlers.hasKey(node->type)) print("stream_r_node::1380 Missing case for r_type: ",TO_STRING(node->type));
        else  print("Stream_r_node::1381 crash on r_type: ",TO_STRING(node->type));
    }
}

//Streaming is a diffrent execution approach that pre-resolves the values to direct addresses and bytes to feed into the Type
//while this looses flexibility, it can reach the same performance levels as C++ (within 30% in benchmarks), though, this has
//been shotgunned by the change to use Type indexes instead of adresses.
static void stream_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context=nullptr) {
    #if PRINT_ALL
    print("==STREAM NODES==");
    #endif
    if(!context) { 
        context = frame->context->create();
    }
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        stream_r_node(node,frame,context->ID);
    }
    frame->context->recycle(context);
    // for(int i=frame->active_objects.length()-1;i>=0;i--) {
    //     frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    // }
}   

static void execute_stream(g_ptr<Frame> frame) {
    for(int i =0;i<frame->stored_functions.length();i++) {
        frame->stored_functions[i]();
    }
    for(int i=frame->active_objects.length()-1;i>=0;i--) {
        frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    }
    
    // if(frame->context->byte32_columns.length()>0) {
    //     void* stream_addr = &frame->context->byte32_columns[1];
    //     list<byte32_t>* stream = (list<byte32_t>*)stream_addr;
    //     for(int i=0;i<frame->context->row_length(1,32);i++) {
    //         print("A");
    //         (*(std::function<void()>*)&(*stream)[i])();
    //         print("B");
    //     }
    // } else {
    //     print("execute_stream::1380 frame context is missing execution functions");
    // }
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