#pragma once

#include<tensor.hpp>

namespace Eigen {

    g_ptr<tensor> weight(int rows, int cols, float scalar = 0.01f) {
        return make<tensor>(MatrixXf::Random(rows, cols)*scalar);
    }

    g_ptr<tensor> bias(int cols) {
        return make<tensor>(MatrixXf::Zero(1, cols));
    }

    list<Pass> classification_head(g_ptr<tensor> w, g_ptr<tensor> b, g_ptr<tensor> targets) {
        return {
            {MATMUL, w}, 
            {ADD_BIAS, b}, 
            {SOFTMAX_CE, targets}
        };
    }

    float compute_loss(g_ptr<tensor> output, g_ptr<tensor> target, Opp type) {
        int bs = output->data_.rows();
        
        switch(type) {
            case MSE: 
                return (output->data_ - target->data_).squaredNorm() / float(bs);
            case CROSS_ENTROPY: {
                // Assumes output is probabilities (from separate SOFTMAX)
                constexpr float eps = 1e-8f;
                auto probs = output->data_.array().max(eps);
                return -(target->data_.array() * probs.log()).sum() / float(bs);
            }
            case SOFTMAX_CE: {
                // Assumes output is logits (no SOFTMAX applied)
                MatrixXf x = output->data_;
                
                VectorXf m = x.rowwise().maxCoeff();
                x = x.colwise() - m;
                MatrixXf ex = x.array().exp().matrix();
                VectorXf sumex = ex.rowwise().sum();
                VectorXf lse = sumex.array().log() + m.array();
                
                MatrixXf log_softmax = output->data_.colwise() - lse;
                return -(target->data_.array() * log_softmax.array()).sum() / float(bs);
            }
            case SIGMOID_BCE: {
                // Assumes output is logits (no SIGMOID applied)
                MatrixXf x = output->data_;
                MatrixXf y = target->data_;
                
                MatrixXf max_val = x.cwiseMax(0.0f);
                MatrixXf stable_bce = (max_val.array() - x.array() * y.array() + 
                ((-x.array().abs()).exp() + 1.0f).log()).matrix();
                
                return stable_bce.sum() / float(bs);
            }
            
            default:
                print("WARNING: invalid loss_type in compute_loss: ", type);
                return 0.0f;
        }
    }

    void seed_gradient(g_ptr<tensor> output, g_ptr<tensor> target, Opp type) {
        switch(type) {
            case MSE: 
                output->grad_ = 2.0f * (output->data_ - target->data_);
                break;
            case CROSS_ENTROPY:
                output->grad_ = output->data_ - target->data_;
                break;
            case SOFTMAX_CE: case SIGMOID_BCE:
                print("WARNING: Don't use seed_gradient with fused ops!");
                break;
            default:
                print("WARNING: invalid loss_type in seed_gradient: ",type);
                break;
        }
    }

    void sgd_step(list<g_ptr<tensor>>& params, float learning_rate) {
        for(auto& param : params) {
            param->data_ -= learning_rate * param->grad_;
        }
    }

    void zero_gradients(list<g_ptr<tensor>>& params) {
        for(auto& param : params) {
            param->grad_.setZero();
        }
    }


    float train_step(g_ptr<tensor> output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, float learning_rate, Opp loss_type, Reduction reduction = MEAN) 
    {
        float loss_value = 0.0f;
        
        if(opp_is_fused(loss_type)) {
            Pass loss_pass;
            loss_pass.op = loss_type;
            loss_pass.param = target;
            loss_pass.reduction = reduction;
            
            auto loss_node = output->forward(loss_pass);
            loss_value = loss_node->data_(0, 0);
            
            loss_node->grad_ = MatrixXf::Ones(1, 1);
            loss_node->backward();
            
        } else {
            loss_value = compute_loss(output, target, loss_type);
            seed_gradient(output, target, loss_type);
            output->backward();
        }

        sgd_step(params,learning_rate);
        zero_gradients(params);
        
        return loss_value;
    }

    //This is just a helper! Decompose into more customizable steps later for more user control
    void train_network(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass> network, list<g_ptr<tensor>> params,
        Opp loss_type, int epochs, float learning_rate, int batch_size = 0, int log_interval = 500, bool show_result = false, Reduction reduction = MEAN
        ) {
            g_ptr<tensor> output;
            float loss_value = 0;
            int num_samples = inputs->data_.rows();
            
            for(int epoch = 0; epoch < epochs; epoch++) {
                float epoch_loss = 0.0f;
                int num_batches = 0;

                if(batch_size>0) {
                    for(int i = 0; i < num_samples; i += batch_size) {
                        auto input_batch = inputs->get_batch(i, batch_size);
                        auto target_batch = targets->get_batch(i, batch_size);
        
                        output = input_batch;
                        for(auto p : network) {
                            output = output->forward(p);
                        }
                        
        
                        float loss = train_step(output, target_batch, params, learning_rate, loss_type, reduction);
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
                } else {
                    // Forward pass
                    output = inputs;
                    for(auto p : network) {
                        output = output->forward(p);
                    }
                    
                    // Backward pass + update
                    loss_value = train_step(output, targets, params, learning_rate, loss_type, reduction);
                    
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
            }
            if(show_result) {
                if(batch_size>0) {
                    auto final_output = inputs;
                    for(auto p : network) {
                        final_output = final_output->forward(p);
                    }

                        print("\nFinal predictions:\n", final_output->data_);
                        print("Target:\n", targets->data_);
                    
                } else {
                    print("\nFinal predictions:\n",output->data_);
                    print("Target:\n",targets->data_);
                }
            }
        }
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
