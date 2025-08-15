#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <util/util.hpp>
#include <core/input.hpp>

class Camera {
public:
    Camera(float fov, float aspect, float nearClip, float farClip,int _type);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float sensitivity = 0.15f;
    vec3 front = vec3(0.0f, 0.0f, -1.0f);
    vec3 right = vec3(1.0f, 0.0f, 0.0f);
    vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
    bool lock = false;
    float speedMod = 0.01f;
    void setPosition(const vec3& pos);
    void setTarget(const vec3& target);
    void setUp(const glm::vec3& up);

    void update(float tpf);
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    vec3 getPosition() const;
    int getType(){return type;}

    void toIso(){type=1;}
    void toOrbit(){type=2;}
    void toFly(){type=3;}
    void toStatic(){type=4;}
    void toFirstPerson(){type=5;}

    vec3 getForward() const { return front; }
    vec3 getRight()   const { return right; }
    vec3 getUp()      const { return up; }


    // Optional: Add movement methods later

private:
    vec3 position;
    vec3 target;
    glm::vec3 up;
    float fov, aspect, nearClip, farClip,speed,height,zoomLevel;
    int type;
};
