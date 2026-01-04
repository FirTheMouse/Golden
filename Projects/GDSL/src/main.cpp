#include<modules/standard.hpp>
#include<util/engine_util.hpp>
#include<util/string_generator.hpp>
#include<util/logger.hpp>

#include <chrono>
#include <iostream>

#define MAIN_TEST 0

int main() {

    int test_ammount = 15;

    #if MAIN_TEST
    //Adding: this is because I haven't made it so we can loop construct in GDSL yet
    editTextFile(root()+"/Projects/GDSL/src/golden.gld",[test_ammount](std::string& text){
        size_t startpos = text.find("#START")+6;
        std::string to_add;
        list<std::string> added;
        for(int i=0;i<test_ammount;i++) {
            int repeats = 0;
            std::string n_name = sgen::randsgen(sgen::RANDOM);
            while(added.has(n_name)&&repeats<10) {
                n_name = sgen::randsgen(sgen::RANDOM);
                repeats++;
            } 
            if(repeats>=9) {
                print("GDSL::25 Running out of names to add at the top of main.cpp for GDSL");
            } else {
                to_add.append("\nnum "+n_name+";");
            }
        }
        text.insert(startpos,to_add);
    });
    #endif


        
    // type num {
    //     int a;
    // }

    // #START
    // #END

    // int n_len = _size(num);

    // int i = 0;
    // while(i<n_len) {
    //     num[i].a = i;
    //     i = i+1;
    // }

    // #i = 0;
    // #while(i<n_len) {
    // #   print(num[i].a);
    // #    i = i+1;
    // #}

    int mode = 0; //0 == Compile and run, 1 == benchmark // 2 == testing
    if(mode==0) {
        base_module::initialize();
        literals_module::initialize();
        array_module::initialize();
        variables_module::initialize();
        opperator_module::initialize();
        control_module::initialize();
        type_module::initialize();
        data_module::initialize();
        paren_module::initialize();
        functions_module::initialize();
        std::string code = readFile(root()+"/Projects/GDSL/src/golden.gld");
        list<g_ptr<Token>> tokens = tokenize(code);
        list<g_ptr<a_node>> nodes = parse_tokens(tokens);
        balance_precedence(nodes);
        g_ptr<s_node> root = parse_scope(nodes);
        parse_nodes(root);
        discover_symbols(root);
        g_ptr<Frame> frame = resolve_symbols(root);
        execute_r_nodes(frame); 
        //Streaming
        // stream_r_nodes(frame);
        // print("==EXECUTING STREAM==");
        // execute_stream(frame);

        // list<list<std::function<void(int)>>> f_table;
        // list<list<std::string>> s_table;
        // list<vec4> comps;
        // int z = 0;
        // f_table << list<std::function<void(int)>>{};
        // s_table << list<std::string>{};
        // z = 0;

        // s_table[z] << "execute_r_nodes"; //0
        // f_table[z] << [frame](int i){
        //     execute_r_nodes(frame); 
        // };

        // s_table[z] << "stream_r_nodes"; //1
        // f_table[z] << [frame](int i){
        //     if(frame->stored_functions.length()==0) {
        //         stream_r_nodes(frame);
        //     }
        // };
        // s_table[z] << "execute_stream"; //2
        // f_table[z] << [frame](int i){
        //     execute_stream(frame); 
        // };


        // s_table[z] << "CPP"; //3
        // f_table[z] << [](int i){
        // struct num {
        //     int value = 0;
        // };
        //     num nums[50];
        //     for(int i=1;i<50;i=(i+1)) {
        //         nums[i].value = (nums[i-1].value+2);
        //     }
        //     print(nums[20].value);
        // };

        // comps << vec4(0,0 , 0,3);
        // comps << vec4(0,2 , 0,3);
        // run_rig(f_table,s_table,comps,true,1,500);
        
    }
    else if (mode==1) {

        base_module::initialize();
        literals_module::initialize();
        array_module::initialize();
        variables_module::initialize();
        opperator_module::initialize();
        control_module::initialize();
        type_module::initialize();
        data_module::initialize();
        paren_module::initialize();
        functions_module::initialize();
        std::string code = readFile(root()+"/Projects/GDSL/src/golden.gld");
        list<g_ptr<Token>> tokens = tokenize(code);
        list<g_ptr<a_node>> nodes = parse_tokens(tokens);
        balance_precedence(nodes);
        g_ptr<s_node> root = parse_scope(nodes);
        parse_nodes(root);
        discover_symbols(root);
        g_ptr<Frame> frame = resolve_symbols(root);

        Log::rig r;
        // r.add_process("clean",[&](int i){
           
        // });

        r.add_process("GDSL_Stream",[&](int i){
            if(frame->stored_functions.length()==0) {
                stream_r_nodes(frame);
            }
        });

        r.add_process("GDSL",[&](int i){
            execute_stream(frame); 
        });

        // r.add_process("GDSL",[&](int i){
        //     execute_r_nodes(frame);  
        // });
        r.add_process("CPP",[&](int v){
            struct num {
                int a;
            };
            std::vector<num> nums;
            for(int i=0;i<test_ammount;i++) {
                nums.emplace_back(num());
            }
            int n_len = nums.size();
            int i = 0;
            while(i<n_len) {
                nums[i].a = i;
                i = i+1;
            }
        });

        r.add_comparison("GDSL","CPP");
        r.run(1000,true,1);
    }
    else if (mode==2) {
        g_ptr<Type> test = make<Type>();
        test->note_value("test",16);
        auto obj = test->create();
        test->get("test",obj->ID,16);
    }

    print("==DONE==");

    #if MAIN_TEST
    //Removing to clean up what was added
    editTextFile(root()+"/Projects/GDSL/src/golden.gld",[](std::string& text){
        const std::string startTag = "#START";
        const std::string endTag   = "#END";
    
        size_t start = text.find(startTag);
        if (start == std::string::npos) return;
    
        start += startTag.size();
    
        while (start < text.size() && (text[start] == '\n' || text[start] == '\r' || text[start] == ' ' || text[start] == '\t'))
            ++start;
    
        size_t end = text.find(endTag, start); 
        if (end == std::string::npos) return;
    
        text.replace(start, end - start, "");
    });
    #endif

    return 0;
}

//#include<util/engine_util.hpp>
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

// g_ptr<Type> data
//         template<typename T = std::string>
//         T get(const std::string& label)
//         { return data->get<T>(label, 0); }

//         template<typename T = std::string>
//         void add(const std::string& label, T info)
//         { 
//             if (!data->notes.hasKey(label)) {
//                 data->note_value(sizeof(T), label);
//             }
//             data->set<T>(label, info, 0);
//         }

//         template<typename T = std::string>
//         void set(const std::string& label, T info)
//         { 
//             if (!data->notes.hasKey(label)) {
//                 data->note_value(sizeof(T), label);
//             }
//             data->set<T>(label, info, 0);
//         }

//         bool has(const std::string& label)
//         { return data->notes.hasKey(label); }

//         /// @brief  starts false, switches on each call
//         bool toggle(const std::string& label) {
//             if (!has(label)) set<bool>(label, true);
//             bool toReturn = !get<bool>(label);
//             set<bool>(label, toReturn);
//             return toReturn;
//         }

//         bool check(const std::string& label) {
//             if (!has(label)) return false;
//             try {
//                 return get<bool>(label);
//             }
//             catch(std::exception e) {
//                 print("Object::check Attempted to check a non-bool");
//                 return false;
//             }
//         }

//         void flagOn(const std::string& label) { set<bool>(label, true); }
//         void flagOff(const std::string& label) { set<bool>(label, false); }

//         template<typename T = int>
//         T inc(const std::string& label, T by)
//         { 
//             if (has(label)) {
//                 T current = get<T>(label);
//                 set<T>(label, current + by);
//             } else {
//                 add<T>(label, by);
//             }
//             return get<T>(label);
//         }

