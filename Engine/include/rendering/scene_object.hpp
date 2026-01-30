#pragma once

#include <iostream>
#include <util/engine_util.hpp>
#include <core/object.hpp>


namespace Golden {
class Scene;


enum class P_State {
    NONE,           // completely static
    PASSIVE,        // does not use velocity, gravity, or drag, but has collisons
    DETERMINISTIC,  // animation/interpolated
    ACTIVE,          // uses velocity-based motion
    FREE,            // uses velocity and has collisons, but doesn't have gravity or drag
    GHOST,           // uses velocity, but has no collisons, gravity, or drag
    PARTICLE        // uses velocity and has gravity and drag, but no collisons.
};

//          Preloops | Velocity | Collison
//None:        no         no        no
//Passive:     no         no        yes
//Active:      yes        yes       yes
//Free:        no         yes       yes
//Ghost        no         yes       no
//Particle:    yes        yes       no

inline bool p_state_uses_velocity(const P_State& p) { return (p==P_State::ACTIVE||p==P_State::FREE||p==P_State::GHOST||p==P_State::PARTICLE);}
inline bool p_state_collides(const P_State& p) { return (p==P_State::ACTIVE||p==P_State::FREE||p==P_State::PASSIVE);}
inline bool p_state_preloops(const P_State& p) { return (p==P_State::ACTIVE||p==P_State::PARTICLE);}

struct Velocity {
    vec3 position = vec3(0,0,0);
    vec3 rotation = vec3(0,0,0); // degrees per second or radians/sec
    vec3 scale = vec3(0,0,0);

    float length() {
        return position.length()+rotation.length()+scale.length();
    }
};

struct AnimState {
    float elapsed = 0.0f;
    float duration = 0.0f;
};

struct CollisionLayer {
    uint32_t layer = 0;
    uint32_t mask = 0;

    CollisionLayer(uint32_t entityLayer = 1, uint32_t collisionMask = 1) 
    : layer(entityLayer), mask(collisionMask) {}

    void addLayer(int i) { layer = layer | (1 << i); }
    void addCollision(int i) { mask = mask | (1 << i); }
    void removeLayer(int i) { layer = layer & ~(1 << i); }
    void removeCollision(int i) { mask = mask & ~(1 << i); }

    void addLayer(list<int> l) {for(auto i : l) addLayer(i);}
    void addCollision(list<int> l) {for(auto i : l) addCollision(i);}
    void removeLayer(list<int> l) {for(auto i : l) removeLayer(i);}
    void removeCollision(list<int> l) {for(auto i : l) removeCollision(i);}

    void setLayer(int i) { layer = (1 << i); }
    void setCollision(int i) { mask = (1 << i); }
    void setLayer(list<int> l) {layer = 1; addLayer(l);}
    void setCollision(list<int> l) {mask = 1; addCollision(l);}

    inline bool isOnLayer(int i) const { return (layer & (1 << i)) != 0; }
    inline bool canCollideWithLayer(int i) const { return (mask & (1 << i)) != 0; }
    inline bool canCollideWith(const CollisionLayer& other) const {
        return (mask & other.layer); //&& (other.mask & layer);
    }

    list<int> onLayers() {
        list<int> result;
        for(int i =0;i<32;i++) {
            if(layer & (1<<i)) result << 1;
        }
        return result;
    }
    list<int> collidesWith() {
        list<int> result;
        for(int i =0;i<32;i++) {
            if(mask & (1<<i)) result << 1;
        }
        return result;
    }
};

struct P_Prop {

};

class S_Object : virtual public Object {
protected:

public:
    Scene* scene = nullptr;

    S_Object() {}
    virtual void remove();

    virtual BoundingBox getWorldBounds() = 0;
    // virtual BoundingBox getScreenBounds(Camera&) = 0; 
    
    void saveBinary(const std::string& path) const;
    void loadBinary(const std::string& path);

    void saveBinary(std::ostream& out) const;
    void loadBinary(std::istream& in);

    virtual ~S_Object() = default;

};

}