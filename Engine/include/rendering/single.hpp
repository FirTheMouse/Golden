#pragma once

#include<rendering/scene_object.hpp>
#include<rendering/model.hpp>


namespace Golden {
class Single : virtual public S_Object
{
protected:
    bool checkGet(int typeId);
public:
    Single() {}
    virtual void remove() override;
    virtual void recycle();
    ~Single() {remove();};

    g_ptr<Model> model;

    explicit Single(g_ptr<Model> modelPtr)
    : model(std::move(modelPtr)) {}

    explicit Single(Model&& m) {
        model = make<Model>(std::move(m));
    }

    void setModel(g_ptr<Model> modelPtr) {
        model = std::move(modelPtr);
    }

    void setModel(Model&& m) {
        model = make<Model>(std::move(m));
    }
    
    g_ptr<Model> getModel();

    vec3 position = vec3(0,0,0);
    vec3 scaleVec = vec3(1,1,1);
    glm::quat rotation = glm::quat(1,0,0,0);

    vec3 endPosition = vec3(0,0,0);
    vec3 endScale = vec3(1,1,1);
    glm::quat endRotation = glm::quat(1,0,0,0);

    g_ptr<Single> parent;
    list<g_ptr<Single>> children;

    //Snap is a turing-complet stateful transform propagation system, the child should always own the joint!
    list<g_ptr<Single>> parents;
    std::function<bool()> joint = nullptr;
    std::function<bool()> physicsJoint = nullptr;
    bool unlockJoint = false;
    //This is meant to be used for Snap to add break-points in parent chains, but it doesn't have to be
    bool isAnchor = false;

    list<int> opt_ints;
    list<int> opt_idx_cache;
    list<int> opt_idx_cache_2;
    list<float> opt_floats;
    vec3 opt_vec_3;
    vec3 opt_vec3_2;
    vec3 opt_vec_3_3;

    //This is mainly intended for debug
    list<double> timers;
    list<std::string> timer_labels;


    virtual glm::mat4& getTransform();
    virtual glm::mat4& getEndTransform();
    virtual AnimState& getAnimState();
    virtual P_State& getPhysicsState();
    virtual Velocity& getVelocity();
    

    Single& setPhysicsState(P_State p);
    Single& setVelocity(const Velocity& vel);
    Single& startAnim(float duration);

    vec3 markerPos(std::string marker);
    list<std::string> markers() {
        return getModel()->markers.keySet();
    }

    BoundingBox getWorldBounds() override;

    
    vec3 getPosition();
    glm::quat getRotation();
    vec3 getRotationEuler();
    vec3 getScale();
    CollisionLayer& getLayer();

    virtual float distance(g_ptr<Single> other) {return getPosition().distance(other->getPosition());}
    virtual vec3 direction(g_ptr<Single> other) {return getPosition().direction(other->getPosition());}
    virtual Single& setPosition(const vec3& pos,bool update = true);
    virtual Single& move(const vec3& delta,bool update = true);
    virtual Single& move(const vec3& delta,float duration,bool update=true);
    virtual Single& move(float x, float y, float z);
    virtual Single& faceTo(const vec3& targetPos);
    virtual Single& faceTowards(const vec3& direction, const vec3& up);

    virtual Single& rotate(float angle, const vec3& axis,bool update = true);
    virtual vec3 facing();
    virtual vec3 up();
    virtual vec3 right();

    Single& scale(const vec3& v,bool update=true);
    Single& setScale(const vec3& v,bool update=true);
    Single& scale (float s,bool update=true) {scale(vec3(s,s,s),update); return *this;}
    Single& setScale (float s,bool update=true) {setScale(vec3(s,s,s),update); return *this;}

    virtual Single& setLinearVelocity(const vec3& v);
    virtual Single& impulseL(const vec3& v);
    virtual Single& setRotVelocity(const vec3& v);
    virtual Single& impulseR(const vec3& v);
    virtual Single& setScaleVelocity(const vec3& v);
    virtual Single& impulseS(const vec3& v);
    void setColor(const glm::vec4& color);

    void hide();
    void show();
    bool culled();

    void updateTransform(bool joined = true);
    void updateEndTransform();

};

}