#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>
#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <gui/quad.hpp>
#include <fsh/f_object.hpp>
#include <util/util.hpp>


namespace Golden
{
struct gui_event
{
    float start;
    float end;
    std::function<bool()> event;

    gui_event(float _start, float _end, std::function<bool()> _event) :
    start(_start), end(_end), event(_event) {}
};


class fsh_scene : virtual public Scene
{
public:
    using Scene::Scene;

    
    std::vector<gui_event> events;
    void queueEvent(float end,std::function<bool()> event)
    {
        events.emplace_back(sceneTime,end,event);
    }

    void updateScene(float tpf) override
    {
        Scene::updateScene(tpf);
        for (std::vector<gui_event>::iterator it = events.begin() ; it != events.end(); ) {
        if ((sceneTime-it->start)>=it->end&&it->event()) {
            it = events.erase(it);
        } else {
            ++it;
        }
        }

    }
};
}