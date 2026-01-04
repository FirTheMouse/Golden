#pragma once

#include<ml_util.hpp>
#include<core/gpuType.hpp>
#include<methods.hpp>

enum Opp {
    NOP, ADD, MATMUL, RELU, ADD_BIAS, SIGMOID, SOFTMAX, SOFTMAX_CE,
    SIGMOID_BCE,      
    MSE, CROSS_ENTROPY
};

enum Reduction { NONE, MEAN, SUM };

inline bool opp_is_fused(const Opp& op) {
    return (op==SOFTMAX_CE||op==SIGMOID_BCE);
}

enum Device {
    CPU,
    GPU
};
 
namespace Eigen {

    class tensor;

    struct Pass {
        Opp op = NOP;
        g_ptr<tensor> param = nullptr;
        
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
    };

 class tensor : public Object {
 public:
     MatrixXf data_;
     MatrixXf grad_;

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
   
     g_ptr<tensor> forward(const Pass& pass) {
         g_ptr<tensor> result = make<tensor>();

         Opp op = pass.op;
         g_ptr<tensor> other = pass.param;

         if(other) result->inputs_ = {this,other};
         else result->inputs_ = {this}; //For unary
         result->op_ = op;
         result->reduction_ = pass.reduction;
         switch(op) {
             case NOP: return result;
             case ADD: result->data_ = data_ + other->data_; break;
             case MATMUL:
                if (device_ == CPU && other->device_ == CPU) {
                    result->data_ = data_ * other->data_;
                    result->device_ = CPU;
                }
                else if (device_ == GPU && other->device_ == GPU) {
                    result->gpu_data_ = matmul(
                        gpu_data_, data_.rows(), data_.cols(),
                        other->gpu_data_, other->data_.rows(), other->data_.cols()
                    );
                    
                    result->data_.resize(data_.rows(), other->data_.cols());
                }
                else {
                    if (device_ != CPU) to_cpu();
                    if (other->device_ != CPU) other->to_cpu();
                    result->data_ = data_ * other->data_;
                    result->device_ = CPU;
                }
            break;
             case RELU:  
                if (device_ == GPU) {
                    result->gpu_data_ = relu(gpu_data_, data_.rows(), data_.cols(), 
                                                gpu_allocator);
                    result->data_.resize(data_.rows(), data_.cols());
                    result->device_ = GPU;
                } else {
                    result->data_ = data_.cwiseMax(0.0f);
                    result->device_ = CPU;
                }
             break;
             case SIGMOID: result->data_ = (1.0f / (1.0f + (-data_.array()).exp())).matrix(); break;
             case ADD_BIAS: 
                if (device_ == GPU && other->device_ == GPU) {
                    result->gpu_data_ = add_bias(gpu_data_, data_.rows(), data_.cols(), 
                                                    other->gpu_data_, gpu_allocator);
                    result->data_.resize(data_.rows(), data_.cols());
                    result->device_ = GPU;
                } else {
                    result->data_ = data_.rowwise() + other->data_.row(0);
                    result->device_ = CPU;
                }
             break;
             case SOFTMAX: {
                 auto x = data_;
                 VectorXf row_max = x.rowwise().maxCoeff();
                 x = x.colwise() - row_max;
                 auto ex = x.array().exp();
                 VectorXf row_sum = ex.rowwise().sum();
                 result->data_ = (ex.colwise() / row_sum.array()).matrix();
                 break;
             }
             case SOFTMAX_CE: {
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
                break;
            } 
            case SIGMOID_BCE: {
                
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
                break;
            }         
             default:
                 print("WARNING: invalid op_ in forward: ",op_);
                 break;
         }


         if (result->device_ == GPU && gpu_allocator) {
            size_t grad_bytes = result->data_.rows() * result->data_.cols() * sizeof(float);
            result->gpu_grad_ = gpu_allocator->allocate(grad_bytes);
            gpu_allocator->memset_gpu(result->gpu_grad_, 0, grad_bytes);
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
                    
                    // Continue backward through the graph
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
                 MatrixXf s = data_;          // NOTE: data_ here is softmax output
                 MatrixXf g = grad_;
                 MatrixXf dot = (g.array() * s.array()).rowwise().sum().matrix(); // (batch x 1)
                 inputs_[0]->grad_ += (s.array() * (g.array().colwise() - dot.col(0).array())).matrix();
                 inputs_[0]->backward();
                 break;
             }
             case SOFTMAX_CE: {
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
                inputs_[0]->backward();
                break;
            }
            case SIGMOID_BCE: {
                int bs = inputs_[0]->data_.rows();
                MatrixXf sigmoid = (1.0f / (1.0f + (-inputs_[0]->data_.array()).exp())).matrix();
                MatrixXf grad_logits = sigmoid - inputs_[1]->data_;
                if(reduction_ == MEAN) {
                    grad_logits /= float(bs);
                }
                
                inputs_[0]->grad_ += grad_logits;
                inputs_[0]->backward();
                break;
            }
             default:
                 print("WARNING: invalid op_ in backward: ",op_);
                 break;
         }
 
     }
 

 
 };
 
 
 
 
}