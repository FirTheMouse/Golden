#include <rendering/camera.hpp>

Camera::Camera(float fov, float aspect, float nearClip, float farClip,int _type)
    : position(0.0f, 0.0f, 0.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f),
      fov(fov), aspect(aspect), nearClip(nearClip), farClip(farClip),type(_type),speed(18),height(35),zoomLevel(0.5) {}

void Camera::setPosition(const vec3& pos) { position = pos; }
void Camera::setTarget(const vec3& tgt) {target = tgt;}
void Camera::setUp(const glm::vec3& u) { up = u; }

void Camera::update(float tpf)
{
    if(!lock)
    {
    if(type == 1) //Isocam movement
    {
    up = worldUp.toGlm();
    vec3 camMove(0.0f,0.0f,0.0f);
    if(!Input::get().keyPressed(LALT)) {
        if(Input::get().keyPressed(KeyCode::W)) camMove.addZ(-1);
        if(Input::get().keyPressed(KeyCode::S)) camMove.addZ(1);
        if(Input::get().keyPressed(KeyCode::A)) camMove.addX(-1);
        if(Input::get().keyPressed(KeyCode::D)) camMove.addX(1);
    }
        camMove.addY(-Input::get().getScrollY());

        if(position.y()<=2&&Input::get().getScrollY()>0) camMove.addY(-camMove.y());
        if(position.y()>=100&&Input::get().getScrollY()<0) camMove.addY(-camMove.y());
        if(position.y()<10) speed=8 * speedMod;
        else if(position.y()<45) speed=18 * speedMod;
        else if(position.y()<80) speed=30 * speedMod;
        else speed=50 * speedMod;
        camMove = camMove*speed*tpf;
        position = position+camMove;
        target = vec3(position.x(),0.0f,position.z()-(position.y()+5));
    }
    else if(type == 2) //Orbitcam movement
    {
    up = worldUp.toGlm();
    static float orbitYaw = 0.0f;
    static float orbitPitch = glm::radians(45.0f);  // slightly angled down
    static float orbitDistance = 20.0f;
    static int debounce = 0;

    orbitYaw -= Input::get().getScrollX() * 0.03f;
    if(Input::get().keyPressed(LSHIFT)) {orbitDistance = glm::clamp((float)(orbitDistance - Input::get().getScrollY() * 0.1f), 2.0f, 100.0f);
    debounce=1;}
    else if(debounce==0)  orbitPitch = glm::clamp((float)(orbitPitch + Input::get().getScrollY() * 0.01f), glm::radians(-25.0f), glm::radians(85.0f));
    else if(Input::get().getScrollY()<=0.04f&&Input::get().getScrollY()>=-0.04f) debounce = 0;
    
    // Convert spherical to Cartesian
    position = vec3(
        target.x() + orbitDistance * cos(orbitPitch) * sin(orbitYaw),
        target.y() + orbitDistance * sin(orbitPitch),
        target.z() + orbitDistance * cos(orbitPitch) * cos(orbitYaw)
    );

    vec3 camMove(0.0f,0.0f,0.0f);
    if(Input::get().keyPressed(LSHIFT)) {
        if(Input::get().keyPressed(KeyCode::W)) camMove.addZ(-0.2f);
        if(Input::get().keyPressed(KeyCode::S)) camMove.addZ(0.2f);
        if(Input::get().keyPressed(KeyCode::A)) camMove.addX(-0.2f);
        if(Input::get().keyPressed(KeyCode::D)) camMove.addX(0.2f);
    }
    target+=camMove;

    }
    else if(type == 3) //Flycam movement
    {
        float dx = Input::get().getMouseDeltaX();
        float dy = Input::get().getMouseDeltaY();
        Input::get().syncMouse(); // consume input

        yaw += dx * sensitivity;
        pitch += dy * sensitivity;

        // Clamp pitch
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        // Calculate direction
        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = vec3(glm::normalize(dir));
        right = glm::normalize(glm::cross(front.toGlm(), worldUp.toGlm()));
        up = glm::normalize(glm::cross(right.toGlm(), front.toGlm()));

        // WASD movement 
        vec3 camMove(0.0f,0.0f,0.0f);
        if (Input::get().keyPressed(KeyCode::W)) camMove += front;
        if (Input::get().keyPressed(KeyCode::S)) camMove -= front;
        if (Input::get().keyPressed(KeyCode::A)) camMove -= right;
        if (Input::get().keyPressed(KeyCode::D)) camMove += right;
        // if (Input::get().keyPressed(KeyCode::SPACE)) camMove.y() += 1;
        // if (Input::get().keyPressed(KeyCode::LSHIFT)) camMove.y() -= 1;

        if (glm::length(camMove.toGlm()) > 0.0f)
            camMove = camMove.normalized() * speed * tpf * speedMod;

        position += camMove;
    }
    else if(type == 4) //static cam, doesn't do a thing!
    {

    }
    else if(type == 5) //First person cam
    {
        float dx = Input::get().getMouseDeltaX();
        float dy = Input::get().getMouseDeltaY();
        Input::get().syncMouse(); // consume input

        yaw += dx * sensitivity;
        pitch += dy * sensitivity;

        // Clamp pitch
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        // Calculate direction
        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = vec3(glm::normalize(dir));
        right = glm::normalize(glm::cross(front.toGlm(), worldUp.toGlm()));
        up = glm::normalize(glm::cross(right.toGlm(), front.toGlm()));
    }
    }
}

glm::mat4 Camera::getViewMatrix() const {
    if(type==1||type==2) {
    return glm::lookAt(position.toGlm(), target.toGlm(), up);
    }
    else { 
    return glm::lookAt(position.toGlm(), (position + front).toGlm(), up); 
    }
}
glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
}
vec3 Camera::getPosition() const {
    return position;
}
