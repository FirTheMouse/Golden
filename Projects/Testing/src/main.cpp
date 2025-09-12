#include <chrono>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#include<core/type.hpp>
#include<util/string_generator.hpp>


double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}


void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<vec4> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
    list<list<double>> t_table;

    for(int c=0;c<f_table.length();c++) {
        t_table.push(list<double>{});
        for(int r=0;r<f_table[c].length();r++) {
            t_table[c].push(0.0);
        }
    }

    for(int m = 0;m<(warm_up?2:1);m++) {
        int C_ITERATIONS = m==0?1:C_ITS;

        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]=0.0;
            }
        }

        for(int i = 0;i<C_ITERATIONS;i++)
        {
            for(int c=0;c<f_table.length();c++) {
                for(int r=0;r<f_table[c].length();r++) {
                    // if(r==0) print("Running: ",s_table[c][r]);
                    double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
                    t_table[c][r]+=time;
                }
            }
        }
        print("-------------------------");
        print(m==0 ? "      ==COLD==" : "       ==WARM==");
        print("-------------------------");
        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]/=C_ITERATIONS;
                print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
            }
            print("-------------------------");
        }
        for(auto v : comps) {
            double factor = t_table[v.x()][v.y()]/t_table[v.z()][v.w()];
            std::string sfs;
            double tolerance = 5.0;
            if (std::abs(factor - 1.0) < tolerance/100.0) {
                sfs = "around the same as ";
            } else if (factor > 1.0) {
                double percentage = (factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% slower than ";
            } else {
                double percentage = (1.0/factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% faster than ";
            }
            print("Factor [",s_table[v.x()][v.y()],"/",s_table[v.z()][v.w()],
            "]: ",factor," (",s_table[v.x()][v.y()]," is ",sfs,s_table[v.z()][v.w()],")");
        }
        print("-------------------------");

    }
}

struct lt {
    uint8_t tt[100];
    std::string name = "DEF";
};

int main() {

    // g_ptr<Type> t = make<Type>();
    // t->type_name = "test";
    // //for(int i=0;i<4;i++) t->add_column(4);
    // //for(int i=0;i<13;i++) t->add_rows(4);
    // lt l;
    // l.name = "test_name";
    // t->push<lt>(l);
    // print("Name: ",t->get<lt>(0).name,"!");

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


    print("==DONE==");
    list<list<std::function<void(int)>>> f_table;
    list<list<std::string>> s_table;
    list<vec4> comps;
    int z = 0;
    f_table << list<std::function<void(int)>>{};
    s_table << list<std::string>{};
    g_ptr<Type> type = make<Type>();
    z = 0;
    type->add_column(4);
    s_table[z] << "push_32"; //0
    f_table[z] << [type](int i){
       byte32_t t;
       type->push<byte32_t>(t);
    };
    s_table[z] << "push_64"; //1
    f_table[z] << [type](int i){
       byte64_t t;
       type->push<byte64_t>(t);
    };
    s_table[z] << "push_128"; //2
    f_table[z] << [type](int i){
       lt t;
       type->push<lt>(t);
    };
    s_table[z] << "get_32"; //3
    f_table[z] << [type](int i){
       volatile byte32_t b = type->get<byte32_t>(0,i);
    };
    s_table[z] << "get_64"; //4
    f_table[z] << [type](int i){
       volatile byte64_t b = type->get<byte64_t>(0,i);
    };
    s_table[z] << "get_128"; //5
    f_table[z] << [type](int i){
        volatile lt b = type->get<lt>(0,i);
    };
    list<lt> ltl;
    s_table[z] << "list:push_128"; //6
    f_table[z] << [type,&ltl](int i){
        lt t;
        ltl.push(t);
    };
    s_table[z] << "list:get_128"; //7
    f_table[z] << [type,&ltl](int i){
        volatile lt b = ltl.get(i);
    };
 
    comps << vec4(0,1 , 0,0);
    comps << vec4(0,4 , 0,3);
    comps << vec4(0,2 , 0,1);
    comps << vec4(0,5 , 0,4);
    comps << vec4(0,2 , 0,6);
    comps << vec4(0,5 , 0,7);

    run_rig(f_table,s_table,comps,true,200,50000);

    return 0;
}

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