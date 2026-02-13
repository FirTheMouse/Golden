#include<gui/quad.hpp>
#include<rendering/scene.hpp>
#include<gui/text.hpp>




namespace Golden
{

bool Quad::checkGet(int typeId)
{
    if(!scene) {
        print("Quad::checkGet::21 tried to get property ",typeId," but Scene is null!");
        return false;
    }
    // if(!isActive()) {
    //     print("Quad::checkGet::25 tried to get a property ",typeId," of an innactive Object!");
    //     return false;
    // }
    return true;
}

void Quad::remove()
{
    //If it's got a parent relationship, like text...
    //I'll just sweep it under the rug for now and use deactivate, coordinating remove across
    //grouped items is too much of a headache, plus if I add GC later it can clean these deactivated
    //objects in one nice clean pass

    //Snap should simplify all of this! But when I need a remove I'll just use the logic correctly, for now, again ignoring it.
    //I never remove anything anyways.
//     if(lockToParent)
//    {
//     callingChild=true;
//     run("onRemove");
//     parent->remove();
//     callingChild=false;
//     scene->deactivate(this);
//     return;
//    }
//    else if(!children.empty())
//    {
//     list<g_ptr<Quad>> swaped;
//     swaped <= children;
//     for(int i=swaped.length()-1;i>=0;i--)
//     {
//         auto c = swaped[i];
//         if(c->lockToParent&&!c->callingChild)
//         {
//             scene->deactivate(c);
//             c->run("onRemove");
//             // if(c->ID>=c->scene->quadActive.length()) continue;

//             // c->S_Object::remove();
//             // c->scene->quadActive.remove(c->ID);
//             // c->scene->quads.remove(c->ID);
//             // c->scene->guiTransforms.remove(c->ID);
//             // c->scene->guiEndTransforms.remove(c->ID);
//             // c->scene->slots.remove(c->ID);

//             // if(c->ID<scene->quads.length())
//             // scene->quads.get(c->ID,"Quad::remove::62")->ID =c->ID;
            
//         }
//     }
//     detatchAll();
//     }

    run("onRemove");
    scene->deactivate(this);
    S_Object::remove();
    
    scene->quadActive.remove(ID);
    scene->quads.remove(ID);
    scene->guiTransforms.remove(ID);
    scene->guiEndTransforms.remove(ID);
    scene->quadPhysicsStates.remove(ID);
    scene->quadAnimStates.remove(ID);
    scene->quadVelocities.remove(ID);
    scene->slots.remove(ID);

    if(ID<scene->quads.length())
    scene->quads.get(ID,"Quad::remove::43")->ID = ID;
}

void Quad::hide() {
    GET(scene->quadCulled,ID)=true;
}
void Quad::show() {
    GET(scene->quadCulled,ID)=false;
}
bool Quad::culled() {return  GET(scene->quadCulled,ID);}

g_ptr<Geom> Quad::getGeom() {
    return GET(scene->geoms,ID);
}
glm::mat4& Quad::getTransform() {
    return GET(scene->guiTransforms,ID);
}

glm::mat4& Quad::getEndTransform() {
    if(checkGet(1)) {
    return GET(scene->guiEndTransforms,ID);
    }
    else {
        return GET(scene->guiEndTransforms,0); }
}

AnimState& Quad::getAnimState() {
    if(checkGet(6)) {
    return GET(scene->quadAnimStates,ID);
    }
    else return GET(scene->quadAnimStates,0);
}

P_State& Quad::getPhysicsState() {
    if (checkGet(2)) {
        return GET(scene->quadPhysicsStates,ID);
    }
    else {
    static P_State dummy = P_State::NONE;
    return dummy; // Safe fallback
    }

}

Velocity& Quad::getVelocity() {
    if (checkGet(3)) {
        return GET(scene->quadVelocities,ID);
    }
    else {
        static Velocity dummy;
        return dummy; // Safe fallback
    }
}

vec2 Quad::getPosition() {
    // position = vec2(glm::vec3(getTransform()[3]));
    // return position;
    return vec2(getTransform()[3]);
}

float Quad::getRotation() {
    glm::vec2 right = glm::normalize(glm::vec2(getTransform()[0]));
    // rotation = atan2(right.y, right.x);
    // return rotation;
    return atan2(right.y, right.x);
}

vec2 Quad::getScale() {
    glm::mat4 mat = getTransform();
    vec2 scale = {
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1]))
    };
    // scaleVec = scale;
    // return scaleVec;
    return scale;
}

vec2 Quad::getCenter() {
    glm::mat4 mat = getTransform();

    // Extract rotation and scale
    glm::vec3 right = glm::vec3(mat[0]);
    glm::vec3 up    = glm::vec3(mat[1]);

    // Half-extents in transformed space
    glm::vec3 halfExtents = (right + up) * 0.5f;

    // Position is translation component
    glm::vec3 pos = glm::vec3(mat[3]);

    // Center is position + half-extents
    vec2 center(pos + halfExtents);
    return center;
}
float Quad::getDepth() {
    return (float)getTransform()[3][3];
}

vec4 Quad::getData() {
    return GET(scene->guiData,ID);
}
unsigned int Quad::getTexture() {
    return getGeom()->texture;
}
vec4 Quad::getColor() {
    return GET(scene->guiColor,ID);
}

CollisionLayer& Quad::getLayer() {
    if(checkGet(11)) {
        return GET(scene->quadCollisonLayers,ID);
    } else {
        static CollisionLayer dummy;
        return dummy;
    }
}

BoundingBox Quad::getWorldBounds()
{
    BoundingBox worldBox = getGeom()->localBounds;
    worldBox.transform(getTransform());
    return worldBox;
}

Quad& Quad::setData(const vec4& d) {
    GET(scene->guiData,ID) = d;
    return *this;
}
Quad& Quad::setColor(const vec4& d) {
    GET(scene->guiColor,ID) = d;
    return *this;
}
Quad& Quad::setTexture(const unsigned int& t) {
    getGeom()->texture = t;
    return *this;
}
Quad& Quad::setDepth(float dep, bool update)
{
    depth = dep;
    if(update)
        updateTransform();
    return *this;
}

Quad& Quad::setPhysicsState(P_State p)
{
    getPhysicsState() = p;
    return *this;
}

Quad& Quad::setLinearVelocity(const vec2& v) {
    getVelocity().position = vec3(v,0);
    return *this;
}

Quad& Quad::impulseL(const vec2& v) {
    if (checkGet(9)) {
        getVelocity().position += vec3(v,0);
    }
    return *this;
}

list<std::string>& Quad::getSlots()
{
    return GET(scene->slots,ID);;
}

Quad& Quad::addSlot(const std::string& slot)
{
    if(!getSlots().has(slot))
    {
        getSlots() << slot;
        //scene->loaded_slots[slot] << this;
    }
    return *this;
}

void Quad::addChild(g_ptr<Quad> q)
{
    if(q->parent) q->parents << this;
    else q->parent = this;
    children << q;
}

Quad& Quad::startAnim(float duration) {
    if (checkGet(7)) {
    AnimState& a = getAnimState();
    a.duration=duration;
    a.elapsed = 0.0f;
    }
    return *this;
}

Quad& Quad::setPosition(const vec2& pos, bool update)
{
    position = pos;
    if(update)
        updateTransform();
    return *this;
}

Quad& Quad::setCenter(const vec2& pos, bool update)
{
        vec2 scale = getScale();
        setPosition(pos-(scale*0.5f),update);
    return *this;
}  

Quad& Quad::rotate(float angle,bool update) 
{
    rotation = angle;
    if(update)
        updateTransform();
    return *this;
}

//WARNING: Doesn't work with update right now!
Quad& Quad::rotateCenter(float angle,bool update) {
    if(update) {
        vec2 center = getCenter();
        rotation = angle;
        updateTransform();
        vec2 newCenter = getCenter();
        position += (center - newCenter);
        updateTransform();
    } else {
        print("Quad::rotateCenter::455 Can't do rotateCenter without updating right now, implment the math later for temporary matrix transforms");
    }
    return *this;
}

Quad& Quad::scale(const vec2& scale, bool update)
{
    scaleVec = scale;
    if(update)
        updateTransform();
    return *this;
}

Quad& Quad::setScale(const vec2& _scale, bool update)
{
    return scale(_scale,update);
}

Quad& Quad::scaleBy(const vec2& _scale,bool update)
{
    return scale(scaleVec*_scale,update);
}

Quad& Quad::scaleCenter(const vec2& scale,bool update) {
    if(update) {
        vec2 center = getCenter();
        scaleVec = scale;
        updateTransform();
        vec2 newCenter = getCenter();
        position += (center - newCenter);
        updateTransform();
    } else {
        print("Quad::scaleCenter::542 Can't do scaleCenter without updating right now, implment the math later for temporary matrix transforms");
    }
    return *this;
}

Quad& Quad::move(const vec2& pos,bool update) {
    position+=pos;
    if(update) updateTransform();
    return *this;
}

Quad& Quad::move(const vec2& delta,float animationDuration,bool update) {
    endPosition+=delta;
    if(update) {
        startAnim(animationDuration);
        updateEndTransform();
    }
    return *this;
}

//Rotation is 2.6x the performance impact of hte other opperations, if this ever becomes a bottleneck consider systems like 
//an affine matrix, hot-path optimizations that ignore rotation if 0, or special handeling for that case.

void Quad::updateTransform(bool joined) {

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

    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleVec.toGlm(),1));
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f),rotation,glm::vec3(0,0,1));
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(position.toGlm(),0));

    mat = transMat * rotMat * scaleMat; 
    mat[3][3] = depth;

}

void Quad::updateEndTransform() {
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(endScale.toGlm(),1));
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f),endRotation,glm::vec3(0,0,1));
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(endPosition.toGlm(),0));
    
    getEndTransform() = transMat * rotMat * scaleMat;  
}

bool Quad::pointInQuad(vec2 point) {
    glm::mat4 mat = getTransform();

    // Extract position (translation), right and up vectors (including rotation + scale)
    glm::vec2 origin = glm::vec2(mat[3]);
    glm::vec2 right  = glm::vec2(mat[0]);
    glm::vec2 up     = glm::vec2(mat[1]);

    // Vector from origin to point
    glm::vec2 toPoint = point.toGlm() - origin;

    // Project onto local axes
    float projRight = glm::dot(toPoint, glm::normalize(right));
    float projUp    = glm::dot(toPoint, glm::normalize(up));

    // Compare against length of local axes (scale)
    float lenRight = glm::length(right);
    float lenUp    = glm::length(up);

    return (projRight >= 0 && projRight <= lenRight &&
            projUp    >= 0 && projUp    <= lenUp);
}

}

// void Quad::remove()
// {
//     S_Object::remove();
    
//     std::vector<g_ptr<Quad>>& quads          = scene->quads;
//     std::vector<glm::mat4>& guiTransforms    = scene->guiTransforms;
//     std::vector<glm::mat4>& guiEndTransforms = scene->guiEndTransforms;
//     std::vector<list<std::string>>& slots    = scene->slots;

//     size_t last = quads.size() - 1;
//     size_t idx = ID;

//     if (idx != last) {
//         // Swap all vectors to keep indexing consistent
//         std::swap(quads[idx],           quads[last]);
//         std::swap(guiTransforms[idx],   guiTransforms[last]);
//         std::swap(guiEndTransforms[idx],guiEndTransforms[last]);
//         std::swap(slots[idx],           slots[last]);

//         // Fix the swapped quad's ID 
//         size_t uuid = quads[idx]->ID = idx;
//     }

//     // Pop off the last elements
//     quads.pop_back();
//     guiTransforms.pop_back();
//     guiEndTransforms.pop_back();
//     slots.pop_back();
// }

    // vec2 Quad::getCenter() {
//     vec2 scale = getScale();
//     return getPosition() + (scale*0.5f);
// }

// S_Object& Quad::rotate(float angle) {
//     glm::mat4& mat = getTransform();
//     vec2 position = getPosition();
//     vec2 scale = getScale();
//     glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,0,1));
//     rotation[0] *= scale.x();
//     rotation[1] *= scale.y();
//     mat = rotation;
//     mat[3] = glm::vec4((position).toGlm(),1.0f, 1.0f);

//     if (s_type() == S_TYPE::INSTANCED) {
//         requestInstanceUpdate();
//     }
//     return *this;
// }