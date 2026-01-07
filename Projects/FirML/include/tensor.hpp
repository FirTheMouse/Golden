#pragma once

#include<ml_util.hpp>
#include<core/gpuType.hpp>
#include<methods.hpp>

 
namespace Eigen {

    class tensor;

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

                } 
                else {

                }
                break; 
            }
            case MATMUL: {
                if(is_forward) {

                } 
                else {

                }
                break;
            }
            case RELU: {
                if(is_forward) {

                }
                else {

                }
                break;
            }
            case SIGMOID: {
                if(is_forward) {

                }
                else {

                }
                break;
            }
            case ADD_BIAS: {
                if(is_forward) {

                } 
                else {

                }
                break;
            }
            case SOFTMAX: {
                if(is_forward) {

                }
                else {

                }
                break;
            }
            case SOFTMAX_CE: {
                if(is_forward) {

                }
                else {

                }
                break;
            }
        default:
                print("WARNING: invalid op_ in opperate: ",op_);
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