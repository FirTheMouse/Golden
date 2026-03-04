#pragma once

#include<util/util.hpp>
#include<core/type.hpp>

namespace GDSL {

//Controls for the compiler printing, for debugging
#define PRINT_ALL 1
#define PRINT_STYLE 0
//Very important when adding modules, but will add overhead on execution, causes the reg class
//to check if a hash exists before using it
#define CHECK_REG 1


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
//In January of 2026 I turned it from a compiler for a specific language into a general framework usable in all Golden projects as an
//embedable programming language designer.

//ARCHIVE REFRACTOR NOTES GOLDEN 0.3.1
//This needs to be simplified. 


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

//A value holds all the actaul worked-with information, from it's address in a Type (if Type is used) to the actual ptr to the data.
//NOTE TO SELF: Should any map storing these be nuked before running? Given as they are smart pointers so if a map holds one it won't get deleted properly
//from a memory managment standpoint these are pretty bad, but they're here more for the prototype stage, and eventually users should be able to easily replace them.
class Value : public Object {
public:
    Value() {}
    Value(uint32_t _type) : type(_type) {}
    Value(uint32_t _type, size_t _size) : type(_type), size(_size) {}
    Value(uint32_t _type, size_t _size, uint32_t _sub_type) : type(_type), size(_size), sub_type(_sub_type) {}
    ~Value() {}
    uint32_t type = 0;
    uint32_t sub_type = 0;
    void* data = nullptr;
    int address = -1;
    size_t size = 0;
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
            return "[null]";
        }
        std::function<std::string(void*)> fallback_func = [this](void* ptr){return "[missing value_to_string for type "+TO_STRING(type)+"]";};
        return value_to_string.getOrDefault(type,fallback_func)(data);
    }

    void negate() {
        if(data) {
            try {
                negate_value.get(type)(data);
            }
            catch(std::exception e) {
                print("value::300 missing negate_value handler for ",TO_STRING(type));
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

g_ptr<Value> fallback_value = nullptr;
class Frame;

//Single unified Node for everything
class Node : public Object {
public:
    Node() {
        value = make<Value>();
    }
    Node(uint32_t _type, char c) : type(_type) {
        name += c;
    }

    uint32_t type = 0;
    std::string name = "";
    g_ptr<Value> value = nullptr;
    g_ptr<Node> left = nullptr;
    g_ptr<Node> right = nullptr;
    list<g_ptr<Node>> children;
    g_ptr<Frame> frame = nullptr;

    Node* parent;
    Node* owner;
    Node* owned_scope;
    Node* scope;

    map<std::string,g_ptr<Value>> value_table;

    uint32_t getType() {return type;}
    void setType(uint32_t _type) {type = _type;}

    g_ptr<Node> spawn_node() {
        g_ptr<Node> new_node = make<Node>();
        new_node->parent = this;
        children << new_node;
        return new_node; 
    }

    //Could probably replace this using the Object data system, keep them for explicitness but may just cmd+f replace them later
    list<g_ptr<Node>> opt_sub;
    list<g_ptr<Node>> opt_sub_2; //Kludge for scopes
    std::string opt_str;

    //Noted scope special cases:
    //Children = scope children
    //opt_sub = a_nodes
    //opt_sub_2 = t_nodes

    std::string to_string(int depth = 0, int index = 0) {
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + "Node #" + std::to_string(index) + " Type: " + TO_STRING(type) + "\n";
    
        if(value && value->type != 0) {
            to_return += indent + "  Value: " + value->to_string() + " (type: " + TO_STRING(value->type) + ")\n";
        }
    
        if(!name.empty()) {
            to_return += indent + "  Name: " + name + "\n";
        }
     
        if(left) {
            to_return += indent + "  Left:\n";
            to_return += left->to_string(depth + 1, 0);
        }
    
        if(right) {
            to_return += indent + "  Right:\n";
            to_return += right->to_string(depth + 1, 0);
        }
    
        if(!children.empty()) {
            to_return += indent + "  Children: " + std::to_string(children.size()) + "\n";
            int i = 0;
            for(auto& child : children) {
                to_return += child->to_string(depth + 1, i++);
            }
        }
    
        if(!opt_sub.empty()) {
            to_return += indent + "  A-Nodes: " + std::to_string(opt_sub.size()) + "\n";
            int i = 0;
            for(auto& child : opt_sub) {
                to_return += child->to_string(depth + 1, i++);
            }
        }
    
        if(!opt_sub_2.empty()) {
            to_return += indent + "  T-Nodes: " + std::to_string(opt_sub_2.size()) + "\n";
            int i = 0;
            for(auto& child : opt_sub_2) {
                to_return += child->to_string(depth + 1, i++);
            }
        }
    
        return to_return;
    }
};


class Frame : public Object {
    public:
    Frame() {
        if(!context) {
            context = make<Type>();
        }
    }
    uint32_t type = 0;
    g_ptr<Type> context = nullptr;
    std::string name;
    list<g_ptr<Node>> nodes;
    list<g_ptr<Value>> stack;
    g_ptr<Node> return_to = nullptr;
    list<g_ptr<Object>> active_objects;
    list<void*> active_memory;
    list<std::function<void()>> stored_functions;
};

static list<g_ptr<Node>> _ctx_dummy_result;
static int _ctx_dummy_index = 0;


struct Context {
    Context() : result(_ctx_dummy_result), index(_ctx_dummy_index) {}
    Context(list<g_ptr<Node>>& _result, int& _index) : result(_result), index(_index) {}

    g_ptr<Node> node;
    g_ptr<Node> left;
    g_ptr<Node> out;
    g_ptr<Node> root;
    g_ptr<Frame> frame;
    list<g_ptr<Node>>& result;
    list<g_ptr<Node>> nodes;
    int& index;
    uint32_t state;

    std::string source;


    Context sub() {
        return Context(result, index);
    }
};

using Handler = std::function<void(Context& ctx)>;
map<char,Handler> tokenizer_functions;
map<char,Handler> tokenizer_state_functions;
Handler tokenizer_default_function = nullptr;

map<uint32_t,Handler> a_functions;
Handler a_default_function;

map<uint32_t,Handler> t_functions;
Handler t_default_function;

map<uint32_t, Handler> discover_handlers;

map<uint32_t, Handler> r_handlers;

map<uint32_t, Handler> exec_handlers;

static list<g_ptr<Node>> tokenize(const std::string& code) {
    list<g_ptr<Node>> result;
    uint32_t state = 0;
    int index = 0;
    Context ctx(result,index);
    ctx.source = code;

    if(!tokenizer_default_function) {
        print("GDSL::tokenize warning! No defined default function, please define one");
    }

    while (index<code.length()) {
        char c = code.at(index);
        if(ctx.state!=0&&tokenizer_state_functions.hasKey(ctx.state)) {
            auto state_func = tokenizer_state_functions.get(ctx.state);
            state_func(ctx);
        } else {
            auto func = tokenizer_functions.getOrDefault(c,tokenizer_default_function);
            func(ctx);
        }
        ++index;
    }  

    #if PRINT_ALL
    for(auto t : result) {
        if(t->getType()) {
            print(TO_STRING(t->getType()),": ",t->name);
        }
    }
    #endif

    return result;
}

static list<g_ptr<Node>> parse_tokens(list<g_ptr<Node>> tokens,bool local = false) {
        #if PRINT_ALL
        print("==PARSE TOKENS PASS (A)==");
        #endif
        
        if(!a_default_function)
            print("GDSL::parse_tokens a_stage requires a default function!");

        list<g_ptr<Node>> result;
        int index = 0;
        
        while(index < tokens.length()) {
            if(!tokens[index]) break;
            Context ctx(result, index);
            ctx.nodes = tokens;
            #if PRINT_ALL
                print("In parse tokens, running: ",tokens[index]->name," [",TO_STRING(tokens[index]->getType()),", idx: ",ctx.index,"]");
            #endif
            a_functions.getOrDefault(tokens[index]->getType(),a_default_function)(ctx);
            index++;
        }

        #if PRINT_ALL
        int i = 0;
        for (auto& node : result) {
            print(node->to_string(0,i++));
        }
        #endif

        return result;
}



static g_ptr<Node> find_scope(g_ptr<Node> start, std::function<bool(g_ptr<Node>)> check) {
    g_ptr<Node> on_scope = start;
    while(true) {
        if(check(on_scope)) {
            return on_scope;
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

static g_ptr<Node> find_scope_name(const std::string& match,g_ptr<Node> start) {
    return find_scope(start,[match](g_ptr<Node> c){
        return c->name == match;
    });
}


static g_ptr<Node> parse_a_node(g_ptr<Node> node,g_ptr<Node> root,g_ptr<Node> left = nullptr) {
    g_ptr<Node> result = make<Node>();
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.root = root;
    ctx.node = node;
    ctx.out = result;
    ctx.left = left;
    if(!t_default_function)
        print("GDSL::parse_a_node t_stage requires a default function!");
    t_functions.getOrDefault(node->type,t_default_function)(ctx);
    return ctx.out;
}

static void parse_sub_nodes(Context& ctx, bool deduplicate = false) {
    if(!ctx.node->children.empty()) {
        g_ptr<Node> last = nullptr;
        for(auto c : ctx.node->children) {
            g_ptr<Node> sub = parse_a_node(c, ctx.root, last);
            if(sub) {
                if(deduplicate && last && sub->left == last) {
                    ctx.out->children.erase(last);
                }
                last = sub;
                ctx.out->children << last;
            } else {
                last = nullptr;
            }
        }
    }
}

static void parse_nodes(g_ptr<Node> root) {
    #if PRINT_ALL
    print("==PARSE NODES PASS (T)==");
    #endif
    g_ptr<Node> last = nullptr;
    for(int i = 0; i < root->opt_sub.size(); i++) {
        auto node = root->opt_sub[i];
        g_ptr<Node> sub = parse_a_node(node,root,last);
        last = sub;
        if(sub) {
            root->opt_sub_2 << last;
        }
    }

    #if PRINT_ALL
    for(auto t : root->opt_sub_2) {
        print(t->to_string());
    }
    #endif
    
    for(auto child_scope : root->children) {
        parse_nodes(child_scope);
    }
}

static void discover_symbol(g_ptr<Node> node,Context& ctx) {
    if(discover_handlers.hasKey(node->type)) {
        auto func = discover_handlers.get(node->type);
        ctx.node = node;
        func(ctx);
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

static void discover_symbols(g_ptr<Node> root) {
    int idx = 0;
    list<g_ptr<Node>> results;
    Context ctx(results, idx);
    ctx.root = root;
    
    for(int i = 0; i < root->opt_sub_2.size(); i++) {
        discover_symbol(root->opt_sub_2[i], ctx);
    }
    
    for(auto child_scope : root->children) {
        discover_symbols(child_scope);
    }
}

static g_ptr<Node> resolve_symbol(g_ptr<Node> node,g_ptr<Node> scope,g_ptr<Frame> frame,g_ptr<Node> result = nullptr) {
    if(!result) result = make<Node>();
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.node = node;
    ctx.root = scope;
    ctx.frame = frame;
    ctx.out = result; 
    if(r_handlers.hasKey(node->type)) {
        result->frame = scope->frame;
        r_handlers.get(node->type)(ctx);
    }
    else {
        print("resolve_symbol::1222 Missing case for t_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return result;
}

static g_ptr<Frame> resolve_symbols(g_ptr<Node> root) {
    #if PRINT_ALL
    print("==RESOLVE SYMBOLS PASS (R)==");
    #endif
    g_ptr<Frame> frame = make<Frame>();
    frame->type = root->type;
    frame->name = root->name;
    root->frame = frame;

    for(int i = 0; i < root->opt_sub_2.size(); i++) {
        auto node = root->opt_sub_2[i];
        g_ptr<Node> rnode = resolve_symbol(node,root,frame);
        if(rnode) frame->nodes << rnode;
    }

    #if PRINT_ALL
    print("==RESOLVED SYMBOLS: ",frame->name,"==");
    for(auto r : frame->nodes) {
        print(r->to_string());
    }
    #endif
    
    for(auto child_scope : root->children) {
        resolve_symbols(child_scope);
    }
    return frame;
}   

static g_ptr<Node> execute_r_node(g_ptr<Node> node,g_ptr<Frame> frame) {
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.node = node;
    ctx.frame = frame;
    if(exec_handlers.hasKey(node->type)) {
        exec_handlers.get(node->type)(ctx);
    }
    else {
        print("execute_r_node::1343 Missing case for r_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return node;
}


static void execute_r_nodes(g_ptr<Frame> frame, g_ptr<Object> context=nullptr) {
    if(!context) { 
        //This is bassicly just to push rows and mark them as usable or not to a thread
        //Want to replace this later with something more efficent, or, make more use of context
        context = frame->context->create();
    }

    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame);
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

static void execute_constructor(g_ptr<Frame> frame) {
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame);
        if(!frame->isActive()) {
            break;
        }
    }
}

map<uint32_t, std::function<std::function<void()>(Context&)>> stream_handlers;

static void stream_r_node(g_ptr<Node> node,g_ptr<Frame> frame) {
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.node = node;
    ctx.frame = frame;
    if(stream_handlers.hasKey(node->type)) {
        std::function<void()> func = stream_handlers.get(node->type)(ctx);
        if(func) {
            frame->stored_functions << func;
        }
    }
    else {
        print("stream_r_node::1380 Missing case for r_type: ",TO_STRING(node->type));
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
        stream_r_node(node,frame);
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
}   


}
