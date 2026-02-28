#include<mnist.hpp>
#include<cog_lang_test.hpp>


using namespace Eigen;

#define ON_WINDOWS 0

// std::string run_char_lm(const std::string& text, int epochs, float learning_rate, int batch_size, int logging_interval) {
//     map<char, int> char_to_idx;
//     list<char> idx_to_char;
//     int vocab_size = 0;

//     for(char c : text) {
//         if(!char_to_idx.hasKey(c)) {
//             char_to_idx.put(c, vocab_size);
//             idx_to_char.push(c);
//             vocab_size++;
//         }
//     }

//     print("Vocabulary size: ", vocab_size);

//     // Create training examples
//     int context_len = 10;  // Increased from 3 to 10!
//     int num_examples = text.length() - context_len;

//     // Now inputs are just character indices, not one-hot vectors
//     auto inputs = make<tensor>(MatrixXf(num_examples, context_len));
//     auto targets = make<tensor>(MatrixXf::Zero(num_examples, vocab_size));

//     for(int i = 0; i < num_examples; i++) {
//         // Store character indices directly
//         for(int j = 0; j < context_len; j++) {
//             char c = text[i + j];
//             int idx = char_to_idx.get(c);
//             inputs->data_(i, j) = (float)idx;
//         }

//         // Target is still one-hot
//         char target_char = text[i + context_len];
//         int target_idx = char_to_idx.get(target_char);
//         targets->data_(i, target_idx) = 1.0f;
//     }

//     // Network with embeddings
//     int embed_dim = 16;  // Each character gets a 16-dimensional vector
//     int hidden_dim = 128;

//     // Embedding matrix: vocab_size x embed_dim
//     auto embed = weight(vocab_size, embed_dim, 0.1f);

//     // After embedding, we'll have context_len * embed_dim features
//     auto w1 = weight(context_len * embed_dim, hidden_dim, 0.01f);
//     auto b1 = bias(hidden_dim);
//     auto w2 = weight(hidden_dim, vocab_size, 0.01f);
//     auto b2 = bias(vocab_size);

//     list<Pass> network = {
//         {EMBED, embed},
//         {MATMUL, w1}, {ADD_BIAS, b1}, {TANH},
//         {MATMUL, w2}, {ADD_BIAS, b2}
//     };

//     list<g_ptr<tensor>> params = {embed, w1, b1, w2, b2};

//     g_ptr<t_config> ctx = make<t_config>();
//     ctx->epochs = epochs;
//     ctx->learning_rate = learning_rate;
//     ctx->grad_clip = 1.0f;
//     ctx->reduction = MEAN;
//     ctx->batch_size = batch_size;

//     print("Training char lm...");
//     train_network(
//         inputs,
//         targets,
//         network,
//         params,
//         SOFTMAX_CE,
//         ctx,
//         logging_interval
//     );

//     // Generate text
//     print("\nGenerating text:");
//     std::string seed = text.substr(0, context_len);
//     std::string generated = seed;

//     for(int i = 0; i < 200; i++) {
//         std::string context = generated.substr(generated.length() - context_len);

//         // Input is now just character indices
//         auto input = make<tensor>(MatrixXf(1, context_len));
//         for(int j = 0; j < context_len; j++) {
//             int idx = char_to_idx.get(context[j]);
//             input->data_(0, j) = (float)idx;
//         }

//         // Forward pass
//         auto output = input;
//         for(auto p : network) {
//             output = output->forward(p);
//         }

//         // Sample from distribution
//         MatrixXf logits = output->data_.row(0);
//         VectorXf m = logits.rowwise().maxCoeff();
//         MatrixXf stable = logits.colwise() - m;
//         MatrixXf exp_vals = stable.array().exp().matrix();
//         float sum = exp_vals.sum();
//         MatrixXf probs = exp_vals / sum;

//         float r = (float)rand() / RAND_MAX;
//         float cumsum = 0.0f;
//         int next_idx = 0;
//         for(int j = 0; j < vocab_size; j++) {
//             cumsum += probs(0, j);
//             if(r <= cumsum) {
//                 next_idx = j;
//                 break;
//             }
//         }

//         generated += idx_to_char[next_idx];
//     }

//     return generated;
// }

// std::string run_char_lm_attention(const std::string& text, int epochs, float learning_rate, int batch_size, int logging_interval) {
//     map<char, int> char_to_idx;
//     list<char> idx_to_char;
//     int vocab_size = 0;

//     for(char c : text) {
//         if(!char_to_idx.hasKey(c)) {
//             char_to_idx.put(c, vocab_size);
//             idx_to_char.push(c);
//             vocab_size++;
//         }
//     }

//     print("Vocabulary size: ", vocab_size);

//     int context_len = 10;
//     int num_examples = text.length() - context_len;

//     auto inputs = make<tensor>(MatrixXf(num_examples, context_len));
//     auto targets = make<tensor>(MatrixXf::Zero(num_examples, vocab_size));

//     for(int i = 0; i < num_examples; i++) {
//         for(int j = 0; j < context_len; j++) {
//             char c = text[i + j];
//             int idx = char_to_idx.get(c);
//             inputs->data_(i, j) = (float)idx;
//         }

//         char target_char = text[i + context_len];
//         int target_idx = char_to_idx.get(target_char);
//         targets->data_(i, target_idx) = 1.0f;
//     }

//     int embed_dim = 16;
//     int d_k = 16;

//     auto embed = weight(vocab_size, embed_dim, 0.1f);
//     auto qkv_weights = weight(embed_dim, 3 * d_k, 0.1f);
//     auto w_out = weight(d_k, vocab_size, 0.01f);
//     auto b_out = bias(vocab_size);

//     list<Pass> network;
//     network.push(Pass{EMBED, embed});
    
//     Pass reshape_pass;
//     reshape_pass.op = RESHAPE;
//     reshape_pass.i1 = context_len;
//     reshape_pass.i2 = -1;
//     network.push(reshape_pass);
    
//     network.push(Pass{QKV_PROJ, qkv_weights});
    
//     Pass attn_pass;
//     attn_pass.op = ATTENTION;
//     attn_pass.i1 = context_len;
//     attn_pass.i2 = d_k;
//     network.push(attn_pass);
    
//     // ADD THESE:
//     Pass last_pass;
//     last_pass.op = LAST_POSITION;
//     network.push(last_pass);
    
//     network.push(Pass{MATMUL, w_out});
//     network.push(Pass{ADD_BIAS, b_out});
    
//     list<g_ptr<tensor>> params = {embed, qkv_weights, w_out, b_out};

//     g_ptr<t_config> ctx = make<t_config>();
//     ctx->epochs = epochs;
//     ctx->learning_rate = learning_rate;
//     ctx->grad_clip = 1.0f;
//     ctx->reduction = MEAN;
//     ctx->batch_size = 0;

//     print("Training with attention...");
//     train_network(
//         inputs,
//         targets,
//         network,
//         params,
//         SOFTMAX_CE,
//         ctx,
//         logging_interval
//     );

//     // Generate
//     print("\nGenerating text:");
//     std::string seed = text.substr(0, context_len);
//     std::string generated = seed;

//     for(int i = 0; i < 200; i++) {
//         std::string context = generated.substr(generated.length() - context_len);

//         auto input = make<tensor>(MatrixXf(1, context_len));
//         for(int j = 0; j < context_len; j++) {
//             int idx = char_to_idx.get(context[j]);
//             input->data_(0, j) = (float)idx;
//         }

//         auto output = input;
//         for(auto p : network) {
//             output = output->forward(p);
//         }

//         MatrixXf logits = output->data_.row(0);
//         VectorXf m = logits.rowwise().maxCoeff();
//         MatrixXf stable = logits.colwise() - m;
//         MatrixXf exp_vals = stable.array().exp().matrix();
//         float sum = exp_vals.sum();
//         MatrixXf probs = exp_vals / sum;

//         float r = (float)rand() / RAND_MAX;
//         float cumsum = 0.0f;
//         int next_idx = 0;
//         for(int j = 0; j < vocab_size; j++) {
//             cumsum += probs(0, j);
//             if(r <= cumsum) {
//                 next_idx = j;
//                 break;
//             }
//         }

//         generated += idx_to_char[next_idx];
//     }

//     return generated;
// }

//TO ADD:

//NEEDED:
//Module abstraction
//N-dimensonal tensors
//Broadcasting

//Important:
//Device managment
//In-place ops / views
//Customizable autograd

//Convience:
//Distributed training
//Serilization improvments
//Pooling

//Opps for regression testing the MNIST:
//MATMUL, RELU, ADD_BIAS, SOFTMAX_CE
//Golden::u_allocator* tensor::gpu_allocator = nullptr;



int main() {

    int win_size_x = 450;
    int win_size_y = 200;

    #if ON_WINDOWS
        win_size_x*=2;
        win_size_y*=2;
    #endif

    // #ifdef __APPLE__
    //     tensor::gpu_allocator = Golden::create_metal_allocator();
    // #elif defined(__CUDA__)
    //     tensor::gpu_allocator = Golden::create_cuda_allocator();
    // #endif

    Window window = Window(win_size_x, win_size_y, "FirML 0.0.7");
    auto scene = make<Scene>(window,2);
    scene->camera.toIso();
    scene->tickEnvironment(0);
    Data d = helper::make_config(scene,K);
    run_cog_mnist(scene,20000);
    // run_language_test(scene);
    // run_babble_test(scene);
   
    // Log::Line l;
    // l.start();

    // run_mnist(scene,4);

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
    // print("Time: ",l.end()/1000000,"ms");
    // print(result);


    start::run(window,d,[&]{

    });

    return 0;
}




