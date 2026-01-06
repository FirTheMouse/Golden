#pragma once

#include<ml_util.hpp>
#include<core/gpuType.hpp>
#include<methods.hpp>

enum Opp {
    NOP, ADD, MATMUL, RELU, ADD_BIAS, SIGMOID, SOFTMAX, SOFTMAX_CE,
    SIGMOID_BCE, TANH, EMBED, CONCAT,
    MSE, CROSS_ENTROPY, //Loss types
    RNN_STEP, BATCH_NORM,  //Stateful
    ATTENTION, QKV_PROJ, RESHAPE, LAST_POSITION, DROPOUT
};

enum Reduction { NONE, MEAN, SUM };

inline bool opp_is_fused(const Opp& op) {
    return (op==SOFTMAX_CE||op==SIGMOID_BCE);
}

list<int> create_shuffle_indices(int n) {
    list<int> indices;
    for(int i = 0; i < n; i++) indices.push(i);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    for(int i = n-1; i > 0; i--) {
        std::uniform_int_distribution<> dis(0, i);
        int j = dis(gen);
        std::swap(indices[i], indices[j]);
    }
    return indices;
}

enum Device { CPU, GPU };

enum Mode { TRAIN, EVAL };
 
namespace Eigen {

    class tensor;

    struct Pass {
        Opp op = NOP;
        g_ptr<tensor> param = nullptr;
        g_ptr<tensor> state = nullptr;

        //All these extra paramaters are here to add flexiblity to the operations, so we can accomdate
        //everything with minimal changes in the future
        
        // Integer slots - operations define their own semantics
        int i1 = 0;
        int i2 = 0;
        int i3 = 0;
        int i4 = 0;
        
        // Float slots
        float f1 = 0.0f;
        float f2 = 0.0f;
        float f3 = 0.0f;
        
        // Boolean slots
        bool b1 = false;
        bool b2 = false;
        bool b3 = false;
        
        // Variable-length integer data (for shapes, axes, slicing)
        list<int> ints;
        
        // Common reduction type for losses and reductions
        Reduction reduction = MEAN;
        Mode mode = TRAIN;
    };

 class tensor : public Object {
 public:
     MatrixXf data_;
     MatrixXf grad_;
     map<std::string, MatrixXf> cache_;

     void* gpu_data_ = nullptr;
     void* gpu_grad_ = nullptr;
     Device device_ = CPU;

    inline static Golden::u_allocator* gpu_allocator;

     Opp op_ = NOP;
     Reduction reduction_ = MEAN;
 
     list<g_ptr<tensor>> inputs_;
 
     tensor() {
         grad_ = MatrixXf::Zero(0, 0);
     }
     tensor(const MatrixXf& data) : data_(data) {
        grad_ = MatrixXf::Zero(data.rows(), data.cols());
     }

     ~tensor() {
        if (gpu_data_ && gpu_allocator) {
            gpu_allocator->free(gpu_data_);
        }
        if (gpu_grad_ && gpu_allocator) {
            gpu_allocator->free(gpu_grad_);
        }

        detach();
    }

    void detach() {
        inputs_.clear();
        op_ = NOP;
        cache_.clear();
    }
    
    static void clear_graph(g_ptr<tensor> root) {
        // Recursively clear computational graph
        if(!root || root->inputs_.empty()) return;
        
        for(auto& input : root->inputs_) {
            clear_graph(input);
        }
        root->detach();
    }

    void to_gpu() {
        if (device_ == CPU && gpu_allocator) {
            size_t bytes = data_.size() * sizeof(float);
            gpu_data_ = gpu_allocator->allocate(bytes);
            size_t grad_bytes = grad_.size() * sizeof(float);
            if (grad_bytes > 0) {
                gpu_grad_ = gpu_allocator->allocate(grad_bytes);
                gpu_allocator->memset_gpu(gpu_grad_, 0, grad_bytes);
            }
            // Transpose the data during copy since MPS expects row-major
            MatrixXf transposed = data_.transpose();
            gpu_allocator->copy_to_gpu(gpu_data_, transposed.data(), bytes);
            device_ = GPU;
        }
    }

    void to_cpu() {
        if (device_ != CPU && gpu_data_ && gpu_allocator) {
            size_t bytes = data_.size() * sizeof(float);
            // Copy from GPU to a temporary buffer
            MatrixXf temp(data_.cols(), data_.rows());
            gpu_allocator->copy_to_cpu(temp.data(), gpu_data_, bytes);
            
            // Transpose back to column-major for Eigen
            data_ = temp.transpose();
            
            device_ = CPU;
        }
    }

    void sync_to_cpu() {
        // Copy GPU data to CPU for inspection, but stay marked as GPU tensor
        if (device_ == GPU && gpu_data_ && gpu_allocator) {
            size_t bytes = data_.size() * sizeof(float);
            MatrixXf temp(data_.cols(), data_.rows());
            gpu_allocator->copy_to_cpu(temp.data(), gpu_data_, bytes);
            data_ = temp.transpose();
        }
    }
 
     g_ptr<tensor> get_batch(int start, int batch_size) {
         int rows = std::min(batch_size, (int)data_.rows() - start);
         auto batch = make<tensor>(data_.block(start, 0, rows, data_.cols()));
         return batch;
     }

     g_ptr<tensor> get_batch_by_indices(const list<int>& indices) {
        auto result = make<tensor>(MatrixXf(indices.length(), data_.cols()));
        for(int i = 0; i < indices.length(); i++) {
            result->data_.row(i) = data_.row(indices[i]);
        }
        return result;
    }

    g_ptr<tensor> operate(bool is_forward, Pass& pass) {
        
        g_ptr<tensor> result = nullptr;
        Opp op = op_;
        g_ptr<tensor> other = nullptr;

        //Unifying the forward and backward pass to make opp maintance and addition 
        //easier by having the forward and backward passs next to each other
        if(is_forward) {
            result = make<tensor>();
            op = pass.op;
            other = pass.param;

            if(other) result->inputs_ = {this,other};
            else result->inputs_ = {this}; //For unary
    
            result->op_ = op;
            result->reduction_ = pass.reduction;
        }

        switch(op) {
            case NOP: 
                return result;
            case ADD: {
                if(is_forward) {
                    result->data_ = data_ + other->data_;
                } 
                else {
                    inputs_[0]->grad_ += grad_;
                    inputs_[1]->grad_ += grad_;
                    inputs_[0]->operate(false,pass);
                    inputs_[1]->operate(false,pass);
                }
                break; 
            }
            case MATMUL: {
                if(is_forward) {
                    if (device_ == CPU && other->device_ == CPU) {
                        result->data_ = data_ * other->data_;
                        result->device_ = CPU;
                    }
                    else if (device_ == GPU && other->device_ == GPU) {
                        result->gpu_data_ = matmul(
                            gpu_data_, data_.rows(), data_.cols(),
                            other->gpu_data_, other->data_.rows(), other->data_.cols()
                        );
                        result->device_ = GPU;
                        result->data_.resize(data_.rows(), other->data_.cols());
                    }
                    else {
                        if (device_ != CPU) to_cpu();
                        if (other->device_ != CPU) other->to_cpu();
                        result->data_ = data_ * other->data_;
                        result->device_ = CPU;
                    }
                } 
                else {
                    if (device_ == GPU && 
                        inputs_[0]->device_ == GPU && 
                        inputs_[1]->device_ == GPU) {
                        void* grad_a_buffer = matmul(
                            gpu_grad_, data_.rows(), data_.cols(),inputs_[1]->gpu_data_, 
                            inputs_[1]->data_.cols(), inputs_[1]->data_.rows()  
                        );
                        
                        add_inplace(inputs_[0]->gpu_grad_, grad_a_buffer, 
                                        inputs_[0]->data_.rows() * inputs_[0]->data_.cols());
                        
                        gpu_allocator->free(grad_a_buffer);
                        
                        void* grad_b_buffer = matmul(
                            inputs_[0]->gpu_data_, inputs_[0]->data_.cols(), inputs_[0]->data_.rows(),  
                            gpu_grad_, data_.rows(), data_.cols() 
                        );
                        
                        add_inplace(inputs_[1]->gpu_grad_, grad_b_buffer,
                                        inputs_[1]->data_.rows() * inputs_[1]->data_.cols());
                        
                        gpu_allocator->free(grad_b_buffer);
                    } else {
                        inputs_[0]->grad_ += grad_ * inputs_[1]->data_.transpose();
                        inputs_[1]->grad_ += inputs_[0]->data_.transpose() * grad_;
                    }
                    
                    inputs_[0]->operate(false,pass);
                    inputs_[1]->operate(false,pass);
                }
                break;
            }
            case RELU: {
                if(is_forward) {
                    if (device_ == GPU) {
                        result->gpu_data_ = relu(gpu_data_, data_.rows(), data_.cols(), 
                                                    gpu_allocator);
                        result->data_.resize(data_.rows(), data_.cols());
                        result->device_ = GPU;
                    } else {
                        result->data_ = data_.cwiseMax(0.0f);
                        result->device_ = CPU;
                    }
                }
                else {
                    inputs_[0]->grad_ += grad_.cwiseProduct((inputs_[0]->data_.array() > 0).cast<float>().matrix());
                    inputs_[0]->operate(false,pass);
                }
                break;
            }
            case SIGMOID: {
                if(is_forward) {
                    result->data_ = (1.0f / (1.0f + (-data_.array()).exp())).matrix();
                }
                else {
                    auto sig = inputs_[0]->data_;
                    auto sig_data = (1.0f / (1.0f + (-sig.array()).exp()));
                    inputs_[0]->grad_ += grad_.cwiseProduct((sig_data * (1.0f - sig_data)).matrix());
                    inputs_[0]->operate(false,pass); 
                }
                break;
            }
            case ADD_BIAS: {
                if(is_forward) {
                    if (device_ == GPU && other->device_ == GPU) {
                        result->gpu_data_ = add_bias(gpu_data_, data_.rows(), data_.cols(), 
                                                        other->gpu_data_, gpu_allocator);
                        result->data_.resize(data_.rows(), data_.cols());
                        result->device_ = GPU;
                    } else {
                        result->data_ = data_.rowwise() + other->data_.row(0);
                        result->device_ = CPU;
                    }
                } 
                else {
                    inputs_[0]->grad_ += grad_;
                    inputs_[1]->grad_ += grad_.colwise().sum();
                    inputs_[0]->operate(false,pass);
                    inputs_[1]->operate(false,pass);
                }
                break;
            }
            case SOFTMAX: {
                if(is_forward) {
                    auto x = data_;
                    VectorXf row_max = x.rowwise().maxCoeff();
                    x = x.colwise() - row_max;
                    auto ex = x.array().exp();
                    VectorXf row_sum = ex.rowwise().sum();
                    result->data_ = (ex.colwise() / row_sum.array()).matrix();
                }
                else {
                    MatrixXf s = data_;          // NOTE: data_ here is softmax output
                    MatrixXf g = grad_;
                    MatrixXf dot = (g.array() * s.array()).rowwise().sum().matrix(); // (batch x 1)
                    inputs_[0]->grad_ += (s.array() * (g.array().colwise() - dot.col(0).array())).matrix();
                    inputs_[0]->operate(false,pass);
                }
                break;
            }
            case SOFTMAX_CE: {
                if(is_forward) {
                    int bs = data_.rows();
                    MatrixXf x = data_;
                    
                    VectorXf m = x.rowwise().maxCoeff();
                    x = x.colwise() - m; 
                    MatrixXf ex = x.array().exp().matrix();
                    VectorXf sumex = ex.rowwise().sum();
                    VectorXf lse = sumex.array().log() + m.array(); 
        
                    MatrixXf log_softmax = data_.colwise() - lse;
                    
                    float loss_sum = -(other->data_.array() * log_softmax.array()).sum();
                    
                    float loss = (pass.reduction == MEAN) ? loss_sum / float(bs) : loss_sum;
                    
                    result->data_ = MatrixXf::Constant(1, 1, loss);
                    result->inputs_ = {this, other};
                }
                else {
                    int bs = inputs_[0]->data_.rows();
                    MatrixXf x = inputs_[0]->data_;
                    VectorXf m = x.rowwise().maxCoeff();
                    x = x.colwise() - m;
                    MatrixXf ex = x.array().exp().matrix();
                    VectorXf sumex = ex.rowwise().sum();
                    MatrixXf softmax = ex.array().colwise() / sumex.array();
                    MatrixXf grad_logits = softmax - inputs_[1]->data_;
                    if(reduction_ == MEAN) {
                        grad_logits /= float(bs);
                    }

                    inputs_[0]->grad_ += grad_logits;               
                    inputs_[0]->operate(false,pass);
                }
                break;
        } 
            case SIGMOID_BCE: {
                    if(is_forward) {
                            int bs = data_.rows();
                            
                            MatrixXf x = data_;
                            MatrixXf y = other->data_;
                            
                            MatrixXf max_val = x.cwiseMax(0.0f);
                            MatrixXf stable_bce = (max_val.array() - x.array() * y.array() + 
                            ((-x.array().abs()).exp() + 1.0f).log()).matrix();
                            
                            float loss_sum = stable_bce.sum();
                            float loss = (pass.reduction == MEAN) ? loss_sum / float(bs) : loss_sum;
                            
                            result->data_ = MatrixXf::Constant(1, 1, loss);
                            result->inputs_ = {this, other};
                        } 
                    else {
                        int bs = inputs_[0]->data_.rows();
                        MatrixXf sigmoid = (1.0f / (1.0f + (-inputs_[0]->data_.array()).exp())).matrix();
                        MatrixXf grad_logits = sigmoid - inputs_[1]->data_;
                        if(reduction_ == MEAN) {
                            grad_logits /= float(bs);
                        }
                        
                        inputs_[0]->grad_ += grad_logits;
                        inputs_[0]->operate(false,pass);
                    }
                    break;
            }
            case TANH: {
                    if(is_forward) {
                        result->data_ = data_.array().tanh().matrix();
                    }
                    else {
                        MatrixXf tanh_output = data_;
                        MatrixXf tanh_grad = (1.0f - tanh_output.array().square()).matrix();
                        inputs_[0]->grad_ += grad_.cwiseProduct(tanh_grad);
                        inputs_[0]->operate(false,pass);
                    }
                break;
            }
            case EMBED: {
                    if(is_forward) {
                        int batch_size = data_.rows();
                        int seq_len = data_.cols();
                        int embed_dim = other->data_.cols();
                        result->data_ = MatrixXf(batch_size, seq_len * embed_dim);
                        
                        for(int i = 0; i < batch_size; i++) {
                            for(int j = 0; j < seq_len; j++) {
                                int idx = (int)data_(i, j);
                                result->data_.block(i, j * embed_dim, 1, embed_dim) = 
                                    other->data_.row(idx);
                            }
                        }
                    } 
                    else {
                        int batch_size = inputs_[0]->data_.rows();
                        int seq_len = inputs_[0]->data_.cols();
                        int embed_dim = inputs_[1]->data_.cols();
                        
                        for(int i = 0; i < batch_size; i++) {
                            for(int j = 0; j < seq_len; j++) {
                                int idx = (int)inputs_[0]->data_(i, j);
                                inputs_[1]->grad_.row(idx) += 
                                    grad_.block(i, j * embed_dim, 1, embed_dim);
                            }
                        }
                        inputs_[1]->operate(false,pass);
                    }
                    break;
            }
            case CONCAT: {
                    if(is_forward) {
                        result->data_ = MatrixXf(data_.rows(), data_.cols() + other->data_.cols());
                        result->data_.leftCols(data_.cols()) = data_;
                        result->data_.rightCols(other->data_.cols()) = other->data_;
                    }
                    else {
                        inputs_[0]->grad_ += grad_.leftCols(inputs_[0]->data_.cols());
                        inputs_[1]->grad_ += grad_.rightCols(inputs_[1]->data_.cols());
                        inputs_[0]->operate(false,pass);
                        inputs_[1]->operate(false,pass);
                    }
                break;
            }  
            case RNN_STEP: {   
                    if(is_forward) {          
                        int batch_size = data_.rows();
                        int input_dim = data_.cols();
                        int hidden_dim = other->data_.cols();
                        
                        if(!pass.state) {
                            pass.state = make<tensor>(MatrixXf::Zero(batch_size, hidden_dim));
                        }
                        
                        MatrixXf prev_h = pass.state->data_;
                        
                        MatrixXf concat_input(batch_size, input_dim + hidden_dim);
                        concat_input.leftCols(input_dim) = data_;
                        concat_input.rightCols(hidden_dim) = prev_h;
                        
                        MatrixXf pre_activation = concat_input * other->data_;
                        result->data_ = pre_activation.array().tanh().matrix();
                        
                        auto concat_tensor = make<tensor>(concat_input);
                        concat_tensor->op_ = NOP;
                        result->inputs_ = {this, other, concat_tensor};
                        result->op_ = RNN_STEP;
                        
                        pass.state->data_ = result->data_;
                        pass.state->grad_ = MatrixXf::Zero(batch_size, hidden_dim);
                    }
                    else {
                        int batch_size = data_.rows();
                        int hidden_dim = data_.cols();
                        int input_dim = inputs_[0]->data_.cols();
                        
                        MatrixXf tanh_output = data_;
                        MatrixXf grad_tanh = (1.0f - tanh_output.array().square()).matrix();
                        MatrixXf grad_pre_activation = grad_.cwiseProduct(grad_tanh);
                        
                        MatrixXf concat_input = inputs_[2]->data_;
                        inputs_[1]->grad_ += concat_input.transpose() * grad_pre_activation;
                        
                        MatrixXf grad_concat = grad_pre_activation * inputs_[1]->data_.transpose();
                        
                        MatrixXf grad_input = grad_concat.leftCols(input_dim);
                        MatrixXf grad_prev_hidden = grad_concat.rightCols(hidden_dim);
                        
                        inputs_[0]->grad_ += grad_input;
                        
                        inputs_[0]->operate(false,pass);
                        inputs_[1]->operate(false,pass);
                    }
                    break;
            } 
            case ATTENTION: {   
                    if(is_forward) {             
                        int seq_len = pass.i1;
                        int d_k = pass.i2;
                        
                        // Split Q, K, V
                        MatrixXf Q = data_.leftCols(d_k);
                        MatrixXf K = data_.middleCols(d_k, d_k);
                        MatrixXf V = data_.rightCols(d_k);
                        
                        // Compute scores: Q * K^T / sqrt(d_k)
                        float scale = 1.0f / std::sqrt((float)d_k);
                        MatrixXf scores = (Q * K.transpose()) * scale;
                        
                        // Softmax row-wise
                        MatrixXf attention_weights(seq_len, seq_len);
                        for(int i = 0; i < seq_len; i++) {
                            VectorXf row = scores.row(i);
                            float max_val = row.maxCoeff();
                            VectorXf stable = row.array() - max_val;
                            VectorXf exp_vals = stable.array().exp();
                            attention_weights.row(i) = exp_vals / exp_vals.sum();
                        }
                        
                        // Output: attention_weights * V
                        result->data_ = attention_weights * V;
                        result->inputs_ = {this};
                        result->op_ = ATTENTION;
                    }
                    else {
                        // Get dimensions from Pass, not from data
                        int seq_len = inputs_[0]->data_.rows();  // This should match the sequence length
                        int d_k = data_.cols();  // Output dimension
                        float scale = 1.0f / std::sqrt((float)d_k);
                        
                        // Re-compute Q, K, V from input
                        MatrixXf Q = inputs_[0]->data_.leftCols(d_k);
                        MatrixXf K = inputs_[0]->data_.middleCols(d_k, d_k);
                        MatrixXf V = inputs_[0]->data_.rightCols(d_k);
                        
                        // Re-compute attention weights
                        MatrixXf scores = (Q * K.transpose()) * scale;
                        MatrixXf attn_weights(seq_len, seq_len);
                        for(int i = 0; i < seq_len; i++) {
                            VectorXf row = scores.row(i);
                            float max_val = row.maxCoeff();
                            VectorXf stable = row.array() - max_val;
                            VectorXf exp_vals = stable.array().exp();
                            attn_weights.row(i) = exp_vals / exp_vals.sum();
                        }
                        
                        // Backprop through attention
                        MatrixXf grad_V = attn_weights.transpose() * grad_;
                        MatrixXf grad_attn = grad_ * V.transpose();
                        
                        // Backprop through softmax
                        MatrixXf grad_scores(seq_len, seq_len);
                        for(int i = 0; i < seq_len; i++) {
                            VectorXf attn_row = attn_weights.row(i);
                            VectorXf grad_row = grad_attn.row(i);
                            float dot = grad_row.dot(attn_row);
                            grad_scores.row(i) = (grad_row.array() - dot) * attn_row.array();
                        }
                        grad_scores *= scale;
                        
                        // Backprop to Q and K
                        MatrixXf grad_Q = grad_scores * K;
                        MatrixXf grad_K = grad_scores.transpose() * Q;
                        
                        // Accumulate gradients
                        inputs_[0]->grad_.leftCols(d_k) += grad_Q;
                        inputs_[0]->grad_.middleCols(d_k, d_k) += grad_K;
                        inputs_[0]->grad_.rightCols(d_k) += grad_V;
                        
                        inputs_[0]->operate(false,pass);
                    }
                break;
            } 
            case QKV_PROJ: {     
                    if(is_forward) {           
                        int seq_len = data_.rows();
                        int embed_dim = data_.cols();
                        int d_k = other->data_.cols() / 3;
                        
                        MatrixXf W_q = other->data_.leftCols(d_k);
                        MatrixXf W_k = other->data_.middleCols(d_k, d_k);
                        MatrixXf W_v = other->data_.rightCols(d_k);
                        
                        MatrixXf Q = data_ * W_q;
                        MatrixXf K = data_ * W_k;
                        MatrixXf V = data_ * W_v;
                        
                        result->data_ = MatrixXf(seq_len, 3 * d_k);
                        result->data_.leftCols(d_k) = Q;
                        result->data_.middleCols(d_k, d_k) = K;
                        result->data_.rightCols(d_k) = V;
                        
                        result->inputs_ = {this, other};
                        result->op_ = QKV_PROJ;
                    }
                    else {
                        int seq_len = inputs_[0]->data_.rows();
                        int d_k = data_.cols() / 3;
                        int embed_dim = inputs_[0]->data_.cols();
                        
                        MatrixXf W_q = inputs_[1]->data_.leftCols(d_k);
                        MatrixXf W_k = inputs_[1]->data_.middleCols(d_k, d_k);
                        MatrixXf W_v = inputs_[1]->data_.rightCols(d_k);
                        
                        MatrixXf grad_Q = grad_.leftCols(d_k);
                        MatrixXf grad_K = grad_.middleCols(d_k, d_k);
                        MatrixXf grad_V = grad_.rightCols(d_k);
                        
                        // Gradient w.r.t. input
                        inputs_[0]->grad_ += grad_Q * W_q.transpose() + grad_K * W_k.transpose() + grad_V * W_v.transpose();
                        
                        // Gradient w.r.t. weights
                        inputs_[1]->grad_.leftCols(d_k) += inputs_[0]->data_.transpose() * grad_Q;
                        inputs_[1]->grad_.middleCols(d_k, d_k) += inputs_[0]->data_.transpose() * grad_K;
                        inputs_[1]->grad_.rightCols(d_k) += inputs_[0]->data_.transpose() * grad_V;
                        
                        inputs_[0]->operate(false,pass);
                        inputs_[1]->operate(false,pass);
                    }
                break;
            } 
            case RESHAPE: {
                    if(is_forward) {
                        int new_rows = pass.i1;
                        int new_cols = pass.i2;
                        
                        // Handle -1 to mean "infer this dimension"
                        if(new_rows == -1 && new_cols == -1) {
                            print("ERROR: RESHAPE can't infer both dimensions");
                            result->data_ = data_;
                            break;
                        }
                        
                        int total_elements = data_.size();
                        
                        if(new_rows == -1) {
                            new_rows = total_elements / new_cols;
                        } else if(new_cols == -1) {
                            new_cols = total_elements / new_rows;
                        }
                        
                        if(total_elements != new_rows * new_cols) {
                            print("ERROR: RESHAPE size mismatch. Have ", total_elements, " elements, trying to reshape to ", new_rows, "x", new_cols);
                            result->data_ = data_;
                            break;
                        }
                        
                        result->data_ = MatrixXf::Map(data_.data(), new_rows, new_cols);
                        result->inputs_ = {this};
                        result->op_ = RESHAPE;
                    } else {
                        // Gradient flows back through reshape by reshaping the gradient back to original shape
                        int orig_rows = inputs_[0]->data_.rows();
                        int orig_cols = inputs_[0]->data_.cols();
                        
                        inputs_[0]->grad_ += MatrixXf::Map(grad_.data(), orig_rows, orig_cols);
                        inputs_[0]->operate(false,pass);
                    }
                break;
            } 
            case LAST_POSITION: {
                if(is_forward) {
                    int rows = data_.rows();
                    int cols = data_.cols();
                    result->data_ = data_.row(rows - 1).eval();
                    result->inputs_ = {this};
                    result->op_ = LAST_POSITION;
                }
                else {
                    int rows = inputs_[0]->data_.rows();
                
                    // Gradient only flows to the last position
                    inputs_[0]->grad_.row(rows - 1) += grad_.row(0);
                    inputs_[0]->operate(false,pass);
                }
            break;
        }
            case DROPOUT: {
                if(is_forward) {
                    float dropout_rate = pass.f1;
                    if(pass.mode == TRAIN) {
                        float keep_prob = 1.0f - dropout_rate;
                        MatrixXf mask = (MatrixXf::Random(data_.rows(), data_.cols()).array() > 
                                        -1.0f + 2.0f * keep_prob).cast<float>() / keep_prob;
                        result->data_ = data_.cwiseProduct(mask);
                        result->cache_["mask"] = mask; 
                    } else {
                        result->data_ = data_;
                    }
                } else {
                    if(cache_.hasKey("mask")) {
                        inputs_[0]->grad_ += grad_.cwiseProduct(cache_["mask"]);
                    } else {
                        inputs_[0]->grad_ += grad_;
                    }
                    inputs_[0]->operate(false, pass);
                }
                break;
            }    
        default:
                print("WARNING: invalid op_ in forward: ",op_);
                break;
        }

        if(is_forward) {
            if (result->device_ == GPU && gpu_allocator) {
                size_t grad_bytes = result->data_.rows() * result->data_.cols() * sizeof(float);
                result->gpu_grad_ = gpu_allocator->allocate(grad_bytes);
                gpu_allocator->memset_gpu(result->gpu_grad_, 0, grad_bytes);
            }
        
            result->grad_ = MatrixXf::Zero(result->data_.rows(), result->data_.cols());
        }
        return result;
    }

    g_ptr<tensor> forward(Pass& pass) {
        return operate(true,pass);
    }

    void backward() {
        Pass dummy;
        operate(false,dummy);
        return;
    }

};
 
 
 
 
}