#pragma once
#include<linking/one.hpp>


class two : public Object {
public:
int b = 0;
void say_b() {
    std::cout << b << std::endl;
}
g_ptr<one> o = make<one>();
void say_o() {
    std::cout << o->a << std::endl;
}
};