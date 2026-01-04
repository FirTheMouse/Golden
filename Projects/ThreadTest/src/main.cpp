#include<core/gpuType.hpp>
#include<test_gpu_computer.hpp>


using namespace Golden;

void test_gpu_type() {
    auto type = make<gpuType>();
    
    print("=== GPU Type Sanity Check ===");
    
    // Test 1: Verify allocator exists
    if (!type->allocator) {
        print("FAIL: No GPU allocator created!");
        return;
    }
    print("✓ Allocator created");
    
    // Test 2: Add data to CPU columns
    print("\nAdding 100 floats to CPU column...");
    for (int i = 0; i < 100; i++) {
        type->push<float>(float(i), 0);
    }
    
    size_t num_floats = type->row_length(0, 4);
    print("✓ CPU column has ", num_floats, " floats");
    
    // Test 3: Allocate GPU buffer
    print("\nAllocating GPU buffer for ", num_floats * sizeof(float), " bytes...");
    void* gpu_buffer = type->allocator->allocate(num_floats * sizeof(float));
    
    if (!gpu_buffer) {
        print("FAIL: GPU allocation returned nullptr!");
        return;
    }
    print("✓ GPU buffer allocated at address: ", gpu_buffer);
    type->byte4_gpu_columns.push(gpu_buffer);
    
    // Test 4: Copy to GPU
    print("\nCopying CPU data to GPU...");
    void* cpu_data = &type->byte4_columns[0][0];
    print("  CPU data starts at: ", cpu_data);
    print("  GPU buffer at: ", gpu_buffer);
    
    // These should be DIFFERENT addresses if one is CPU and one is GPU
    if (cpu_data == gpu_buffer) {
        print("WARNING: CPU and GPU pointers are identical!");
        print("This might mean we're not actually using GPU memory");
    }
    
    type->allocator->copy_to_gpu(gpu_buffer, cpu_data, num_floats * sizeof(float));
    print("✓ Copy to GPU completed");
    
    // Test 5: Corrupt CPU data to ensure we're reading from GPU
    print("\nCorrupting CPU data to verify GPU independence...");
    for (int i = 0; i < num_floats; i++) {
        type->byte4_columns[0][i] = 0xDEADBEEF;  // Garbage value
    }
    print("✓ CPU data corrupted with 0xDEADBEEF");
    
    // Test 6: Read back from GPU
    print("\nReading back from GPU...");
    std::vector<float> readback(num_floats);
    type->allocator->copy_to_cpu(readback.data(), gpu_buffer, num_floats * sizeof(float));
    print("✓ Copy from GPU completed");
    
    // Test 7: Verify data is correct original values, not corrupted CPU values
    print("\nVerifying data integrity...");
    bool all_match = true;
    int first_error = -1;
    
    for (int i = 0; i < num_floats; i++) {
        float expected = float(i);
        float actual = readback[i];
        
        if (expected != actual) {
            if (first_error == -1) first_error = i;
            all_match = false;
        }
        
        // Check a few samples
        if (i < 5 || i >= 95) {
            print("  Index ", i, ": expected ", expected, ", got ", actual);
        }
    }
    
    if (!all_match) {
        print("\nFAIL: Data mismatch! First error at index ", first_error);
        print("Expected: ", float(first_error));
        print("Got: ", readback[first_error]);
        return;
    }
    
    print("\n✓ All ", num_floats, " values match original data");
    print("✓ GPU memory is independent of CPU memory");
    
    // Test 8: Verify CPU data is still corrupted
    print("\nVerifying CPU corruption persisted...");
    uint32_t corrupted_value = *reinterpret_cast<uint32_t*>(&type->byte4_columns[0][0]);
    if (corrupted_value == 0xDEADBEEF) {
        print("✓ CPU data is still corrupted (0xDEADBEEF)");
        print("✓ GPU data survived CPU corruption - memory spaces are separate!");
    } else {
        print("WARNING: CPU data was somehow restored?");
        print("Got: 0x", std::hex, corrupted_value);
    }
    
    // Test 9: Free GPU memory
    print("\nFreeing GPU buffer...");
    type->allocator->free(gpu_buffer);
    print("✓ GPU memory freed without crash");
    
    print("\n=== ALL TESTS PASSED ===");
    print("GPU memory allocation, transfer, and isolation verified!");
}



int main() {

    print("Hello world!");
    //test_gpu_type();
    test_gpu_compute();

    return 0;
}