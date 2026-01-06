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

    struct SequenceConfig {
        bool is_sequence = false;
        int seq_length = 0;
    };
    
    SequenceConfig check_for_sequences(const list<Pass>& network) {
        SequenceConfig config;
        for(const auto& p : network) {
            if(p.op == RNN_STEP && p.ints.length() > 0) {
                config.is_sequence = true;
                config.seq_length = p.ints[0];
                break;
            }
        }
        return config;
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

    void reset_state(list<Pass>& network) {
        for(auto& pass : network) {
            if(pass.state != nullptr) {
                pass.state->data_.setZero();
                pass.state->grad_.setZero();
            }
        }
    }

    void clip_gradients(list<g_ptr<tensor>>& params, float max_norm) {
        // Compute total gradient norm
        float total_norm = 0.0f;
        for(auto& param : params) {
            total_norm += param->grad_.squaredNorm();
        }
        total_norm = std::sqrt(total_norm);
        
        // If norm exceeds max_norm, scale all gradients down
        if(total_norm > max_norm) {
            float scale = max_norm / total_norm;
            for(auto& param : params) {
                param->grad_ *= scale;
            }
        }
    }


    float train_step(g_ptr<tensor> output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, float learning_rate, Opp loss_type, 
        float grad_clip, Reduction reduction) 
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
        
        if(grad_clip > 0.0f) {
            clip_gradients(params, grad_clip);
        }

        sgd_step(params,learning_rate);
        zero_gradients(params);
        
        return loss_value;
    }

    //This is just a helper! Decompose into more customizable steps later for more user control
    void train_network(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass>& network, list<g_ptr<tensor>> params,  Opp loss_type, 
        int epochs, float learning_rate, int batch_size = 0, int log_interval = 500, bool show_result = false,  float grad_clip = 0.0f, Reduction reduction = MEAN
        ) {
            g_ptr<tensor> output;
            float loss_value = 0;
            int num_samples = inputs->data_.rows();
            
            // Check if we're in sequence mode
            bool is_sequence = false;
            int seq_length = 0;
            for(const auto& p : network) {
                if(p.state != nullptr && p.ints.length() > 0 && p.ints[0] > 0) {
                    is_sequence = true;
                    seq_length = p.ints[0];
                    break;
                }
            }
            
            for(int epoch = 0; epoch < epochs; epoch++) {
                float epoch_loss = 0.0f;
                int num_batches = 0;

                if(is_sequence) {
                    // SEQUENCE PROCESSING MODE
                    if(batch_size > 0) {
                        for(int i = 0; i < num_samples; i += batch_size) {
                            auto input_batch = inputs->get_batch(i, batch_size);
                            auto target_batch = targets->get_batch(i, batch_size);
                            
                            reset_state(network);
                            
                            float batch_loss = 0.0f;
                            list<g_ptr<tensor>> timestep_outputs;
                            
                            for(int t = 0; t < seq_length; t++) {
                                // Extract timestep t
                                auto input_t = make<tensor>(MatrixXf(input_batch->data_.rows(), 1));
                                input_t->data_ = input_batch->data_.col(t);
                                
                                output = input_t;
                                for(auto& p : network) {
                                    output = output->forward(p);
                                }
                                timestep_outputs.push(output);
                                
                                // Get target for this timestep
                                int target_dim = targets->data_.cols() / seq_length;
                                auto target_t = make<tensor>(MatrixXf(target_batch->data_.rows(), target_dim));
                                target_t->data_ = target_batch->data_.block(0, t * target_dim, target_batch->data_.rows(), target_dim);
                                
                                float step_loss = train_step(output, target_t, params, learning_rate, loss_type, grad_clip, reduction);
                                batch_loss += step_loss;
                            }
                            
                            // Backward through time (already handled by train_step calls above)
                            
                            epoch_loss += batch_loss / seq_length;
                            num_batches++;
                            
                            if(std::isnan(batch_loss) || std::isinf(batch_loss)) {
                                print("Exploded at epoch ", epoch);
                                break;
                            }
                        }
                    } else {
                        for(int i = 0; i < num_samples; i++) {
                            reset_state(network);
                            
                            float seq_loss = 0.0f;
                            
                            for(int t = 0; t < seq_length; t++) {
                                auto input_t = make<tensor>(MatrixXf(1, 1));
                                input_t->data_(0, 0) = inputs->data_(i, t);
                                
                                output = input_t;
                                for(auto& p : network) {
                                    output = output->forward(p);
                                }
                                
                                int target_dim = targets->data_.cols() / seq_length;
                                auto target_t = make<tensor>(MatrixXf(1, target_dim));
                                target_t->data_ = targets->data_.block(i, t * target_dim, 1, target_dim);
                                
                                float step_loss = train_step(output, target_t, params, learning_rate, loss_type, grad_clip, reduction);
                                seq_loss += step_loss;
                            }

                            //Add gradient clipping properly eventually
                            // float max_grad_norm = 1.0f;
                            // clip_gradients(params, max_grad_norm);
                            
                            epoch_loss += seq_loss / seq_length;
                        }
                        num_batches = num_samples;
                    }
                    
                    if(log_interval > 0 && epoch % log_interval == 0) {
                        print("Epoch ", epoch, " Loss: ", epoch_loss / num_batches);
                    }
                    
                } else {
                    if(batch_size > 0) {
                            for(int i = 0; i < num_samples; i += batch_size) {
                                auto input_batch = inputs->get_batch(i, batch_size);
                                auto target_batch = targets->get_batch(i, batch_size);
                
                                output = input_batch;
                                for(auto p : network) {
                                    output = output->forward(p);
                                }


                                float loss = train_step(output, target_batch, params, learning_rate, loss_type, grad_clip, reduction);
                                epoch_loss += loss;
                                num_batches++;
                
                                if(std::isnan(loss) || std::isinf(loss)) {
                                    print("Exploded at epoch ", epoch);
                                    print("w1 max: ", params[0]->data_.maxCoeff());
                                    print("w1 min: ", params[0]->data_.minCoeff());
                                    break;
                                }
                            }

                            if(log_interval > 0 && epoch % log_interval == 0) {
                                print("Epoch ", epoch, " Loss: ", epoch_loss / num_batches);
                            }
                        } else {
                            for(int i = 0; i < num_samples; i++) {
                                auto input_example = inputs->get_batch(i, 1);
                                auto target_example = targets->get_batch(i, 1);
                                
                                output = input_example;
                                for(auto& p : network) {
                                    output = output->forward(p);
                                }
                                
                                loss_value = train_step(output, target_example, params, learning_rate, loss_type, grad_clip, reduction);
                                epoch_loss += loss_value;
                                output = nullptr;
                                
                                if(std::isnan(loss_value) || std::isinf(loss_value)) {
                                    print("Exploded at epoch ", epoch, " example ", i);
                                    print("w1 max: ", params[0]->data_.maxCoeff());
                                    print("w1 min: ", params[0]->data_.minCoeff());
                                    break;
                                }
                            }
                            
                            num_batches = num_samples;
                            
                            if(log_interval > 0 && epoch % log_interval == 0) {
                                print("Epoch ", epoch, " Loss: ", epoch_loss / num_batches);
                            }
                    }
                }
            }
            
            if(show_result) {
                if(is_sequence) {
                    print("\nSequence training complete");
                } else {
                    if(batch_size > 0) {
                        auto final_output = inputs;
                        for(auto p : network) {
                            final_output = final_output->forward(p);
                        }
                        print("\nFinal predictions:\n", final_output->data_);
                        print("Target:\n", targets->data_);
                    } else {
                        print("\nFinal predictions:\n", output->data_);
                        print("Target:\n", targets->data_);
                    }
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
