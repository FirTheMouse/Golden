#include<tensor.hpp>
#include<ml_util.hpp>
#include<MNIST.hpp>
#include<core.hpp>



using namespace Eigen;

#define ON_WINDOWS 0

// void test_gpu_matmul() {
//     print("=== Testing GPU Matrix Multiply ===\n");
    
//     // Create two small test matrices on CPU
//     // A = [1 2]    B = [5 6]
//     //     [3 4]        [7 8]
//     // Expected C = A*B = [19 22]
//     //                    [43 50]
    
//     MatrixXf cpu_a(2, 2);
//     cpu_a << 1, 2,
//              3, 4;
    
//     MatrixXf cpu_b(2, 2);
//     cpu_b << 5, 6,
//              7, 8;
    
//     print("Matrix A (CPU):");
//     print(cpu_a);
//     print("\nMatrix B (CPU):");
//     print(cpu_b);
    
//     // Create tensors
//     auto a = make<Eigen::tensor>(cpu_a);
//     auto b = make<Eigen::tensor>(cpu_b);
    
//     // First verify CPU multiply works (should already work)
//     print("\n--- Testing CPU multiply ---");
//     auto cpu_result = a->forward({MATMUL, b});
//     print("CPU Result:");
//     print(cpu_result->data_);
    
//     // Now test GPU multiply
//     print("\n--- Testing GPU multiply ---");
//     print("Moving tensors to GPU...");
//     a->to_gpu();
//     b->to_gpu();
//     print("Tensor A on device:", a->device_);
//     print("Tensor B on device:", b->device_);

//     print("\n--- Verifying GPU transfer ---");
// auto a_verify = make<Eigen::tensor>(cpu_a);
// a_verify->gpu_data_ = a->gpu_data_;
// a_verify->device_ = GPU;
// a_verify->to_cpu();
// print("A after GPU round-trip:");
// print(a_verify->data_);

// auto b_verify = make<Eigen::tensor>(cpu_b);
// b_verify->gpu_data_ = b->gpu_data_;
// b_verify->device_ = GPU;
// b_verify->to_cpu();
// print("B after GPU round-trip:");
// print(b_verify->data_);
    
//     // Do the multiply
//     print("Executing GPU matrix multiply...");
//     auto gpu_result = a->forward({MATMUL, b});
    
//     print("Result device:", gpu_result->device_);
    
//     // Bring result back to CPU to inspect it
//     if (gpu_result->device_ != CPU) {
//         print("Copying result back to CPU...");
//         gpu_result->to_cpu();
//     }
    
//     print("GPU Result:");
//     print(gpu_result->data_);
    
//     // Verify they match
//     print("\n--- Verification ---");
//     MatrixXf expected(2, 2);
//     expected << 19, 22,
//                 43, 50;
    
//     float diff = (gpu_result->data_ - expected).norm();
//     if (diff < 0.001f) {
//         print("✓ SUCCESS! GPU result matches expected values");
//     } else {
//         print("✗ FAIL! Difference:", diff);
//     }
// }

// void test_gpu_xor_training() {
//     print("=== Training XOR on GPU ===\n");
    
//     // XOR truth table as training data
//     MatrixXf inputs(4, 2);
//     inputs << 0, 0,
//               0, 1,
//               1, 0,
//               1, 1;
    
//     MatrixXf targets(4, 1);
//     targets << 0,
//                1,
//                1,
//                0;
    
//     print("XOR Training Data:");
//     print("Inputs:\n", inputs);
//     print("Targets:\n", targets);
    
//     // Create a simple network: 2 -> 4 -> 1 (input -> hidden -> output)
//     auto x = make<Eigen::tensor>(inputs);
//     auto y = make<Eigen::tensor>(targets);
//     auto w1 = Eigen::weight(2, 4, 0.5f);  // Input to hidden weights
//     auto b1 = Eigen::bias(4);              // Hidden bias
//     auto w2 = Eigen::weight(4, 1, 0.5f);  // Hidden to output weights
//     auto b2 = Eigen::bias(1);              // Output bias
    
//     print("\nMoving network to GPU...");
//     x->to_gpu();
//     y->to_gpu();
//     w1->to_gpu();
//     b1->to_gpu();
//     w2->to_gpu();
//     b2->to_gpu();
//     print("✓ All tensors on GPU");
    
//     list<g_ptr<Eigen::tensor>> params = {w1, b1, w2, b2};
//     float learning_rate = 0.5f;
    
//     print("\nTraining for 1000 epochs...\n");
    
//     for (int epoch = 0; epoch < 1000; epoch++) {
//         // Forward pass - all on GPU
//         auto h = x->forward({MATMUL, w1});
//         h = h->forward({ADD_BIAS, b1});
//         h = h->forward({RELU});
//         auto output = h->forward({MATMUL, w2});
//         output = output->forward({ADD_BIAS, b2});
        
//         // Compute loss (MSE) - bring output to CPU briefly to compute loss
//         output->to_cpu();
//         y->to_cpu();
//         float loss = (output->data_ - y->data_).array().square().mean();
        
//         // Move back to GPU for backward pass
//         output->to_gpu();
//         y->to_gpu();
        
//         // Backward pass - all on GPU
//         // Seed the gradient
//         output->grad_ = 2.0f * (output->data_ - y->data_) / float(inputs.rows());
//         // Need to copy gradient to GPU too
//         size_t grad_bytes = output->grad_.size() * sizeof(float);
//         MatrixXf grad_transposed = output->grad_.transpose();
//         tensor::gpu_allocator->copy_to_gpu(output->gpu_grad_, grad_transposed.data(), grad_bytes);
        
//         output->backward();
        
//         // Update weights on GPU
//         for (auto& param : params) {
//             add_inplace(param->gpu_data_, param->gpu_grad_, 
//                              param->data_.rows() * param->data_.cols(), -learning_rate);
//             // Zero gradients
//             tensor::gpu_allocator->memset_gpu(param->gpu_grad_, 0, 
//                                      param->data_.rows() * param->data_.cols() * sizeof(float));
//         }
        
//         if (epoch % 100 == 0) {
//             print("Epoch ", epoch, " Loss: ", loss);
//         }
//     }
    
//     print("\nTraining complete! Testing final predictions...");
    
//     // Test final network
//     auto final_h = x->forward({MATMUL, w1});
//     final_h = final_h->forward({ADD_BIAS, b1});
//     final_h = final_h->forward({RELU});
//     auto final_output = final_h->forward({MATMUL, w2});
//     final_output = final_output->forward({ADD_BIAS, b2});
    
//     final_output->to_cpu();
//     print("\nFinal Predictions:");
//     print(final_output->data_);
//     print("\nTargets:");
//     print(targets);
// }

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

void test_text_perdictor() {

}

std::string run_char_lm(const std::string& text, int epochs, float learning_rate, int batch_size, int logging_interval) {
    map<char, int> char_to_idx;
    list<char> idx_to_char;
    int vocab_size = 0;

    for(char c : text) {
        if(!char_to_idx.hasKey(c)) {
            char_to_idx.put(c, vocab_size);
            idx_to_char.push(c);
            vocab_size++;
        }
    }

    print("Vocabulary size: ", vocab_size);

    // Create training examples
    int context_len = 10;  // Increased from 3 to 10!
    int num_examples = text.length() - context_len;

    // Now inputs are just character indices, not one-hot vectors
    auto inputs = make<tensor>(MatrixXf(num_examples, context_len));
    auto targets = make<tensor>(MatrixXf::Zero(num_examples, vocab_size));

    for(int i = 0; i < num_examples; i++) {
        // Store character indices directly
        for(int j = 0; j < context_len; j++) {
            char c = text[i + j];
            int idx = char_to_idx.get(c);
            inputs->data_(i, j) = (float)idx;
        }

        // Target is still one-hot
        char target_char = text[i + context_len];
        int target_idx = char_to_idx.get(target_char);
        targets->data_(i, target_idx) = 1.0f;
    }

    // Network with embeddings
    int embed_dim = 16;  // Each character gets a 16-dimensional vector
    int hidden_dim = 128;

    // Embedding matrix: vocab_size x embed_dim
    auto embed = weight(vocab_size, embed_dim, 0.1f);

    // After embedding, we'll have context_len * embed_dim features
    auto w1 = weight(context_len * embed_dim, hidden_dim, 0.01f);
    auto b1 = bias(hidden_dim);
    auto w2 = weight(hidden_dim, vocab_size, 0.01f);
    auto b2 = bias(vocab_size);

    list<Pass> network = {
        {EMBED, embed},
        {MATMUL, w1}, {ADD_BIAS, b1}, {TANH},
        {MATMUL, w2}, {ADD_BIAS, b2}
    };

    list<g_ptr<tensor>> params = {embed, w1, b1, w2, b2};

    // Train
    train_network(
        inputs,
        targets,
        network,
        params,
        SOFTMAX_CE,
        epochs,
        learning_rate,
        batch_size,
        logging_interval,
        false,
        MEAN
    );

    // Generate text
    print("\nGenerating text:");
    std::string seed = text.substr(0, context_len);
    std::string generated = seed;

    for(int i = 0; i < 200; i++) {
        std::string context = generated.substr(generated.length() - context_len);

        // Input is now just character indices
        auto input = make<tensor>(MatrixXf(1, context_len));
        for(int j = 0; j < context_len; j++) {
            int idx = char_to_idx.get(context[j]);
            input->data_(0, j) = (float)idx;
        }

        // Forward pass
        auto output = input;
        for(auto p : network) {
            output = output->forward(p);
        }

        // Sample from distribution
        MatrixXf logits = output->data_.row(0);
        VectorXf m = logits.rowwise().maxCoeff();
        MatrixXf stable = logits.colwise() - m;
        MatrixXf exp_vals = stable.array().exp().matrix();
        float sum = exp_vals.sum();
        MatrixXf probs = exp_vals / sum;

        float r = (float)rand() / RAND_MAX;
        float cumsum = 0.0f;
        int next_idx = 0;
        for(int j = 0; j < vocab_size; j++) {
            cumsum += probs(0, j);
            if(r <= cumsum) {
                next_idx = j;
                break;
            }
        }

        generated += idx_to_char[next_idx];
    }

    return generated;
}

std::string run_char_lm_rnn(const std::string& text, int epochs, float learning_rate, int batch_size) {
    map<char, int> char_to_idx;
    list<char> idx_to_char;
    int vocab_size = 0;
    
    for(char c : text) {
        if(!char_to_idx.hasKey(c)) {
            char_to_idx.put(c, vocab_size);
            idx_to_char.push(c);
            vocab_size++;
        }
    }
    
    print("Vocabulary size: ", vocab_size);
    
    // Create sequences
    int seq_length = 10;
    int num_sequences = text.length() - seq_length;
    
    auto inputs = make<tensor>(MatrixXf(num_sequences, seq_length));
    auto targets = make<tensor>(MatrixXf::Zero(num_sequences, seq_length * vocab_size));
    
    for(int i = 0; i < num_sequences; i++) {
        for(int t = 0; t < seq_length; t++) {
            char c = text[i + t];
            inputs->data_(i, t) = (float)char_to_idx.get(c);
            
            char target_c = text[i + t + 1];
            int target_idx = char_to_idx.get(target_c);
            targets->data_(i, t * vocab_size + target_idx) = 1.0f;
        }
    }
    
    int embed_dim = 16;
    int hidden_dim = 64;
    
    auto embed = weight(vocab_size, embed_dim, 0.1f);
    auto rnn_weights = weight(embed_dim + hidden_dim, hidden_dim, 0.01f);
    auto output_w = weight(hidden_dim, vocab_size, 0.01f);
    auto output_b = bias(vocab_size);
    
    list<Pass> network = {
        {EMBED, embed},
        {RNN_STEP, rnn_weights, nullptr, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, false, false, false, {seq_length}},
        {MATMUL, output_w},
        {ADD_BIAS, output_b}
    };
    
    list<g_ptr<tensor>> params = {embed, rnn_weights, output_w, output_b};
    
    print("Training RNN...");
    train_network(
        inputs,
        targets,
        network,
        params,
        SOFTMAX_CE,
        epochs,
        learning_rate,
        batch_size,
        100,
        false,
        1.0f,
        MEAN
    );
    
    // Generate - need to manually manage the state
    print("\nGenerating text:");
    std::string generated = text.substr(0, 1);
    
    // Create a fresh network for generation (without sequence unrolling)
    list<Pass> gen_network = {
        {EMBED, embed},
        {RNN_STEP, rnn_weights, nullptr, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, false, false, false, {}}, // No ints - not sequence mode!
        {MATMUL, output_w},
        {ADD_BIAS, output_b}
    };
    
    // Reset state before generation
    reset_state(gen_network);
    
    for(int i = 0; i < 200; i++) {
        char current_char = generated[generated.length() - 1];
        int char_idx = char_to_idx.get(current_char);
        
        auto input = make<tensor>(MatrixXf(1, 1));
        input->data_(0, 0) = (float)char_idx;
        
        auto output = input;
        for(auto& p : gen_network) {
            output = output->forward(p);
        }
        
        // Sample
        MatrixXf logits = output->data_.row(0);
        VectorXf m = logits.rowwise().maxCoeff();
        MatrixXf stable = logits.colwise() - m;
        MatrixXf exp_vals = stable.array().exp().matrix();
        float sum = exp_vals.sum();
        MatrixXf probs = exp_vals / sum;
        
        float r = (float)rand() / RAND_MAX;
        float cumsum = 0.0f;
        int next_idx = 0;
        for(int j = 0; j < vocab_size; j++) {
            cumsum += probs(0, j);
            if(r <= cumsum) {
                next_idx = j;
                break;
            }
        }
        
        generated += idx_to_char[next_idx];
    }
    
    return generated;
}

void test_qkv_proj() {
    print("Testing QKV_PROJ...");
    
    int seq_len = 10;
    int embed_dim = 16;
    int d_k = 16;
    
    // Create input: (seq_len x embed_dim)
    auto input = make<tensor>(MatrixXf::Random(seq_len, embed_dim));
    
    // Create weights: (embed_dim x 3*d_k)
    auto qkv_weights = make<tensor>(MatrixXf::Random(embed_dim, 3 * d_k) * 0.01f);
    
    print("Input: ", input->data_.rows(), "x", input->data_.cols());
    print("Weights: ", qkv_weights->data_.rows(), "x", qkv_weights->data_.cols());
    
    // Forward
    Pass qkv_pass = {QKV_PROJ, qkv_weights};
    auto output = input->forward(qkv_pass);
    
    print("Output: ", output->data_.rows(), "x", output->data_.cols());
    print("Expected: ", seq_len, "x", 3*d_k);
    
    // Backward
    output->grad_ = MatrixXf::Ones(seq_len, 3 * d_k);
    output->backward();
    
    print("Input grad: ", input->grad_.rows(), "x", input->grad_.cols());
    print("Weight grad: ", qkv_weights->grad_.rows(), "x", qkv_weights->grad_.cols());
    
    if(input->grad_.hasNaN()) print("ERROR: Input grad has NaN!");
    if(qkv_weights->grad_.hasNaN()) print("ERROR: Weight grad has NaN!");
    
    print("QKV_PROJ test passed!");
}

void test_attention() {
    print("Testing ATTENTION...");
    
    int seq_len = 10;
    int d_k = 16;
    
    // Create input: (seq_len x 3*d_k) - concatenated Q, K, V
    auto input = make<tensor>(MatrixXf::Random(seq_len, 3 * d_k) * 0.1f);
    
    print("Input: ", input->data_.rows(), "x", input->data_.cols());
    
    // Forward
    Pass attn_pass;
    attn_pass.op = ATTENTION;
    attn_pass.i1 = seq_len;
    attn_pass.i2 = d_k;
    
    auto output = input->forward(attn_pass);
    
    print("Output: ", output->data_.rows(), "x", output->data_.cols());
    print("Expected: ", seq_len, "x", d_k);
    
    if(output->data_.hasNaN()) {
        print("ERROR: Output has NaN!");
        return;
    }
    
    // Backward
    output->grad_ = MatrixXf::Ones(seq_len, d_k);
    output->backward();
    
    print("Input grad: ", input->grad_.rows(), "x", input->grad_.cols());
    
    if(input->grad_.hasNaN()) print("ERROR: Gradient has NaN!");
    else print("ATTENTION test passed!");
}

std::string run_char_lm_attention(const std::string& text, int epochs, float learning_rate, int batch_size, int logging_interval) {
    map<char, int> char_to_idx;
    list<char> idx_to_char;
    int vocab_size = 0;

    for(char c : text) {
        if(!char_to_idx.hasKey(c)) {
            char_to_idx.put(c, vocab_size);
            idx_to_char.push(c);
            vocab_size++;
        }
    }

    print("Vocabulary size: ", vocab_size);

    int context_len = 10;
    int num_examples = text.length() - context_len;

    auto inputs = make<tensor>(MatrixXf(num_examples, context_len));
    auto targets = make<tensor>(MatrixXf::Zero(num_examples, vocab_size));

    for(int i = 0; i < num_examples; i++) {
        for(int j = 0; j < context_len; j++) {
            char c = text[i + j];
            int idx = char_to_idx.get(c);
            inputs->data_(i, j) = (float)idx;
        }

        char target_char = text[i + context_len];
        int target_idx = char_to_idx.get(target_char);
        targets->data_(i, target_idx) = 1.0f;
    }

    int embed_dim = 16;
    int d_k = 16;

    auto embed = weight(vocab_size, embed_dim, 0.1f);
    auto qkv_weights = weight(embed_dim, 3 * d_k, 0.1f);
    auto w_out = weight(d_k, vocab_size, 0.01f);
    auto b_out = bias(vocab_size);

    list<Pass> network;
    network.push(Pass{EMBED, embed});
    
    Pass reshape_pass;
    reshape_pass.op = RESHAPE;
    reshape_pass.i1 = context_len;
    reshape_pass.i2 = -1;
    network.push(reshape_pass);
    
    network.push(Pass{QKV_PROJ, qkv_weights});
    
    Pass attn_pass;
    attn_pass.op = ATTENTION;
    attn_pass.i1 = context_len;
    attn_pass.i2 = d_k;
    network.push(attn_pass);
    
    // ADD THESE:
    Pass last_pass;
    last_pass.op = LAST_POSITION;
    network.push(last_pass);
    
    network.push(Pass{MATMUL, w_out});
    network.push(Pass{ADD_BIAS, b_out});
    
    list<g_ptr<tensor>> params = {embed, qkv_weights, w_out, b_out};


    print("Training with attention...");
    train_network(
        inputs,
        targets,
        network,
        params,
        SOFTMAX_CE,
        epochs,
        learning_rate,
        0,  // No batching - process one sequence at a time
        logging_interval,
        false,
        1.0f,
        MEAN
    );

    // Generate
    print("\nGenerating text:");
    std::string seed = text.substr(0, context_len);
    std::string generated = seed;

    for(int i = 0; i < 200; i++) {
        std::string context = generated.substr(generated.length() - context_len);

        auto input = make<tensor>(MatrixXf(1, context_len));
        for(int j = 0; j < context_len; j++) {
            int idx = char_to_idx.get(context[j]);
            input->data_(0, j) = (float)idx;
        }

        auto output = input;
        for(auto p : network) {
            output = output->forward(p);
        }

        MatrixXf logits = output->data_.row(0);
        VectorXf m = logits.rowwise().maxCoeff();
        MatrixXf stable = logits.colwise() - m;
        MatrixXf exp_vals = stable.array().exp().matrix();
        float sum = exp_vals.sum();
        MatrixXf probs = exp_vals / sum;

        float r = (float)rand() / RAND_MAX;
        float cumsum = 0.0f;
        int next_idx = 0;
        for(int j = 0; j < vocab_size; j++) {
            cumsum += probs(0, j);
            if(r <= cumsum) {
                next_idx = j;
                break;
            }
        }

        generated += idx_to_char[next_idx];
    }

    return generated;
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
   
    Log::Line l;
    l.start();

    //test_xor_cpu();
   //test_gpu_xor_training();
    //test_gpu_matmul();
    
    run_mnist(scene,4);

    // std::string training_text = 
    // "the quick brown fox jumps over the lazy dog. "
    // "a quick brown dog jumps over the lazy fox. "
    // "the lazy dog sleeps under the warm sun. "
    // "the quick fox runs through the green forest. "
    // "a brown fox hunts in the quiet night. "
    // "the dog and the fox are good friends. "
    // "the quick brown fox jumps over the lazy dog. "
    // "the lazy dog rests by the old tree. "
    // "a quick fox sees a lazy dog sleeping. "
    // "the brown dog plays with the quick fox. ";

    // training_text = readFile(root()+"/Projects/FirML/assets/images/test1.txt");

    // std::string result = "";
    // //result = run_char_lm(training_text,100,0.01f,32,1);
    // result = run_char_lm_attention(training_text,100,0.01f,32,1);
    print("Time: ",l.end()/1000000,"ms");
    // print(result);


    start::run(window,d,[&]{

    });

    return 0;
}




