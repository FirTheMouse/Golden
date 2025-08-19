#pragma once
#include<iostream>
#include<core/object.hpp>

class two;

class one : public Object {
public:
one();
int a = 0;
void say_a() {
    std::cout << a << std::endl;
}
list<g_ptr<two>> twos;
void take_two();
};