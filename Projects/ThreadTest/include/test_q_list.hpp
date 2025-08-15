#include<util/map.hpp>
#include<functional>
#include<atomic>
#include<future>

// template<typename... Args>
// void print(Args&&... args) {
//   (std::cout << ... << args) << std::endl;
// }

//If coming back to optimize this, look at the lock-free version below
//It's not needed right now (6/9/2025) but in the future it could be useful

//IF: UTIL 265 ERROR
//Race condition between expressing intent and execution
//(like getting closest enemies then using that on something)
//If this shows up, inspect potential fixes like safe get methods

//Performance boosted version
template<typename T>
class q_list {
private:
    list<T> impl; //Implments but doesn't extend list

    std::atomic<bool> is_processing{false};
    std::atomic<bool> is_adding{false};
    std::atomic<bool> is_removing{false};
    std::atomic<bool> is_swapping{false};
    
    size_t marked_index; //ID marked by modifying opperations
    size_t switch_index; //ID to return when requesting marked

    size_t switch_length; //Length to return during modifying opperations
public:
    void clear() {impl.clear();}

    bool isProccessing() {return is_processing.load();}
    bool isAdding() {return is_adding.load();}
    bool isRemoving() {return is_removing.load();}
    //Dormant code for now, intend to add swapping in the future for
    bool isSwapping() {return is_swapping.load();}


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
            impl[index] = impl[last_index];
        }
        impl.pop();
        
        is_removing.store(false);
    }
    
    q_list<T>& operator<<(T value) {push(value); return *this;}

    void push(const T& value) {
        while (is_removing.load()||is_adding.load()) {
            std::this_thread::yield();
        }

        switch_length = impl.length();

        is_adding.store(true);

        impl.push(value);

        is_adding.store(false);
    }

    size_t length() const {
        if(is_adding.load()||is_removing.load())
        {
            return switch_length;
        }
        else return impl.length();
    }

    T& operator[](size_t index) {return get(index);}

    T& get(size_t index)            // <-- note: no `from` parameter
    {
        bool currently_adding   = is_adding.load(std::memory_order_relaxed);
        bool currently_removing = is_removing.load(std::memory_order_relaxed);
        bool currently_swapping = is_swapping.load(std::memory_order_relaxed);

        size_t safe_marked_index  = marked_index;   // copy once
        size_t safe_switch_index  = switch_index;

        if (currently_adding)
        {
            // while writer is still pushing, length() may grow,
            // but index was already checked by caller
            return impl.get(index);                 // <-- plain fast call
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
};

// 06/18/2025 Debug version
/// @brief A thread safe list designed to function with the Sim system
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
    
//     q_list<T>& operator<<(T value) {push(value); return *this;}

//     void push(const T& value) {
//         while (is_removing.load()||is_adding.load()) {
//             std::this_thread::yield();
//         }

//         switch_length = impl.length();

//         is_adding.store(true);

//         impl.push(value);

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

//     T& get(size_t index,const std::string& from = "undefined") {
//         bool currently_adding = is_adding.load();
//         bool currently_removing = is_removing.load();
//         bool currently_swapping = is_swapping.load();
//         size_t safe_marked_index = marked_index; 
//         size_t safe_switch_index = switch_index; 

//         if(currently_adding)
//         {
//             return impl.get(index,from+" | q_list::get()::95");
//         }
//         else if(currently_removing)
//         {
//             if(index == safe_switch_index) {
//                 std::cout << "[Warn "+from+"|q_list::get()::101] attempted to get an inactive value" << std::endl;
//                 return impl.get(safe_marked_index,from+" | q_list::get()::100");}
//             else if(index == safe_marked_index)
//                 return impl.get(safe_switch_index,from+" | q_list::get()::103");
//             else return impl.get(index,from+" | q_list::get()::104");
//         }
//         else if(currently_swapping)
//         {
//             if(index == switch_index)
//                 return impl.get(marked_index,from+" | q_list::get()::108");
//             else if(index == marked_index)
//                 return impl.get(switch_index,from+" | q_list::get()::110");
//             else return impl.get(index,from+" | q_list::get()::112");
//         }
//         else {
//             return impl.get(index,from+" | q_list::get()::115");
//         }
//     }
// };

//6/9/2025 Version

// template<typename T>
// class q_list {
// private:
//     list<T> impl; //Implments but doesn't extend list
//     list<std::function<void(list<T>&)>> pending_ops; 
//     std::atomic<bool> is_processing{false};
    
//     // Copy cache for locked access
//     mutable map<size_t, T> copy_cache;  // Index -> copied value
//     mutable std::mutex copy_cache_mutex; 
// public:
//     size_t length() const {return impl.length();}
//     void clear() {impl.clear();}

//     bool isProcessing() {return is_processing.load();}

//     void removeSwap(size_t index) {
//         if (is_processing.load()) {
//             pending_ops << [index, this](list<T>& d) {
//                 if (index < d.length()) {
//                     size_t last_index = d.length() - 1;
//                     if (index != last_index) {
//                         d[index] = d[last_index];
//                     }
//                     d.pop();
                    
//                     // CRITICAL: Clear copy cache after structural change
//                     copy_cache.clear();
//                 }
//             };
//         } else {
//             // Direct execution
//             if (index < impl.length()) {
//                 size_t last_index = impl.length() - 1;
//                 if (index != last_index) {
//                     impl[index] = impl[last_index];
//                 }
//                 impl.pop();
//             }
//         }
//     }

//     void set(size_t index, const T& value) {
//         if (is_processing.load()) {
//             //Add a lambda to the qeue, a pending opperation to be executed once safe
//             pending_ops << [index, value](list<T>& d) {
//                 if (index < d.length()) d[index] = value;
//             };
//         } else { 
//             impl[index] = value;
//         }
//     }
    
//     void push(const T& value) {
//         if (is_processing.load()) {
//             pending_ops << [value](list<T>& d) { d << value; };
//         } else {
//             impl << value;
//         }
//     }

//     void operator<<(T value) {push(value);}

//     T& operator[](size_t index) {
//         if (is_processing.load()) {
//             std::lock_guard<std::mutex> lock(copy_cache_mutex);  // Protect access
//             if (!copy_cache.hasKey(index)) {
//                 copy_cache.put(index, impl.get(index,"q_list 83"));
//             }
//             return copy_cache.get(index);
//         } else {
//             return impl.get(index,"q_list 87");
//         }
//     }

//     T& get(size_t index,const std::string& from) {
//         if (is_processing.load()) {
//             std::lock_guard<std::mutex> lock(copy_cache_mutex);  // Protect access
//             if (!copy_cache.hasKey(index)) {
//                 copy_cache.put(index, impl.get(index,from+" | q_list 95"));
//             }
//             return copy_cache.get(index);
//         } else {
//             return impl.get(index,from+" | q_list 99");
//         }
//     }
    
//     void begin_processing() {
//         is_processing.store(true);
//         for(size_t i = 0; i < pending_ops.length(); ++i) {
//             pending_ops[i](impl);  // Call each operation function
//         }
//         pending_ops.clear();
//     }
    
//     void end_processing() {

//         std::lock_guard<std::mutex> lock(copy_cache_mutex);  
//         for(auto& key : copy_cache.keySet()) {
//             size_t index = key;
//             if (index < impl.length()) {
//                 impl[index] = copy_cache[index];
//             }
//         }
//         copy_cache.clear();

//         for(size_t i = 0; i < pending_ops.length(); ++i) {
//             pending_ops[i](impl);
//         }
//         pending_ops.clear();
        
//         is_processing.store(false);
//     }

//     /// @todo Take a closer look at these serilization methods later
//     //These are just stopgaps for now so I can continue with other dev stuff

//     void saveBinary(std::ostream& out) const {
//         // Always serialize the current state (impl), not pending operations
//         uint32_t count = impl.length();
//         out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
//         // For POD types, we can write directly
//         if constexpr (std::is_trivially_copyable_v<T>) {
//             for (size_t i = 0; i < impl.length(); ++i) {
//                 out.write(reinterpret_cast<const char*>(&impl[i]), sizeof(T));
//             }
//         } else {
//             // For complex types, delegate to their saveBinary method
//             for (size_t i = 0; i < impl.length(); ++i) {
//                 impl[i]->saveBinary(out);
//             }
//         }
//     }
    
//     void loadBinary(std::istream& in) {
//         // Clear existing data
//         impl.clear();
//         pending_ops.clear();
//         copy_cache.clear();
        
//         uint32_t count;
//         in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
//         if constexpr (std::is_trivially_copyable_v<T>) {
//             for (uint32_t i = 0; i < count; ++i) {
//                 T item;
//                 in.read(reinterpret_cast<char*>(&item), sizeof(T));
//                 impl << item;
//             }
//         } else {
//             for (uint32_t i = 0; i < count; ++i) {
//                 auto item = make<typename T::element_type>(); // For g_ptr types
//                 item->loadBinary(in);
//                 impl << item;
//             }
//         }
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
//         impl.clear();
//         pending_ops.clear();
//         copy_cache.clear();
        
//         uint32_t count;
//         in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
//         for (uint32_t i = 0; i < count; ++i) {
//             T item = deserialize(in);
//             impl << item;
//         }
//     }
    
//     list<T>& operator->() { return impl; }
//     const list<T>& operator->() const { return impl; }
// };

// A lock-free system for if the mutex version ever becomes an issue

// template<typename T>
// class q_list {
// private:
//     list<T> impl; //Implments but doesn't extend list
//     list<std::function<void(list<T>&)>> pending_ops; 
//     std::atomic<bool> is_processing{false};

//     thread_local static map<size_t, T> thread_copy_cache;
    
//     // Track which threads have caches for merging
//     mutable std::atomic<bool> merge_needed{false};
//     mutable list<map<size_t, T>*> thread_caches;
//     mutable std::mutex cache_list_mutex;  // Only for tracking cache pointers
// public:
//     void set(size_t index, const T& value) {
//         if (is_processing.load()) {
//             //Add a lambda to the qeue, a pending opperation to be executed once safe
//             pending_ops << [index, value](list<T>& d) {
//                 if (index < d.length()) d[index] = value;
//             };
//         } else { 
//             impl[index] = value;
//         }
//     }
    
//     void push(const T& value) {
//         if (is_processing.load()) {
//             pending_ops << [value](list<T>& d) { d << value; };
//         } else {
//             impl << value;
//         }
//     }

//     void operator<<(T value) {push(value);}

//     T& operator[](size_t index) {
//         if (is_processing.load()) {
//             // Each thread has its own cache - no contention!
//             if (!thread_copy_cache.hasKey(index)) {
//                 thread_copy_cache.put(index, impl[index]);
                
//                 // Register this thread's cache for merging (one-time setup)
//                 if (thread_copy_cache.keySet().length() == 1) {
//                     std::lock_guard<std::mutex> lock(cache_list_mutex);
//                     thread_caches << &thread_copy_cache;
//                 }
//                 merge_needed.store(true);
//             }
//             return thread_copy_cache.get(index);
//         } else {
//             return impl[index];
//         }
//     }
    
//     void begin_processing() {
//         is_processing.store(true);
//         merge_needed.store(false);
        
//         // Clear all thread caches from previous cycle
//         {
//             std::lock_guard<std::mutex> lock(cache_list_mutex);
//             for (auto* cache : thread_caches) {
//                 cache->clear();
//             }
//         }
        
//         // Execute pending operations
//         pending_ops([this](auto& op) { op(impl); });
//         pending_ops.clear();
//     }
    
//     void end_processing() {
//         if (merge_needed.load()) {
//             std::lock_guard<std::mutex> lock(cache_list_mutex);

//             for (auto* cache : thread_caches) {
//             for(auto& key : cache->keySet()) {
//                 size_t index = key;
//                 if (index < impl.length()) {
//                     impl[index] = cache->get(index);
//                 }
//             }
//             cache->clear();
//             }
//         }
//         is_processing.store(false);
//     }
    
//     list<T>& operator->() { return impl; }
//     const list<T>& operator->() const { return impl; }
// };
// template<typename T>
// thread_local map<size_t, T> q_list<T>::thread_copy_cache;