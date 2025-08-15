#pragma once

#include <core/scriptable.hpp>
#include <rendering/single.hpp>

namespace Golden {

class T_Object : public virtual Scriptable, public virtual Single
{
public:
    using Single::Single;
    T_Object()
    {

    }

};

};