#pragma once

#include<util/basic.hpp>


class bitlist {
private:
    void* ptr;

    size_t size_ = 0;
    size_t capacity_ = 0;
public:
    bitlist() {

    }

    void resize(const uint32_t& by) {
        if(ptr) free(ptr);
        ptr = malloc(by);
        //Potentially need a size refresh if we contracted past initial size
        capacity_ = by; 
    }




};