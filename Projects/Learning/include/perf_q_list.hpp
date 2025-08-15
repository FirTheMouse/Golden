#include<util/list.hpp>

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