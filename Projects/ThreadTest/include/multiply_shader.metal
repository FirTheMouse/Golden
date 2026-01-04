#include <metal_stdlib>
using namespace metal;

// Compute kernel that multiplies each element by 2
kernel void multiply_by_two(
    device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    uint id [[thread_position_in_grid]])
{
    output[id] = input[id] * 2.0f;
}