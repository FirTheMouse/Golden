#include<core/helper.hpp>
#include<modules/standard.hpp>

#include <chrono>
#include <iostream>

double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}

int main() {

    int mode = 2; //0 == Compile and run, 1 == benchmark // 2 == testing

    if(mode==0) {
        base_module::initialize();
        variables_module::initialize();
        literals_module::initialize();
        opperator_module::initialize();
        property_module::initialize();
        reg_b_types();
        init_t_keys();
        std::string code = readFile("../Projects/GDSL/src/golden.gld");
        list<g_ptr<Token>> tokens = tokenize(code);
        reg_a_types();
        reg_s_types();
        reg_t_types();
        a_function_blob();
        list<g_ptr<a_node>> nodes = parse_tokens(tokens);
        balance_precedence(nodes);
        scope_function_blob();
        g_ptr<s_node> root = parse_scope(nodes);
        t_function_blob_top();
        t_function_blob_bottom();
        parse_nodes(root);
        reg_r_types();
        discover_function_blob();
        discover_symbols(root);
        r_function_blob();
        exec_function_blob();
        g_ptr<Frame> frame = resolve_symbols(root);
        execute_r_nodes(frame); 
    }
    else if (mode==1) {
        // benchmark_performance();
        // benchmark_bulk_operations();
        g_ptr<Type> test = make<Type>();
        test->note_value("test",16);
        auto obj = test->create();
        test->get("test",obj->ID,16);
    }
    else if (mode==2) {
        #define USE_BOOL 0
        #define USE_INT 1
        #define USE_MULTI 1
        for(int m = 0;m<2;m++) {

        double vector_time_push_avg = 0;
        double vector_time_avg = 0;
        double list_time_push_avg = 0;
        double list_time_avg = 0;
        double type_time_push_avg = 0;
        double type_time_avg = 0;

        int ITERATIONS = 1000;
        int C_ITERATIONS = m==0?1:500;

        for(int c = 0;c<C_ITERATIONS;c++)
        {
        int value = 0;
        #if USE_BOOL
        std::vector<bool> v_test;
        #endif
        #if USE_INT
        std::vector<int> v_test_2;
        #endif
        #if USE_MULTI
        std::vector<float> v_test_float;
        std::vector<float> v_test_float_2;
        std::vector<float> v_test_float_3;
        std::vector<std::string> v_test_string;
        std::vector<std::string> v_test_string_2;
        #endif
        double vector_time_push = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            v_test.push_back(i%2);
            #endif
            #if USE_INT
            v_test_2.push_back(i);
            #endif
            #if USE_MULTI
            v_test_float.push_back(0.3f);
            v_test_float_2.push_back(8.4f);
            v_test_float_3.push_back(132.18f);
            v_test_string.push_back("Hello");
            v_test_string_2.push_back("World");
            #endif
        });
        vector_time_push_avg+=vector_time_push;

        double vector_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = v_test[i];
            b=!b;
            #endif
            #if USE_INT
            volatile int a = v_test_2[i];
            //value+=a;
            #endif
            #if USE_MULTI
            volatile float f = v_test_float[i];
            //float x = f*2.0f;
            //v_test_float_2[i] = x;
            volatile float f2 = v_test_float_2[i];
            //v_test_float_3[i] = f2;
            volatile float f3 = v_test_float_3[i];
            // volatile std::string s = v_test_string[i];
            // volatile std::string s2 = v_test_string_2[i];
            #endif
        });
        vector_time_avg+=vector_time;


        #if USE_BOOL
        list<bool> l_test;
        #endif
        #if USE_INT
        list<int> l_test_2;
        #endif
        #if USE_MULTI
        list<float> l_test_float;
        list<float> l_test_float_2;
        list<float> l_test_float_3;
        list<std::string> l_test_string;
        list<std::string> l_test_string_2;
        #endif

        double list_time_push = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            l_test << i%2;
            #endif
            #if USE_INT
            l_test_2 << i;
            #endif
            #if USE_MULTI
            l_test_float << 0.3;
            l_test_float_2 << 8.4;
            l_test_float_3 << 132.18;
            l_test_string << "Hello";
            l_test_string_2 << "World";
            #endif
        });
        list_time_push_avg+=list_time_push;

        double list_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = l_test[i];
            b=!b;
            #endif
            #if USE_INT
            volatile int a = l_test_2[i];
            //value+=a;
            #endif
            #if USE_MULTI
            float f = l_test_float[i];
            //float x = f*2.0f;
            //l_test_float_2[i] = x;
            volatile float f2 = l_test_float_2[i];
            //l_test_float_3[i] = f2;
            volatile float f3 = l_test_float_3[i];
            // volatile std::string s = l_test_string[i];
            // volatile std::string s2 = l_test_string_2[i];
            #endif
        });
        list_time_avg+=list_time;
        
        //Lunchbox pattern
        g_ptr<Type> t_test = make<Type>();
        #if USE_BOOL
        t_test->note_value("bool",1);
        #endif
        #if USE_INT
        t_test->note_value("int",4);
        #endif
        #if USE_MULTI
        t_test->note_value("float1",4);
        t_test->note_value("float2",4);
        t_test->note_value("float3",4);
        t_test->note_value("string1",24);
        t_test->note_value("string2",24);
        #endif
        #if USE_BOOL
        auto* bool_list = (list<uint8_t>*)t_test->adress_column("bool");
        #endif
        #if USE_INT
        auto* int_list = (list<uint32_t>*)t_test->adress_column("int");
        #endif
        #if USE_MULTI
        auto* float_list = (list<uint32_t>*)t_test->adress_column("float1");
        auto* float_list_2 = (list<uint32_t>*)t_test->adress_column("float2");
        auto* float_list_3 = (list<uint32_t>*)t_test->adress_column("float3");
        auto* string_list = (list<byte24_t>*)t_test->adress_column("string1");
        auto* string_list_2 = (list<byte24_t>*)t_test->adress_column("string2");
        #endif
        //list<g_ptr<Object>> t_objs;
        double type_time_push = time_function(ITERATIONS,[&](int i){
            //auto object = t_test->create();
            #if USE_BOOL
                t_test->byte4_columns[0].push(uint8_t{});
                bool b = false;
                t_test->set(bool_list,&b,i,1);
            #endif
            #if USE_INT
                t_test->add_rows(4);
                int j = 4;
                t_test->set(int_list,&j,i,4);
            #endif
            #if USE_MULTI
                #if !USE_INT
                    t_test->add_rows(4);
                #endif
                float a = 0.3f;
                t_test->set(float_list,&a,i,4);
                float a2 = 8.4f;
                t_test->set(float_list_2,&a2,i,4);
                float a3 = 132.18f;
                t_test->set(float_list_3,&a3,i,4);
                std::string s1 = "Hello";
                t_test->byte24_columns[0].push(byte24_t{});
                t_test->set(string_list,&s1,i,24);
                std::string s2 = "World";
                t_test->byte24_columns[1].push(byte24_t{});
                t_test->set(string_list,&s2,i,24);
            #endif
            //t_objs << object;
        });
        type_time_push_avg+=type_time_push;
        double type_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = (int)(*bool_list)[i]; //Lunchbox pattern
            b=!b;
            #endif
            #if USE_INT
            volatile int a = (int)(*int_list)[i]; //Lunchbox pattern
            //value+=a;
            #endif
            #if USE_MULTI
            float f = (float)(*float_list)[i];
            //float x = f*2.0f;
            //t_test->set(float_list_2,&x,i,4);
            //memcpy(&(*(list<uint32_t>*)float_list_2)[i], &x, 4);
            float f2 = (float)(*float_list_2)[i];
            //t_test->set(float_list_3,&f2,i,4);
            //memcpy(&(*(list<uint32_t>*)float_list_3)[i], &f2, 4);
            volatile float f3 = (float)(*float_list_3)[i];
            // volatile std::string s = (std::string)(*string_list)[i];
            // volatile std::string s2 = (std::string)(*string_list_2)[i];
            #endif
        });
        type_time_avg+=type_time;

        //Array pattern
        // g_ptr<Type> t_test = make<Type>();
        // t_test->to_array();
        // double type_time_push = time_function(ITERATIONS,[&](int i){
        //     #if USE_BOOL
        //         t_test->push<bool>(1);
        //     #endif
        //     #if USE_INT
        //         t_test->push<int>(4);
        //     #endif
        // });
        // type_time_push_avg+=type_time_push;
        // double type_time = time_function(ITERATIONS,[&](int i){
        //     #if USE_BOOL
        //     volatile bool b = t_test->get<bool>(i); //Array pattern
        //     b=!b;
        //     #endif
        //     #if USE_INT
        //     volatile int a = t_test->get<int>(i); //Array pattern
        //     a=(a+1);
        //     #endif
        // });
        // type_time_avg+=type_time;

       // Map pattern
        // g_ptr<Type> t_test = make<Type>();
        // #if USE_BOOL
        // t_test->note_value(1,"bool");
        // #endif
        // #if USE_INT
        // //t_test->note_value(4,"int");
        // #endif
        // list<g_ptr<Object>> t_objs;

        // double type_time_push = time_function(ITERATIONS,[&](int i){
        //     //auto object = t_test->create();
        //     #if USE_BOOL
        //         t_test->set<bool>("bool",1,object->ID);
        //     #endif
        //     #if USE_INT
        //        // t_test->set<int>("int",4,object->ID);
        //        //t_test->add<int>(std::to_string(i),4); //Data pattern
        //        t_test->push<int>(4);
        //     #endif
        //     //t_objs << object;
        // });
        // type_time_push_avg+=type_time_push;
        // t_test->add_row(4);
        // double type_time = time_function(ITERATIONS,[&](int i){
        //     #if USE_BOOL
        //     volatile bool b = t_test->get<bool>("bool",t_objs[i]->ID); //Map pattern
        //     b=!b;
        //     #endif
        //     #if USE_INT
        //     //volatile int a = t_test->get<int>("int",t_objs[i]->ID); //Map pattern
        //     //a=(a+1);
        //     // volatile int a = t_test->get<int>(std::to_string(i)); //Data pattern
        //     volatile int a = t_test->get<int>(i); //Data pattern
        //     //t_test->set<int>(std::to_string(i),8);
        //     #endif
        // });
        // type_time_avg+=type_time;
    }
    vector_time_push_avg/=C_ITERATIONS;
    list_time_push_avg/=C_ITERATIONS;
    type_time_push_avg/=C_ITERATIONS;
    vector_time_avg/=C_ITERATIONS;
    list_time_avg/=C_ITERATIONS;
    type_time_avg/=C_ITERATIONS;

    print(m==0 ? "==COLD==" : "==WARM==");
    print(" VECTOR_PUSH: ",vector_time_push_avg," ns (" ,vector_time_push_avg / ITERATIONS," ns per operation)");
    print(" LIST_PUSH: ",list_time_push_avg," ns (" ,list_time_push_avg / ITERATIONS," ns per operation)");
    print(" TYPE_PUSH: ",type_time_push_avg," ns (" ,type_time_push_avg / ITERATIONS," ns per operation)");
    print(" Slowdown L/V factor push: ",list_time_push_avg / vector_time_push_avg,"x");
    print(" Slowdown T/L factor push: ",type_time_push_avg / list_time_push_avg,"x");
    print(" Slowdown T/V factor push: ",type_time_push_avg / vector_time_push_avg,"x\n");
    
    print(" VECTOR: ",vector_time_avg," ns (" ,vector_time_avg / ITERATIONS," ns per operation)");
    print(" LIST: ",list_time_avg," ns (" ,list_time_avg / ITERATIONS," ns per operation)");
    print(" TYPE: ",type_time_avg," ns (" ,type_time_avg / ITERATIONS," ns per operation)");
    print(" Slowdown L/V factor: ",list_time_avg / vector_time_avg,"x");
    print(" Slowdown T/L factor: ",type_time_avg / list_time_avg,"x");
    print(" Slowdown T/V factor: ",type_time_avg / vector_time_avg,"x\n");
    }
    }

    print("==DONE==");
    return 0;
}


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


// g_ptr<Type> t_test = make<Type>();
//         t_test->to_array();
//         double type_time_push = time_function(ITERATIONS,[&](int i){
//             #if USE_BOOL
//             t_test->push<bool>(i%2);
//             #endif
//             #if USE_INT
//             t_test->push<int>(i);
//             #endif
//         });
//         type_time_push_avg+=type_time_push;
//         type_time_push_avg/=(c+1);
//         double type_time = time_function(ITERATIONS,[&](int i){
//             #if USE_BOOL
//             volatile bool b = t_test->get<bool>(i);
//             b=!b;
//             #endif
//             #if USE_INT
//             volatile int a = t_test->get<int>(i);
//             a=(a+1);
//             #endif
//         });
//         type_time_avg+=type_time;
//         type_time_avg/=(c+1);


// #if USE_BOOL
// list<bool> l_test;
// #endif
// #if USE_INT
// list<int> l_test_2;
// #endif
// list<g_ptr<Object>> objs;
// double list_time_push = time_function(ITERATIONS,[&](int i){
//     auto object = make<Object>();
//     #if USE_BOOL
//         object->set<bool>("bool",1);
//     #endif
//     #if USE_INT
//         object->set<int>("int",4);
//     #endif
//     objs << object;
// });
// list_time_push_avg+=list_time_push;
// list_time_push_avg/=(c+1);
// double list_time = time_function(ITERATIONS,[&](int i){
//     #if USE_BOOL
//     volatile bool b = objs[i]->get<bool>("bool");
//     b=!b;
//     #endif
//     #if USE_INT
//     volatile int a = objs[i]->get<int>("int");
//     a=(a+1);
//     #endif
// });
// list_time_avg+=list_time;
// list_time_avg/=(c+1);


    // print("VECTOR_PUSH: ",vector_time_push," ns (" ,vector_time_push / ITERATIONS," ns per operation)");
    // print("LIST_PUSH: ",list_time_push," ns (" ,list_time_push / ITERATIONS," ns per operation)");
    // print("TYPE_PUSH: ",type_time_push," ns (" ,type_time_push / ITERATIONS," ns per operation)");
    // print("Slowdown L/V factor push: ",list_time_push / vector_time_push,"x");
    // print("Slowdown T/L factor push: ",type_time_push / list_time_push,"x\n");
    
    // print("VECTOR: ",vector_time," ns (" ,vector_time / ITERATIONS," ns per operation)");
    // print("LIST: ",list_time," ns (" ,list_time / ITERATIONS," ns per operation)");
    // print("TYPE: ",type_time," ns (" ,type_time / ITERATIONS," ns per operation)");
    // print("Slowdown L/V factor: ",list_time / vector_time,"x");
    // print("Slowdown T/L factor: ",type_time / list_time,"x\n");

    //    const int ITERATIONS = 1000; // Run multiple times for statistical validity

    //     // C++ baseline
    //     auto start = std::chrono::high_resolution_clock::now();
    //         struct person {
    //             std::string name = "noname";
    //         };
    //         person joe;
    //         joe.name = "Joe";
    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto cpp_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    //     start = std::chrono::high_resolution_clock::now();
    //         execute_r_nodes(frame); 
    //     end = std::chrono::high_resolution_clock::now();
    //     auto golden_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    //     print("C++: ", cpp_time.count()," ns per execution\n");
    //     print("Golden: ", golden_time.count()," ns per execution\n");
    //     print("Slowdown: ", (double)golden_time.count() / cpp_time.count(), "x\n");

