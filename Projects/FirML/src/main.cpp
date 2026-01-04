#include<tensor.hpp>
#include<ml_util.hpp>
#include<MNIST.hpp>
#include<core.hpp>



using namespace Eigen;

#define ON_WINDOWS 0

void test_gpu_matmul() {
    print("=== Testing GPU Matrix Multiply ===\n");
    
    // Create two small test matrices on CPU
    // A = [1 2]    B = [5 6]
    //     [3 4]        [7 8]
    // Expected C = A*B = [19 22]
    //                    [43 50]
    
    MatrixXf cpu_a(2, 2);
    cpu_a << 1, 2,
             3, 4;
    
    MatrixXf cpu_b(2, 2);
    cpu_b << 5, 6,
             7, 8;
    
    print("Matrix A (CPU):");
    print(cpu_a);
    print("\nMatrix B (CPU):");
    print(cpu_b);
    
    // Create tensors
    auto a = make<Eigen::tensor>(cpu_a);
    auto b = make<Eigen::tensor>(cpu_b);
    
    // First verify CPU multiply works (should already work)
    print("\n--- Testing CPU multiply ---");
    auto cpu_result = a->forward({MATMUL, b});
    print("CPU Result:");
    print(cpu_result->data_);
    
    // Now test GPU multiply
    print("\n--- Testing GPU multiply ---");
    print("Moving tensors to GPU...");
    a->to_gpu();
    b->to_gpu();
    print("Tensor A on device:", a->device_);
    print("Tensor B on device:", b->device_);

    print("\n--- Verifying GPU transfer ---");
auto a_verify = make<Eigen::tensor>(cpu_a);
a_verify->gpu_data_ = a->gpu_data_;
a_verify->device_ = GPU_METAL;
a_verify->to_cpu();
print("A after GPU round-trip:");
print(a_verify->data_);

auto b_verify = make<Eigen::tensor>(cpu_b);
b_verify->gpu_data_ = b->gpu_data_;
b_verify->device_ = GPU_METAL;
b_verify->to_cpu();
print("B after GPU round-trip:");
print(b_verify->data_);
    
    // Do the multiply
    print("Executing GPU matrix multiply...");
    auto gpu_result = a->forward({MATMUL, b});
    
    print("Result device:", gpu_result->device_);
    
    // Bring result back to CPU to inspect it
    if (gpu_result->device_ != CPU) {
        print("Copying result back to CPU...");
        gpu_result->to_cpu();
    }
    
    print("GPU Result:");
    print(gpu_result->data_);
    
    // Verify they match
    print("\n--- Verification ---");
    MatrixXf expected(2, 2);
    expected << 19, 22,
                43, 50;
    
    float diff = (gpu_result->data_ - expected).norm();
    if (diff < 0.001f) {
        print("✓ SUCCESS! GPU result matches expected values");
    } else {
        print("✗ FAIL! Difference:", diff);
    }
}

void test_gpu_xor_training() {
    print("=== Training XOR on GPU ===\n");
    
    // XOR truth table as training data
    MatrixXf inputs(4, 2);
    inputs << 0, 0,
              0, 1,
              1, 0,
              1, 1;
    
    MatrixXf targets(4, 1);
    targets << 0,
               1,
               1,
               0;
    
    print("XOR Training Data:");
    print("Inputs:\n", inputs);
    print("Targets:\n", targets);
    
    // Create a simple network: 2 -> 4 -> 1 (input -> hidden -> output)
    auto x = make<Eigen::tensor>(inputs);
    auto y = make<Eigen::tensor>(targets);
    auto w1 = Eigen::weight(2, 4, 0.5f);  // Input to hidden weights
    auto b1 = Eigen::bias(4);              // Hidden bias
    auto w2 = Eigen::weight(4, 1, 0.5f);  // Hidden to output weights
    auto b2 = Eigen::bias(1);              // Output bias
    
    print("\nMoving network to GPU...");
    x->to_gpu();
    y->to_gpu();
    w1->to_gpu();
    b1->to_gpu();
    w2->to_gpu();
    b2->to_gpu();
    print("✓ All tensors on GPU");
    
    list<g_ptr<Eigen::tensor>> params = {w1, b1, w2, b2};
    float learning_rate = 0.5f;
    
    print("\nTraining for 1000 epochs...\n");
    
    for (int epoch = 0; epoch < 1000; epoch++) {
        // Forward pass - all on GPU
        auto h = x->forward({MATMUL, w1});
        h = h->forward({ADD_BIAS, b1});
        h = h->forward({RELU});
        auto output = h->forward({MATMUL, w2});
        output = output->forward({ADD_BIAS, b2});
        
        // Compute loss (MSE) - bring output to CPU briefly to compute loss
        output->to_cpu();
        y->to_cpu();
        float loss = (output->data_ - y->data_).array().square().mean();
        
        // Move back to GPU for backward pass
        output->to_gpu();
        y->to_gpu();
        
        // Backward pass - all on GPU
        // Seed the gradient
        output->grad_ = 2.0f * (output->data_ - y->data_) / float(inputs.rows());
        // Need to copy gradient to GPU too
        size_t grad_bytes = output->grad_.size() * sizeof(float);
        MatrixXf grad_transposed = output->grad_.transpose();
        tensor::gpu_allocator->copy_to_gpu(output->gpu_grad_, grad_transposed.data(), grad_bytes);
        
        output->backward();
        
        // Update weights on GPU
        for (auto& param : params) {
            metal_add_inplace(param->gpu_data_, param->gpu_grad_, 
                             param->data_.rows() * param->data_.cols(), -learning_rate);
            // Zero gradients
            tensor::gpu_allocator->memset_gpu(param->gpu_grad_, 0, 
                                     param->data_.rows() * param->data_.cols() * sizeof(float));
        }
        
        if (epoch % 100 == 0) {
            print("Epoch ", epoch, " Loss: ", loss);
        }
    }
    
    print("\nTraining complete! Testing final predictions...");
    
    // Test final network
    auto final_h = x->forward({MATMUL, w1});
    final_h = final_h->forward({ADD_BIAS, b1});
    final_h = final_h->forward({RELU});
    auto final_output = final_h->forward({MATMUL, w2});
    final_output = final_output->forward({ADD_BIAS, b2});
    
    final_output->to_cpu();
    print("\nFinal Predictions:");
    print(final_output->data_);
    print("\nTargets:");
    print(targets);
}

void test_xor_cpu() {
    print("=== Training XOR on CPU ===\n");
    
    // XOR data
    MatrixXf inputs(4, 2);
    inputs << 0, 0,
              0, 1,
              1, 0,
              1, 1;
    
    MatrixXf targets(4, 1);
    targets << 0,
               1,
               1,
               0;
    
    print("XOR Training Data:");
    print("Inputs:\n", inputs);
    print("Targets:\n", targets);
    
    // Create network
    auto x = make<Eigen::tensor>(inputs);
    auto y = make<Eigen::tensor>(targets);
    auto w1 = Eigen::weight(2, 4, 0.5f);
    auto b1 = Eigen::bias(4);
    auto w2 = Eigen::weight(4, 1, 0.5f);
    auto b2 = Eigen::bias(1);
    
    list<g_ptr<Eigen::tensor>> params = {w1, b1, w2, b2};
    
    list<Pass> network = {
        {MATMUL, w1},
        {ADD_BIAS, b1},
        {SIGMOID},
        {MATMUL, w2},
        {ADD_BIAS, b2}
    };
    
    print("\nTraining on CPU for 1000 epochs...\n");
    
    Eigen::train_network(x, y, network, params, MSE, 1000, 0.01f, 0, 100, true);
}

int main() {

    int win_size_x = 450;
    int win_size_y = 200;

    #if ON_WINDOWS
        win_size_x*=2;
        win_size_y*=2;
    #endif

    #ifdef __APPLE__
        Eigen::tensor::gpu_allocator = Golden::create_metal_allocator();
        print("Metal GPU allocator initialized");
    #else
        print("No GPU support on this platform");
    #endif

    Window window = Window(win_size_x, win_size_y, "FirML 0.0.2");
    auto scene = make<Scene>(window,2);
    scene->camera.toIso();
    scene->tickEnvironment(0);
    Data d = helper::make_config(scene,K);
   
    //test_xor_cpu();
   //test_gpu_xor_training();
    //test_gpu_matmul();
    run_mnist(scene,4);

    start::run(window,d,[&]{

    });

    return 0;
}




