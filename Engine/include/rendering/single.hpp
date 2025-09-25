#pragma once

#include<rendering/scene_object.hpp>
#include<rendering/model.hpp>


namespace Golden {
class Single : virtual public S_Object
{
protected:
    bool checkGet(int typeId);

    vec3 position = vec3(0,0,0);
    vec3 scaleVec = vec3(1,1,1);
    glm::quat rotation = glm::quat(1,0,0,0);

    vec3 endPosition = vec3(0,0,0);
    vec3 endScale = vec3(1,1,1);
    glm::quat endRotation = glm::quat(1,0,0,0);
public:
    Single() {}
    virtual void remove();
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
        return model->markers.keySet();
    }

    BoundingBox getWorldBounds();

    
    vec3 getPosition();
    glm::quat getRotation();
    vec3 getRotationEuler();
    vec3 getScale();

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

    void updateTransform();
    void updateEndTransform();

};

}