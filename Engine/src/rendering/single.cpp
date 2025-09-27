#include <rendering/single.hpp>
#include <rendering/scene.hpp>
#include<rendering/model.hpp>

#define SINGLE_DEBUG 1
#define TO_STRING(x) #x
#define TO_STRING_EXPAND(x) TO_STRING(x)

#if SINGLE_DEBUG
    #define GET(container, id) \
        container.get(id, debug_trace_path + \
                         std::string(__FUNCTION__) + " at " + std::string(__FILE__) + ":" + TO_STRING_EXPAND(__LINE__))
#else
    #define GET(container, id) container.get(id)
#endif

namespace Golden
{
bool Single::checkGet(int typeId)
{
    if(!scene) {
        print("Single::checkGet::9 tried to get property ",typeId," but Scene is null!");
        return false;
    }
    // if(!isActive()) {
    //     print("Single::checkGet::13 tried to get a property ",typeId," of an innactive Object!");
    //     return false;
    // }
    return true;
}

vec3 Single::markerPos(std::string marker)
{
    return vec3(getTransform() * glm::vec4(model->markers.get(marker).toGlm(), 1.0f));
}

void Single::remove()
{
    stop();
    debug_trace_path = ("Single::remove::48");
    size_t last_idx = scene->active.length() - 1;
    try {
    size_t id = (ID != last_idx) ? scene->singles.get(last_idx,debug_trace_path)->ID : 0;
    size_t idx = ID;

    model = nullptr;

    scene->active.remove(ID);
    scene->culled.remove(ID);
    scene->singles.remove(ID);
    scene->transforms.remove(ID);
    scene->endTransforms.remove(ID);
    scene->animStates.remove(ID);
    scene->velocities.remove(ID);
    scene->physicsStates.remove(ID);
    scene->collisonLayers.remove(ID);
    scene->collisionShapes.remove(ID);

    if(ID!=last_idx)
    {
            auto obj = scene->singles.get(id);
            obj->ID = idx;
        } else {
            print("ERROR: Swapped object ID=", id, " not found in objects map!");
    }
    }
    catch(const std::exception& e)
    {
        print("single::remove::72 Failed removal at index ",ID,": ",e.what());
    }
}

void Single::recycle()
{
    scene->recycle(this,dtype);
}

glm::mat4& Single::getTransform() {
    if(checkGet(0)) {
        return GET(scene->transforms,ID);
    }
    else {
        return GET(scene->transforms,0);
    }
}

glm::mat4& Single::getEndTransform() {
    if(checkGet(1)) {
    return GET(scene->endTransforms,ID);
    }
    else {
      return GET(scene->endTransforms,0); }
}

AnimState& Single::getAnimState() {
    if(checkGet(6)) {
    return GET(scene->animStates,ID);
    }
    else return GET(scene->animStates,0);
}

P_State& Single::getPhysicsState() {
    if (checkGet(2)) {
        return GET(scene->physicsStates,ID);
    }
    else {
    static P_State dummy = P_State::NONE;
    return dummy; // Safe fallback
    }

}

Velocity& Single::getVelocity() {
    if (checkGet(3)) {
        return GET(scene->velocities,ID);
    }
    else {
        static Velocity dummy;
        return dummy; // Safe fallback
    }
}

CollisionLayer& Single::getLayer() {
    if(checkGet(11)) {
        return GET(scene->collisonLayers,ID);
    } else {
        static CollisionLayer dummy;
        return dummy;
    }
}

vec3 Single::getPosition() {
    #if SINGLE_DEBUG
    debug_trace_path = "Single::getPosition::86";
    #endif
    position = vec3(glm::vec3(getTransform()[3]));
    return position;
}

glm::quat Single::getRotation() {
    #if SINGLE_DEBUG
       debug_trace_path = "Single::getRotation::104";
    #endif
    rotation = glm::quat_cast(getTransform());;
    return rotation;
}

vec3 Single::getRotationEuler() {
    #if SINGLE_DEBUG
       debug_trace_path = ("Single::getRotationEuler::111");
    #endif
    return glm::eulerAngles(getRotation());
}

vec3 Single::getScale() {
    glm::mat4 mat = getTransform();
    glm::vec3 scale{
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };
    scaleVec = vec3(scale);
    return scaleVec;
}

Single& Single::setPhysicsState(P_State p_state) {
    if (checkGet(4)) {
        GET(scene->physicsStates,ID) = p_state;
    }
    return *this;
}

Single& Single::setVelocity(const Velocity& v) {
    if (checkGet(5)) {
        GET(scene->velocities,ID) = v;
    }
    return *this;
}

Single& Single::setPosition(const vec3& pos,bool update) {
    position = pos;
    if(update) updateTransform();
    return *this;
}

Single& Single::startAnim(float duration) {
    if (checkGet(7)) {
    AnimState& a = getAnimState();
    a.duration=duration;
    a.elapsed = 0.0f;
    }
    return *this;
}

Single& Single::move(const vec3& pos,bool update) {
    position+=pos;
    if(update) updateTransform();
    return *this;
}

Single&  Single::move(const vec3& delta,float animationDuration,bool update) {
    endPosition+=delta;
    if(update) {
        startAnim(animationDuration);
        updateEndTransform();
    }
    return *this;
}

Single& Single::move(float x, float y, float z)
{
   return move(vec3(x,y,z));
}


Single& Single::rotate(float angle, const vec3& axis, bool update) {
    glm::quat deltaRot = glm::angleAxis(glm::radians(angle), axis.toGlm());
    rotation = deltaRot * rotation;
    if(update) updateTransform();
    return *this;
}

Single& Single::scale(const vec3& v, bool update)
{
    scaleVec = scaleVec*v;
    if(update)
    {
        updateTransform();
    }
    return *this;
}

Single& Single::setScale(const vec3& v, bool update)
{
    scaleVec = v;
    if(update)
    {
        updateTransform();
    }
    return *this;
}

vec3 Single::facing()
{
    glm::vec3 direction = glm::normalize(glm::vec3(getTransform()[2]));
    return vec3(direction);
}

vec3 Single::up()
{
    glm::vec3 direction = glm::normalize(glm::vec3(getTransform()[1]));
    return vec3(direction);
}

vec3 Single::right()
{
    glm::vec3 direction = glm::normalize(glm::vec3(getTransform()[0]));
    return vec3(direction);
}

Single& Single::setLinearVelocity(const vec3& v) {
    if (checkGet(8)) {
        getVelocity().position = v;
    }
    return *this;
}

Single& Single::impulseL(const vec3& v) {
    if (checkGet(9)) {
        getVelocity().position += v;
    }
    return *this;
}

Single& Single::setRotVelocity(const vec3& v) {
    if (checkGet(10)) {
        getVelocity().rotation = v;
    }
    return *this;
}

Single& Single::impulseR(const vec3& v) {
    if (checkGet(11)) {
        getVelocity().rotation += v;
    }
    return *this;
}

Single& Single::setScaleVelocity(const vec3& v) {
    if (checkGet(12)) {
        getVelocity().scale = v;
    }
    return *this;
}

Single& Single::impulseS(const vec3& v) {
    if (checkGet(13)) {
        getVelocity().scale += v;
    }
    return *this;
}

BoundingBox Single::getWorldBounds()
{
    BoundingBox worldBox = model->localBounds;
    worldBox.transform(getTransform());
    return worldBox;
}

void Single::setColor(const glm::vec4& color)
{model->setColor(color);}


void Single::hide() {scene->culled.get(ID)=true;}
void Single::show() {scene->culled.get(ID)=false;}
bool Single::culled() {return scene->culled.get(ID);}

void Single::updateTransform() {
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scaleVec.toGlm());
    glm::mat4 rotMat = glm::mat4_cast(rotation);
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), position.toGlm());
    getTransform() = transMat * rotMat * scaleMat;
}

void Single::updateEndTransform() {
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), endScale.toGlm());
    glm::mat4 rotMat = glm::mat4_cast(endRotation);
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), endPosition.toGlm());
    
    getEndTransform() = transMat * rotMat * scaleMat;
}


Single& Single::faceTo(const vec3& targetPos) {
    vec3 currentPos = getPosition();
    vec3 toTarget = targetPos - currentPos;

    toTarget.addY(-toTarget.y()); // flatten to XZ plane
    if (toTarget.length() < 1e-5) return *this;

    float angle = atan2(toTarget.x(), toTarget.z());

    glm::mat4& mat = getTransform();

    // Decompose current matrix into position, rotation, and scale
    glm::vec3 position = glm::vec3(mat[3]);
    glm::vec3 scale{
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };

    // Create rotation matrix around Y
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));

    // Apply scale after rotation
    rotation[0] *= scale.x;
    rotation[1] *= scale.y;
    rotation[2] *= scale.z;

    // Final transform: scaled & rotated, with position
mat = rotation;
    mat[3] = glm::vec4(position, 1.0f);

    return *this;
}

Single& Single::faceTowards(const vec3& direction, const vec3& up) {
    glm::vec3 dir = glm::normalize(direction.toGlm());
    glm::vec3 right = glm::normalize(glm::cross(up.toGlm(), dir));
    glm::vec3 newUp = glm::cross(dir, right);

    glm::mat4& mat = getTransform();
    glm::vec3 position = glm::vec3(mat[3]);
    glm::vec3 scale{
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };

    mat = glm::mat4(1.0f);
    mat[0] = glm::vec4(right * scale.x, 0.0f);
    mat[1] = glm::vec4(newUp * scale.y, 0.0f);
    mat[2] = glm::vec4(dir * scale.z, 0.0f); // Z is forward
    mat[3] = glm::vec4(position, 1.0f);

    return *this;
}

}