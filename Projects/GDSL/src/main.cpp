#include<core/helper.hpp>
#include<Golden.hpp>

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

int main() {

    int mode = 0; //0 == Compile and run, 1 == benchmark

    if(mode==0) {
        reg_b_types();
        init_t_keys();
        std::string code = readFile("../Projects/GDSL/src/golden.gld");
        list<g_ptr<Token>> tokens = tokenize(code);
        reg_a_types();
        reg_s_types();
        reg_t_types();
        init_type_key_to_type();
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
        g_ptr<Frame> frame = resolve_symbols(root);
        exec_function_blob();
        execute_r_nodes(frame);
    }
    else if (mode==1) {
        benchmark_performance();
        benchmark_bulk_operations();
    }

    print("==DONE==");
    return 0;
}
