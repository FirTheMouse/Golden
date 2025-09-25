#pragma once
#include<functional>
#include<atomic>
#include<future>
#include<util/list.hpp>
#include<util/q_object.hpp>

#define Q_LIST_DEBUG 0

// template<typename... Args>
// void print(Args&&... args) {
//   (std::cout << ... << args) << std::endl;
// }

using namespace Golden;

/// @brief This is not a ture concurrency primitve, it simply appears to be, index redirection is not a valid method.
/// All safe concurrency features of Golden are down to architecture, not some magical container.
template<typename T>
class q_list : public q_object {
private:
    using storage_type = std::conditional_t<
    std::is_base_of_v<q_object, T>,
    g_ptr<T>,        // Store g_ptr for Objects
    T                // Store value directly for primitives
    >;

    std::atomic<bool> is_processing{false};
    std::atomic<bool> is_adding{false};
    std::atomic<bool> is_removing{false};
    std::atomic<bool> is_swapping{false};
    
    size_t marked_index; //ID marked by modifying opperations
    size_t switch_index; //ID to return when requesting marked

    size_t switch_length; //Length to return during modifying opperations
public:

   list<storage_type> impl; //Implments but doesn't extend list
  // Default constructor
  q_list() = default;
    
  // Move constructor
  q_list(q_list&& other) noexcept 
      : q_object(std::move(other)),
        impl(std::move(other.impl)),
        is_processing(other.is_processing.load()),
        is_adding(other.is_adding.load()),
        is_removing(other.is_removing.load()),
        is_swapping(other.is_swapping.load()),
        marked_index(other.marked_index),
        switch_index(other.switch_index),
        switch_length(other.switch_length) {
      // Reset other's atomics
      other.is_processing.store(false);
      other.is_adding.store(false);
      other.is_removing.store(false);
      other.is_swapping.store(false);
  }
  
  // Move assignment
  q_list& operator=(q_list&& other) noexcept {
      if (this != &other) {
          q_object::operator=(std::move(other));
          impl = std::move(other.impl);
          is_processing.store(other.is_processing.load());
          is_adding.store(other.is_adding.load());
          is_removing.store(other.is_removing.load());
          is_swapping.store(other.is_swapping.load());
          marked_index = other.marked_index;
          switch_index = other.switch_index;
          switch_length = other.switch_length;
          
          // Reset other's atomics
          other.is_processing.store(false);
          other.is_adding.store(false);
          other.is_removing.store(false);
          other.is_swapping.store(false);
      }
      return *this;
  }
  
  // Delete copy operations
  q_list(const q_list&) = delete;
  q_list& operator=(const q_list&) = delete;

    void clear() {impl.clear();}

    bool isProccessing() {return is_processing.load();}
    bool isAdding() {return is_adding.load();}
    bool isRemoving() {return is_removing.load();}
    //Dormant code for now, intend to add swapping in the future for
    bool isSwapping() {return is_swapping.load();}

    inline T* begin() {return impl.begin();}
    inline T* end() {return impl.end();}
    inline T& operator[](size_t index) {
        return impl[index];
    }


    void remove(size_t index) {
        while (is_removing.load()||is_adding.load()) {
            std::this_thread::yield();
        }

        size_t last_index = impl.length()-1;
        switch_length = last_index; //Just happens to be the same
        marked_index = last_index;
        switch_index = index;

        is_removing.store(true);

        //If it's already the end value, no swap needed
        if(index!=last_index) {
            try {
            impl[index] = impl[last_index];
            } catch(const std::exception& e) {
                std::cout << "q_list::remove::57 Failed removal at index" << index << ": " << e.what()<<  std::endl;
            }
        }
        impl.pop();
        if (marked_index >= impl.length()) {          // we just removed the tail
            marked_index = impl.length();             // one past end
        }
        
        is_removing.store(false);
    }
    
    q_list<T>& operator<<(T value) {push(value); return *this;}

    template<typename U>
    typename std::enable_if<std::is_base_of<q_object, U>::value, void>::type
    push(U* raw_ptr) {
        push_impl(g_ptr<T>(raw_ptr));
    }
    
    // Push for g_ptr directly
    template<typename U>
    typename std::enable_if<std::is_base_of<q_object, U>::value, void>::type
    push(g_ptr<U> ptr) {
        push_impl(std::move(ptr));
    }
    
    // Push for primitives
    template<typename U>
    typename std::enable_if<!std::is_base_of<q_object, U>::value, void>::type
    push(U&& value) {
        push_impl(std::forward<U>(value));
    }

private:
    // Internal push implementation
    template<typename U>
    void push_impl(U&& value) {
        while (is_removing.load() || is_adding.load()) {
            std::this_thread::yield();
        }

        switch_length = impl.length();
        is_adding.store(true);
        impl.push(std::forward<U>(value));
        is_adding.store(false);
    }

public:

    // Get for Objects - returns reference to the object
    template<typename U = T>
    typename std::enable_if<std::is_base_of<q_object, U>::value, U&>::type
    get(size_t index, const std::string& from = "q_list::get::163") {
        auto& stored = get_storage(index, from);
        return *stored;  // Dereference g_ptr
    }

    // Get for primitives - returns reference to value
    template<typename U = T>
    typename std::enable_if<!std::is_base_of<q_object, U>::value, U&>::type
    get(size_t index, const std::string& from = "q_list::get::171") {
        return get_storage(index, from);
    }

    // Get g_ptr directly (Objects only)
    template<typename U = T>
    typename std::enable_if<std::is_base_of<q_object, U>::value, g_ptr<U>>::type
    get_ptr(size_t index, const std::string& from = "q_list::get_ptr::178") {
        return get_storage(index, from);
    }

private:

#if Q_LIST_DEBUG
    storage_type& get_storage(size_t index, const std::string& from = "undefined") {
        bool currently_adding = is_adding.load();
        bool currently_removing = is_removing.load();
        bool currently_swapping = is_swapping.load();
        size_t safe_marked_index = marked_index; 
        size_t safe_switch_index = switch_index; 

        if(currently_adding)
            {
                return impl.get(index,from.append(" | q_list::get()::95"));
            }
            else if(currently_removing)
            {
                if(index == safe_switch_index) {
                    std::cout << "[Warn "+from+"|q_list::get()::101] attempted to get an inactive value" << std::endl;
                    return impl.get(safe_marked_index,from+" | q_list::get()::100");}
                else if(index == safe_marked_index)
                    return impl.get(safe_switch_index,from+" | q_list::get()::103");
                else return impl.get(index,from+" | q_list::get()::104");
            }
            else if(currently_swapping)
            {
                if(index == switch_index)
                    return impl.get(marked_index,from+" | q_list::get()::108");
                else if(index == marked_index)
                    return impl.get(switch_index,from+" | q_list::get()::110");
                else return impl.get(index,from+" | q_list::get()::112");
            }
            else {
                if (index >= impl.length())           // array already shrunk
                    throw std::out_of_range("index out of bounds "+from);
                else
                return impl.get(index,from+" | q_list::get()::115");
            }
    }
#else
    storage_type& get_storage(size_t index, const std::string& from) {
    {
        bool currently_adding   = is_adding.load(std::memory_order_relaxed);
        bool currently_removing = is_removing.load(std::memory_order_relaxed);
        bool currently_swapping = is_swapping.load(std::memory_order_relaxed);

        size_t safe_marked_index  = marked_index; 
        size_t safe_switch_index  = switch_index;

        if (currently_adding)
        {
            return impl.get(index);
        }
        else if (currently_removing)
        {
            if (index == safe_switch_index)
                return impl.get(safe_marked_index);
            else if (index == safe_marked_index)
                return impl.get(safe_switch_index);
            else
                return impl.get(index);
        }
        else if (currently_swapping)
        {
            if (index == switch_index)
                return impl.get(marked_index);
            else if (index == marked_index)
                return impl.get(switch_index);
            else
                return impl.get(index);
        }
        else
        {
            return impl.get(index);
        }
    }
    }
#endif
public:

    size_t length() const {
        if(is_adding.load()||is_removing.load())
        {
            return switch_length;
        }
        else return impl.length();
    }

    template<typename U = T>
    typename std::enable_if<std::is_base_of<q_object, U>::value, U&>::type
    operator[](size_t index) {
        return get<U>(index);
    }
    
    template<typename U = T>
    typename std::enable_if<!std::is_base_of<q_object, U>::value, U&>::type
    operator[](size_t index) {
        return get<U>(index);
    }

    template<typename U>
    q_list& operator<<(U&& value) {
        push(std::forward<U>(value));
        return *this;
    }

    

    /// @todo Take a closer look at these serilization methods later
    //These are just stopgaps for now so I can continue with other dev stuff

    void saveBinary(std::ostream& out) const {
        // // Always serialize the current state (impl), not pending operations
        // uint32_t count = impl.length();
        // out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        // // For POD types, we can write directly
        // if constexpr (std::is_trivially_copyable_v<T>) {
        //     for (size_t i = 0; i < impl.length(); ++i) {
        //         out.write(reinterpret_cast<const char*>(&impl[i]), sizeof(T));
        //     }
        // } else {
        //     // For complex types, delegate to their saveBinary method
        //     for (size_t i = 0; i < impl.length(); ++i) {
        //         impl[i]->saveBinary(out);
        //     }
        // }
    }
    
    void loadBinary(std::istream& in) {
        // // Clear existing data
        // impl.clear();
        // pending_ops.clear();
        // copy_cache.clear();
        
        // uint32_t count;
        // in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        // if constexpr (std::is_trivially_copyable_v<T>) {
        //     for (uint32_t i = 0; i < count; ++i) {
        //         T item;
        //         in.read(reinterpret_cast<char*>(&item), sizeof(T));
        //         impl << item;
        //     }
        // } else {
        //     for (uint32_t i = 0; i < count; ++i) {
        //         auto item = make<typename T::element_type>(); // For g_ptr types
        //         item->loadBinary(in);
        //         impl << item;
        //     }
        // }
    }
    
    // For types that need custom serialization
    template<typename SerializeFunc>
    void saveBinaryCustom(std::ostream& out, SerializeFunc serialize) const {
        uint32_t count = impl.length();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (size_t i = 0; i < impl.length(); ++i) {
            serialize(out, impl[i]);
        }
    }
    
    template<typename DeserializeFunc>
    void loadBinaryCustom(std::istream& in, DeserializeFunc deserialize) {
        // impl.clear();
        // pending_ops.clear();
        // copy_cache.clear();
        
        // uint32_t count;
        // in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        // for (uint32_t i = 0; i < count; ++i) {
        //     T item = deserialize(in);
        //     impl << item;
        // }
    }
    
    list<T>& operator->() { return impl; }
    const list<T>& operator->() const { return impl; }
};

// //Performance boosted version
// template<typename T>
// class q_list {
// private:
//     list<T> impl; //Implments but doesn't extend list

//     std::atomic<bool> is_processing{false};
//     std::atomic<bool> is_adding{false};
//     std::atomic<bool> is_removing{false};
//     std::atomic<bool> is_swapping{false};
    
//     size_t marked_index; //ID marked by modifying opperations
//     size_t switch_index; //ID to return when requesting marked

//     size_t switch_length; //Length to return during modifying opperations
// public:

//     q_list() = default;
    
//     // Move constructor
//     q_list(q_list&& other) noexcept 
//         : impl(std::move(other.impl)),
//           is_processing(other.is_processing.load()),
//           is_adding(other.is_adding.load()),
//           is_removing(other.is_removing.load()),
//           is_swapping(other.is_swapping.load()),
//           marked_index(other.marked_index),
//           switch_index(other.switch_index),
//           switch_length(other.switch_length) {
//         // Reset other's atomics
//         other.is_processing.store(false);
//         other.is_adding.store(false);
//         other.is_removing.store(false);
//         other.is_swapping.store(false);
//     }
    
//     // Move assignment operator
//     q_list& operator=(q_list&& other) noexcept {
//         if (this != &other) {
//             impl = std::move(other.impl);
//             is_processing.store(other.is_processing.load());
//             is_adding.store(other.is_adding.load());
//             is_removing.store(other.is_removing.load());
//             is_swapping.store(other.is_swapping.load());
//             marked_index = other.marked_index;
//             switch_index = other.switch_index;
//             switch_length = other.switch_length;
            
//             // Reset other's atomics
//             other.is_processing.store(false);
//             other.is_adding.store(false);
//             other.is_removing.store(false);
//             other.is_swapping.store(false);
//         }
//         return *this;
//     }

//     void clear() {impl.clear();}

//     bool isProccessing() {return is_processing.load();}
//     bool isAdding() {return is_adding.load();}
//     bool isRemoving() {return is_removing.load();}
//     //Dormant code for now, intend to add swapping in the future for
//     bool isSwapping() {return is_swapping.load();}


//     void remove(size_t index) {
//         while (is_removing.load()||is_adding.load()) {
//             std::this_thread::yield();
//         }

//         size_t last_index = impl.length()-1;
//         switch_length = last_index; //Just happens to be the same
//         marked_index = last_index;
//         switch_index = index;

//         is_removing.store(true);

//         //If it's already the end value, no swap needed
//         if(index!=last_index) {
//             impl[index] = impl[last_index];
//         }
//         impl.pop();
        
//         is_removing.store(false);
//     }
    
//     template<typename TT>
//     q_list<T>& operator<<(TT&& value) {
//         push(std::forward<TT>(value)); 
//         return *this;
//     }

//     template<typename TT>
//     void push(TT&& value) {
//         while (is_removing.load()||is_adding.load()) {
//             std::this_thread::yield();
//         }

//         switch_length = impl.length();

//         is_adding.store(true);

//         impl.push(std::forward<TT>(value));  

//         is_adding.store(false);
//     }

//     size_t length() const {
//         if(is_adding.load()||is_removing.load())
//         {
//             return switch_length;
//         }
//         else return impl.length();
//     }

//     T& operator[](size_t index) {return get(index);}

//     T& get(size_t index,const std::string& from = "undefined")            // <-- note: no `from` parameter
//     {
//         bool currently_adding   = is_adding.load(std::memory_order_relaxed);
//         bool currently_removing = is_removing.load(std::memory_order_relaxed);
//         bool currently_swapping = is_swapping.load(std::memory_order_relaxed);

//         size_t safe_marked_index  = marked_index;   // copy once
//         size_t safe_switch_index  = switch_index;

//         if (currently_adding)
//         {
//             // while writer is still pushing, length() may grow,
//             // but index was already checked by caller
//             return impl.get(index);                 // <-- plain fast call
//         }
//         else if (currently_removing)
//         {
//             if (index == safe_switch_index)
//                 return impl.get(safe_marked_index);
//             else if (index == safe_marked_index)
//                 return impl.get(safe_switch_index);
//             else
//                 return impl.get(index);
//         }
//         else if (currently_swapping)
//         {
//             if (index == switch_index)
//                 return impl.get(marked_index);
//             else if (index == marked_index)
//                 return impl.get(switch_index);
//             else
//                 return impl.get(index);
//         }
//         else
//         {
//             return impl.get(index);
//         }
//     }


//     void saveBinary(std::ostream& out) const {
//         // // Always serialize the current state (impl), not pending operations
//         // uint32_t count = impl.length();
//         // out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
//         // // For POD types, we can write directly
//         // if constexpr (std::is_trivially_copyable_v<T>) {
//         //     for (size_t i = 0; i < impl.length(); ++i) {
//         //         out.write(reinterpret_cast<const char*>(&impl[i]), sizeof(T));
//         //     }
//         // } else {
//         //     // For complex types, delegate to their saveBinary method
//         //     for (size_t i = 0; i < impl.length(); ++i) {
//         //         impl[i]->saveBinary(out);
//         //     }
//         // }
//     }
    
//     void loadBinary(std::istream& in) {
//         // // Clear existing data
//         // impl.clear();
//         // pending_ops.clear();
//         // copy_cache.clear();
        
//         // uint32_t count;
//         // in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
//         // if constexpr (std::is_trivially_copyable_v<T>) {
//         //     for (uint32_t i = 0; i < count; ++i) {
//         //         T item;
//         //         in.read(reinterpret_cast<char*>(&item), sizeof(T));
//         //         impl << item;
//         //     }
//         // } else {
//         //     for (uint32_t i = 0; i < count; ++i) {
//         //         auto item = make<typename T::element_type>(); // For g_ptr types
//         //         item->loadBinary(in);
//         //         impl << item;
//         //     }
//         // }
//     }
    
//     // For types that need custom serialization
//     template<typename SerializeFunc>
//     void saveBinaryCustom(std::ostream& out, SerializeFunc serialize) const {
//         uint32_t count = impl.length();
//         out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
//         for (size_t i = 0; i < impl.length(); ++i) {
//             serialize(out, impl[i]);
//         }
//     }
    
//     template<typename DeserializeFunc>
//     void loadBinaryCustom(std::istream& in, DeserializeFunc deserialize) {
//         // impl.clear();
//         // pending_ops.clear();
//         // copy_cache.clear();
        
//         // uint32_t count;
//         // in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
//         // for (uint32_t i = 0; i < count; ++i) {
//         //     T item = deserialize(in);
//         //     impl << item;
//         // }
//     }
    
//     list<T>& operator->() { return impl; }
//     const list<T>& operator->() const { return impl; }
// };
