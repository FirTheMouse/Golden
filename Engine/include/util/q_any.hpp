#pragma once

#include <iostream>
#include <atomic>
#include <util/util.hpp>
#include<util/q_object.hpp>
#include<util/q_map.hpp>
#include <typeinfo>
#include <new>

namespace Golden {


/// @brief Just a wrapper, my own version of any that in the future I could turn into something better once I know more
class q_any {
    private:
        // The actual storage is now handled by std::any
        // This gives us proper destruction, copying, and type safety
        std::any storage;
        
    public:
        // Default constructor creates empty q_any
        q_any() = default;
        
        // Constructor from any type (excluding q_any to prevent recursion)
        // This mirrors your original interface while using std::any internally
        template<typename T>
        q_any(T&& value, typename std::enable_if<!std::is_same<typename std::decay<T>::type, q_any>::value>::type* = nullptr) 
            : storage(std::forward<T>(value)) {
            // std::any handles the type storage and lifetime management automatically
        }
        
        // Copy constructor - std::any handles the deep copying for us
        q_any(const q_any& other) : storage(other.storage) {
            // std::any's copy constructor properly handles copying the contained object
            // including calling copy constructors and managing memory correctly
        }
        
        // Assignment from another q_any
        q_any& operator=(const q_any& other) {
            if (this != &other) {
                if (other.storage.has_value()) {
                    storage = other.storage;
                } else {
                    storage.reset(); // explicitly clear to avoid dangling
                }
            }
            return *this;
        }
        
        // Assignment operator for non-q_any types
        // This maintains your original interface while leveraging std::any's robustness
        template<typename T>
        typename std::enable_if<!std::is_same<typename std::decay<T>::type, q_any>::value, q_any&>::type
        operator=(T&& value) {
            storage = std::forward<T>(value);  // std::any handles destruction of old value
            return *this;
        }
        
        // Store a value - this maintains your original method interface
        // while internally using std::any's assignment which handles everything properly
        template<typename T>
        void store(T&& value) {
            storage = std::forward<T>(value);
            // No need for manual type tracking or placement new
            // std::any handles all the complexity for us
        }
        
        // Check if the q_any contains a value
        // This maps directly to std::any's has_value() method
        bool empty() const {
            return !storage.has_value();
        }
        
        // Get type information - using std::any's type method
        // This is more reliable than hash codes since it gives you the actual type_info
        const std::type_info& type() const {
            return storage.type();
        }
        
        // Get type hash for compatibility with your existing code
        // This maintains the same interface you had before
        size_t get_type_hash() const {
            return storage.has_value() ? storage.type().hash_code() : 0;
        }
        
        // Reset to empty state
        // std::any's reset() properly calls destructors and cleans up memory
        void reset() {
            storage.reset();
        }
        
        // Friend functions for casting - these will delegate to std::any_cast
        template<typename T>
        friend T& q_any_cast(q_any& operand);
        
        template<typename T>
        friend const T& q_any_cast(const q_any& operand);
        
        template<typename T>
        friend T* q_any_cast(q_any* operand);
        
        template<typename T>
        friend const T* q_any_cast(const q_any* operand);
    };
    
    // Casting functions that wrap std::any_cast
    // These maintain your q_any_cast interface while using the proven std::any_cast implementation
    
    template<typename T>
    T& q_any_cast(q_any& operand) {
        // std::any_cast handles all the type checking and safe casting for us
        // It will throw std::bad_any_cast if the types don't match
        try {
            return std::any_cast<T&>(operand.storage);
        } catch (const std::bad_any_cast& e) {
            print("q_any::113 bad cast - requested type: ", typeid(T).name());
            print("q_any::113 bad cast - stored type: ", operand.storage.type().name());
            // Re-throw as std::bad_cast to maintain your original interface
            throw std::bad_cast();
        }
    }
    
    template<typename T>
    const T& q_any_cast(const q_any& operand) {
        try {
            return std::any_cast<const T&>(operand.storage);
        } catch (const std::bad_any_cast& e) {
            print("q_any::123 bad cast");
            throw std::bad_cast();
        }
    }
    
    template<typename T>
    T* q_any_cast(q_any* operand) {
        // For pointer versions, return nullptr on failure instead of throwing
        // This matches your original interface behavior
        if (!operand) {
            return nullptr;
        }
        
        return std::any_cast<T>(&operand->storage);
        // std::any_cast with pointer returns nullptr on type mismatch
        // so no exception handling needed here
    }
    
    template<typename T>
    const T* q_any_cast(const q_any* operand) {
        if (!operand) {
            return nullptr;
        }
        
        return std::any_cast<T>(&operand->storage);
    }

// Old and tombstoned AI generated version, come back to this in the future

// class q_any {
// private:
//     // Storage for the actual data - adjust size as needed
//     alignas(std::max_align_t) char storage[64];
//     std::atomic<size_t> type_hash{0};
//     std::atomic<bool> has_value{false};
    
// public:
//     q_any() = default;
    
//     // Constructor from any type (but NOT q_any itself)
//     template<typename T>
//     q_any(T&& value, typename std::enable_if<!std::is_same<typename std::decay<T>::type, q_any>::value>::type* = nullptr) {
//         store(std::forward<T>(value));
//     }
    
//     // Copy constructor - handle q_any copying without recursion
//     q_any(const q_any& other) {
//         if (other.has_value.load()) {
//             std::memcpy(storage, other.storage, sizeof(storage));
//             type_hash.store(other.type_hash.load());
//             has_value.store(true);
//         }
//     }
    
//     // Assignment from another q_any
//     q_any& operator=(const q_any& other) {
//         if (this != &other) {
//             reset();
//             if (other.has_value.load()) {
//                 std::memcpy(storage, other.storage, sizeof(storage));
//                 type_hash.store(other.type_hash.load());
//                 has_value.store(true);
//             }
//         }
//         return *this;
//     }
    
//     // Assignment operator - only for non-q_any types
//     template<typename T>
//     typename std::enable_if<!std::is_same<typename std::decay<T>::type, q_any>::value, q_any&>::type
//     operator=(T&& value) {
//         store(std::forward<T>(value));
//         return *this;
//     }
    
//     // Store a value
//     template<typename T>
//     void store(T&& value) {
//         using DecayedT = typename std::decay<T>::type;
//         static_assert(sizeof(DecayedT) <= sizeof(storage), "Type too large for q_any storage");
//         static_assert(alignof(DecayedT) <= alignof(std::max_align_t), "Type alignment too strict");
        
//         // Destroy existing value if any
//         reset();
        
//         // Construct new value in place
//         new(storage) DecayedT(std::forward<T>(value));
//         type_hash.store(typeid(DecayedT).hash_code());
//         has_value.store(true);
//     }
    
//     // Check if has value
//     bool empty() const {
//         return !has_value.load();
//     }
    
//     // Get type hash
//     size_t get_type_hash() const {
//         return type_hash.load();
//     }
    
//     // Reset to empty state
//     void reset() {
//         // In a full implementation, you'd need to call the destructor
//         // For now, just mark as empty
//         has_value.store(false);
//         type_hash.store(0);
//     }
    
//     // Friend function for casting
//     template<typename T>
//     friend T& q_any_cast(q_any& operand);
    
//     template<typename T>
//     friend const T& q_any_cast(const q_any& operand);
    
//     template<typename T>
//     friend T* q_any_cast(q_any* operand);
    
//     template<typename T>
//     friend const T* q_any_cast(const q_any* operand);
// };

// // The casting functions - Firian style!
// template<typename T>
// T& q_any_cast(q_any& operand) {
//     using DecayedT = typename std::decay<T>::type;
    
//     if (operand.empty()) {
//         throw std::bad_cast(); // Or your preferred error handling
//     }
    
//     size_t target_hash = typeid(DecayedT).hash_code();
//     if (operand.type_hash.load() != target_hash) {
//         throw std::bad_cast();
//     }
    
//     return *reinterpret_cast<DecayedT*>(operand.storage);
// }

// template<typename T>
// const T& q_any_cast(const q_any& operand) {
//     using DecayedT = typename std::decay<T>::type;
    
//     if (operand.empty()) {
//         throw std::bad_cast();
//     }
    
//     size_t target_hash = typeid(DecayedT).hash_code();
//     if (operand.type_hash.load() != target_hash) {
//         throw std::bad_cast();
//     }
    
//     return *reinterpret_cast<const DecayedT*>(operand.storage);
// }

// template<typename T>
// T* q_any_cast(q_any* operand) {
//     if (!operand || operand->empty()) {
//         return nullptr;
//     }
    
//     using DecayedT = typename std::decay<T>::type;
//     size_t target_hash = typeid(DecayedT).hash_code();
//     if (operand->type_hash.load() != target_hash) {
//         return nullptr;
//     }
    
//     return reinterpret_cast<DecayedT*>(operand->storage);
// }

// template<typename T>
// const T* q_any_cast(const q_any* operand) {
//     if (!operand || operand->empty()) {
//         return nullptr;
//     }
    
//     using DecayedT = typename std::decay<T>::type;
//     size_t target_hash = typeid(DecayedT).hash_code();
//     if (operand->type_hash.load() != target_hash) {
//         return nullptr;
//     }
    
//     return reinterpret_cast<const DecayedT*>(operand->storage);
// }

}