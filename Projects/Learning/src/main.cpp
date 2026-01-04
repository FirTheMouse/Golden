#include<util/util.hpp>
#include<util/engine_util.hpp>
#include<core/object.hpp>

#include<core/helper.hpp>
#include<util/logger.hpp>

#include <Eigen/Dense>

using namespace Eigen;

enum Opp {
   NOP, ADD, MATMUL, RELU, ADD_BIAS, SIGMOID, SOFTMAX
};

enum LossType {
    MSE, CROSS_ENTROPY
};

class tensor : public Object {
public:
    MatrixXf data_;
    MatrixXf grad_;
    Opp op_ = NOP;

    list<g_ptr<tensor>> inputs_;

    tensor() {
        grad_ = MatrixXf::Zero(0, 0);
    }
    tensor(const MatrixXf& data) : data_(data) {
       grad_ = MatrixXf::Zero(data.rows(), data.cols());
    }

    g_ptr<tensor> get_batch(int start, int batch_size) {
        int rows = std::min(batch_size, (int)data_.rows() - start);
        auto batch = make<tensor>(data_.block(start, 0, rows, data_.cols()));
        return batch;
    }
  
    g_ptr<tensor> forward(const Opp& op, g_ptr<tensor> other = nullptr) {
        g_ptr<tensor> result = make<tensor>();
        if(other) result->inputs_ = {this,other};
        else result->inputs_ = {this}; //For unary
        result->op_ = op;
        switch(op) {
            case NOP: return result;
            case ADD: result->data_ = data_ + other->data_; break;
            case MATMUL: result->data_ = data_ * other->data_; break;
            case RELU: result->data_ = data_.cwiseMax(0.0f); break;
            case SIGMOID: result->data_ = (1.0f / (1.0f + (-data_.array()).exp())).matrix(); break;
            case ADD_BIAS: result->data_ = data_.rowwise() + other->data_.row(0); break;
            case SOFTMAX: {
                auto x = data_;
                VectorXf row_max = x.rowwise().maxCoeff();
                x = x.colwise() - row_max;
                auto ex = x.array().exp();
                VectorXf row_sum = ex.rowwise().sum();
                result->data_ = (ex.colwise() / row_sum.array()).matrix();
                break;
            }
            default:
                print("WARNING: invalid op_ in forward: ",op_);
                break;
        }
        result->grad_ = MatrixXf::Zero(result->data_.rows(), result->data_.cols());
        return result;
    }

    void backward() {
        // if(grad_.isZero() && op_!=NOP) {
        //     grad_ = MatrixXf::Ones(data_.rows(), data_.cols());
        // }

        switch(op_) {
            case NOP: //For leaf or unintilized nodes, where we don't want it running backwards
                return;
            case ADD:
                    inputs_[0]->grad_ += grad_;
                    inputs_[1]->grad_ += grad_;
                    inputs_[0]->backward();
                    inputs_[1]->backward();
                break;
            case MATMUL:
                    inputs_[0]->grad_ += grad_ * inputs_[1]->data_.transpose();
                    inputs_[1]->grad_ += inputs_[0]->data_.transpose() * grad_;
                    inputs_[0]->backward();
                    inputs_[1]->backward();
                break;
            case RELU:
                inputs_[0]->grad_ += grad_.cwiseProduct((inputs_[0]->data_.array() > 0).cast<float>().matrix());
                inputs_[0]->backward();
                break;
            case SIGMOID: {
                auto sig = inputs_[0]->data_;
                auto sig_data = (1.0f / (1.0f + (-sig.array()).exp()));
                inputs_[0]->grad_ += grad_.cwiseProduct((sig_data * (1.0f - sig_data)).matrix());
                inputs_[0]->backward(); 
                break;
            }
            case ADD_BIAS:
                inputs_[0]->grad_ += grad_;
                inputs_[1]->grad_ += grad_.colwise().sum();
                inputs_[0]->backward();
                inputs_[1]->backward();
                break;
            case SOFTMAX: {
                //Currently simplified
                MatrixXf s = data_;          // NOTE: data_ here is softmax output
                MatrixXf g = grad_;
                MatrixXf dot = (g.array() * s.array()).rowwise().sum().matrix(); // (batch x 1)
                inputs_[0]->grad_ += (s.array() * (g.array().colwise() - dot.col(0).array())).matrix();
                inputs_[0]->backward();
                break;
            }
            default:
                print("WARNING: invalid op_ in backward: ",op_);
                break;
        }

    }

    float compute_loss(g_ptr<tensor> pred, g_ptr<tensor> target) {
        return (pred->data_ - target->data_).squaredNorm();
    }

};

struct Pass {
    Opp op = NOP;
    g_ptr<tensor> param = nullptr;
};


float train_step(
    g_ptr<tensor> output,
    g_ptr<tensor> target,
    list<g_ptr<tensor>>& params,
    float learning_rate,
    LossType loss_type
) {

    float loss_value = 0.0f;
    switch(loss_type) {
        case MSE:
            loss_value = (output->data_ - target->data_).squaredNorm();
            output->grad_ = 2.0f * (output->data_ - target->data_);
            break;
        case CROSS_ENTROPY: {
            //CE needs correction to be correct, this is more of a 'happy acciden't in that it works with fused.
            //Reutrn to this once I know more.
            //next step is a fused CE-with-logits and a grad checker

            constexpr float eps = 1e-8f;
            auto probs = output->data_.array().max(eps);
            loss_value = -(target->data_.array() * probs.log()).sum();

            // Gradient: output - target (when combined with softmax)
            output->grad_ = output->data_ - target->data_;
            break;
        }
        default:
            print("WARNING: invalid loss_type in train_step: ",loss_type);
            break;
    }
    
    output->backward();
    
    for(auto& param : params) {
        param->data_ -= learning_rate * param->grad_;
    }
    
    for(auto& param : params) {
        param->grad_.setZero();
    }
    
    return loss_value;
}

g_ptr<tensor> weight(int rows, int cols, float scalar = 0.01f) {
    return make<tensor>(MatrixXf::Random(rows, cols)*scalar);
}

g_ptr<tensor> bias(int cols) {
    return make<tensor>(MatrixXf::Zero(1, cols));
}


void train_network(
    g_ptr<tensor> inputs,
    g_ptr<tensor> targets,
    list<Pass> network,
    list<g_ptr<tensor>> params,
    LossType loss_type,
    int epochs,
    float learning_rate,
    int log_interval = 500
) {
    g_ptr<tensor> output;
    float loss_value = 0;
    
    for(int epoch = 0; epoch < epochs; epoch++) {
        // Forward pass
        output = inputs;
        for(auto p : network) {
            output = output->forward(p.op, p.param);
        }
        
        // Backward pass + update
        loss_value = train_step(output, targets, params, learning_rate, loss_type);
        
        // Logging
        if(epoch % log_interval == 0) {
            float loss = (output->data_ - targets->data_).squaredNorm();
            print("Epoch ",epoch," Loss: ",loss);
        }

        if(std::isnan(loss_value) || std::isinf(loss_value)) {
            print("Exploded at epoch ", epoch);
            print("w1 max: ", params[0]->data_.maxCoeff());
            print("w1 min: ", params[0]->data_.minCoeff());
            break;
        }
    }
    
    // Final results
    print("\nFinal predictions:\n",output->data_);
    print("Target:\n",targets->data_);
}

//Fold into train_network for cleaner API
void train_network_batched(
    g_ptr<tensor> inputs,
    g_ptr<tensor> targets,
    list<Pass> network,
    list<g_ptr<tensor>> params,
    LossType loss_type,
    int epochs,
    int batch_size,
    float learning_rate,
    int log_interval = 500
) {
    int num_samples = inputs->data_.rows();
    g_ptr<tensor> output;
    for(int epoch = 0; epoch < epochs; epoch++) {
        float epoch_loss = 0.0f;
        int num_batches = 0;
        
        for(int i = 0; i < num_samples; i += batch_size) {
            auto input_batch = inputs->get_batch(i, batch_size);
            auto target_batch = targets->get_batch(i, batch_size);

            // if(epoch == 0 && i == 0) {
            //     print("First batch input:\n", input_batch->data_);
            //     print("First batch target:\n", target_batch->data_);
            // }
            

            output = input_batch;
            for(auto p : network) {
                output = output->forward(p.op, p.param);
            }
            

            float loss = train_step(output, target_batch, params, learning_rate, CROSS_ENTROPY);
            epoch_loss += loss;
            num_batches++;

            if(std::isnan(loss) || std::isinf(loss)) {
                print("Exploded at epoch ", epoch);
                print("w1 max: ", params[0]->data_.maxCoeff());
                print("w1 min: ", params[0]->data_.minCoeff());
                break;
            }
        }
        
        if(log_interval>0) {
            if(epoch % log_interval == 0) {
                print("Epoch ", epoch, " Loss: ", epoch_loss / num_batches);
            }
        }
    }

    auto final_output = inputs;
    for(auto p : network) {
        final_output = final_output->forward(p.op, p.param);
    }

    if(log_interval>0) {
        print("\nFinal predictions:\n", final_output->data_);
        print("Target:\n", targets->data_);
    }
}





// Utility to read big-endian integers
int32_t read_int(std::ifstream& file) {
    unsigned char bytes[4];
    file.read((char*)bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

std::pair<g_ptr<tensor>, g_ptr<tensor>> load_mnist(
    const std::string& image_file,
    const std::string& label_file,
    int max_samples = -1  // -1 = load all
) {
    // Load images
    std::ifstream img_stream(image_file, std::ios::binary);
    if(!img_stream) throw std::runtime_error("Can't open " + image_file);
    
    int magic = read_int(img_stream);
    int num_images = read_int(img_stream);
    int rows = read_int(img_stream);
    int cols = read_int(img_stream);
    
    if(max_samples > 0) num_images = std::min(num_images, max_samples);
    
    auto images = make<tensor>(MatrixXf(num_images, rows * cols));
    
    for(int i = 0; i < num_images; i++) {
        for(int j = 0; j < rows * cols; j++) {
            unsigned char pixel;
            img_stream.read((char*)&pixel, 1);
            images->data_(i, j) = pixel / 255.0f;  // normalize to [0,1]
        }
    }
    
    // Load labels
    std::ifstream lbl_stream(label_file, std::ios::binary);
    if(!lbl_stream) throw std::runtime_error("Can't open " + label_file);
    
    magic = read_int(lbl_stream);
    int num_labels = read_int(lbl_stream);
    
    if(max_samples > 0) num_labels = std::min(num_labels, max_samples);
    
    auto labels = make<tensor>(MatrixXf::Zero(num_labels, 10));
    
    for(int i = 0; i < num_labels; i++) {
        unsigned char label;
        lbl_stream.read((char*)&label, 1);
        labels->data_(i, label) = 1.0f;  // one-hot encoding
    }
    
    return {images, labels};
}

unsigned int mnist_to_texture(g_ptr<tensor> images, int img_index) {
    // Extract one 28x28 image
    int w = 28, h = 28;
    unsigned char* data = new unsigned char[w * h * 4];  // RGBA
    
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            // Flip vertically: read from bottom to top
            int flipped_i = (h - 1) - i;
            float val = images->data_(img_index, flipped_i * w + j);
            unsigned char pixel = (unsigned char)(val * 255.0f);
            
            int idx = (i * w + j) * 4;
            data[idx + 0] = pixel;
            data[idx + 1] = pixel;
            data[idx + 2] = pixel;
            data[idx + 3] = 255;
        }
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Nearest for pixel art look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    delete[] data;
    return tex;
}

int main() {
    //1280, 768
    Window window = Window(450,200, "FirML 0.0.2");
    auto scene = make<Scene>(window,2);
    scene->camera.toIso();
    scene->tickEnvironment(0);
    Data d = helper::make_config(scene,K);
    auto source_code = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",100);

    int amt = 1000;

    auto [train_imgs, train_labels] = load_mnist(
        root()+"/Projects/Learning/assets/images/train-images-idx3-ubyte", 
        root()+"/Projects/Learning/assets/images/train-labels-idx1-ubyte", 
        amt
    );
    auto [test_imgs, test_labels] = load_mnist(
        root()+"/Projects/Learning/assets/images/t10k-images-idx3-ubyte", 
        root()+"/Projects/Learning/assets/images/t10k-labels-idx1-ubyte", 
        amt
    );
    print("Loaded ", train_imgs->data_.rows(), " images");

    // g_ptr<Quad> q = make<Quad>();
    // scene->add(q);
    // q->setTexture(mnist_to_texture(train_imgs, 0), 0);
    // q->scale(vec2(28 * 10, 28 * 10));  // Scale up 10x so you can see it



    auto w1 = weight(784, 128, 0.1f);
    auto b1 = bias(128);
    auto w2 = weight(128, 10, 0.1f);
    auto b2 = bias(10);

    list<Pass> network = {
        {MATMUL, w1}, {ADD_BIAS, b1}, {RELU},
        {MATMUL, w2}, {ADD_BIAS, b2}, {SOFTMAX}
    };
    list<g_ptr<tensor>> params = {w1, b1, w2, b2};

    Log::Line l;
    l.start();

    train_network_batched(train_imgs, train_labels, network, params, CROSS_ENTROPY, 10, 64, 0.01f, 1);

    double time = l.end();
    print("Took ",time/1000000,"ms");

    // Run forward pass on test data
    auto test_output = test_imgs;
    for(auto p : network) {
        test_output = test_output->forward(p.op, p.param);
    }

    // Calculate accuracy
    int correct = 0;
    for(int i = 0; i < test_output->data_.rows(); i++) {
        // Find predicted class (max value in row)
        int pred_class = 0;
        float max_val = test_output->data_(i, 0);
        for(int j = 1; j < 10; j++) {
            if(test_output->data_(i, j) > max_val) {
                max_val = test_output->data_(i, j);
                pred_class = j;
            }
        }
        
        // Find true class (which column is 1 in one-hot encoding)
        int true_class = 0;
        for(int j = 0; j < 10; j++) {
            if(test_labels->data_(i, j) == 1.0f) {
                true_class = j;
                break;
            }
        }
        
        if(pred_class == true_class) correct++;
    }

    float accuracy = 100.0f * correct / test_output->data_.rows();
    print("Test Accuracy: ", accuracy, "%");
    
    list<int> indices = {randi(0, test_imgs->data_.rows() - 1), randi(0, test_imgs->data_.rows() - 1), randi(0, test_imgs->data_.rows() - 1)};
  

    for(int i = 0; i < 3; i++) {
        int idx = indices[i];
        
        // Display the image
        g_ptr<Quad> q = make<Quad>();
        scene->add(q);
        q->setTexture(mnist_to_texture(test_imgs, idx), 0);
        q->setPosition({(float)(i * 300.0f), 0});  // Space them out
        q->scale(vec2(280, 280));
        
        // Get prediction
        auto single_img = test_imgs->get_batch(idx, 1);
        auto output = single_img;
        for(auto p : network) {
            output = output->forward(p.op, p.param);
        }
        
        // Find predicted class (max output)
        int predicted = 0;
        for(int j = 1; j < 10; j++) {
            if(output->data_(0, j) > output->data_(0, predicted)) {
                predicted = j;
            }
        }
        
        // Display text showing prediction
        print("Image ", i, ": Predicted ", predicted);
        text::makeText(std::to_string(predicted),source_code,scene,{100.0f+(float)(i * 300.0f), 350},1);
    }

    start::run(window,d,[&]{

    });

    return 0;
}


//CE V2:
// constexpr float eps = 1e-8f;

// auto probs = output->data_.array().max(eps);
// int bs = output->data_.rows();

// loss_value = -(target->data_.array() * probs.log()).rowwise().sum().mean();
// output->grad_ = (output->data_ - target->data_) / float(bs);

//CE V3:
// constexpr float eps = 1e-8f;
// int bs = output->data_.rows();

// auto probs = output->data_.array().max(eps);

// // mean CE over batch
// loss_value = -(target->data_.array() * probs.log()).rowwise().sum().mean();

// // dL/ds = -y / s  (and divide by batch if you're using mean loss)
// output->grad_ = (-(target->data_.array() / probs) / float(bs)).matrix();

