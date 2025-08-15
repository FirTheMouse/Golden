#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <util/util.hpp>
#include <core/object.hpp>

namespace Golden {
class Scene;

class S_Object : public std::enable_shared_from_this<S_Object>, public Golden::Object {
public:
    shared_ptr<Model> model;
    Scene* scene = nullptr;
    std::vector<Cell*> inCells;
    bool instance = false;
    bool cleaned = false;
    bool locked = false;
    bool lifted = false;
    std::string dtype = "UNSET";
    std::string type = "UNSET";
    std::string name = "UNSET";
    int hp = 4;
    size_t id;
    size_t uid;
    std::vector<shared_ptr<Object>> has;


    Object() {};
    virtual ~Object() = default; 

    virtual void cleanUp();

    inline void setModel(const Model& _model) {model = make_shared<Model>(_model);}

    inline vec3 getPosition() {
        if(instance) return vec3(model->instanceTransforms[id][3]);
        model->Iid=id;
        return model->getPosition();
        }

    void updateCells();

    inline void transform(const glm::mat4& transform) {model->Iid=id; model->transform(transform);}

    inline glm::mat4& getTransform() {model->Iid=id; return model->getTransform();}

    inline void setPosition(const vec3& pos) {model->Iid=id; model->setPosition(pos);}

    inline void setScale(float f) {model->Iid=id; model->setScale(f);}

    inline Model& faceTo(const vec3& targetPos) {model->Iid=id; return model->faceTo(targetPos);}

    inline float height() {model->Iid=id; return model->localBounds.max.y;}
    inline float width() {model->Iid=id; return model->localBounds.max.x;}
    inline float depth() {model->Iid=id; return model->localBounds.max.z;}

    Model& moveTo(const vec3& pos,float speed);

    Model& move(const vec3& movement);
    //X Y Z
    Model& move(float x = 0.0f, float y = 0.0f, float z = 0.0f);

};

}




void S_Object::updateCells()
{
    if(scene)
    {
    for(auto o : inCells) o->remove(this);
    inCells.clear();
    model->scene->grid.getCell(getPosition()).add(shared_from_this());
    }
}

void S_Object::cleanUp()
{
    if(!cleaned)
    {
    cleaned = true;
   //cout << "Clean up " << id << " stage 1 " << endl;
    //cout << "Clean up " << id << " stage 2 " << endl;
    Scene* localScene = scene;
    scene=nullptr;
   // cout << "Clean up " << id << " stage 3 " << endl;
    if (auto* ent = dynamic_cast<Entity*>(this)) {
        Sim::get().removeEntity(ent);
    }
   // cout << "Clean up " << id << " stage 4 " << endl;
    localScene->remove(this);
    //cout << "DONE" << endl;
    }
    else cout << "Attemped double clean" << endl;

}


Model& S_Object::move(const vec3& movement) {model->Iid=id; model->move(movement); updateCells(); return *model;}
Model& S_Object::move(float x, float y, float z) {model->Iid=id; model->move(x,y,z); updateCells(); return *model;}
Model& S_Object::moveTo(const vec3& pos,float speed) {model->Iid = id; model->moveTo(pos,speed); updateCells(); return *model;}