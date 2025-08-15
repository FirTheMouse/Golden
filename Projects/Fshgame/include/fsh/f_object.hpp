#pragma once

#include <rendering/single.hpp>
#include <core/scriptable.hpp>
#include <gui/quad.hpp>

namespace Golden
{

class F_Object : virtual public Scriptable, virtual public Quad  {
public:
    F_Object () {}
    ~F_Object() = default;

};

}