#pragma once

#include <rendering/single.hpp>
#include <core/scriptable.hpp>
#include <guide/gui_cell_object.hpp>

namespace Golden
{

class G_Object : virtual public Single, virtual public Scriptable, virtual public C_Object  {
public:
    using Single::Single;
    G_Object () {}
    ~G_Object() = default;

    bool selectable = true;
    float deadTime = 0.3f;
    bool valid = true;
    float size = 1;
    float sizeX = 0;
    float sizeY = 0;

    void remove() override {
        Single::remove();
    }

    virtual bool isActive() {return false;}

};

class Button : virtual public G_Object
{
public:
    Button(const std::string& _name,float _size);
    std::string name;
    float weight = 0.3f;

    bool isActive() override {return true;}

};

}