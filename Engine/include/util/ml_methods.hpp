#pragma once

#include<core/gpu_allocator.hpp>

void* matmul(void* gpu_a, int a_rows, int a_cols,
                   void* gpu_b, int b_rows, int b_cols);
void add_inplace(void* gpu_dest, void* gpu_source, int num_elements,float scale = 1.0f);
void* add_bias(void* gpu_data, int rows, int cols, void* gpu_bias, Golden::u_allocator* allocator);
void* relu(void* gpu_data, int rows, int cols, Golden::u_allocator* allocator);
