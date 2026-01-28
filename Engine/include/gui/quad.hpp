#pragma once

#include<rendering/scene_object.hpp>
#include<rendering/geom.hpp>

namespace Golden
{

/// @todo Remember to add a remove method with pop and swap for SoA!!
class Quad : virtual public S_Object 
{
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

    bool checkGet(int typeId);

    vec2 position = vec2(0,0);
    vec2 scaleVec = vec2(1,1);
    float rotation = 0.0f; // Radians, around Z-axis (right-hand rule)

    vec2 endPosition = vec2(0,0);
    vec2 endScale = vec2(1,1);
    float endRotation = 0.0f;


    g_ptr<Geom> geom;

    g_ptr<Quad> parent;
    list<g_ptr<Quad>> children;

    //Snap is a turing-complet statefultransform propagation system, the child should always own the joint!
    list<g_ptr<Quad>> parents;
    std::function<bool()> joint = nullptr;
    std::function<bool()> physicsJoint = nullptr;
    bool unlockJoint = false;
    //This is meant to be used for Snap to add break-points in parent chains, but it doesn't have to be
    bool isAnchor = false;

    //These are just here as shared calculation sketchpads for Twig-Snap
    float opt_y_offset = 0;
    float opt_x_offset = 0;
    vec2 opt_offset = {0,0};
    vec2 opt_delta = {0,0};
    float opt_float = 0;
    float opt_float_2 = 0;
    int opt_int = 0;
    bool opt_flag = false;
    char opt_char = ' ';
    g_ptr<Quad> opt_ptr = nullptr;
    vec2 t_vec = {0,0};

    list<float> opt_list_float;
    list<int> opt_list_lint;


    bool instanced = false;

    void hide();
    void show();
    bool culled();

    void addChild(g_ptr<Quad> q);

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
    BoundingBox getWorldBounds() override;

    Quad& setData(const vec4& d);
    Quad& setTexture(const unsigned int& t);
    Quad& setColor(const vec4& _color);
    Quad& setColor(glm::vec4 _color) {return setColor(vec4(_color));}
    void setUV(vec4 _uv) {setData(_uv);}
    void setUV(glm::vec4 _uv) {setData(vec4(_uv));}
    void setGeom(g_ptr<Geom> geomPtr) {
        geom = std::move(geomPtr);
    }


    Quad& startAnim(float duration);
    Quad& setPhysicsState(P_State p);
    Quad& setLinearVelocity(const vec2& v);
    Quad& impulseL(const vec2& v);
    Quad& setPosition(const vec2& pos, bool update = true);
    Quad& setCenter(const vec2& pos, bool update = true);

    Quad& rotate(float angle, bool update = true);
    Quad& rotateCenter(float angle, bool update = true);
    Quad& scale(const vec2& scale, bool update = true);
    Quad& setScale(const vec2& _scale, bool update = true);
    Quad& scaleBy(const vec2& _scale, bool update = true);
    Quad& scaleCenter(const vec2& _scale, bool update = true);
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

   
    void updateTransform(bool joined = true);
    void updateEndTransform();
    

    bool pointInQuad(vec2 p);


};
        

}