#pragma once

#include <core/object.hpp>
#include <util/util.hpp>


namespace Golden {

struct value_ {
    value_(size_t _size, std::string _name) 
    : size(_size), name(_name) {}
    value_() {}
    size_t size;
    std::string name;
};

struct _note {
    _note() {}
    _note(int _index, size_t _size) : index(_index), size(_size) {}
    _note(int _index, size_t _size, int _sub_index) : index(_index), size(_size), sub_index(_sub_index) {}
    int index = 0;
    int sub_index = 0;
    size_t size = 0;
};

static _note note_fallback;

struct byte16_t { uint8_t data[16]; };
struct byte24_t { uint8_t data[24]; };
struct byte32_t { uint8_t data[32]; };
struct byte64_t { uint8_t data[64]; };

struct _fallback_list {
    _fallback_list() {}
    _fallback_list(size_t _size) : size(_size) {}
    size_t size = 32;
    list<uint64_t> rows;
};

class Type : public q_object {
private:
  
public:
    Type() {
        free_stack_top.store(0);
    }

    ~Type() {}

    list<g_ptr<Object>> objects;

    //Consider making notes be uint32_t,_note, and have all the puts pre-hash, this will allow us to do enum notes and avoid the string stuff
    map<std::string,_note> notes; // Where reflection info is stored
    list<_note> array; //Ordered array for usage of Type as a MultiArray

    size_t sizes[8] = {1,2,4,8,16,24,32,64};
    list<list<uint8_t>> byte1_columns;   // bool, char (1 byte)
    list<list<uint16_t>> byte2_columns;  // uint16_t (2 bytes)
    list<list<uint32_t>> byte4_columns;  // int, float (4 bytes)  
    list<list<uint64_t>> byte8_columns;  // double, pointers (8 bytes)
    list<list<byte16_t>> byte16_columns; // vec4, padded vec3 (16 bytes)
    list<list<byte24_t>> byte24_columns; // strings, list, medium objects (24 bytes)
    list<list<byte32_t>> byte32_columns; // strings on some systems (32 bytes)
    list<list<byte64_t>> byte64_columns; // mat4, large objects (64 bytes)

    //In the future, add a fallback path for larger data sizes, similar to Bevy's system by not using list but instead manual
    //managment and a size tab in the column itself
    size_t next_size(size_t size) {
        size_t toReturn = 0;
        for(int i=0;i<8;i++) {
            if(sizes[i]>=size) {
                toReturn = sizes[i];
                break;
            }
        }
        return toReturn;
    }

    //Consider splitting the stratgeies into their own types of Type via inhereitence, I'm doing it manual right now because I prefer
    //composition, but it may be better to have all this in constructers and private methods.

    //Adds a new place to all the rows in a column
    void add_rows(size_t size = 0) {
        switch(size) {
        case 0:
            for(int i=0;i<8;i++) add_rows(sizes[i]);
        break;
        case 1:
            for(auto& list : byte1_columns) {
                list.push(uint8_t{}); 
            }
        break;
        case 2:
        for(auto& list : byte2_columns) {
            list.push(uint16_t{}); 
        }
        break;
        case 4:
            for(auto& list : byte4_columns) {
                list.push(uint32_t{});   
            }
        break;
        case 8:
            for(auto& list : byte8_columns) {
                list.push(uint64_t{}); 
            }
        break;
        case 16:
            for(auto& list : byte16_columns) {
                list.push(byte16_t{});  
            }
        break;
        case 24:
            for(auto& list : byte24_columns) {
                list.push(byte24_t{});  
            }
        break;
        case 32:
        for(auto& list : byte32_columns) {
            list.push(byte32_t{});  
        }
        break;
        case 64:
            for(auto& list : byte64_columns) {
                list.push(byte64_t{});  
            }
        break;
        default: print("add_rows::100 invalid size ",size); break;
        }
    }

    // Adds a new place to the indicated row in a column
    void add_row(int index, size_t size = 0) {
        switch(size) {
        case 0:
            for(int i=0;i<8;i++) add_row(index,sizes[i]);
        break;
        case 1:
            byte1_columns[index].push(uint8_t{}); 
        break;
        case 2:
            byte2_columns[index].push(uint16_t{}); 
        break;
        case 4:
            byte4_columns[index].push(uint32_t{});   
        break;
        case 8:
            byte8_columns[index].push(uint64_t{}); 
        break;
        case 16:
            byte16_columns[index].push(byte16_t{});  
        break;
        case 24:
            byte24_columns[index].push(byte24_t{});  
        break;
        case 32:
            byte32_columns[index].push(byte32_t{});  
        break;
        case 64:
            byte64_columns[index].push(byte64_t{});  
        break;
        default: print("add_row::130 invalid size ",size); break;
        }
    }

    // Adds a new row to a column
    void add_column(size_t size = 0) {
        switch(size) {
        case 0:
            for(int i=0;i<8;i++) add_column(sizes[i]);
        break;
        case 1:
            byte1_columns.push(list<uint8_t>());
        break;
        case 2:
            byte2_columns.push(list<uint16_t>());
        break;
        case 4:
            byte4_columns.push(list<uint32_t>());
        break;
        case 8:
            byte8_columns.push(list<uint64_t>());
        break;
        case 16:
            byte16_columns.push(list<byte16_t>());
        break;
        case 24:
            byte24_columns.push(list<byte24_t>());
        break;
        case 32:
            byte32_columns.push(list<byte32_t>());
        break;
        case 64:
            byte64_columns.push(list<byte64_t>());
        break;
        default: print("add_column::130 invalid size ",size); byte32_columns.get(64,"breaking"); break;
        }
    }

    // Returns the ammount of rows in a column
    size_t column_length(size_t size) {
        switch(size) {
            case 1: return byte1_columns.length();
            case 2: return byte2_columns.length();
            case 4: return byte4_columns.length();
            case 8: return byte8_columns.length();
            case 16: return byte16_columns.length();
            case 24: return byte24_columns.length();
            case 32: return byte32_columns.length();
            case 64: return byte64_columns.length();
            default: print("column_length::130 invalid size ",size); return 0;
        }
    }

    //Returns the amount of places in a row
    size_t row_length(int index,size_t size) {
        switch(size) {
            case 1: return byte1_columns[index].length();
            case 2: return byte2_columns[index].length();
            case 4: return byte4_columns[index].length();
            case 8: return byte8_columns[index].length();
            case 16: return byte16_columns[index].length();
            case 24: return byte24_columns[index].length();
            case 32: return byte32_columns[index].length();
            case 64: return byte64_columns[index].length();
            default: print("row_length::180 invalid size ",size); return 0;
        }
    }

    inline bool has(int index) {return array.length()<index;}
    inline bool has(const std::string& label) {return notes.hasKey(label);}
    bool validate(list<std::string> check) {
        for(auto c : check) {
            if(!notes.hasKey(c)) {
                print("validate::190 type missing ",c);
                return false;
            }
        }
        return true;
    }
    //Maybe add some more explcitness to validate, like listing *what* is actually missing

    _note& get_note(const std::string& label) {
        return notes.getOrDefault(label,note_fallback);
    }
    _note& get_note(int index) {
        return array[index];
    }

    /// @brief For use in the MAP strategy
    void note_value(const std::string& name, size_t size) {
        _note note(column_length(size),size); add_column(size); notes.put(name,note);
    }

    /// @brief For use in the MAP strategy
    void* adress_column(const std::string& name) {
        _note note = notes.getOrDefault(name,note_fallback);
        switch(note.size) {
            case 0: return nullptr; //print("adress_column::175 Note not found for ",name); 
            case 1: return &byte1_columns[note.index];
            case 2: return &byte2_columns[note.index];
            case 4: return &byte4_columns[note.index];
            case 8: return &byte8_columns[note.index];
            case 16: return &byte16_columns[note.index];
            case 24: return &byte24_columns[note.index];
            case 32: return &byte32_columns[note.index];
            case 64: return &byte64_columns[note.index];
            default: print("adress_column::180 Invalid note size ",note.size); return nullptr;
        }
    }

    static void set(void* ptr,void* value,size_t index,size_t size) {
        // print("S: ",size," I: ",index);
        // print("L: ",(*(list<uint32_t>*)ptr).length());
        switch(size) {
            case 1: memcpy(&(*(list<uint8_t>*)ptr)[index], value, 1); break;
            case 2: memcpy(&(*(list<uint16_t>*)ptr)[index], value, 2); break;
            case 4: memcpy(&(*(list<uint32_t>*)ptr)[index], value, 4); break;
            case 8: memcpy(&(*(list<uint64_t>*)ptr)[index], value, 8); break;
            case 16: memcpy(&(*(list<byte16_t>*)ptr)[index].data, value, 16); break;
            case 24: memcpy(&(*(list<byte24_t>*)ptr)[index].data, value, 24); break;
            case 32: memcpy(&(*(list<byte32_t>*)ptr)[index].data, value, 32); break;
            case 64: memcpy(&(*(list<byte64_t>*)ptr)[index].data, value, 64); break;
            default: print("type::set::280 Bad size for type value: ",size); break;
        }
    }

    void set(const std::string& label,void* value,size_t index,size_t size) {
        void* ptr = adress_column(label);
        if (!ptr) return;
        return set(ptr,value,index,size);
    }

    template<typename T>
    void set(const std::string& label,T value,size_t index) {
        size_t size = sizeof(T);
        set(label,&value,index,size);
    }


    inline static void* get(void* ptr, size_t index, size_t size) {        
        switch(size) {
            case 1: return &(*(list<uint8_t>*)ptr)[index];
            case 2: return &(*(list<uint16_t>*)ptr)[index];
            case 4: return &(*(list<uint32_t>*)ptr)[index]; 
            case 8: return &(*(list<uint64_t>*)ptr)[index];
            case 16: return &(*(list<byte16_t>*)ptr)[index].data;
            case 24: return &(*(list<byte24_t>*)ptr)[index].data;
            case 32: return &(*(list<byte32_t>*)ptr)[index].data;
            case 64: return &(*(list<byte64_t>*)ptr)[index].data;
            default: 
                print("type::get::312 Bad size for type value: ",size);
            return nullptr;
        }
    }

    void* get(const std::string& label, size_t index, size_t size) {
        void* ptr = adress_column(label);
        if (!ptr) return nullptr;
        return get(ptr,index,size);
    }
    
    template<typename T>
    T& get(const std::string& label, size_t index) {
        size_t size = sizeof(T);
        void* data_ptr = get(label, index, size);
        if (!data_ptr) print("get::270 value not found");
        
        return *(T*)data_ptr;
    }

    template<typename T>
    T& get(void* ptr, size_t index) {
        size_t size = sizeof(T);
        void* data_ptr = get(ptr, index, size);
        if (!data_ptr) print("get::285 value not found");
        return *(T*)data_ptr;
    }

    //We're keeping these methods verbose for performance, drawing out every possible case means less indirection from method calls and switches

    /// @brief for use in the DATA strategy
    void push(const std::string& name,void* value,size_t size,int t = 0) {
        switch(size) {
            case 1: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,1,byte1_columns[t].length()); byte1_columns[t].push(*(uint8_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 2: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,2,byte2_columns[t].length()); byte2_columns[t].push(*(uint16_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 4: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,4,byte4_columns[t].length()); byte4_columns[t].push(*(uint32_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 8: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,8,byte8_columns[t].length()); byte8_columns[t].push(*(uint64_t*)value);  notes.put(name,note); array<<note;
            }
            break;
            case 16: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,16,byte16_columns[t].length()); byte16_columns[t].push(*(byte16_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 24: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,24,byte24_columns[t].length()); byte24_columns[t].push(*(byte24_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 32: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,32,byte32_columns[t].length()); byte32_columns[t].push(*(byte32_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 64: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,64,byte64_columns[t].length()); byte64_columns[t].push(*(byte64_t*)value);  notes.put(name,note); array<<note;
            }
            break;
            default: 
            size = next_size(size);
            if(size==0) {
                //ptr fallback
            } else push(name,value,size,t);
            break;
        }
    }

    void add(const std::string& name, void* value,size_t size,int t) {
        push(name,value,size,t);
    }

    template<typename T>
    void add(const std::string& name,T value,int t = 0) {
        add(name,&value,sizeof(T),t);
    }

    inline void* get(int index,int sub_index,size_t size) {
        switch(size) {
            case 0: return nullptr; 
            case 1: return &byte1_columns[index][sub_index];
            case 2: return &byte2_columns[index][sub_index];
            case 4: return &byte4_columns[index][sub_index];
            case 8: return &byte8_columns[index][sub_index];
            case 16: return &byte16_columns[index][sub_index];
            case 24: return &byte24_columns[index][sub_index];
            case 32: return &byte32_columns[index][sub_index];
            case 64: return &byte64_columns[index][sub_index];
            default: print("get::330 Invalid size ",size); return nullptr;
        }
    }

    void* get_from_note(const _note& note) {
        return get(note.index,note.sub_index,note.size);
    }

    void* data_get(const std::string& name) {
        return get_from_note(notes.getOrDefault(name,note_fallback));
    }

    void* array_get(int index) {
        return get_from_note(array[index]);
    }

   template<typename T>
   T& get(const std::string& label) {
    return *(T*)data_get(label);
   }

   template<typename T>
   T& get(int index) {
    return *(T*)array_get(index);
   }

   void set(int index,int sub_index,size_t size,void* value) {
    switch(size) {
        case 1: memcpy(&(byte1_columns[index])[sub_index], value, 1); break;
        case 2: memcpy(&(byte2_columns[index])[sub_index], value, 2); break;
        case 4: memcpy(&(byte4_columns[index])[sub_index], value, 4); break;
        case 8: memcpy(&(byte8_columns[index])[sub_index], value, 8); break;
        case 16: memcpy(&(byte16_columns[index])[sub_index], value, 16); break;
        case 24: memcpy(&(byte24_columns[index])[sub_index], value, 24); break;
        case 32: memcpy(&(byte32_columns[index])[sub_index], value, 32); break;
        case 64: memcpy(&(byte64_columns[index])[sub_index], value, 64); break;
        default: print("set::360 Invalid size ",size); break;
    }
    }

   void set_from_note(const _note& note,void* value) {
        set(note.index,note.sub_index,note.size,value);
    }

    void data_set(const std::string& name,void* value) {
        set_from_note(notes.getOrDefault(name,note_fallback),value);
    }

    void array_set(int index,void* value) {
        set_from_note(array[index],value);
    }

    template<typename T>
    void set(const std::string& label,T value) {
     data_set(label,&value);
    }

    //Uses array set
    template<typename T>
    void set(int index,T value) {
     array_set(index,&value);
    }

    //Directly sets the value of a place
    template<typename T>
    void set(int index,int sub_index,T value) {
     set(index,sub_index,sizeof(T),&value);
    }
    


    /// @brief For use in the ARRAY strategy
    void push(void* value, size_t size,int t) {
        switch(size) {
            case 1: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,1,byte1_columns[t].length()); byte1_columns[t].push(*(uint8_t*)value); array<<note;
            }
            break;
            case 2: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,2,byte2_columns[t].length()); byte2_columns[t].push(*(uint16_t*)value); array<<note;
            }
            break;
            case 4: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,4,byte4_columns[t].length()); byte4_columns[t].push(*(uint32_t*)value); array<<note;
            }
            break;
            case 8: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,8,byte8_columns[t].length()); byte8_columns[t].push(*(uint64_t*)value); array<<note;
            }
            break;
            case 16: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,16,byte16_columns[t].length()); byte16_columns[t].push(*(byte16_t*)value); array<<note;
            }
            break;
            case 24: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,24,byte24_columns[t].length()); byte24_columns[t].push(*(byte24_t*)value); array<<note;
            }
            break;
            case 32: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,32,byte32_columns[t].length()); byte32_columns[t].push(*(byte32_t*)value); array<<note;
            }
            break;
            case 64: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,64,byte64_columns[t].length()); byte64_columns[t].push(*(byte64_t*)value); array<<note;
            }
            break;
            default: 
            size = next_size(size);
            if(size==0) {
                print("push::540 Pointer fallback triggered");
            } else push(value,size,t);
            break;
        }
    }

    template<typename T>
    void push(T value,int t = 0) {
        push(&value,sizeof(T),t);
    }    

    std::string type_name = "bullets";
    list< std::function<void(g_ptr<Object>)> > init_funcs;
    list<int> free_ids;
    std::atomic<size_t> free_stack_top{0};  // Points to next free slot

    std::function<g_ptr<Object>()> make_func = [](){
        auto object = make<Object>();
        return object;
    };

    int get_next() {
        size_t current_top, new_top;
        do {
            current_top = free_stack_top.load();
            if (current_top == 0) return -1;
            new_top = current_top - 1;
        } while (!free_stack_top.compare_exchange_weak(current_top, new_top));
        
        return free_ids.get(new_top);
    }
    
    void return_id(int id) {
        size_t current_top = free_stack_top.load();
        if (current_top < free_ids.size()) {
            // There's space, just write and increment pointer
            size_t slot = free_stack_top.fetch_add(1);
            free_ids.get(slot) = id;
        } else {
            // Need to grow the list
            free_ids.push(id);
            free_stack_top.fetch_add(1);
        }
    }
    

    g_ptr<Object> create() {
        int next_id = get_next();
        if(next_id!=-1)
        {
            auto obj = objects.get(next_id);
            obj->recycled.store(false);
            //Might want to clean object data here
            return obj;
        }
        else
        {
            g_ptr<Object> object = make_func();
            object->type_ = this; //May want to move this into the makeFunc to give more user control
            for(int i=0;i<init_funcs.size();i++) {
                init_funcs[i](object);
            }
            store(object);
            return object;
        }
    }

    private:
    void store(g_ptr<Object> object)
    {
        object->ID = objects.size();
        add_rows();
        objects.push(object);
    }
    public:

    void add_initializers(list<std::function<void(g_ptr<Object>)>> inits) {
        for(auto i : inits) add_initializer(i);
    }

    void add_initializer(std::function<void(g_ptr<Object>)> init) {
        init_funcs << init;
    }

    void operator+(std::function<void(g_ptr<Object>)> init) {
        add_initializer(init);
    }

    void operator+(list<std::function<void(g_ptr<Object>)>> inits) {
        for(auto i : inits) add_initializer(i);
    }



    void recycle(g_ptr<Object> object) {
        if(object->recycled.load()) {
            return;
        }
        object->recycled.store(true);

        return_id(object->ID);
        deactivate(object);
    }

    void deactivate(g_ptr<Object> object) {
        object->stop();
    }

    void reactivate(g_ptr<Object> object) {
        object->resurrect();
    }

};

}
