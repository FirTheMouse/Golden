#pragma once

#include<ml_util.hpp>

namespace Eigen {

    float time_1 = 0;
    float time_2 = 0;
    float time_3 = 0;
    float time_4 = 0;
    float time_5 = 0;
    float time_6 = 0;
    map<Opp,float> time_map;
    map<Opp,float> f_time_map;


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
        auto t = make<tensor>(list<int>{rows, cols});
        // Initialize with random values
        for(int i = 0; i < t->numel(); i++) {
            t->flat(i) = randf(-1.0f, 1.0f) * scalar;
        }
        return t;
    }
    
    g_ptr<tensor> bias(int cols) {
        auto t = make<tensor>(list<int>{1, cols});
        t->fill(0.0f);
        return t;
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
        std::ofstream file(filepath, std::ios::binary);
        
        for(auto& p : params) {
            int ndim = p->ndim();
            file.write((char*)&ndim, sizeof(int));
            
            for(int i = 0; i < ndim; i++) {
                int dim = p->shape_[i];
                file.write((char*)&dim, sizeof(int));
            }
            
            int numel = p->numel();
            for(int i = 0; i < numel; i++) {
                float val = p->flat(i);
                file.write((char*)&val, sizeof(float));
            }
        }
        
        file.close();
    }
    
    void load_checkpoint(list<g_ptr<tensor>>& params, const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        
        for(auto& p : params) {
            int ndim;
            file.read((char*)&ndim, sizeof(int));
            
            list<int> shape;
            for(int i = 0; i < ndim; i++) {
                int dim;
                file.read((char*)&dim, sizeof(int));
                shape.push(dim);
            }
            
            if(p->shape_ != shape) {
                print("WARNING: Loaded shape doesn't match param shape");
            }
            
            int numel = p->numel();
            for(int i = 0; i < numel; i++) {
                float val;
                file.read((char*)&val, sizeof(float));
                p->flat(i) = val;
            }
        }
        
        file.close();
    }

    //Fold this into operate in tensor eventually if we start needing to maintain these
    float compute_loss(g_ptr<tensor> output, g_ptr<tensor> target, Opp type) {
        int bs = output->shape_[0];
        
        switch(type) {
            case MSE: {
                auto out_mat = output->as_matrix();
                auto tgt_mat = target->as_matrix();
                return (out_mat - tgt_mat).squaredNorm() / float(bs);
            }
            case CROSS_ENTROPY: {
                constexpr float eps = 1e-8f;
                auto out_mat = output->as_matrix();
                auto tgt_mat = target->as_matrix();
                auto probs = out_mat.array().max(eps);
                return -(tgt_mat.array() * probs.log()).sum() / float(bs);
            }
            case SOFTMAX_CE: {
                auto x = output->as_matrix();
                
                VectorXf m = x.rowwise().maxCoeff();
                x = x.colwise() - m;
                MatrixXf ex = x.array().exp().matrix();
                VectorXf sumex = ex.rowwise().sum();
                VectorXf lse = sumex.array().log() + m.array();
                
                MatrixXf log_softmax = output->as_matrix().colwise() - lse;
                return -(target->as_matrix().array() * log_softmax.array()).sum() / float(bs);
            }
            case SIGMOID_BCE: {
                auto x = output->as_matrix();
                auto y = target->as_matrix();
                
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
                int correct = 0;
                int total = predictions->shape_[0];
                
                auto pred_mat = predictions->as_matrix();
                auto tgt_mat = targets->as_matrix();
                
                for(int i = 0; i < total; i++) {
                    int pred_class, true_class;
                    pred_mat.row(i).maxCoeff(&pred_class);
                    tgt_mat.row(i).maxCoeff(&true_class);
                    if(pred_class == true_class) correct++;
                }
                return float(correct) / float(total);
            }
            case MSE:
            case SIGMOID_BCE: {
                return -compute_loss(predictions, targets, loss_type);
            }
            default:
                return 0.0f;
        }
    }

    void seed_gradient(g_ptr<tensor> output, g_ptr<tensor> target, Opp type) {
        switch(type) {
            case MSE: {
                auto out_mat = output->as_matrix();
                auto tgt_mat = target->as_matrix();
                auto diff = 2.0f * (out_mat - tgt_mat);
                
                for(int i = 0; i < output->shape_[0]; i++) {
                    for(int j = 0; j < output->shape_[1]; j++) {
                        output->grad_[i * output->shape_[1] + j] = diff(i, j);
                    }
                }
                break;
            }
            case CROSS_ENTROPY: {
                auto out_mat = output->as_matrix();
                auto tgt_mat = target->as_matrix();
                auto diff = out_mat - tgt_mat;
                
                for(int i = 0; i < output->shape_[0]; i++) {
                    for(int j = 0; j < output->shape_[1]; j++) {
                        output->grad_[i * output->shape_[1] + j] = diff(i, j);
                    }
                }
                break;
            }
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
                pass.state->fill(0.0f);
                pass.state->zero_grad();
            }
        }
    }

    void clip_gradients(list<g_ptr<tensor>>& params, float max_norm) {
        // Compute total gradient norm across all parameters
        float total_norm_sq = 0.0f;
        
        for(auto& param : params) {
            if(param->grad_.length() == 0) continue;
            
            #if defined(__APPLE__) || !defined(__CUDA__)
                // CPU path - use BLAS norm
                float param_norm = blas_norm(param->grad_.length(), 
                                            param->grad_.data());
                total_norm_sq += param_norm * param_norm;
            #else
                // CUDA path - gradients might be on GPU
                if(param->device_ == GPU && param->gpu_grad_) {
                    float param_norm = blas_norm(param->grad_.length(), 
                                                (float*)param->gpu_grad_);
                    total_norm_sq += param_norm * param_norm;
                } else {
                    // Fallback to CPU
                    float param_norm = blas_norm(param->grad_.length(), 
                                                param->grad_.data());
                    total_norm_sq += param_norm * param_norm;
                }
            #endif
        }
        
        float total_norm = std::sqrt(total_norm_sq);
        
        // Only clip if norm exceeds threshold
        if(total_norm > max_norm) {
            float scale = max_norm / total_norm;
            
            for(auto& param : params) {
                if(param->grad_.length() == 0) continue;
                
                #if defined(__APPLE__) || !defined(__CUDA__)
                    // CPU path - use BLAS scale
                    blas_scale(param->grad_.length(), scale, param->grad_.data());
                #else
                    // CUDA path
                    if(param->device_ == GPU && param->gpu_grad_) {
                        blas_scale(param->grad_.length(), scale, (float*)param->gpu_grad_);
                    } else {
                        blas_scale(param->grad_.length(), scale, param->grad_.data());
                    }
                #endif
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
                    if(param->numel() == 0) continue;
                    
                    #if defined(__APPLE__) || !defined(__CUDA__)
                        // CPU path: param = param - lr * grad
                        // Use saxpy: y = alpha*x + y
                        blas_axpy(param->numel(), 
                                    -learning_rate_,                    // alpha (negative!)
                                    param->grad_.data(),                // x (gradient)
                                    param->storage_->data.data());      // y (parameters)
                    #else
                        // CUDA path
                        if(param->device_ == GPU && param->gpu_data_ && param->gpu_grad_) {
                            blas_axpy(param->numel(),
                                        -learning_rate_,
                                        (float*)param->gpu_grad_,
                                        (float*)param->gpu_data_);
                        } else {
                            // CPU fallback
                            blas_axpy(param->numel(),
                                        -learning_rate_,
                                        param->grad_.data(),
                                        param->storage_->data.data());
                        }
                    #endif
                }
            }
            
            void zero_grad() override {
                for(auto& p : params_) p->zero_grad();
            }
        };

        class optm_adam : public optimizer {
            float beta1_, beta2_, epsilon_;
            int t_;
            
            map<g_ptr<tensor>, list<float>> m_;
            map<g_ptr<tensor>, list<float>> v_;
            
        public:
            optm_adam(list<g_ptr<tensor>>& params, float learning_rate, 
                      float beta1 = 0.9f, float beta2 = 0.999f, float epsilon = 1e-8f)
                : optimizer(params, learning_rate), 
                  beta1_(beta1), beta2_(beta2), epsilon_(epsilon), t_(0) {
                
                for(auto& p : params_) {
                    m_[p].resize(p->numel());
                    v_[p].resize(p->numel());
                    for(int i = 0; i < p->numel(); i++) {
                        m_[p][i] = 0.0f;
                        v_[p][i] = 0.0f;
                    }
                }
            }
            
            void step(int accumulation_steps = 1) override {
                t_++;
                
                float bias_correction1 = 1.0f - std::pow(beta1_, t_);
                float bias_correction2 = 1.0f - std::pow(beta2_, t_);
                
                for(auto& p : params_) {
                    int n = p->numel();
                    if(n == 0) continue;
                    
                    float* grad_ptr = p->grad_.data();
                    float* param_ptr = p->storage_->data.data();
                    float* m_ptr = m_[p].data();
                    float* v_ptr = v_[p].data();
                    
                    blas_scale(n, beta1_, m_ptr);
                    blas_axpy(n, (1.0f - beta1_), grad_ptr, m_ptr);
                    blas_scale(n, beta2_, v_ptr);

                    #ifdef __APPLE__
                        float one_minus_beta2 = 1.0f - beta2_;
                        list<float> grad_sq(n);
                        vDSP_vsq(grad_ptr, 1, grad_sq.data(), 1, n);  
                        blas_axpy(n, one_minus_beta2, grad_sq.data(), v_ptr); 
                    #else
                        float one_minus_beta2 = 1.0f - beta2_;
                        for(int i = 0; i < n; i++) {
                            v_ptr[i] += one_minus_beta2 * grad_ptr[i] * grad_ptr[i];
                        }
                    #endif
                    
                    float alpha = learning_rate_ / bias_correction1;
                    float sqrt_bias_correction = std::sqrt(bias_correction2);
                    
                    #ifdef __APPLE__
                        list<float> v_corrected(n);
                        list<float> v_sqrt(n);
                        list<float> denominator(n);
                        list<float> update(n);
                        
                        float inv_bias2 = 1.0f / bias_correction2;
                        vDSP_vsmul(v_ptr, 1, &inv_bias2, v_corrected.data(), 1, n);
 
                        int n_int = n;
                        vvsqrtf(v_sqrt.data(), v_corrected.data(), &n_int);
                        vDSP_vsadd(v_sqrt.data(), 1, &epsilon_, denominator.data(), 1, n);
                        
                        vvdivf(update.data(), m_ptr, denominator.data(), &n_int);
                        
                        blas_axpy(n, -alpha, update.data(), param_ptr);
                    #else
                        // Fallback: optimized manual loop
                        for(int i = 0; i < n; i++) {
                            float m_hat = m_ptr[i];
                            float v_hat = v_ptr[i] / bias_correction2;
                            float denom = std::sqrt(v_hat) + epsilon_;
                            param_ptr[i] -= alpha * m_hat / denom;
                        }
                    #endif
                }
            }
            
            void zero_grad() override {
                for(auto& p : params_) {
                    p->zero_grad();
                }
            }
        };

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
        int num_samples = inputs->shape_[0];
        int val_size = static_cast<int>(num_samples * val_split);
        int train_size = num_samples - val_size;
        
        DataSplit split;
        split.train_inputs = make<tensor>(list<int>{train_size, inputs->shape_[1]});
        split.train_targets = make<tensor>(list<int>{train_size, targets->shape_[1]});
        
        for(int i = 0; i < train_size; i++) {
            for(int j = 0; j < inputs->shape_[1]; j++) {
                split.train_inputs->at({i, j}) = inputs->at({i, j});
            }
            for(int j = 0; j < targets->shape_[1]; j++) {
                split.train_targets->at({i, j}) = targets->at({i, j});
            }
        }
        
        split.val_inputs = make<tensor>(list<int>{val_size, inputs->shape_[1]});
        split.val_targets = make<tensor>(list<int>{val_size, targets->shape_[1]});
        
        for(int i = 0; i < val_size; i++) {
            for(int j = 0; j < inputs->shape_[1]; j++) {
                split.val_inputs->at({i, j}) = inputs->at({train_size + i, j});
            }
            for(int j = 0; j < targets->shape_[1]; j++) {
                split.val_targets->at({i, j}) = targets->at({train_size + i, j});
            }
        }
        
        return split;
    }

    
    float process_batch(g_ptr<tensor>& output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, Opp loss_type, 
        Reduction reduction, g_ptr<optimizer> optim = nullptr, float grad_clip = 0.0f, int accumulation_steps = 1) 
    {
        float loss_value = 0.0f;
        Log::Line l; l.start(); 
        if(opp_is_fused(loss_type)) {
            Pass loss_pass;
            loss_pass.op = loss_type;
            loss_pass.param = target;
            loss_pass.reduction = reduction;


            auto loss_node = output->forward(loss_pass);
            loss_value = loss_node->flat(0);
            time_2 += l.end(); l.start();
            Opp old_op = loss_node->op_;
            if(optim) {
                loss_node->grad_[0] = 1.0f;
                loss_node->backward();
            }
            float end = l.end();
            time_map.getOrPut(old_op,end)+=end;
            time_3+= end;
        } else {
            loss_value = compute_loss(output, target, loss_type);
            if(optim) {
                seed_gradient(output, target, loss_type);
                output->backward();
                }
        }

        if(optim) {
            if(accumulation_steps > 1) {
                for(auto& p : params) {
                    for(int i = 0; i < p->grad_.length(); i++) {
                        p->grad_[i] /= float(accumulation_steps);
                    }
                }
            }

            l.start();
            if(grad_clip > 0.0f) {
                clip_gradients(params, grad_clip);
            }
            time_4+=l.end(); l.start();

            if(optim->should_step(accumulation_steps)) {
                optim->step(accumulation_steps);
                optim->zero_grad();
            }
            time_5+=l.end(); l.start();
        }

        return loss_value;
    }

    struct e_result {
        float loss;
        float accuracy;
    };
    
    e_result evaluate(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass>& network, list<g_ptr<tensor>>& params, 
                       Opp loss_type, Reduction reduction) 
    {
        float total_loss = 0.0f;
        int total_correct = 0;
        int num_samples = inputs->shape_[0];

        set_network_mode(network, EVAL);
        
        for(int i = 0; i < num_samples; i++) {
            auto input = inputs->get_batch(i, 1);
            auto target = targets->get_batch(i, 1);
            
            auto output = input;
            for(auto& p : network) {
                output = output->forward(p);
            }
            
            total_loss += process_batch(output, target, params, loss_type, reduction);
            
            auto out_mat = output->as_matrix();
            auto tgt_mat = target->as_matrix();
            int pred_class, true_class;
            out_mat.row(0).maxCoeff(&pred_class);
            tgt_mat.row(0).maxCoeff(&true_class);
            if(pred_class == true_class) total_correct++;
        }

        set_network_mode(network, TRAIN);
        
        return {total_loss / num_samples, float(total_correct) / num_samples};
    }


    float train_step(g_ptr<tensor>& output, g_ptr<tensor> target, list<g_ptr<tensor>>& params, Opp loss_type, 
                     Reduction reduction, g_ptr<optimizer> optim, float grad_clip = 0.0f, int accumulation_steps = 1) 
    {
        Log::Line m; m.start();
        float loss = process_batch(output, target, params, loss_type, reduction, optim, grad_clip);
        time_1 += m.end();
        tensor::clear_graph(output);
        return loss;
    }


    void train_network(g_ptr<tensor> inputs, g_ptr<tensor> targets, list<Pass>& network, list<g_ptr<tensor>> params,  Opp loss_type, 
        g_ptr<t_config>& ctx, int log_interval = 500
        ) {
            g_ptr<tensor> output;
            float loss_value = 0;
            int num_samples = inputs->shape_[0];

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

            g_ptr<tensor> w1 = network[0].param;
            g_ptr<tensor> b1 = network[1].param;

            float best_val_loss = INFINITY;
            int patience_counter = 0;

            
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
                num_samples = inputs->shape_[0];
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
                    if(batch_size > 0) {
                        for(int i = 0; i < num_samples; i += batch_size) {
                            auto input_batch = inputs->get_batch(i, batch_size);
                            auto target_batch = targets->get_batch(i, batch_size);
                            
                            reset_state(network);
                            
                            float batch_loss = 0.0f;
                            list<g_ptr<tensor>> timestep_outputs;
                            
                            for(int t = 0; t < seq_length; t++) {
                                auto input_t = make<tensor>(list<int>{input_batch->shape_[0], 1});
                                for(int row = 0; row < input_batch->shape_[0]; row++) {
                                    input_t->at({row, 0}) = input_batch->at({row, t});
                                }
                                
                                output = input_t;
                                for(auto& p : network) {
                                    output = output->forward(p);
                                }
                                timestep_outputs.push(output);
                                
                                int target_dim = targets->shape_[1] / seq_length;
                                auto target_t = make<tensor>(list<int>{target_batch->shape_[0], target_dim});
                                for(int row = 0; row < target_batch->shape_[0]; row++) {
                                    for(int col = 0; col < target_dim; col++) {
                                        target_t->at({row, col}) = target_batch->at({row, t * target_dim + col});
                                    }
                                }
                                
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
                                auto input_t = make<tensor>(list<int>{1, 1});
                                input_t->at({0, 0}) = inputs->at({i, t});
                                
                                output = input_t;
                                for(auto& p : network) {
                                    p.mode = TRAIN;
                                    output = output->forward(p);
                                }
                                
                                int target_dim = targets->shape_[1] / seq_length;
                                auto target_t = make<tensor>(list<int>{1, target_dim});
                                for(int col = 0; col < target_dim; col++) {
                                    target_t->at({0, col}) = targets->at({i, t * target_dim + col});
                                }
                                
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

                print("Time 1: ",time_1/1000000,"ms");
                print("Time 2: ",time_2/1000000,"ms");
                print("Time 3: ",time_3/1000000,"ms");
                print("Time 4: ",time_4/1000000,"ms");
                print("Time 5: ",time_5/1000000,"ms");
                print("Time 6: ",time_6/1000000,"ms");
                print("Backpass times:");
                for(auto e : time_map.entrySet()) {
                    print(" OPP: ",e.key,": ",e.value/1000000,"ms");
                }
                // print("Forwardpass times:");
                // for(auto e : f_time_map.entrySet()) {
                //     print(" OPP: ",e.key,": ",e.value/1000000,"ms");
                // }

                time_1 = 0; time_2 = 0; time_3 = 0; time_4 = 0; time_5 = 0; time_6 = 0;
                time_map.clear(); f_time_map.clear();

                if(std::isnan(loss_value) || std::isinf(loss_value)) {
                    print("Exploded at epoch ", epoch);
                    print("w1 max: ", params[0]->as_matrix().maxCoeff());
                    print("w1 min: ", params[0]->as_matrix().minCoeff());
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