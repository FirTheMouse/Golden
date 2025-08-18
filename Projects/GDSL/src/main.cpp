#include<core/helper.hpp>
#include<modules/standard.hpp>

#include <chrono>
#include <iostream>

void benchmark_performance() {
    const size_t ITERATIONS = 10000;
    
    // Setup for C++ direct access
    struct Player {
        int health = 100;
    };
    Player player;
    
    // Setup for Type system  
    g_ptr<Type> type = make<Type>();
    type->note_value(4, "health");
    g_ptr<Object> obj = type->create();
    type->set<int>("health", 100, obj->ID);
    size_t id = obj->ID;
    void* ptr = type->adress_column("health");
    //Direct array access (non-dynamic, not concurrent)
    auto* array_list = (list<uint32_t>*)type->adress_column("health");
    uint32_t* array = &(*array_list)[0]; 
    
    // Benchmark C++ direct access
    auto start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ITERATIONS; i++) {
        player.health+=1;

        //Flexible version
        // volatile int health = player.health;  // Prevent optimization
        // player.health = health + 1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto cpp_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Benchmark Type system (raw API)
    start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ITERATIONS; i++) {
        *(int*)&array[id] += 1;

        //Flexibile version
        // void* health_ptr = type->get_o(ptr, id, 4);
        // volatile int health = *(int*)health_ptr;  // Prevent optimization
        // int new_health = health + 1;
        // type->set_o(ptr, &new_health, id, 4);
    }
    end = std::chrono::high_resolution_clock::now();
    auto type_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Results
    std::cout << "C++ Direct Access: " << cpp_time.count() << " ns (" 
              << cpp_time.count() / ITERATIONS << " ns per operation)\n";
    std::cout << "Type System: " << type_time.count() << " ns (" 
              << type_time.count() / ITERATIONS << " ns per operation)\n";
    std::cout << "Slowdown factor: " << (double)type_time.count() / cpp_time.count() << "x\n";
}

void benchmark_bulk_operations() {
    const size_t ENTITY_COUNT = 10000;
    
    // Setup traditional C++ objects (AoS - Array of Structures)
    struct Player {
        int health = 100;
        float speed = 5.0f;
        bool alive = true;
        char padding[64];  // Simulate real object size
    };
    std::vector<Player> players(ENTITY_COUNT);
    
    // Setup your Type system (SoA - Structure of Arrays)
    g_ptr<Type> type = make<Type>();
    type->note_value(4, "health");
    type->note_value(4, "speed"); 
    type->note_value(1, "alive");
    
    for(size_t i = 0; i < ENTITY_COUNT; i++) {
        auto obj = type->create();
        type->set<int>("health", 100, obj->ID);
        type->set<float>("speed", 5.0f, obj->ID);
        type->set<bool>("alive", true, obj->ID);
    }
    
    // Get DIRECT array pointers (eliminate list indirection)
    auto* health_list = (list<uint32_t>*)type->adress_column("health");
    auto* speed_list = (list<uint32_t>*)type->adress_column("speed");
    auto* alive_list = (list<uint8_t>*)type->adress_column("alive");
    
    uint32_t* health_array = &(*health_list)[0];  // Direct pointer to array data
    uint32_t* speed_array = &(*speed_list)[0];    // Direct pointer to array data
    uint8_t* alive_array = &(*alive_list)[0];     // Direct pointer to array data
    
    // Test 1: Apply damage to all entities
    auto start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ENTITY_COUNT; i++) {
        if(players[i].alive) {
            players[i].health -= 10;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto cpp_health_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ENTITY_COUNT; i++) {
        if(alive_array[i]) {
            health_array[i] -= 10;  // Pure array access
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto type_health_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Test 2: Update all speeds
    start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ENTITY_COUNT; i++) {
        players[i].speed *= 1.1f;
    }
    end = std::chrono::high_resolution_clock::now();
    auto cpp_speed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    start = std::chrono::high_resolution_clock::now();
    for(size_t i = 0; i < ENTITY_COUNT; i++) {
        *(float*)&speed_array[i] *= 1.1f;  // Direct array access with cast
    }
    end = std::chrono::high_resolution_clock::now();
    auto type_speed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Results
    std::cout << "\n=== BULK OPERATIONS BENCHMARK (Direct Arrays) ===\n";
    std::cout << "Health update (C++): " << cpp_health_time.count() << " ns\n";
    std::cout << "Health update (Type): " << type_health_time.count() << " ns\n";
    std::cout << "Health speedup: " << (double)cpp_health_time.count() / type_health_time.count() << "x\n\n";
    
    std::cout << "Speed update (C++): " << cpp_speed_time.count() << " ns\n";
    std::cout << "Speed update (Type): " << type_speed_time.count() << " ns\n";
    std::cout << "Speed speedup: " << (double)cpp_speed_time.count() / type_speed_time.count() << "x\n";
}

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
        benchmark_performance();
        benchmark_bulk_operations();
        // g_ptr<Type> test = make<Type>();
        // test->to_array();
        // bool a = 1;
        // bool b = 0;
        // test->push<bool>(a);
        // test->push<bool>(b);
        // print(test->get<bool>(0));
        // print(test->get<bool>(1));
    }
    else if (mode==2) {
        #define USE_BOOL 0
        #define USE_INT 1
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
        double vector_time_push = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            v_test.push_back(i%2);
            #endif
            #if USE_INT
            v_test_2.push_back(i);
            #endif
        });
        vector_time_push_avg+=vector_time_push;
        vector_time_push_avg/=(c+1);
        double vector_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = v_test[i];
            b=!b;
            #endif
            #if USE_INT
            volatile int a = v_test_2[i];
            value+=a;
            #endif
        });
        vector_time_avg+=vector_time;
        vector_time_avg/=(c+1);

        #if USE_BOOL
        list<bool> l_test;
        #endif
        #if USE_INT
        list<int> l_test_2;
        #endif
        double list_time_push = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            l_test << i%2;
            #endif
            #if USE_INT
            l_test_2 << i;
            #endif
        });
        list_time_push_avg+=list_time_push;
        list_time_push_avg/=(c+1);
        double list_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = l_test[i];
            b=!b;
            #endif
            #if USE_INT
            volatile int a = l_test_2[i];
            value+=a;
            #endif
        });
        list_time_avg+=list_time;
        list_time_avg/=(c+1);
        
        //Lunchbox pattern
        g_ptr<Type> t_test = make<Type>();
        #if USE_BOOL
        t_test->note_value(1,"bool");
        #endif
        #if USE_INT
        t_test->note_value(4,"int");
        #endif
        #if USE_BOOL
        auto* bool_list = (list<uint8_t>*)t_test->adress_column("bool");
        #endif
        #if USE_INT
        auto* int_list = (list<uint32_t>*)t_test->adress_column("int");
        #endif
        list<g_ptr<Object>> t_objs;
        double type_time_push = time_function(ITERATIONS,[&](int i){
            //auto object = t_test->create();
            #if USE_BOOL
                t_test->byte4_columns[0].push(uint8_t{});
                bool b = false;
                t_test->set(bool_list,&b,i,1);
            #endif
            #if USE_INT
                t_test->byte4_columns[0].push(uint16_t{});
                int j = 4;
                t_test->set(int_list,&j,i,4);
            #endif
            //t_objs << object;
        });
        type_time_push_avg+=type_time_push;
        type_time_push_avg/=(c+1);
        double type_time = time_function(ITERATIONS,[&](int i){
            #if USE_BOOL
            volatile bool b = *(bool*)&bool_list[i]; //Lunchbox pattern
            b=!b;
            #endif
            #if USE_INT
            volatile int a = *(int*)&int_list[i]; //Lunchbox pattern
            value+=a;
            #endif
        });
        type_time_avg+=type_time;
        type_time_avg/=(c+1);

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
        // type_time_push_avg/=(c+1);
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
        // type_time_avg/=(c+1);

        //Map pattern
        // g_ptr<Type> t_test = make<Type>();
        // #if USE_BOOL
        // t_test->note_value(1,"bool");
        // #endif
        // #if USE_INT
        // t_test->note_value(4,"int");
        // #endif
        // list<g_ptr<Object>> t_objs;
        // double type_time_push = time_function(ITERATIONS,[&](int i){
        //     auto object = t_test->create();
        //     #if USE_BOOL
        //         t_test->set<bool>("bool",1,object->ID);
        //     #endif
        //     #if USE_INT
        //         t_test->set<int>("int",4,object->ID);
        //     #endif
        //     t_objs << object;
        // });
        // type_time_push_avg+=type_time_push;
        // type_time_push_avg/=(c+1);
        // double type_time = time_function(ITERATIONS,[&](int i){
        //     #if USE_BOOL
        //     volatile bool b = t_test->get<bool>("bool",t_objs[i]->ID); //Map pattern
        //     b=!b;
        //     #endif
        //     #if USE_INT
        //     volatile int a = t_test->get<int>("int",t_objs[i]->ID); //Map pattern
        //     a=(a+1);
        //     #endif
        // });
        // type_time_avg+=type_time;
        // type_time_avg/=(c+1);
    }

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

