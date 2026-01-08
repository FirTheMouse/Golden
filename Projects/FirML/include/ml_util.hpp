#pragma once

#include<util/util.hpp>
#include<util/engine_util.hpp>
#include<core/object.hpp>

#include<core/helper.hpp>
#include<util/logger.hpp>

#include <Eigen/Dense>

#include<core/gpuType.hpp>
#include<methods.hpp>

#ifdef __APPLE__
    #include <Accelerate/Accelerate.h>
#else
    extern "C" {
        #include <cblas.h>  // OpenBLAS or generic
    }
#endif

#define TIMERS 0

enum Opp {
    NOP, ADD, MATMUL, RELU, ADD_BIAS, SIGMOID, SOFTMAX, SOFTMAX_CE,
    SIGMOID_BCE, TANH, EMBED, CONCAT,
    MSE, CROSS_ENTROPY, //Loss types
    RNN_STEP, BATCH_NORM,  //Stateful
    ATTENTION, QKV_PROJ, RESHAPE, LAST_POSITION, DROPOUT
};

enum Reduction { NONE, MEAN, SUM };

enum Device { CPU, GPU };

enum Mode { TRAIN, EVAL };

enum class TType {
    FLOAT32,
    FLOAT16,  // For future
    INT32,    // For future
    INT8,     // For quantization
    BOOL      // For masks
};

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

// Storage: Shared data container using q_object refcounting
struct Storage : public q_object {
    list<float> data;
    
    Storage() = default;
    
    Storage(int size) {
        data.resize(size);
        for(int i = 0; i < size; i++) {
            data[i] = 0.0f;
        }
    }
    
    Storage(list<float>&& d) : data(std::move(d)) {}
};

class tensor : public q_object {
public:
    list<float> grad_; 
    list<int> shape_;
    list<int> strides_;

    g_ptr<Storage> storage_ = nullptr;
    int storage_offset_ = 0;
    
    map<std::string, list<float>> cache_;
    map<std::string, g_ptr<tensor>> cache_tensors_;

    Device device_ = CPU;
    void* gpu_data_ = nullptr;
    void* gpu_grad_ = nullptr;
    inline static Golden::u_allocator* gpu_allocator;
    
    Opp op_ = NOP;
    list<g_ptr<tensor>> inputs_;
    Reduction reduction_ = MEAN;
    TType t_type_ = TType::FLOAT32; 
    bool requires_grad_ = true;


    static list<int> derive_strides(const list<int>& shape) {
        list<int> strides;
        int stride = 1;

        for(int i=shape.length()-1;i>=0;i--) {
            strides.insert(stride,0);
            stride *= shape[i];
        }
        return strides;
    }

    int flatten_index(const list<int>& indices) const {
        int flat_idx = 0;
        for(int i=0;i<indices.length();i++) {
            flat_idx += indices[i] * strides_[i];
        }
        return flat_idx;
    }

    // Default constructor
    tensor() {}

    // Create tensor with shape
    tensor(const list<int>& shape) : shape_(shape) {
        strides_ = derive_strides(shape);
        int total = numel();
        
        storage_ = make<Storage>(total);
        
        grad_.resize(total);
        for(int i = 0; i < total; i++) {
            grad_[i] = 0.0f;
        }
    }
    
    // Create tensor with shape and data
    tensor(const list<int>& shape, const list<float>& data) : shape_(shape) {
        strides_ = derive_strides(shape);
        
        storage_ = make<Storage>();
        storage_->data = data;  // Copy data into storage
        
        grad_.resize(numel());
        for(int i = 0; i < numel(); i++) {
            grad_[i] = 0.0f;
        }
    }
    
    // Copy constructor: share storage
    tensor(const tensor& other) 
        : storage_(other.storage_),  // g_ptr handles refcount automatically!
            shape_(other.shape_),
            strides_(other.strides_),
            storage_offset_(other.storage_offset_),
            device_(other.device_),
            gpu_data_(other.gpu_data_),
            gpu_grad_(other.gpu_grad_),
            op_(other.op_),
            reduction_(other.reduction_),
            t_type_(other.t_type_) {
        
        // Gradients are NOT shared
        grad_.resize(other.grad_.length());
        for(int i = 0; i < grad_.length(); i++) {
            grad_[i] = 0.0f;
        }
    }
    
    // Destructor - g_ptr handles storage cleanup automatically!
    ~tensor() {
        if(gpu_data_ && gpu_allocator) {
            gpu_allocator->free(gpu_data_);
            gpu_allocator->free(gpu_grad_);
        }
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

    g_ptr<tensor> get_batch(int start, int batch_size) {
        if(ndim() < 2) {
            print("tensor::get_batch: requires at least 2D tensor!");
            return nullptr;
        }
        
        int rows = std::min(batch_size, shape_[0] - start);
        auto result = make<tensor>(list<int>{rows, shape_[1]});
        
        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < shape_[1]; j++) {
                result->at({i, j}) = at({start + i, j});
            }
        }
        
        return result;
    }

    g_ptr<tensor> get_batch_by_indices(const list<int>& indices) {
        if(ndim() < 2) {
            print("tensor::get_batch_by_indices: requires at least 2D tensor!");
            return nullptr;
        }
        
        auto result = make<tensor>(list<int>{(int)indices.length(), shape_[1]});
        
        if(is_contiguous()) {
            int row_size = shape_[1];
            for(int i = 0; i < indices.length(); i++) {
                float* src = storage_->data.data() + storage_offset_ + indices[i] * row_size;
                float* dst = result->storage_->data.data() + i * row_size;
                std::memcpy(dst, src, row_size * sizeof(float));
            }
        } else {
            // Fallback to element-wise
            for(int i = 0; i < indices.length(); i++) {
                for(int j = 0; j < shape_[1]; j++) {
                    result->at({i, j}) = at({indices[i], j});
                }
            }
        }
        
        return result;
    }

       
    inline float& flat(int i) { 
        return storage_->data[storage_offset_ + i]; 
    }
    
    inline const float& flat(int i) const { 
        return storage_->data[storage_offset_ + i]; 
    }
    
    inline float& at(const list<int>& indices) {
        return storage_->data[storage_offset_ + flatten_index(indices)];
    }
    
    inline const float& at(const list<int>& indices) const {
        return storage_->data[storage_offset_ + flatten_index(indices)];
    }
    
    bool is_contiguous() const {
        return storage_offset_ == 0 && strides_ == derive_strides(shape_);
    }
    
    void make_contiguous() {
        if(is_contiguous()) return;
        
        auto new_storage = make<Storage>(numel());
        
        for(int i = 0; i < numel(); i++) {
            new_storage->data[i] = flat(i);
        }
        
        storage_ = new_storage;
        strides_ = derive_strides(shape_);
        storage_offset_ = 0;
    }
    
    g_ptr<tensor> clone() const {
        auto result = make<tensor>(shape_);
        
        for(int i = 0; i < numel(); i++) {
            result->flat(i) = flat(i);
        }
        
        result->device_ = device_;
        result->t_type_ = t_type_;
        
        if(device_ == GPU) result->to_gpu();
        
        return result;
    }
    
    g_ptr<tensor> reshape(const list<int>& new_shape) {
        // Handle -1 (infer dimension)
        int infer_idx = -1;
        int known_size = 1;
        for(int i = 0; i < new_shape.length(); i++) {
            if(new_shape[i] == -1) {
                if(infer_idx != -1) {
                    print("tensor::reshape: Can only infer one dimension!");
                    return nullptr;
                }
                infer_idx = i;
            } else {
                known_size *= new_shape[i];
            }
        }
        
        list<int> final_shape = new_shape;
        if(infer_idx != -1) {
            final_shape[infer_idx] = numel() / known_size;
        }
        
        int new_total = 1;
        for(int s : final_shape) new_total *= s;
        if(new_total != numel()) {
            print("tensor::reshape: size mismatch! Have ", numel(), " elements, trying to reshape to ", new_total);
            return nullptr;
        }
        
        auto result = make<tensor>();
        result->storage_ = storage_;  
        result->shape_ = final_shape;
        result->strides_ = derive_strides(final_shape);
        result->storage_offset_ = storage_offset_;
        result->device_ = device_;
        result->t_type_ = t_type_;
        
        result->grad_.resize(numel());
        for(int i = 0; i < numel(); i++) {
            result->grad_[i] = 0.0f;
        }
        
        return result;
    }
    
    g_ptr<tensor> transpose(int dim0, int dim1) {
        if(dim0 < 0) dim0 += ndim();
        if(dim1 < 0) dim1 += ndim();
        
        auto result = make<tensor>();
        result->storage_ = storage_;  
        result->storage_offset_ = storage_offset_;
        result->shape_ = shape_;
        result->strides_ = strides_;
        result->device_ = device_;
        result->t_type_ = t_type_;
        
        std::swap(result->shape_[dim0], result->shape_[dim1]);
        std::swap(result->strides_[dim0], result->strides_[dim1]);
        
        result->grad_.resize(numel());
        for(int i = 0; i < numel(); i++) {
            result->grad_[i] = 0.0f;
        }
        
        return result;
    }
    
    inline int numel() const {
        if(shape_.length() == 0) return 0;
        int total = 1;
        for(int s : shape_) total *= s;
        return total;
    }

    inline int ndim() const { return shape_.length(); }
    
    inline const list<int>& shape() const { return shape_; }
    
    inline int size(int dim) const {
        if(dim < 0) dim += shape_.length();
        return shape_[dim];
    }

    void to_gpu() {
        if(device_ == GPU || !gpu_allocator) return;
        
        make_contiguous();
        
        size_t bytes = numel() * sizeof(float);
        gpu_data_ = gpu_allocator->allocate(bytes);
        
        if(!gpu_data_) {
            print("ERROR: Failed to allocate GPU memory");
            return;
        }
        
        gpu_allocator->copy_to_gpu(gpu_data_, storage_->data.data(), bytes);
        
        size_t grad_bytes = grad_.length() * sizeof(float);
        if(grad_bytes > 0) {
            gpu_grad_ = gpu_allocator->allocate(grad_bytes);
            if(!gpu_grad_) {
                gpu_allocator->free(gpu_data_);
                gpu_data_ = nullptr;
                print("ERROR: Failed to allocate GPU gradient memory");
                return;
            }
            gpu_allocator->memset_gpu(gpu_grad_, 0, grad_bytes);
        }
        
        device_ = GPU;
    }

    void to_cpu() {
        if(device_ == CPU || !gpu_data_) return;
        
        // Ensure contiguous before copying back
        if(!is_contiguous()) {
            make_contiguous();
        }
        
        size_t bytes = numel() * sizeof(float);
        gpu_allocator->copy_to_cpu(storage_->data.data(), gpu_data_, bytes);
        
        if(gpu_data_) gpu_allocator->free(gpu_data_);
        if(gpu_grad_) gpu_allocator->free(gpu_grad_);
        
        gpu_data_ = nullptr;
        gpu_grad_ = nullptr;
        device_ = CPU;
    }

    void sync_to_cpu() {
        if(device_ == GPU && gpu_data_) {
            if(!is_contiguous()) {
                print("WARNING: sync_to_cpu on non-contiguous tensor");
                return;
            }
            size_t bytes = numel() * sizeof(float);
            gpu_allocator->copy_to_cpu(storage_->data.data(), gpu_data_, bytes);
        }
    }

    void fill(float value) {
        for(int i = 0; i < numel(); i++) {
            flat(i) = value;
        }
    }
    
    void randn(float mean = 0.0f, float stddev = 1.0f) {
        for(int i = 0; i < numel(); i++) {
            flat(i) = randf(std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::max()) * stddev + mean;
        }
    }
    
    void zero_grad() {
        for(int i = 0; i < grad_.length(); i++) {
            grad_[i] = 0.0f;
        }
    }
    

    Eigen::MatrixXf as_matrix() const {
        if(ndim() < 2) {
            print("ERROR: as_matrix requires at least 2D tensor");
            return Eigen::MatrixXf();
        }
        
        if(!is_contiguous()) {
            print("ERROR: as_matrix requires contiguous tensor");
            return Eigen::MatrixXf();
        }
        
        int rows = shape_[ndim() - 2];
        int cols = shape_[ndim() - 1];
        
        // Map as ROW-MAJOR since our storage is row-major!
        return Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
            const_cast<float*>(storage_->data.data() + storage_offset_), 
            rows, cols
        );
    }
    
    static g_ptr<tensor> from_matrix(const Eigen::MatrixXf& mat) {
        list<int> shape = {(int)mat.rows(), (int)mat.cols()};
        list<float> data;
        data.resize(mat.size());
        
        // Copy row-by-row to handle column-major to row-major conversion
        for(int i = 0; i < mat.rows(); i++) {
            for(int j = 0; j < mat.cols(); j++) {
                data[i * mat.cols() + j] = mat(i, j);
            }
        }
        
        return make<tensor>(shape, data);
    }
    
    
    void print_info() const {
        print("Tensor shape: [");
        for(int i = 0; i < shape_.length(); i++) {
            print(shape_[i]);
            if(i < shape_.length() - 1) print(", ");
        }
        print("] (", numel(), " elements)");
    }

    static g_ptr<tensor> ensure_shape(g_ptr<tensor> t, const list<int>& target_shape) {
        if(t->shape_ == target_shape) return t;
        return t->broadcast_to(target_shape);
    }

    static bool can_broadcast(const list<int>& shape_a, const list<int>& shape_b) {
        int max_dims = std::max(shape_a.length(), shape_b.length());
        
        for(int i = 0; i < max_dims; i++) {
            int dim_a = (i < shape_a.length()) ? shape_a[shape_a.length() - 1 - i] : 1;
            int dim_b = (i < shape_b.length()) ? shape_b[shape_b.length() - 1 - i] : 1;
            
            if(dim_a != dim_b && dim_a != 1 && dim_b != 1) {
                return false;
            }
        }
        return true;
    }

    static list<int> broadcast_shape(const list<int>& shape_a, const list<int>& shape_b) {
        int max_dims = std::max(shape_a.length(), shape_b.length());
        list<int> result;
        
        for(int i = 0; i < max_dims; i++) {
            int dim_a = (i < shape_a.length()) ? shape_a[shape_a.length() - 1 - i] : 1;
            int dim_b = (i < shape_b.length()) ? shape_b[shape_b.length() - 1 - i] : 1;
            
            if(dim_a != dim_b && dim_a != 1 && dim_b != 1) {
                print("tensor::broadcast_shape: Shapes not compatible for broadcasting!");
                return list<int>();
            }
            
            result.insert(std::max(dim_a, dim_b), 0);
        }
        
        return result;
    }

    g_ptr<tensor> broadcast_to(const list<int>& target_shape) {
        if(!can_broadcast(shape_, target_shape)) {
            print("ERROR: Cannot broadcast shape to target");
            return nullptr;
        }
        
        auto result = make<tensor>(target_shape);
        
        broadcast_copy(result, this, target_shape, shape_);
        
        return result;
    }

    static void broadcast_copy(g_ptr<tensor> dest, g_ptr<tensor> src,
                            const list<int>& dest_shape, const list<int>& src_shape) {
        int total = dest->numel();
        
        for(int flat_idx = 0; flat_idx < total; flat_idx++) {
            list<int> dest_idx = unravel_index(flat_idx, dest_shape);
            
            list<int> src_idx;
            int dim_offset = dest_shape.length() - src_shape.length();
            
            for(int i = 0; i < dest_idx.length(); i++) {
                if(i < dim_offset) {
                    continue;
                }
                int src_dim_idx = i - dim_offset;
                int src_dim_size = src_shape[src_dim_idx];
                
                if(src_dim_size == 1) {
                    src_idx.push(0);
                } else {
                    src_idx.push(dest_idx[i]);
                }
            }
            
            dest->flat(flat_idx) = src->at(src_idx);
        }
    }

    static list<int> unravel_index(int flat_idx, const list<int>& shape) {
        list<int> indices;
        for(int i = shape.length() - 1; i >= 0; i--) {
            indices.insert(flat_idx % shape[i], 0);
            flat_idx /= shape[i];
        }
        return indices;
    }

    static void unbroadcast_grad(g_ptr<tensor> target, const list<float>& grad_data,
         const list<int>& grad_shape, const list<int>& target_shape) {
        if(grad_shape == target_shape) {
            // No broadcasting happened, simple accumulation
            for(int i = 0; i < grad_data.length(); i++) {
                target->grad_[i] += grad_data[i];
            }
            return;
        }
        
        // Broadcasting happened, need to sum along broadcast dimensions
        int grad_numel = 1;
        for(int s : grad_shape) grad_numel *= s;
        
        for(int i = 0; i < grad_numel; i++) {
            list<int> grad_idx = tensor::unravel_index(i, grad_shape);
            
            list<int> target_idx;
            int dim_offset = grad_shape.length() - target_shape.length();
            
            for(int j = 0; j < grad_idx.length(); j++) {
                if(j < dim_offset) continue;  // Dimension doesn't exist in target
                
                int target_dim = j - dim_offset;
                if(target_shape[target_dim] == 1) {
                    target_idx.push(0);  // This dimension was broadcast, use 0
                } else {
                    target_idx.push(grad_idx[j]);
                }
            }
            
            target->grad_[target->flatten_index(target_idx)] += grad_data[i];
        }
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
                    bool is_bias = (shape_[0] != other->shape_[0]) && 
                    (shape_[0] > 1 && other->shape_[0] == 1 ||
                     other->shape_[0] > 1 && shape_[0] == 1);
     
                    if(is_bias && shape_.length() == 2 && other->shape_.length() == 2) {
                        // Fast path for bias addition
                        result->shape_ = (shape_[0] > other->shape_[0]) ? shape_ : other->shape_;
                        result->strides_ = derive_strides(result->shape_);
                        result->storage_ = make<Storage>(result->numel());
                        
                        // Direct loop - no broadcasting needed
                        int rows = result->shape_[0];
                        int cols = result->shape_[1];
                        g_ptr<tensor> data_t = (shape_[0] > 1) ? this : other;
                        g_ptr<tensor> bias_t = (shape_[0] == 1) ? this : other;
                        
                        for(int i = 0; i < rows; i++) {
                            for(int j = 0; j < cols; j++) {
                                result->at({i, j}) = data_t->at({i, j}) + bias_t->at({0, j});
                            }
                        }
                    }
                    else {
                        // Determine broadcast shape
                        list<int> result_shape = tensor::broadcast_shape(shape_, other->shape_);
                        if(result_shape.empty()) {
                            print("ADD: Cannot broadcast shapes");
                            return nullptr;
                        }
                        
                        result->shape_ = result_shape;
                        result->strides_ = derive_strides(result_shape);
                        result->storage_ = make<Storage>(result->numel());
                        result->grad_.resize(result->numel());
                        for(int i = 0; i < result->numel(); i++) {
                            result->grad_[i] = 0.0f;
                        }
                        
                        // Broadcast both inputs if needed
                        auto a_broadcast = (shape_ == result_shape) ? 
                            g_ptr<tensor>(this) : broadcast_to(result_shape);
                        auto b_broadcast = (other->shape_ == result_shape) ? 
                            other : other->broadcast_to(result_shape);
                        
                        // Element-wise addition
                        for(int i = 0; i < result->numel(); i++) {
                            result->flat(i) = a_broadcast->flat(i) + b_broadcast->flat(i);
                        }
                        result->device_ = CPU;
                    }
                }
                else {// Backward: gradient flows to both inputs with unbroadcasting

                    bool is_bias = (inputs_[0]->shape_[0] > 1 && inputs_[1]->shape_[0] == 1);
                    #if TIMERS
                    Log::Line l; l.start();
                    #endif
                    if(is_bias) {
                        // Fast path: sum over batch dimension
                        int rows = inputs_[0]->shape_[0];
                        int cols = inputs_[0]->shape_[1];
                        
                        // Gradient to data (first input) - direct copy
                        for(int i = 0; i < grad_.length(); i++) {
                            inputs_[0]->grad_[i] += grad_[i];
                        }
                        
                        // Gradient to bias (second input) - sum over rows
                        for(int j = 0; j < cols; j++) {
                            float sum = 0.0f;
                            for(int i = 0; i < rows; i++) {
                                sum += grad_[i * cols + j];
                            }
                            inputs_[1]->grad_[j] += sum;
                        }
                    } else {
                        list<float> grad_data;
                        grad_data.resize(grad_.length());
                        for(int i = 0; i < grad_.length(); i++) {
                            grad_data[i] = grad_[i];
                        }
                        tensor::unbroadcast_grad(inputs_[0], grad_data, shape_, inputs_[0]->shape_);
                        tensor::unbroadcast_grad(inputs_[1], grad_data, shape_, inputs_[1]->shape_);
                    }
                        
                    #if TIMERS
                    print("     ADD - Unbroadcast time: ",l.end()/1000000,"ms"); l.start();
                    print("         ADD - Going backwards of type: ",inputs_[0]->op_); 
                    #endif
                    inputs_[0]->backward();
                    #if TIMERS
                    print("         ADD - Time: ",l.end()/1000000,"ms"); l.start();
                    print("         ADD - Going backwards of type: ",inputs_[1]->op_);
                    #endif
                    inputs_[1]->backward();
                    #if TIMERS
                    print("         ADD - Time: ",l.end()/1000000,"ms");
                    #endif
                }
                break;
            }
            case MATMUL: {
                if(is_forward) {
                    if(ndim() < 2 || other->ndim() < 2) {
                        print("MATMUL: requires at least 2D tensors");
                        return nullptr;
                    }
                    
                    int m = shape_[ndim() - 2];  // rows of A
                    int k = shape_[ndim() - 1];  // cols of A / rows of B
                    int n = other->shape_[other->ndim() - 1];  // cols of B
                    
                    if(k != other->shape_[other->ndim() - 2]) {
                        print("MATMUL: inner dimensions don't match");
                        return nullptr;
                    }
                    
                    // Result shape
                    result->shape_ = shape_;
                    result->shape_[ndim() - 1] = n;
                    result->strides_ = derive_strides(result->shape_);
                    result->storage_ = make<Storage>(result->numel());
                    result->grad_.resize(result->numel());
                    for(int i = 0; i < result->numel(); i++) {
                        result->grad_[i] = 0.0f;
                    }
                    
                    // Ensure contiguous for Eigen
                    auto a_contig = is_contiguous() ? g_ptr<tensor>(this) : clone();
                    auto b_contig = other->is_contiguous() ? other : other->clone();
                    
                    if(!a_contig->is_contiguous()) a_contig->make_contiguous();
                    if(!b_contig->is_contiguous()) b_contig->make_contiguous();
                    
                    // Use Eigen with row-major specification
                    auto a_mat = a_contig->as_matrix();  // as_matrix() already handles row-major
                    auto b_mat = b_contig->as_matrix();
                    Eigen::MatrixXf c_mat = a_mat * b_mat;
                    
                    // Copy result - iterate to handle column-major to row-major
                    for(int i = 0; i < c_mat.rows(); i++) {
                        for(int j = 0; j < c_mat.cols(); j++) {
                            result->at({i, j}) = c_mat(i, j);
                        }
                    }
                    
                    result->device_ = CPU;
                }
                else { // Backward: grad_A = grad_C @ B^T, grad_B = A^T @ grad_C
                    #if TIMERS
                    Log::Line l; l.start();
                    #endif
                    // Map our row-major gradient to Eigen row-major matrix
                    int grad_rows = shape_[ndim() - 2];
                    int grad_cols = shape_[ndim() - 1];
                    auto grad_mat = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                        grad_.data(), grad_rows, grad_cols);
                    
                    auto a_mat = inputs_[0]->as_matrix();
                    auto b_mat = inputs_[1]->as_matrix();
                    #if TIMERS
                    print("             MATMUL - duration of as_matrix: ",l.end()/1000000,"ms"); l.start();
                    #endif
                    
                    // Compute gradients
                    Eigen::MatrixXf grad_a = grad_mat * b_mat.transpose();
                    Eigen::MatrixXf grad_b = a_mat.transpose() * grad_mat;
                    #if TIMERS
                    print("             MATMUL - duration of transpose: ",l.end()/1000000,"ms"); l.start();
                    #endif
                    
                    // Accumulate gradients with proper row-major iteration
                    for(int i = 0; i < grad_a.rows(); i++) {
                        for(int j = 0; j < grad_a.cols(); j++) {
                            inputs_[0]->grad_[i * grad_a.cols() + j] += grad_a(i, j);
                        }
                    }
                    
                    for(int i = 0; i < grad_b.rows(); i++) {
                        for(int j = 0; j < grad_b.cols(); j++) {
                            inputs_[1]->grad_[i * grad_b.cols() + j] += grad_b(i, j);
                        }
                    }
                    #if TIMERS
                    print("             MATMUL - duration of accumulate: ",l.end()/1000000,"ms"); l.start();

                    print("             MATMUL - Going backwards of type: ",inputs_[0]->op_);
                    #endif
                    inputs_[0]->backward();
                    #if TIMERS
                    print("             MATMUL - Time: ",l.end()/1000000,"ms"); l.start();
                    print("             MATMUL - Going backwards of type: ",inputs_[1]->op_);
                    #endif
                    inputs_[1]->backward();
                    #if TIMERS
                    print("             MATMUL - Time: ",l.end()/1000000,"ms");
                    #endif
                }
                break;
            }
            case RELU: {
                if(is_forward) {
                    result->shape_ = shape_;
                    result->strides_ = strides_;
                    result->storage_ = make<Storage>(numel());
                    result->grad_.resize(numel());
                    for(int i = 0; i < numel(); i++) {
                        result->grad_[i] = 0.0f;
                    }
                    
                    // Apply ReLU
                    for(int i = 0; i < numel(); i++) {
                        result->flat(i) = std::max(0.0f, flat(i));
                    }
                    result->device_ = CPU;
                }
                else { // Backward: gradient passes through where input > 0
                    #if TIMERS
                    Log::Line l; l.start();
                    #endif
                    for(int i = 0; i < inputs_[0]->numel(); i++) {
                        float mask = (inputs_[0]->flat(i) > 0.0f) ? 1.0f : 0.0f;
                        inputs_[0]->grad_[i] += grad_[i] * mask;
                    }
                    #if TIMERS
                    print("                RELU - duration: ",l.end()/1000000,"ms"); l.start();
                    
                    print("                RELU - Going backwards of type: ",inputs_[0]->op_);
                    #endif
                    inputs_[0]->backward();
                    #if TIMERS
                    print("                RELU - Time: ",l.end()/1000000,"ms");
                    #endif
                }
                break;
            }
            case SOFTMAX_CE: {
                if(is_forward) {
                    int batch_size = shape_[0];
                    int num_classes = shape_[1];
                    
                    if(other->shape_[0] != batch_size || other->shape_[1] != num_classes) {
                        print("SOFTMAX_CE: shape mismatch");
                        return nullptr;
                    }
                    
                    // Result is a scalar
                    result->shape_ = {1, 1};
                    result->strides_ = derive_strides(result->shape_);
                    result->storage_ = make<Storage>(1);
                    result->grad_.resize(1);
                    result->grad_[0] = 0.0f;
                    
                    // Compute with Eigen for numerical stability
                    auto logits_mat = as_matrix();
                    auto targets_mat = other->as_matrix();
                    
                    // Log-softmax: x - max(x) - log(sum(exp(x - max(x))))
                    Eigen::VectorXf max_vals = logits_mat.rowwise().maxCoeff();
                    Eigen::MatrixXf shifted = logits_mat.colwise() - max_vals;
                    Eigen::MatrixXf exp_shifted = shifted.array().exp();
                    Eigen::VectorXf sum_exp = exp_shifted.rowwise().sum();
                    Eigen::VectorXf log_sum_exp = sum_exp.array().log() + max_vals.array();
                    
                    Eigen::MatrixXf log_softmax = logits_mat.colwise() - log_sum_exp;
                    
                    // Cross entropy
                    float loss_sum = -(targets_mat.array() * log_softmax.array()).sum();
                    float loss = (pass.reduction == MEAN) ? loss_sum / float(batch_size) : loss_sum;
                    
                    result->flat(0) = loss;
                    result->device_ = CPU;
                    
                    // Cache softmax for backward (gradient is just softmax - targets)
                    Eigen::MatrixXf softmax = exp_shifted.array().colwise() / sum_exp.array();
                    result->cache_.put("softmax_rows", list<float>{(float)batch_size});
                    result->cache_.put("softmax_cols", list<float>{(float)num_classes});
                    result->cache_tensors_.put("softmax", tensor::from_matrix(softmax));
                }
                else { // Backward: gradient = (softmax - targets) / batch_size
                    if(op_ != SOFTMAX_CE) return result;
                    
                    int batch_size = (int)cache_.get("softmax_rows")[0];
                    int num_classes = (int)cache_.get("softmax_cols")[0];
                    
                    auto softmax_tensor = cache_tensors_.get("softmax");
                    auto softmax_mat = softmax_tensor->as_matrix();
                    auto targets_mat = inputs_[1]->as_matrix();
                    
                    // Compute gradient
                    Eigen::MatrixXf grad_logits = softmax_mat - targets_mat;
                    
                    if(reduction_ == MEAN) {
                        grad_logits /= float(batch_size);
                    }
                    
                    // Note: upstream gradient (grad_[0]) is always 1.0 for loss
                    // No need to multiply by it
                    
                    // Accumulate to input gradients
                    for(int i = 0; i < grad_logits.rows(); i++) {
                        for(int j = 0; j < grad_logits.cols(); j++) {
                            inputs_[0]->grad_[i * grad_logits.cols() + j] += grad_logits(i, j);
                        }
                    }

                     // Don't backprop through targets
                    #if TIMERS
                    Log::Line l; l.start();
                    print("SOFTMAX_CE - Going backwards of type: ",inputs_[0]->op_);
                    #endif
                    inputs_[0]->backward();
                    #if TIMERS
                    print("SOFTMAX_CE - Backwards time ",l.end()/1000000,"ms");
                    #endif
                }
                break;
            }
            
            default:
                print("WARNING: invalid op in operate: ", op);
                break;
        }

        if(is_forward) {
            result->grad_.resize(result->numel());
            for(int i = 0; i < result->numel(); i++) {
                result->grad_[i] = 0.0f;
            }
            
            // Allocate GPU gradient if on GPU
            if(result->device_ == GPU && gpu_allocator) {
                size_t grad_bytes = result->numel() * sizeof(float);
                result->gpu_grad_ = gpu_allocator->allocate(grad_bytes);
                gpu_allocator->memset_gpu(result->gpu_grad_, 0, grad_bytes);
            }
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


static g_ptr<tensor> empty_like(g_ptr<tensor> t, const list<int>& shape) {
    auto result = make<tensor>(shape);
    result->device_ = t->device_;
    result->t_type_ = t->t_type_;
    return result;
}

static void prepare_result_device(g_ptr<tensor> result) {
    if(result->device_ == GPU && result->gpu_allocator) {
        size_t bytes = result->numel() * sizeof(float);
        result->gpu_data_ = result->gpu_allocator->allocate(bytes);
        result->gpu_grad_ = result->gpu_allocator->allocate(bytes);
        result->gpu_allocator->memset_gpu(result->gpu_grad_, 0, bytes);
    }
}

