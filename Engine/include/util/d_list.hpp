#pragma once
#include <core/object.hpp>
#include <util/list.hpp>

template <typename T>
class d_list : public Golden::Object, public list<T> {
public:
    using list<T>::list;
    using base = list<T>;
};