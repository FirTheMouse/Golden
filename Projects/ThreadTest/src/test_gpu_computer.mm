#import <Metal/Metal.h>
#include<test_gpu_computer.hpp>
#include <core/gpuType.hpp>
#include <vector>

using namespace Golden;

void test_gpu_compute() {
    print("=== Testing GPU Computation ===\n");
    
    // Create test data on CPU
    const int N = 100;
    auto type = make<gpuType>();
    
    print("Step 1: Creating ", N, " test values on CPU");
    for (int i = 0; i < N; i++) {
        type->push<float>(float(i), 0);
    }
    
    // Allocate GPU buffers
    print("Step 2: Allocating GPU buffers");
    void* gpu_input = type->allocator->allocate(N * sizeof(float));
    void* gpu_output = type->allocator->allocate(N * sizeof(float));
    
    if (!gpu_input || !gpu_output) {
        print("FAIL: GPU allocation failed");
        return;
    }
    
    // Copy input to GPU
    print("Step 3: Copying data to GPU");
    void* cpu_data = &type->byte4_columns[0][0];
    type->allocator->copy_to_gpu(gpu_input, cpu_data, N * sizeof(float));
    
    // Now here's the critical part - we need to actually run a compute shader
    print("Step 4: Setting up Metal compute pipeline");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        print("FAIL: No Metal device");
        return;
    }
    
    // Create a simple compute shader inline
    // This shader multiplies each input element by 2
    NSString* shaderSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        kernel void multiply_by_two(
            device const float* input [[buffer(0)]],
            device float* output [[buffer(1)]],
            uint id [[thread_position_in_grid]])
        {
            output[id] = input[id] * 2.0f;
        }
    )";
    
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource 
                                                  options:nil 
                                                    error:&error];
    if (!library) {
        print("FAIL: Shader compilation failed");
        return;
    }
    
    id<MTLFunction> function = [library newFunctionWithName:@"multiply_by_two"];
    id<MTLComputePipelineState> pipeline = 
        [device newComputePipelineStateWithFunction:function error:&error];
    
    if (!pipeline) {
        print("FAIL: Pipeline creation failed");
        return;
    }
    
    print("Step 5: Executing compute shader on GPU");
    
    // Create command queue and buffer
    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the pipeline and bind our buffers
    [encoder setComputePipelineState:pipeline];
    
    // Cast our void* back to Metal buffers
    id<MTLBuffer> inputBuffer = (__bridge id<MTLBuffer>)gpu_input;
    id<MTLBuffer> outputBuffer = (__bridge id<MTLBuffer>)gpu_output;
    
    [encoder setBuffer:inputBuffer offset:0 atIndex:0];
    [encoder setBuffer:outputBuffer offset:0 atIndex:1];
    
    // Dispatch threads - one thread per element
    MTLSize gridSize = MTLSizeMake(N, 1, 1);
    NSUInteger threadGroupSize = pipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > N) threadGroupSize = N;
    MTLSize threadgroupSize = MTLSizeMake(threadGroupSize, 1, 1);
    
    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];
    
    // Execute and wait
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    print("Step 6: GPU computation completed, copying results back");
    
    // Copy results back to CPU
    std::vector<float> results(N);
    type->allocator->copy_to_cpu(results.data(), gpu_output, N * sizeof(float));
    
    // Verify results
    print("Step 7: Verifying results\n");
    bool all_correct = true;
    int errors = 0;
    
    for (int i = 0; i < N; i++) {
        float expected = float(i) * 2.0f;
        float actual = results[i];
        
        if (expected != actual) {
            all_correct = false;
            errors++;
            if (errors <= 5) {  // Only print first 5 errors
                print("  ERROR at index ", i, ": expected ", expected, " got ", actual);
            }
        }
    }
    
    if (all_correct) {
        print("✓ SUCCESS: All ", N, " values correct!");
        print("✓ Sample results:");
        print("  Input[0] = 0   → GPU computed → Output[0] = ", results[0]);
        print("  Input[50] = 50 → GPU computed → Output[50] = ", results[50]);
        print("  Input[99] = 99 → GPU computed → Output[99] = ", results[99]);
        print("\n=== GPU COMPUTATION VERIFIED ===");
        print("The GPU successfully performed arithmetic operations!");
    } else {
        print("FAIL: ", errors, " incorrect values out of ", N);
    }
    
    // Cleanup
    type->allocator->free(gpu_input);
    type->allocator->free(gpu_output);
}