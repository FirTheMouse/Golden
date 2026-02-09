#include <rendering/single.hpp>
#include <rendering/scene.hpp>
#include<rendering/model.hpp>



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
    return vec3(getTransform() * glm::vec4(getModel()->markers.get(marker).toGlm(), 1.0f));
}

void Single::remove()
{
    size_t id = ID;
    scene->active.impl.removeAt(id);
    scene->culled.impl.removeAt(id);
    scene->singles.impl.removeAt(id);
    if(!scene->models.empty()) {
        scene->models.removeAt(id);
        scene->transforms.impl.removeAt(id);
        scene->endTransforms.impl.removeAt(id);
        scene->animStates.impl.removeAt(id);
        scene->velocities.impl.removeAt(id);
        scene->physicsStates.impl.removeAt(id);
        scene->collisonLayers.impl.removeAt(id);
        scene->collisionShapes.impl.removeAt(id);
        scene->physicsProp.impl.removeAt(id);
    }

    // stop();
    // debug_trace_path = ("Single::remove::48");
    // size_t last_idx = scene->active.length() - 1;
    // try {
    // size_t id = (ID != last_idx) ? scene->singles.get(last_idx,debug_trace_path)->ID : 0;
    // size_t idx = ID;

    // model = nullptr;

    // scene->active.remove(ID);
    // scene->culled.remove(ID);
    // scene->singles.remove(ID);
    // scene->models.removeAt(ID);
    // scene->transforms.remove(ID);
    // scene->endTransforms.remove(ID);
    // scene->animStates.remove(ID);
    // scene->velocities.remove(ID);
    // scene->physicsStates.remove(ID);
    // scene->collisonLayers.remove(ID);
    // scene->collisionShapes.remove(ID);

    // if(ID!=last_idx)
    // {
    //         auto obj = scene->singles.get(id);
    //         obj->ID = idx;
    //     } else {
    //         print("ERROR: Swapped object ID=", id, " not found in objects map!");
    // }
    // }
    // catch(const std::exception& e)
    // {
    //     print("single::remove::72 Failed removal at index ",ID,": ",e.what());
    // }
}

void Single::recycle()
{
    scene->recycle(this,dtype);
}

g_ptr<Model> Single::getModel() {
    if(model) {
        return model;
    } else if (checkGet(13)) {
        return GET(scene->models,ID);
    } else {
        return nullptr;
    }

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
      return GET(scene->endTransforms,0); 
    }
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
    // position = vec3(glm::vec3(getTransform()[3]));
    // return position;
    return vec3(glm::vec3(getTransform()[3]));
}

glm::quat Single::getRotation() {
    // rotation = glm::quat_cast(getTransform());
    // return rotation;
    return glm::quat_cast(getTransform());
}

vec3 Single::getRotationEuler() {
    return glm::eulerAngles(getRotation());
}

vec3 Single::getScale() {
    glm::mat4 mat = getTransform();
    vec3 scale{
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };
    // scaleVec = vec3(scale);
    // return scaleVec;
    return scale;
}

Single& Single::setPhysicsState(P_State p_state) {
    GET(scene->physicsStates,ID) = p_state;
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
    BoundingBox worldBox = getModel()->localBounds;
    worldBox.transform(getTransform());
    return worldBox;
}

void Single::setColor(const vec4& color) {GET(scene->colors,ID)=color;}


void Single::hide() {scene->culled.get(ID)=true;}
void Single::show() {scene->culled.get(ID)=false;}
bool Single::culled() {return scene->culled.get(ID);}

void Single::updateTransform(bool joined) {

    glm::mat4& mat = getTransform();

    if(joined) {
        bool doUpdate = true;
        if(!unlockJoint && joint) {
            doUpdate = joint();
        }

        if(!children.empty()) {
            for(auto c : children) c->updateTransform();
        }

        if(!doUpdate) return;
    }

    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scaleVec.toGlm());
    glm::mat4 rotMat = glm::mat4_cast(rotation);
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), position.toGlm());
    mat = transMat * rotMat * scaleMat;
}

void Single::updateEndTransform() {
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), endScale.toGlm());
    glm::mat4 rotMat = glm::mat4_cast(endRotation);
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), endPosition.toGlm());
    
    getEndTransform() = transMat * rotMat * scaleMat;
}


Single& Single::faceTo(const vec3& targetPos) {
    vec3 currentPos = position; // Use stored position directly
    vec3 toTarget = targetPos - currentPos;

    toTarget.addY(-toTarget.y()); // flatten to XZ plane
    if (toTarget.length() < 1e-5) return *this;

    float angle = atan2(toTarget.x(), toTarget.z());

    // Update the rotation quaternion directly
    rotation = glm::quat(glm::vec3(0, angle, 0)); // Y-axis rotation
    
    // Let updateTransform rebuild the matrix
    updateTransform();

    return *this;
}

Single& Single::faceTowards(const vec3& direction, const vec3& up) {
    glm::vec3 dir = glm::normalize(direction.toGlm());
    glm::vec3 upVec = up.toGlm();
    
    // Build rotation quaternion from look direction
    glm::vec3 right = glm::normalize(glm::cross(upVec, dir));
    glm::vec3 newUp = glm::cross(dir, right);
    
    glm::mat3 rotMatrix;
    rotMatrix[0] = right;
    rotMatrix[1] = newUp;
    rotMatrix[2] = dir;
    
    // Convert rotation matrix to quaternion
    rotation = glm::quat_cast(rotMatrix);
    
    // Let updateTransform rebuild the matrix
    updateTransform();

    return *this;
}

}