// In a .mm file
#ifdef __APPLE__
    #import <Foundation/Foundation.h>
    #import <Metal/Metal.h>
    #import <MetalPerformanceShaders/MetalPerformanceShaders.h>
    #include<util/ml_methods.hpp>
    #include<vector>


    void* matmul(void* gpu_a, int a_rows, int a_cols,
                    void* gpu_b, int b_rows, int b_cols) {
        id<MTLBuffer> bufferA = (__bridge id<MTLBuffer>)gpu_a;
        id<MTLBuffer> bufferB = (__bridge id<MTLBuffer>)gpu_b;
        
        int result_rows = a_rows;
        int result_cols = b_cols;
        size_t result_bytes = result_rows * result_cols * sizeof(float);
        
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLBuffer> bufferC = [device newBufferWithLength:result_bytes 
                                options:MTLResourceStorageModeShared];
        
        // Since we transposed to row-major in to_gpu(), rowBytes should be columns * sizeof(float)
        // This is the standard row-major layout that Metal expects
        MPSMatrixDescriptor* descA = [MPSMatrixDescriptor 
            matrixDescriptorWithRows:a_rows columns:a_cols 
            rowBytes:a_cols*sizeof(float) dataType:MPSDataTypeFloat32];
        MPSMatrixDescriptor* descB = [MPSMatrixDescriptor 
            matrixDescriptorWithRows:b_rows columns:b_cols 
            rowBytes:b_cols*sizeof(float) dataType:MPSDataTypeFloat32];
        MPSMatrixDescriptor* descC = [MPSMatrixDescriptor 
            matrixDescriptorWithRows:result_rows columns:result_cols 
            rowBytes:result_cols*sizeof(float) dataType:MPSDataTypeFloat32];
        
        MPSMatrix* matrixA = [[MPSMatrix alloc] initWithBuffer:bufferA descriptor:descA];
        MPSMatrix* matrixB = [[MPSMatrix alloc] initWithBuffer:bufferB descriptor:descB];
        MPSMatrix* matrixC = [[MPSMatrix alloc] initWithBuffer:bufferC descriptor:descC];
        
        MPSMatrixMultiplication* matmul = [[MPSMatrixMultiplication alloc] 
            initWithDevice:device 
            transposeLeft:NO transposeRight:NO
            resultRows:result_rows resultColumns:result_cols 
            interiorColumns:a_cols alpha:1.0 beta:0.0];
        
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        
        [matmul encodeToCommandBuffer:commandBuffer 
                        leftMatrix:matrixA 
                        rightMatrix:matrixB 
                        resultMatrix:matrixC];
        
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        return (__bridge_retained void*)bufferC;
    }

    void add_inplace(void* gpu_dest, void* gpu_source, int num_elements, float scale) {
        id<MTLBuffer> dest = (__bridge id<MTLBuffer>)gpu_dest;
        id<MTLBuffer> source = (__bridge id<MTLBuffer>)gpu_source;
        
        float* dest_ptr = (float*)dest.contents;
        float* source_ptr = (float*)source.contents;
        
        for (int i = 0; i < num_elements; i++) {
            dest_ptr[i] += scale * source_ptr[i];
        }
    }

 void* add_bias(void* gpu_data, int rows, int cols, void* gpu_bias, Golden::u_allocator* allocator) {
        id<MTLBuffer> data_buffer = (__bridge id<MTLBuffer>)gpu_data;
        id<MTLBuffer> bias_buffer = (__bridge id<MTLBuffer>)gpu_bias;
        
        // Allocate a NEW buffer for the result
        size_t result_bytes = rows * cols * sizeof(float);
        void* result_buffer = allocator->allocate(result_bytes);
        id<MTLBuffer> result = (__bridge id<MTLBuffer>)result_buffer;
        
        // Copy input data to result buffer first
        float* result_ptr = (float*)result.contents;
        float* data_ptr = (float*)data_buffer.contents;
        float* bias_ptr = (float*)bias_buffer.contents;
        
        memcpy(result_ptr, data_ptr, result_bytes);
        
        // Now add bias to the result
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result_ptr[i * cols + j] += bias_ptr[j];
            }
        }
        
        return result_buffer;
    }

    void* relu(void* gpu_data, int rows, int cols, Golden::u_allocator* allocator) {
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)gpu_data;
        
        // Allocate new buffer for result
        size_t result_bytes = rows * cols * sizeof(float);
        void* result_buffer = allocator->allocate(result_bytes);
        id<MTLBuffer> result = (__bridge id<MTLBuffer>)result_buffer;
        
        // Copy and apply ReLU
        float* input_ptr = (float*)buffer.contents;
        float* result_ptr = (float*)result.contents;
        
        int num_elements = rows * cols;
        for (int i = 0; i < num_elements; i++) {
            result_ptr[i] = (input_ptr[i] > 0) ? input_ptr[i] : 0;
        }
        
        return result_buffer;
    }
#endif