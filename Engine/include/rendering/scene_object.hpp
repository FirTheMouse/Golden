#pragma once

#include <iostream>
#include <util/util.hpp>
#include <core/object.hpp>


namespace Golden {
class Scene;


enum class P_State {
    NONE,           // completely static
    PASSIVE,        // moveable, but not moved by simulation
    DETERMINISTIC,  // animation/interpolated
    ACTIVE          // uses velocity-based motion
};

struct Velocity {
    vec3 position = vec3(0,0,0);
    vec3 rotation = glm::vec3(0); // degrees per second or radians/sec
    vec3 scale = glm::vec3(0);
};

struct AnimState {
    float elapsed = 0.0f;
    float duration = 0.0f;
};

class S_Object : virtual public Object {
protected:

public:
    Scene* scene;

    S_Object() {}
    virtual void remove();
    
    void saveBinary(const std::string& path) const;
    void loadBinary(const std::string& path);

    void saveBinary(std::ostream& out) const;
    void loadBinary(std::istream& in);

    virtual ~S_Object() = default;

};

}