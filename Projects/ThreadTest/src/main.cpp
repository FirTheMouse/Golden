#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <iostream>
#include <atomic>
#include <mutex>
#include <test_q_list.hpp>

template<typename... Args>
void print(Args&&... args) {
  (std::cout << ... << args) << std::endl;
}

class Timer {
public:
    Timer() : start_time(std::chrono::high_resolution_clock::now()) {}
    
    void reset() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return duration.count() / 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_time;
};

void benchmark_raw_ops_per_second() {
    print("=== Raw q_list Operations Per Second ===");
    
    q_list<int> test_list;
    const int operations = 1000000;
    
    // Pure insertion test
    Timer timer;
    for (int i = 0; i < operations; ++i) {
        test_list.push(i);
    }
    double insert_time = timer.elapsed_ms();
    
    print("Insertions: ", operations, " ops in ", insert_time, "ms");
    print("Insert rate: ", operations / (insert_time / 1000.0), " ops/sec");
    
    // Pure read test
    timer.reset();
    volatile int sum = 0; // volatile to prevent optimization
    for (int i = 0; i < operations; ++i) {
        sum += test_list.get(i % test_list.length());
    }
    double read_time = timer.elapsed_ms();
    
    print("Reads: ", operations, " ops in ", read_time, "ms");
    print("Read rate: ", operations / (read_time / 1000.0), " ops/sec");
    
    // Mixed operations (90% read, 10% write)
    timer.reset();
    for (int i = 0; i < operations; ++i) {
        if (i % 10 == 0) {
            test_list.push(i);
        } else {
            volatile int val = test_list.get(i % test_list.length());
        }
    }
    double mixed_time = timer.elapsed_ms();
    
    print("Mixed (90% read): ", operations, " ops in ", mixed_time, "ms");
    print("Mixed rate: ", operations / (mixed_time / 1000.0), " ops/sec");
    
    // Removal stress test
    timer.reset();
    int removals = 0;
    while (test_list.length() > 1000) {
        test_list.remove(test_list.length() / 2);
        removals++;
    }
    double remove_time = timer.elapsed_ms();
    
    print("Removals: ", removals, " ops in ", remove_time, "ms");
    print("Remove rate: ", removals / (remove_time / 1000.0), " ops/sec");
    print("");
}

// Threading test globals
std::atomic<bool> stress_running{true};
std::atomic<long> total_reads{0};
std::atomic<long> total_writes{0};
std::atomic<long> total_removes{0};
std::atomic<long> errors{0};

void heavy_reader(q_list<int>& list, int reader_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (stress_running.load()) {
        try {
            size_t len = list.length();
            if (len > 0) {
                std::uniform_int_distribution<> dis(0, len - 1);
                size_t idx = dis(gen);
                volatile int val = list.get(idx);
                total_reads.fetch_add(1);
            }
        } catch (...) {
            errors.fetch_add(1);
        }
    }
}

void heavy_writer(q_list<int>& list, int writer_id) {
    int counter = 0;
    while (stress_running.load()) {
        try {
            list.push(writer_id * 1000000 + counter++);
            total_writes.fetch_add(1);
            
            // Occasionally sleep to let readers work
            if (counter % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        } catch (...) {
            errors.fetch_add(1);
        }
    }
}

void heavy_remover(q_list<int>& list) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (stress_running.load()) {
        try {
            size_t len = list.length();
            if (len > 1000) { // Keep some elements
                std::uniform_int_distribution<> dis(0, len - 1);
                size_t idx = dis(gen);
                list.remove(idx);
                total_removes.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        } catch (...) {
            errors.fetch_add(1);
        }
    }
}

void benchmark_threading_stress() {
    print("=== q_list Threading Stress Test ===");
    
    q_list<int> stress_list;
    
    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        stress_list.push(i);
    }
    
    // Reset counters
    total_reads.store(0);
    total_writes.store(0);
    total_removes.store(0);
    errors.store(0);
    stress_running.store(true);
    
    Timer timer;
    
    // Start multiple readers (simulating game systems reading data)
    std::vector<std::thread> readers;
    for (int i = 0; i < 6; ++i) {
        readers.emplace_back(heavy_reader, std::ref(stress_list), i);
    }
    
    // Start multiple writers (simulating spawning objects)
    std::vector<std::thread> writers;
    for (int i = 0; i < 2; ++i) {
        writers.emplace_back(heavy_writer, std::ref(stress_list), i);
    }
    
    // Start remover (simulating cleanup)
    std::thread remover(heavy_remover, std::ref(stress_list));
    
    // Let it run for a few seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    stress_running.store(false);
    
    for (auto& r : readers) r.join();
    for (auto& w : writers) w.join();
    remover.join();
    
    double total_time = timer.elapsed_ms();
    long total_ops = total_reads.load() + total_writes.load() + total_removes.load();
    
    print("Stress test results (3 seconds, 9 threads):");
    print("Total reads: ", total_reads.load());
    print("Total writes: ", total_writes.load());
    print("Total removes: ", total_removes.load());
    print("Total operations: ", total_ops);
    print("Errors: ", errors.load());
    print("Final list size: ", stress_list.length());
    print("Overall ops/sec: ", total_ops / (total_time / 1000.0));
    print("Read ops/sec: ", total_reads.load() / (total_time / 1000.0));
    print("");
}

// std::vector + mutex comparison
std::mutex vec_mutex;
std::vector<int> stress_vector;

void mutex_reader(int reader_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (stress_running.load()) {
        try {
            std::lock_guard<std::mutex> lock(vec_mutex);
            if (!stress_vector.empty()) {
                std::uniform_int_distribution<> dis(0, stress_vector.size() - 1);
                size_t idx = dis(gen);
                volatile int val = stress_vector[idx];
                total_reads.fetch_add(1);
            }
        } catch (...) {
            errors.fetch_add(1);
        }
    }
}

void mutex_writer(int writer_id) {
    int counter = 0;
    while (stress_running.load()) {
        try {
            std::lock_guard<std::mutex> lock(vec_mutex);
            stress_vector.push_back(writer_id * 1000000 + counter++);
            total_writes.fetch_add(1);
            
            if (counter % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        } catch (...) {
            errors.fetch_add(1);
        }
    }
}

void mutex_remover() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (stress_running.load()) {
        try {
            std::lock_guard<std::mutex> lock(vec_mutex);
            if (stress_vector.size() > 1000) {
                std::uniform_int_distribution<> dis(0, stress_vector.size() - 1);
                size_t idx = dis(gen);
                stress_vector.erase(stress_vector.begin() + idx);
                total_removes.fetch_add(1);
            }
        } catch (...) {
            errors.fetch_add(1);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void benchmark_mutex_comparison() {
    print("=== std::vector + mutex comparison ===");
    
    stress_vector.clear();
    for (int i = 0; i < 1000; ++i) {
        stress_vector.push_back(i);
    }
    
    // Reset counters
    total_reads.store(0);
    total_writes.store(0);
    total_removes.store(0);
    errors.store(0);
    stress_running.store(true);
    
    Timer timer;
    
    std::vector<std::thread> readers;
    for (int i = 0; i < 6; ++i) {
        readers.emplace_back(mutex_reader, i);
    }
    
    std::vector<std::thread> writers;
    for (int i = 0; i < 2; ++i) {
        writers.emplace_back(mutex_writer, i);
    }
    
    std::thread remover(mutex_remover);
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    stress_running.store(false);
    
    for (auto& r : readers) r.join();
    for (auto& w : writers) w.join();
    remover.join();
    
    double total_time = timer.elapsed_ms();
    long total_ops = total_reads.load() + total_writes.load() + total_removes.load();
    
    print("Mutex stress test results (3 seconds, 9 threads):");
    print("Total reads: ", total_reads.load());
    print("Total writes: ", total_writes.load());
    print("Total removes: ", total_removes.load());
    print("Total operations: ", total_ops);
    print("Errors: ", errors.load());
    print("Final vector size: ", stress_vector.size());
    print("Overall ops/sec: ", total_ops / (total_time / 1000.0));
    print("Read ops/sec: ", total_reads.load() / (total_time / 1000.0));
    print("");
}

void benchmark_contention_scenarios() {
    print("=== High Contention Scenarios ===");
    
    // Test 1: Heavy removal during reads
    print("Test 1: Heavy removal contention");
    q_list<int> contention_list;
    for (int i = 0; i < 10000; ++i) {
        contention_list.push(i);
    }
    
    total_reads.store(0);
    total_removes.store(0);
    errors.store(0);
    stress_running.store(true);
    
    Timer timer;
    
    // 8 readers hammering the list
    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&contention_list]() {
            while (stress_running.load()) {
                try {
                    size_t len = contention_list.length();
                    if (len > 0) {
                        for (size_t i = 0; i < std::min(len, size_t(10)); ++i) {
                            volatile int val = contention_list.get(i);
                        }
                        total_reads.fetch_add(10);
                    }
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }
    
    // 2 threads constantly removing
    std::vector<std::thread> removers;
    for (int i = 0; i < 2; ++i) {
        removers.emplace_back([&contention_list]() {
            while (stress_running.load()) {
                try {
                    if (contention_list.length() > 100) {
                        contention_list.remove(contention_list.length() / 2);
                        total_removes.fetch_add(1);
                    }
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stress_running.store(false);
    
    for (auto& r : readers) r.join();
    for (auto& rem : removers) rem.join();
    
    double test_time = timer.elapsed_ms();
    
    print("High contention results:");
    print("Reads: ", total_reads.load(), " (", total_reads.load() / (test_time / 1000.0), " reads/sec)");
    print("Removes: ", total_removes.load());
    print("Errors: ", errors.load());
    print("Surviving elements: ", contention_list.length());
    print("");
}

int main() {
    print("q_list Performance Stress Tests");
    print("===============================");
    print("");
    
    benchmark_raw_ops_per_second();
    benchmark_threading_stress();
    benchmark_mutex_comparison();
    benchmark_contention_scenarios();
    
    print("All stress tests complete!");
    return 0;
}