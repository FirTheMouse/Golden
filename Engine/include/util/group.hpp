#pragma once
#include<core/object.hpp>

namespace Golden {

/// @brief Container for Objects only, accepts and returns g_ptrs, should be a g_ptr itself.
class group : public Object {
protected:
  list<g_ptr<Object>> items;
public:
    group() {
        items = list<g_ptr<Object>>();
    }

    group(size_t size) {
        items = list<g_ptr<Object>>();
        items.reserve(size);
    }

    group(g_ptr<group> other) {
        items = other->items;
    }

    ~group() {
        items.destroy();
    }

    void destroy() {items.destroy();}
    size_t length() const {return items.length();}
    size_t size() const {return items.size();}
    size_t space() const { return items.space();}
    size_t capacity() const { return items.capacity();}

    void put(g_ptr<Object> object) {
        items.push(object);
    }

    void operator<<(g_ptr<Object> object) {
        put(object);
    }

    g_ptr<Object> retrive(g_ptr<Object> object) {
        int idx = items.find(object);
        if(idx!=-1) {
            return items[idx];
        }
        else {
            throw std::out_of_range("Object not found in group!");
        }
    }

    g_ptr<Object> operator[](size_t index) {
        return items[index];
    }
};

}