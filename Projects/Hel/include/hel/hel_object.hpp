#pragma once

#include<rendering/single.hpp>
using str = std::string;

namespace Golden
{
class H_Object : public Single
{
public:
    H_Object(g_ptr<Model> _model) {
        setModel(_model);
        set<str>("dtype","none");
    }

    void remove() override{
        Single::remove();
    }


};

}