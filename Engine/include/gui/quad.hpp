#pragma once

#include<rendering/scene_object.hpp>
#include<rendering/geom.hpp>

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
    }
    explicit Quad(g_ptr<Geom> geomPtr) : geom(geomPtr) {}


    void remove() override;
    // void recycle();
    // void setupQuad();
    // void draw();

    g_ptr<Geom> geom;

    g_ptr<Quad> parent;
    vec2 t_vec = {0,0};
    list<g_ptr<Quad>> children;
    bool callingChild = false;
    Q_Snapshot ogt;

    bool instanced = false;

    bool lockToParent = false;
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
 
    // void setTexture(unsigned int glTex, uint8_t slot = 0) {
    //     textureGL   = glTex;
    //     textureSlot = slot;
    // }

    void hide();
    void show();
    bool culled();


    g_ptr<Geom> getGeom();
    glm::mat4& getTransform();
    glm::mat4& getEndTransform();
    AnimState& getAnimState();
    P_State& getPhysicsState();
    virtual Velocity& getVelocity();

    vec2 getPosition();
    float getRotation();
    vec2 getCenter();
    vec2 getScale();
    vec4 getData();
    unsigned int getTexture();
    CollisionLayer& getLayer();

    Quad& setData(const vec4& d);
    Quad& setTexture(const unsigned int& t);
    void setColor(vec4 _color) {setData(_color);}
    void setColor(glm::vec4 _color) {setData(vec4(_color));}
    void setUV(vec4 _uv) {setData(_uv);}
    void setUV(glm::vec4 _uv) {setData(vec4(_uv));}
    void setGeom(g_ptr<Geom> geomPtr) {
        geom = std::move(geomPtr);
    }


    Quad& startAnim(float duration);
    Quad& setPhysicsState(P_State p);
    Quad& setLinearVelocity(const vec2& v);
    Quad& impulseL(const vec2& v);
    Quad& setPosition(const vec2& pos);
    Quad& setCenter(const vec2& pos);

    Quad& rotate(float angle);
    Quad& scale(const vec2& scale);
    Quad& move(const vec2& pos,bool update = true);
    Quad& move(const vec2& pos,float animationDuration,bool update = true);

    void track(const vec2& delta = vec2(0,0)) {
        if(culled()||!isActive()) {
            t_vec+=delta;
        } else if(t_vec!=vec2(0,0)) {
            move(t_vec);
            t_vec = vec2(0,0);
        }
    }

    vec2 getLeft() { return vec2(getCenter().x()-getScale().x()/2,getCenter().y()); }
    vec2 getRight() { return vec2(getCenter().x()+getScale().x()/2,getCenter().y()); }
    vec2 getTop() { return vec2(getCenter().x(),getCenter().y()-getScale().y()/2); }
    vec2 getBottom() { return vec2(getCenter().x(),getCenter().y()+getScale().y()/2); }

    bool intersects(g_ptr<Quad> other, vec2& outMTV) {
        vec2 cA = getCenter();
        vec2 cB = other->getCenter();
    
        vec2 hA = getScale() * 0.5f;
        vec2 hB = other->getScale() * 0.5f;
    
        float dx = cB.x() - cA.x();
        float px = (hA.x() + hB.x()) - std::fabs(dx);
        if (px <= 0.0f) return false;
    
        float dy = cB.y() - cA.y();
        float py = (hA.y() + hB.y()) - std::fabs(dy);
        if (py <= 0.0f) return false;

        if (px < py)
            outMTV = vec2((dx < 0.0f ? -px : px), 0.0f);
        else
            outMTV = vec2(0.0f, (dy < 0.0f ? -py : py));

        return true;
    }
    bool intersects(g_ptr<Quad> other)
    {
        vec2 mtv;
        return intersects(other,mtv);
    }
    bool intersects(g_ptr<Quad> other, vec2& normal, float& degree) {
        vec2 mtv;
        if (!intersects(other, mtv)) return false;
        degree = (mtv.x() != 0.0f) ? std::fabs(mtv.x()) : std::fabs(mtv.y());
        if (mtv.x() != 0.0f) 
            normal = vec2((mtv.x() < 0.0f) ? -1.0f : 1.0f, 0.0f);
        else 
            normal = vec2(0.0f, (mtv.y() < 0.0f) ? -1.0f : 1.0f);
    
        return true;
    }
 
    bool intersectsSimple(g_ptr<Quad> other) {
        return 
        ( (getTop().y()<=other->getBottom().y()) && (getBottom().y()>=other->getTop().y()) && 
          (getLeft().x()<=other->getRight().x()) && (getRight().x()>=other->getLeft().x())  
        );
    }
    float distance(g_ptr<Quad> other) {
        return std::abs((getCenter() - other->getCenter()).length());
    }
    vec2 direction(vec2 point) {
        return point-getCenter();
    }
    vec2 direction(g_ptr<Quad> other) {
        return direction(other->getCenter());
    }

    list<std::string>& getSlots();
    Quad& addSlot(const std::string& slot);

   
    void updateTransform();
    void updateEndTransform();
    

    bool pointInQuad(vec2 p);


};
        

}