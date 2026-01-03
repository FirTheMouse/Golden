#pragma once

#include<tensor.hpp>

namespace Eigen {

    g_ptr<tensor> weight(int rows, int cols, float scalar = 0.01f) {
        return make<tensor>(MatrixXf::Random(rows, cols)*scalar);
    }

    g_ptr<tensor> bias(int cols) {
        return make<tensor>(MatrixXf::Zero(1, cols));
    }

    //This is just a helper! Decompose into more customizable steps later for more user control
    void train_network(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass> network, list<g_ptr<tensor>> params,
        LossType loss_type, int epochs, float learning_rate, int batch_size = 0, int log_interval = 500, bool show_result = false
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
                } else {
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
            }
            if(show_result) {
                if(batch_size>0) {
                    auto final_output = inputs;
                    for(auto p : network) {
                        final_output = final_output->forward(p.op, p.param);
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