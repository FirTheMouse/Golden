#pragma once

#include <core/object.hpp>
#include <util/group.hpp>
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
    int index = 0;
    size_t size = 0;
};

static _note note_fallback;

struct byte16_t { uint8_t data[16]; };
struct byte32_t { uint8_t data[32]; };
struct byte64_t { uint8_t data[64]; };

class Type : public q_object {
public:
    Type() {
        free_stack_top.store(0);
    }

    ~Type() {}

    list<g_ptr<Object>> objects;

    map<std::string,_note> notes; // Where reflection info is stored


    list<list<uint8_t>> byte1_columns;     // bool, char (1 byte)
    list<list<uint32_t>> byte4_columns;    // int, float (4 bytes)  
    list<list<uint64_t>> byte8_columns;    // double, pointers (8 bytes)
    list<list<byte16_t>> byte16_columns; // vec4, padded vec3 (16 bytes)
    list<list<byte32_t>> byte32_columns; // strings, medium objects (32 bytes)
    list<list<byte64_t>> byte64_columns; // mat4, large objects (64 bytes)

    //In the future, add a fallback path for larger data sizes, similar to Bevy's system by not using list but instead manual
    //managment and a size tab in the column itself


    void note_value(size_t size, const std::string& name) {
        value_ value(size,name);
        note_value(value);
    }

    void note_value(value_& value) {
        switch(value.size) {
            case 1: 
            {_note note(byte1_columns.length(),1); byte1_columns.push(list<uint8_t>()); notes.put(value.name,note);}
            break;
            case 4: 
            {_note note(byte4_columns.length(),4); byte4_columns.push(list<uint32_t>()); notes.put(value.name,note);}
            break;
            case 8: 
            {_note note(byte8_columns.length(),8); byte8_columns.push(list<uint64_t>()); notes.put(value.name,note);}
            break;
            case 16: 
            {_note note(byte16_columns.length(),16); byte16_columns.push(list<byte16_t>()); notes.put(value.name,note);}
            break;
            case 32: 
            {_note note(byte32_columns.length(),32); byte32_columns.push(list<byte32_t>()); notes.put(value.name,note);}
            break;
            case 64: 
            {_note note(byte64_columns.length(),64); byte64_columns.push(list<byte64_t>()); notes.put(value.name,note);}
            break;
            default: print("note_value::170 Invalid value size, name: ",value.name," size: ",value.size); break;
        }
    }

    void* adress_column(const std::string& name) {
        _note note = notes.getOrDefault(name,note_fallback);
        switch(note.size) {
            case 0: return nullptr; //print("adress_column::175 Note not found for ",name); 
            case 1: return &byte1_columns[note.index];
            case 4: return &byte4_columns[note.index];
            case 8: return &byte8_columns[note.index];
            case 16: return &byte16_columns[note.index];
            case 32: return &byte32_columns[note.index];
            case 64: return &byte64_columns[note.index];
            default: print("adress_column::180 Invalid note size ",note.size); return nullptr;
        }
    }

    void set(void* ptr,void* value,size_t index,size_t size) {
        switch(size) {
            case 1: memcpy(&(*(list<uint8_t>*)ptr)[index], value, 1); break;
            case 4: memcpy(&(*(list<uint32_t>*)ptr)[index], value, 4); break;
            case 8: memcpy(&(*(list<uint64_t>*)ptr)[index], value, 8); break;
            case 16: memcpy(&(*(list<byte16_t>*)ptr)[index].data, value, 16); break;
            case 32: memcpy(&(*(list<byte32_t>*)ptr)[index].data, value, 32); break;
            case 64: memcpy(&(*(list<byte64_t>*)ptr)[index].data, value, 64); break;
            default: print("type::set::200 Bad size for type value: ",size); break;
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


    void* get(void* ptr, size_t index, size_t size) {        
        switch(size) {
            case 1: return &(*(list<uint8_t>*)ptr)[index];
            case 4: return &(*(list<uint32_t>*)ptr)[index]; 
            case 8: return &(*(list<uint64_t>*)ptr)[index];
            case 16: return &(*(list<byte16_t>*)ptr)[index].data;
            case 32: return &(*(list<byte32_t>*)ptr)[index].data;
            case 64: return &(*(list<byte64_t>*)ptr)[index].data;
            default: 
                print("type::set::200 Bad size for type value: ",size);
                return nullptr;
        }
    }

    void* get(const std::string& label, size_t index, size_t size) {
        void* ptr = adress_column(label);
        if (!ptr) return nullptr;
        return get(ptr,index,size);
    }
    
    template<typename T>
    T get(const std::string& label, size_t index) {
        size_t size = sizeof(T);
        void* data_ptr = get(label, index, size);
        if (!data_ptr) return T{}; 
        
        return *(T*)data_ptr;
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
        for(auto& list : byte1_columns) {
            list.push(uint8_t{}); 
        }
        for(auto& list : byte4_columns) {
            list.push(uint32_t{});   
        }
        for(auto& list : byte8_columns) {
            list.push(uint64_t{}); 
        }
        for(auto& list : byte16_columns) {
            list.push(byte16_t{});  
        }
        for(auto& list : byte32_columns) {
            list.push(byte32_t{});  
        }
        for(auto& list : byte64_columns) {
            list.push(byte64_t{});  
        }
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
