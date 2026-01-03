#pragma once

#include<ml_util.hpp>

enum Opp {
    NOP, ADD, MATMUL, RELU, ADD_BIAS, SIGMOID, SOFTMAX
 };
 
 enum LossType {
     MSE, CROSS_ENTROPY
 };
 
namespace Eigen {

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
 
}