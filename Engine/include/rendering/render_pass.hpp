#pragma once

#include <rendering/renderer.hpp>


namespace Golden
{
class Scene;


class Render_Pass
{
public:
    Render_Pass() {}

    virtual void execute(Scene& scene) = 0;
};


}