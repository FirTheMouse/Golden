#pragma once

#include<rendering/scene_object.hpp>

namespace Golden
{

struct Q_Snapshot
{
    Q_Snapshot(const vec2& _pos,const vec2& _scale,float _rot) : 
    pos(_pos), scale(_scale), rot(_rot) {} 
    Q_Snapshot() {}
    vec2 pos = vec2(0,0);
    vec2 scale = vec2(1,1);
    float rot = 0.0f;
};

/// @todo Remember to add a remove method with pop and swap for SoA!!
class Quad : virtual public S_Object 
{
protected:
    bool checkGet(int typeId);

    vec2 position = vec2(0,0);
    vec2 scaleVec = vec2(1,1);
    float rotation = 0.0f;

    vec2 endPosition = vec2(0,0);
    vec2 endScale = vec2(1,1);
    float endRotation = 0.0f;
public:
    Quad() {
        //Temporary flag, change this later! 
        //Just for compatiability with GUIDE
        set<bool>("valid",true);
        setupQuad();
    }
    void remove() override;
    void recycle();
    void setupQuad();
    void draw();

    unsigned int VAO = 0, VBO = 0, EBO = 0;

    glm::vec4 color = glm::vec4(1.0f);
    glm::vec4 uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); 
    unsigned int textureGL = 0;   // real GL texture id
    uint8_t      textureSlot = 0; // which texture unit to bind on draw
    g_ptr<Quad> parent;
    list<g_ptr<Quad>> children;
    bool callingChild = false;
    Q_Snapshot ogt;

    bool lockToParent;
     /// @todo Add methods later for removing children and detatching from parent
     void addChild(g_ptr<Quad> q,bool andLock = false);
     inline void detatchAll()
     {
         for(auto c : children)
         {
             c->lockToParent = false;
             c->parent = nullptr;
         }
         children.clear();
     }
     
     inline void removeFromParent()
     {
         if(parent)
         {
             lockToParent = false;
             parent->children.erase_if([this](const g_ptr<Quad>& q){return q->UUID == this->UUID;});
             parent = nullptr;
         }
     }
 
     inline void lockWithParent()
     {
         if(parent)
         {
         lockToParent = true;
         ogt = Q_Snapshot(
             position-parent->position,
             scaleVec-parent->scaleVec,
             rotation-parent->rotation);
         }
     }
 
    void setTexture(unsigned int glTex, uint8_t slot = 0) {
        textureGL   = glTex;
        textureSlot = slot;
    }

    void hide();
    void show();
    bool culled();

    glm::mat4& getTransform();
    glm::mat4& getEndTransform();
    AnimState& getAnimState();
    P_State& getPhysicsState();
    virtual Velocity& getVelocity();

    vec2 getPosition();
    float getRotation();
    vec2 getCenter();
    vec2 getScale();

    Quad& startAnim(float duration);
    Quad& setPhysicsState(P_State p);
    Quad& setLinearVelocity(const vec2& v);
    Quad& setPosition(const vec2& pos);
    Quad& setCenter(const vec2& pos);

    Quad& rotate(float angle);
    Quad& scale(const vec2& scale);
    Quad& move(const vec2& pos,bool update = true);
    Quad& move(const vec2& pos,float animationDuration,bool update = true);

    void setColor(glm::vec4 _color) {color = _color;}
    list<std::string>& getSlots();
    Quad& addSlot(const std::string& slot);

   
    void updateTransform();
    void updateEndTransform();
    

    bool pointInQuad(vec2 p);


};
        

}