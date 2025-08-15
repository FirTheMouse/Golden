#pragma once
#include<util/q_list.hpp>

template<typename K,typename V>
struct q_entry
{
    K key;
    V value;
    q_entry() = default;
    q_entry(const K& k, const V& v) : key(k), value(v) {}
    q_entry(K&& k, V&& v) : key(std::move(k)), value(std::move(v)) {}
    q_entry(const q_entry& other) = default;
    q_entry(q_entry&& other) noexcept = default;
    q_entry& operator=(const q_entry& other) = default;
    q_entry& operator=(q_entry&& other) noexcept = default;
};


template<typename K,typename V>
class q_keylist : public q_list<q_entry<K,V>>
{
private:

public:
    using base = q_list<q_entry<K, V>>;

    q_keylist() = default;
    q_keylist(q_keylist&& other) noexcept : base(std::move(other)) {}
    q_keylist& operator=(q_keylist&& other) noexcept {
        if (this != &other) {
            base::operator=(std::move(other));
        }
        return *this;
    }
    
    // Delete copy constructor and assignment
    q_keylist(const q_keylist&) = delete;
    q_keylist& operator=(const q_keylist&) = delete;


    //Add more control here in the future, with conventions for const and such and r/l
    template<typename KK, typename VV>
    void put(KK&& key, VV&& value) {
        this->push(q_entry<K,V>(std::forward<KK>(key), std::forward<VV>(value)));
    }

    template<typename EE>
    void put(EE&& e) {
        this->push(std::forward<EE>(e));
    }

    V& get(const K& key){
       for(size_t i = 0; i < this->length(); ++i) {
           auto& e = this->base::get(i, "q_keylist::get::54");
           if(e.key == key) return e.value;
       }
       throw std::runtime_error("q_keylist::get::57 Warning: no key found");
    }

    list<V> getAll(const K& key){
        list<V> l;
        for(size_t i = 0; i < this->length(); ++i) {
            auto& e = this->base::get(i, "q_keylist::getAll::63");
            if(e.key == key) l << e.value;
        }
        return l;
    }

    list<V> allValues() {
        list<V> l;
        for(size_t i = 0; i < this->length(); ++i) {
            auto& e = this->base::get(i, "q_keylist::allValues::72");
            l << e.value;
        }
        return l;
    }
    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback){
       for(size_t i = 0; i < this->length(); ++i) {
           auto& e = this->base::get(i, "q_keylist::getOrDefault::80");
           if(e.key == key) return e.value;
       }
       return fallback;
    }

    bool hasKey(const K& key){
       for(size_t i = 0; i < this->length(); ++i) {
           auto& e = this->base::get(i, "q_keylist::hasKey::88");
           if(e.key == key) return true;
       }
       return false;
    }

    list<K> keySet() {  
        list<K> result;
        for(size_t i = 0; i < this->length(); ++i) {
            auto& e = this->base::get(i, "q_keylist::keySet::97");
            result << e.key;
        }
        return result;
    }

    template<typename KK, typename VV>
    bool set(KK& key, VV&& value){
       for(size_t i = 0; i < this->length(); ++i) {
           auto& e = this->base::get(i, "q_keylist::set::106");
           if(e.key == key) {
               e = q_entry<K,V>(std::forward<KK>(key), std::forward<VV>(value));
               return true;
           }
       }
       return false;
    }

    bool has(const K& key){
       for(size_t i = 0; i < this->length(); ++i) {
           auto& e = this->base::get(i, "q_keylist::has::117");
           if(e.key == key) return true;
       }
       return false;
    }

    bool remove(const K& key) {
        for (size_t i = 0; i < this->length(); ++i) {
            auto& entry = this->base::get(i, "q_keylist::remove::125");
            if (entry.key == key) {
                this->base::remove(i);
                return true;
            }
        }
        return false;
    }
};



template<typename K,typename V>
class q_map : public q_object
{
protected:
   q_list<g_ptr<q_keylist<K,V>>> buckets;
   size_t impl_size;
   size_t impl_capacity;

   std::atomic<bool> is_resizing{false};

    size_t marked_index;
    size_t switch_index;

    size_t switch_size;
    size_t switch_capacity;

public:
    q_map()
    {
        impl_size = 0;
        impl_capacity = 8;
        for(int i=0;i<impl_capacity;i++)
        {
           buckets.push(make<q_keylist<K,V>>());
        }
    }

    uint32_t hashString(const std::string& str) {
        uint32_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash;
    }

    template<typename T>
    uint32_t hashT(const T& val)
    {
        if constexpr (std::is_same_v<T,std::string>) {
            return hashString(val);
        }
        else if constexpr (std::is_same_v<T,const char*>) {
            return hashString(std::string(val));
        }
        else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>) {
            return hashString(std::string(val));
        }
        else if constexpr (std::is_same_v<T,int>) {
            return val;
        }
        else {
            return val;
        }
    };

    size_t size() const {
    if(is_resizing.load())
        return switch_size;
    else 
        return impl_size;
    }

    size_t capacity() const {
    if(is_resizing.load()) {
        return switch_capacity;
    }
    else {
        return impl_capacity;
    }
    }

    void resize() {
        bool expected = false;
        if (!is_resizing.compare_exchange_strong(expected, true)) {
            return;
        }

        switch_size = impl_size;
        switch_capacity = impl_capacity;

        size_t new_capacity = impl_capacity * 2;
        q_list<g_ptr<q_keylist<K,V>>> new_buckets;
        
        for(size_t i = 0; i < new_capacity; i++) {   
            new_buckets.push(make<q_keylist<K,V>>());
        }

        for(size_t bucket_idx = 0; bucket_idx < switch_capacity; bucket_idx++) {
            g_ptr<q_keylist<K,V>> old_bucket = buckets.get(bucket_idx,"q_map::resize,205");
            
            for(size_t entry_idx = 0; entry_idx < old_bucket->length(); entry_idx++) {
                try {
                    auto& entry = old_bucket->q_list::get(entry_idx, "q_map::resize::209");
                    size_t new_bucket_idx = hashT(entry.key) % new_capacity;
                    new_buckets.get(new_bucket_idx)->put(entry.key, entry.value);
                } catch(const std::out_of_range& e) {
                    continue;
                }
            }
        }

        buckets = std::move(new_buckets);
        impl_capacity = new_capacity;

        is_resizing.store(false);
    }

    void put(const K& key, V value)
    {
        while (is_resizing.load()) {
            std::this_thread::yield();
        }

        if(size()>=(capacity()*2))
        {
            resize();
        }

        while (is_resizing.load()) {
            std::this_thread::yield();
        }

        buckets.get(hashT(key)%capacity(),"q_map::put::227")->put(key,value);        
        impl_size++;
    }
    

    bool set(const K& key,V value) {
        auto b = buckets.get(hashT(key)%capacity(),"q_map::set::186");
        return b->set(key,value);
    }

    // keylist<K, V> b = buckets[hashT(key)%capacity];
    // if(b.hasKey(key)) return b.get(key);
    // else {throw}
    V& get(const K& key)
    {
       return buckets.get(hashT(key)%capacity(),"q_map::get::274")->get(key);
    }

    V& get(const K& key) const
    {
       return buckets.get(hashT(key)%capacity(),"q_map::get::279")->get(key);
    }

    //Returns all values in the map
    list<V> getAll(){
        list<V> l;
        for(auto& b : buckets) {
            for(auto v : b->allValues()) {
                l << v; }
        }
        return l;
     }

     //Reuturns all values associated with a key
     list<V> getAll(const K& key){
       return buckets.get(hashT(key)%capacity(),"q_map::getAll::268")->getAll(key);
     }

    V& operator[](const K& key) {
        return get(key);
    }

    template<typename VV>
    V& getOrDefault(const K& key,VV&& fallback)
    {
       return buckets.get(hashT(key)%capacity(),"q_map::getOrDefault::278")->getOrDefault(key,fallback);
    }

    bool hasKey(const K& key)
    {
       return buckets.get(hashT(key)%capacity(),"q_map::hasKey::283")->hasKey(key);
    }

    list<K> keySet() {  
        list<K> result;
        for(auto& bucket : buckets) { 
            auto keys = bucket.keySet();
            for(auto& k : keys) {
                result << k;
            }
        }
        return result;
    }

    list<q_entry<K,V>> entrySet() {
        list<q_entry<K,V>> result;
        for(g_ptr<q_keylist<K,V>> e : buckets){
            for(int i=0;i<e->length();i++){
                result << e[i];
            }
        } 
        return result;
    }


    // list<entry<K,V>> entrySet() {
    //     list<entry<K,V>> result;
    //     for(int bucket_idx = 0; bucket_idx < buckets.length(); bucket_idx++) {
    //         keylist<K,V>& bucket = buckets[bucket_idx];
    //         for(int i = 0; i < bucket.length(); i++){
    //             result << bucket[i];
    //         }
    //     } 
    //     return result;
    // }

    void clear()
    {
        // Clear all buckets
        for(size_t i = 0; i < buckets.length(); i++) {
            buckets.get(i, "q_map::clear::355")->base::clear();
        }
        buckets.clear();
        
        impl_size = 0;
        impl_capacity = 8;
        for(size_t i = 0; i < impl_capacity; i++)
        {
            buckets.push(make<q_keylist<K,V>>());
        }
    }

    bool remove(const K& key) {
        bool removed = buckets.get(hashT(key) % capacity(), "q_map::remove::331")->remove(key);
        if(removed) {
            impl_size--; 
        }
        return removed;
    }

      void debugMap() {
        for(size_t i = 0; i < buckets.length(); i++) {
            g_ptr<q_keylist<K,V>> bucket = buckets.get(i, "q_map::debug::365");
            for(size_t j = 0; j < bucket->length(); j++) {
                auto& entry = bucket->q_list::get(j, "q_map::debug_entry::367");
                std::cout << entry.key << std::endl;
            }
        }
    }
};