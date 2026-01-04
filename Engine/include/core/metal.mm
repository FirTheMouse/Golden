#import <Metal/Metal.h>
#include "gpu_allocator.hpp"
#include <cstring>

class MetalAllocator : public Golden::u_allocator {
private:
    id<MTLDevice> device_;
    
public:
    MetalAllocator() {
        // Get the default Metal device (the GPU)
        device_ = MTLCreateSystemDefaultDevice();
        if (!device_) {
            throw std::runtime_error("Failed to create Metal device - no GPU available?");
        }
    }
    
    ~MetalAllocator() {
        // ARC will handle device cleanup
    }
    
    void* allocate(size_t bytes) override {
        // Allocate shared memory buffer
        // Shared mode means both CPU and GPU can access it
        id<MTLBuffer> buffer = [device_ newBufferWithLength:bytes 
                                options:MTLResourceStorageModeShared];
        
        if (!buffer) {
            return nullptr;  // Allocation failed
        }
        
        // Return as void* - we're storing the Objective-C object pointer
        // __bridge_retained transfers ownership to manual memory management
        return (__bridge_retained void*)buffer;
    }
    
    void free(void* ptr) override {
        if (!ptr) return;
        
        // Release the Objective-C object
        // __bridge_transfer takes back ownership and releases it
        id<MTLBuffer> buffer = (__bridge_transfer id<MTLBuffer>)ptr;
        // buffer is released automatically when it goes out of scope
    }
    
    void copy_to_gpu(void* gpu_dst, const void* cpu_src, size_t bytes) override {
        if (!gpu_dst || !cpu_src) return;
        
        // Cast back to Metal buffer
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)gpu_dst;
        
        // For shared memory, we can just use regular memcpy
        // buffer.contents gives us the CPU-accessible pointer to the buffer
        memcpy(buffer.contents, cpu_src, bytes);
    }
    
    void copy_to_cpu(void* cpu_dst, const void* gpu_src, size_t bytes) override {
        if (!cpu_dst || !gpu_src) return;
        
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)gpu_src;
        memcpy(cpu_dst, buffer.contents, bytes);
    }
    
    void memset_gpu(void* gpu_dst, int value, size_t bytes) override {
        if (!gpu_dst) return;
        
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)gpu_dst;
        memset(buffer.contents, value, bytes);
    }
};

namespace Golden {
// Factory function to create the allocator
u_allocator* create_metal_allocator() {
    return new MetalAllocator();
}

}