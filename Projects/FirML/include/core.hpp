#pragma once

#include<tensor.hpp>

namespace Eigen {

    using LRScheduleFn = std::function<float(int epoch, float base_lr)>;

    //Learning rate schedulers
        LRScheduleFn step_decay(int step_size, float gamma = 0.1f) {
            return [=](int epoch, float base_lr) {
                return base_lr * std::pow(gamma, epoch / step_size);
            };
        }
        
        LRScheduleFn exponential_decay(float gamma = 0.95f) {
            return [=](int epoch, float base_lr) {
                return base_lr * std::pow(gamma, epoch);
            };
        }
        
        LRScheduleFn cosine_annealing(int T_max) {
            return [=](int epoch, float base_lr) {
                return base_lr * 0.5f * (1.0f + std::cos(M_PI * epoch / T_max));
            };
        }

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

    void set_network_mode(list<Pass>& network, Mode mode) {
        for(auto& p : network) {
            p.mode = mode;
        }
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

 
    void save_checkpoint(const list<g_ptr<tensor>>& params, const std::string& filepath) {
        //MAKE REAL IO
        std::ofstream file(filepath, std::ios::binary);
        
        for(auto& p : params) {
            int rows = p->data_.rows();
            int cols = p->data_.cols();
            
            file.write((char*)&rows, sizeof(int));
            file.write((char*)&cols, sizeof(int));
            file.write((char*)p->data_.data(), rows * cols * sizeof(float));
        }
        
        file.close();
    }
    
    void load_checkpoint(list<g_ptr<tensor>>& params, const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        
        for(auto& p : params) {
            int rows, cols;
            file.read((char*)&rows, sizeof(int));
            file.read((char*)&cols, sizeof(int));
            
            p->data_.resize(rows, cols);
            file.read((char*)p->data_.data(), rows * cols * sizeof(float));
        }
        
        file.close();
    }

    //Fold this into operate in tensor eventually if we start needing to maintain these
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

    float compute_metric(g_ptr<tensor> predictions, g_ptr<tensor> targets, Opp loss_type) {
        switch(loss_type) {
            case SOFTMAX_CE:
            case CROSS_ENTROPY: {
                // Classification accuracy
                int correct = 0;
                int total = predictions->data_.rows();
                for(int i = 0; i < total; i++) {
                    int pred_class, true_class;
                    predictions->data_.row(i).maxCoeff(&pred_class);
                    targets->data_.row(i).maxCoeff(&true_class);
                    if(pred_class == true_class) correct++;
                }
                return float(correct) / float(total);
            }
            case MSE:
            case SIGMOID_BCE: {
                // For regression/binary, return negative loss (higher = better)
                return -compute_loss(predictions, targets, loss_type);
            }
            default:
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

    enum OPTM {
        SGD, ADAM
    };

    class optimizer : public Object {
        protected:
            list<g_ptr<tensor>>& params_;
            float learning_rate_;
            int accumulation_counter_ = 0;
        public:
            optimizer(list<g_ptr<tensor>>& params, float learning_rate) 
            : params_(params), learning_rate_(learning_rate) {}
        
            
            virtual void step(int accumulation_steps = 1) = 0; 
            virtual void zero_grad() = 0;

            float get_lr() const { return learning_rate_; }
            void set_lr(float lr) { learning_rate_ = lr; }

            bool should_step(int accumulation_steps) {
                accumulation_counter_++;
                if(accumulation_counter_ >= accumulation_steps) {
                    accumulation_counter_ = 0;
                    return true;
                }
                return false;
            }
        };

    class optm_sgd : public optimizer {
    public:
        optm_sgd(list<g_ptr<tensor>>& params, float learning_rate) 
            : optimizer(params, learning_rate) {}
        
        void step(int accumulation_steps = 1) override {
            for(auto& param : params_) {
                param->data_ -= learning_rate_ * param->grad_;
            }
        }
        
        void zero_grad() override {
            for(auto& p : params_) p->grad_.setZero();
        }
    };

    class optm_adam : public optimizer {
        float beta1_, beta2_, epsilon_;
        int t_;  // Timestep counter
        
        // Per-parameter state
        map<g_ptr<tensor>, MatrixXf> m_;  // First moment
        map<g_ptr<tensor>, MatrixXf> v_;  // Second moment
        
    public:
        optm_adam(list<g_ptr<tensor>>& params, float learning_rate, 
                  float beta1 = 0.9f, float beta2 = 0.999f, float epsilon = 1e-8f)
            : optimizer(params, learning_rate), 
              beta1_(beta1), beta2_(beta2), epsilon_(epsilon), t_(0) {
            
            // Initialize moments for each parameter
            for(auto& p : params_) {
                m_[p] = MatrixXf::Zero(p->data_.rows(), p->data_.cols());
                v_[p] = MatrixXf::Zero(p->data_.rows(), p->data_.cols());
            }
        }
        
        void step(int accumulation_steps = 1) override {
            t_++;  // Increment timestep
            
            for(auto& p : params_) {
                // Update moments
                m_[p] = beta1_ * m_[p] + (1.0f - beta1_) * p->grad_;
                v_[p] = beta2_ * v_[p] + (1.0f - beta2_) * p->grad_.array().square().matrix();
                
                // Bias correction
                MatrixXf m_hat = m_[p] / (1.0f - std::pow(beta1_, t_));
                MatrixXf v_hat = v_[p] / (1.0f - std::pow(beta2_, t_));
                
                // Update (convert Array back to Matrix)
                MatrixXf update = (learning_rate_ * m_hat.array() / (v_hat.array().sqrt() + epsilon_)).matrix();
                p->data_ -= update;
            }
        }
        
        void zero_grad() override {
            for(auto& p : params_) {
                p->grad_.setZero();
            }
        }
    };

    //Configuration object for network training
    class t_config : public Object {
        public:
            t_config() {}

            t_config(int epochs_, float learning_rate_, int batch_size_) 
            : epochs(epochs_), learning_rate(learning_rate_), batch_size(batch_size_) {}

            bool validate() {
                if(epochs <= 0) {
                    print("t_config: epochs must be > 0");
                    return false;
                }
                if(learning_rate <= 0.0f) {
                    print("t_config: learning_rate must be > 0");
                    return false;
                }
                return true;
            }

            void config_basic(int epochs_, float learning_rate_) {
                epochs = epochs_;
                learning_rate = learning_rate_;
            }

            void config_batched(int epochs_, float learning_rate_, int batch_size_) {
                config_basic(epochs_,learning_rate_);
                batch_size = batch_size_;
            }

            int epochs;
            float learning_rate;
            int batch_size = 0;
            bool shuffle = false;
            float grad_clip = 0.0f;
            Reduction reduction = MEAN;
            OPTM optim = OPTM::SGD;
            LRScheduleFn lr_schedule = nullptr; 

            int gradient_accumulation_steps = 1; 

            bool use_validation = false;
            float validation_split = 0.2f;
            int patience = 10;
            //ALL OF THESE ARE NOT YET INTEGRATED!
            float lr_decay;
            int checkpoint_interval;


    };

    //DTO for network traning
    class t_metric : public Object {
    public:
        list<float> train_losses;
        list<float> val_losses;
        list<float> accuracies;
        float best_loss;
        int best_epoch;
    };

    g_ptr<optimizer> create_optimizer(OPTM type, list<g_ptr<tensor>>& params, g_ptr<t_config>& ctx) {
        switch(type) {
            case SGD:
                return make<optm_sgd>(params, ctx->learning_rate);
            case ADAM:
                return make<optm_adam>(params, ctx->learning_rate);
            default:
                print("create_optimizer::invalid optimizer type: ",type);
                return nullptr;
        }
    }

    struct DataSplit {
        g_ptr<tensor> train_inputs, train_targets;
        g_ptr<tensor> val_inputs, val_targets;
    };
    
    DataSplit split_data(g_ptr<tensor> inputs, g_ptr<tensor> targets, float val_split) {
        int num_samples = inputs->data_.rows();
        int val_size = static_cast<int>(num_samples * val_split);
        int train_size = num_samples - val_size;
        
        DataSplit split;
        split.train_inputs = make<tensor>(inputs->data_.topRows(train_size));
        split.train_targets = make<tensor>(targets->data_.topRows(train_size));
        split.val_inputs = make<tensor>(inputs->data_.bottomRows(val_size));
        split.val_targets = make<tensor>(targets->data_.bottomRows(val_size));
        
        return split;
    }

    
    //Training and evaluation
    float process_batch(g_ptr<tensor>& output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, Opp loss_type, 
        Reduction reduction, g_ptr<optimizer> optim = nullptr, float grad_clip = 0.0f, int accumulation_steps = 1) 
    {
        float loss_value = 0.0f;

        if(opp_is_fused(loss_type)) {
            Pass loss_pass;
            loss_pass.op = loss_type;
            loss_pass.param = target;
            loss_pass.reduction = reduction;

            auto loss_node = output->forward(loss_pass);
            loss_value = loss_node->data_(0, 0);

            // Only backward if training (optim != nullptr)
            if(optim) {
                loss_node->grad_ = MatrixXf::Ones(1, 1);
                loss_node->backward();
            }
        } else {
            loss_value = compute_loss(output, target, loss_type);
            // Only backward if training
            if(optim) {
                seed_gradient(output, target, loss_type);
                output->backward();
                }
        }

        // Only update weights if training
        if(optim) {

            if(accumulation_steps > 1) {
                for(auto& p : params) {
                    p->grad_ /= float(accumulation_steps);
                }
            }

            if(grad_clip > 0.0f) {
                clip_gradients(params, grad_clip);
            }

            if(optim->should_step(accumulation_steps)) {
                optim->step(accumulation_steps);
                optim->zero_grad();
            }
        }

        return loss_value;
    }

    //Result of evaluation
    struct e_result {
        float loss;
        float accuracy;
    };
    
    e_result evaluate(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass>& network, list<g_ptr<tensor>>& params, 
                       Opp loss_type, Reduction reduction) 
    {
        float total_loss = 0.0f;
        int total_correct = 0;
        int num_samples = inputs->data_.rows();

        set_network_mode(network, EVAL);
        
        for(int i = 0; i < num_samples; i++) {
            auto input = inputs->get_batch(i, 1);
            auto target = targets->get_batch(i, 1);
            
            auto output = input;
            for(auto& p : network) {
                output = output->forward(p);
            }
            
            total_loss += process_batch(output, target, params, loss_type, reduction);
            
            // Compute accuracy
            int pred_class, true_class;
            output->data_.row(0).maxCoeff(&pred_class);
            target->data_.row(0).maxCoeff(&true_class);
            if(pred_class == true_class) total_correct++;
        }

        set_network_mode(network, TRAIN);
        
        return {total_loss / num_samples, float(total_correct) / num_samples};
    }


    float train_step(g_ptr<tensor>& output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, Opp loss_type, 
                     Reduction reduction, g_ptr<optimizer> optim, float grad_clip = 0.0f, int accumulation_steps = 1) 
    {
        float loss = process_batch(output, target, params, loss_type, reduction, optim, grad_clip);
        //This will be replaced later once I integrate the pool
        tensor::clear_graph(output);
        return loss;
    }


    //This is just a helper! Decompose into more customizable steps later for more user control
    void train_network(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass>& network, list<g_ptr<tensor>> params,  Opp loss_type, 
        g_ptr<t_config>& ctx, int log_interval = 500
        ) {
            g_ptr<tensor> output;
            float loss_value = 0;
            int num_samples = inputs->data_.rows();

            if(!ctx->validate()) {
                print("train_network::invalid t_config");
                return;
            }

            std::string A_ROOT = root()+"/Projects/FirML/assets/shaders/";

            int epochs = ctx->epochs;
            float learning_rate = ctx->learning_rate;
            float grad_clip = ctx->grad_clip;
            int batch_size = ctx->batch_size;
            Reduction reduction = ctx->reduction;
            OPTM optim_type = ctx->optim;
            int accumulation_steps = ctx->gradient_accumulation_steps;
            g_ptr<optimizer> optim = create_optimizer(optim_type,params,ctx);

            float best_val_loss = INFINITY;
            int patience_counter = 0;

            
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

            DataSplit split;
            if(ctx->use_validation) {
                split = split_data(inputs, targets, ctx->validation_split);
                inputs = split.train_inputs;
                targets = split.train_targets;
                num_samples = inputs->data_.rows();  // Update sample count
            }

            set_network_mode(network, TRAIN);

            
            for(int epoch = 0; epoch < epochs; epoch++) {
                float epoch_loss = 0.0f;
                int num_batches = 0;

                if(ctx->lr_schedule) {
                    float new_lr = ctx->lr_schedule(epoch, ctx->learning_rate);
                    optim->set_lr(new_lr);
                }

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
                                
                                loss_value = train_step(output, target_t, params, loss_type, reduction, optim, grad_clip, accumulation_steps);
                                batch_loss += loss_value;
                            }
                            epoch_loss += batch_loss / seq_length;
                            num_batches++;
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
                                    p.mode = TRAIN;
                                    output = output->forward(p);
                                }
                                
                                int target_dim = targets->data_.cols() / seq_length;
                                auto target_t = make<tensor>(MatrixXf(1, target_dim));
                                target_t->data_ = targets->data_.block(i, t * target_dim, 1, target_dim);
                                
                                loss_value = train_step(output, target_t, params, loss_type, reduction, optim, grad_clip, accumulation_steps);
                                seq_loss += loss_value;
                            }
                            epoch_loss += seq_loss / seq_length;
                        }
                        num_batches = num_samples;
                    }
                } else {
                    if(batch_size > 0) {
                            list<int> indices;
                            for(int i = 0; i < num_samples; i++) indices.push(i);
                            if(ctx->shuffle) {
                                indices = create_shuffle_indices(num_samples);
                            }
                            for(int i = 0; i < num_samples; i += batch_size) {
                                int actual_batch = std::min(batch_size, num_samples - i);
                                list<int> batch_indices;
                                for(int j = 0; j < actual_batch; j++) {
                                    batch_indices.push(indices[i + j]);
                                }
                                
                                auto input_batch = inputs->get_batch_by_indices(batch_indices);
                                auto target_batch = targets->get_batch_by_indices(batch_indices);
                
                                output = input_batch;
                                for(auto p : network) {
                                    output = output->forward(p);
                                }


                                loss_value = train_step(output, target_batch, params, loss_type, reduction, optim, grad_clip, accumulation_steps);
                                epoch_loss += loss_value;
                                num_batches++;
                            }
                        } else {
                            for(int i = 0; i < num_samples; i++) {
                                auto input_example = inputs->get_batch(i, 1);
                                auto target_example = targets->get_batch(i, 1);
                                
                                output = input_example;
                                for(auto& p : network) {
                                    output = output->forward(p);
                                }
                                
                                loss_value = train_step(output, target_example, params, loss_type, reduction, optim, grad_clip, accumulation_steps);
                                epoch_loss += loss_value;
                                output = nullptr;
                            }
                            num_batches = num_samples;
                    }
                }

                if(std::isnan(loss_value) || std::isinf(loss_value)) {
                    print("Exploded at epoch ", epoch);
                    print("w1 max: ", params[0]->data_.maxCoeff());
                    print("w1 min: ", params[0]->data_.minCoeff());
                    break;
                }

                if(ctx->use_validation) {
                    auto eval = evaluate(split.val_inputs, split.val_targets, network, params, loss_type, reduction);
    
                    if(log_interval > 0 && epoch % log_interval == 0) {
                        print("Epoch ", epoch, 
                              " Train Loss: ", epoch_loss / num_batches,
                              " Val Loss: ", eval.loss,
                              " Val Acc: ", eval.accuracy * 100.0f, "%");
                    }
                    
                    if(eval.loss < best_val_loss) {
                        best_val_loss = eval.loss;
                        patience_counter = 0;
                        save_checkpoint(params, A_ROOT+"best_model.ckpt");
                    } else {
                        patience_counter++;
                        if(patience_counter >= ctx->patience) {
                            print("Early stopping at epoch ", epoch);
                            break;
                        }
                    }
                    if(ctx->checkpoint_interval > 0 && epoch % ctx->checkpoint_interval == 0) {
                        save_checkpoint(params, A_ROOT+"checkpoint_epoch_" + std::to_string(epoch) + ".ckpt");
                    }
                } else {
                    if(log_interval > 0 && epoch % log_interval == 0) {
                        print("Epoch ", epoch, " Loss: ", epoch_loss / num_batches);
                    }
                }
            }
            
        }
}

