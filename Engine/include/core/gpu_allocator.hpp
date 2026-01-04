#pragma once

#include<util/util.hpp>

namespace Golden {

    struct u_allocator {
        virtual ~u_allocator() = default;
        
        virtual void* allocate(size_t bytes) = 0;
        virtual void free(void* ptr) = 0;
        virtual void copy_to_gpu(void* gpu_dst, const void* cpu_src, size_t bytes) = 0;
        virtual void copy_to_cpu(void* cpu_dst, const void* gpu_src, size_t bytes) = 0;
        virtual void memset_gpu(void* gpu_dst, int value, size_t bytes) = 0;
    };
    
    #ifdef __APPLE__
        // Metal implementation (declared here, defined in .mm file)
        u_allocator* create_metal_allocator();
    #endif

}