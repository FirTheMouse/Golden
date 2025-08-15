#pragma once

#include <guide/gui_object.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>
#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <rendering/model.hpp>
#include <rendering/single.hpp>
#include <guide/gui_quad.hpp>
#include <guide/gui_grid.hpp>
#include <guide/gui_cell_object.hpp>
#include <util/util.hpp>
#include<gui/text.hpp>


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


class gui_scene : virtual public Scene
{
public:
    using Scene::Scene;

    std::vector<g_ptr<S_Object>> selectable; 
    //list<g_ptr<Quad>> toSave; 

    void add(const g_ptr<S_Object>& sobj) override {
       // print("adding quad ",guiEndTransforms.length());
        Scene::add(sobj);
        if(auto obj = g_dynamic_pointer_cast<G_Object>(sobj))
        {
            selectable.push_back(sobj);
        }
        else if (auto qobj = g_dynamic_pointer_cast<Quad>(sobj))
        {
            selectable.push_back(sobj);
        }
    }

    bool rayIntersectsAABB(const glm::vec3& origin, const glm::vec3& dir, const BoundingBox& box, float& outT) {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i) {
            if (fabs(dir[i]) < 1e-6f) {
                if (origin[i] < box.min[i] || origin[i] > box.max[i]) return false;
            } else {
                float invD = 1.0f / dir[i];
                float t0 = (box.min[i] - origin[i]) * invD;
                float t1 = (box.max[i] - origin[i]) * invD;
                if (t0 > t1) std::swap(t0, t1);
                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);
                if (tMax < tMin) return false;
            }
        }

    outT = tMin;
    return true;
    }

    g_ptr<S_Object>  nearestElement() {
        int windowWidth = window.width;
        int windowHeight = window.height;
        double xpos, ypos;
        glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);

        // Flip y for OpenGL
        float glY = windowHeight - float(ypos);

        // Get view/proj
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();
        glm::ivec4 viewport(0, 0, windowWidth, windowHeight);

        // Unproject near/far points
        glm::vec3 winNear(float(xpos), glY, 0.0f);   // depth = 0
        glm::vec3 winFar(float(xpos), glY, 1.0f);    // depth = 1
        glm::vec3 rayOrigin = glm::unProject(winNear, view, projection, viewport);
        glm::vec3 rayEnd    = glm::unProject(winFar, view, projection, viewport);
        glm::vec3 rayDir    = glm::normalize(rayEnd - rayOrigin);

        g_ptr<S_Object> closestHit = nullptr;
        float closestT = std::numeric_limits<float>::max();
        float lastVolume = 0.0f;
        for (auto obj : selectable) {

        if (auto qobj = g_dynamic_pointer_cast<Quad>(obj)) {
                if (!qobj->get<bool>("valid")) continue;
                if(!qobj->isActive()) continue;

                if (qobj->pointInQuad(vec2(xpos*2,ypos*2))) {
                    return obj;
                }
        }
        

        if(auto gobj = g_dynamic_pointer_cast<G_Object>(obj))
        {
            if(!gobj->valid) continue;
            BoundingBox worldBox = gobj->getWorldBounds();
            float t;
            if (rayIntersectsAABB(rayOrigin, rayDir, worldBox, t)) {
                if (t > 0.0f) {
                    float volume = worldBox.volume();
                    bool isCloser = t < closestT - 0.001f;
                    bool isTie = fabs(t - closestT) < 0.001f && volume > lastVolume;

                    if (isCloser || isTie) {
                        closestT = t;
                        lastVolume = volume;
                        closestHit = obj;
                    }
                }
            }
        }
            
        }

        return closestHit;
    }

    void removeObject(g_ptr<S_Object> sobj) override {
            selectable.erase(
            std::remove(selectable.begin(), selectable.end(), sobj),
            selectable.end()
            );
            //toSave.erase(sobj);
            sobj->remove();
            sobj=nullptr;
    }

    void removeSlectable(g_ptr<S_Object> sobj) {
        selectable.erase(
        std::remove(selectable.begin(), selectable.end(), sobj),
        selectable.end()
        );
        if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
        {
            if(quad->has("char"))
            {
                for(auto q : *quad->get<txt>("chars"))
                {
                    selectable.erase(
                        std::remove(selectable.begin(), selectable.end(), getObject<S_Object>(q)),
                        selectable.end()
                        );
                }
            }
        }
    }
    //Messy system right now, but it's small enough that performance isn't super important
    //in the future consider pre-caclulating end times and queueing events there
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