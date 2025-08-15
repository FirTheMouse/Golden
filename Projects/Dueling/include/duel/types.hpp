#pragma once
#include<string>

namespace Golden
{
    #define FOREACH_S_TYPE(TYPE) \
    TYPE(None) \
    TYPE(HurtBubble)   \
    TYPE(MageBolt)   \
    TYPE(FireBolt)   \
    TYPE(Shield)    \
    TYPE(HomingMageBolt) \
    TYPE(ArcingMageBolt) \
    TYPE(EmpowerArcana)

    #define GENERATE_ENUM(name) name,
    #define GENERATE_STRING(name) #name,

    enum class sType {
        FOREACH_S_TYPE(GENERATE_ENUM)
    };

    static const char* sTypeNames[] = {
        FOREACH_S_TYPE(GENERATE_STRING)
    };

    inline const char* toString(sType type) {
        return sTypeNames[static_cast<int>(type)];
    }

    //DeferCollison -> When collided, activates unique logic, a request for the collider to defer to the collided
    //PassiveCollison -> Does not actively scan or check collisons, relies upon other objects to collide with it
    //NoCollison -> Simply does not register any collison
    //SingleCollide -> Only allows one collison 

    //Projectile -> Only collides once, with an agent, will tag NoCollison after colliding an agent


    #define FOREACH_TAG_TYPE(TYPE) \
    TYPE(None) \
    TYPE(DeferCollison) \
    TYPE(NoCollison) \
    TYPE(PassiveCollison) \
    TYPE(SingleCollide) \
    TYPE(Abjuration)   \
    TYPE(Projectile)    \
    TYPE(Adaptive)    \
    TYPE(Fire)   \
    TYPE(Ice)    \
    TYPE(Lightning) \
    TYPE(Arcane) \
    TYPE(Damaging) \
    TYPE(Moving) \
    TYPE(Effect) \
    TYPE(Duration) 

    #define GENERATE_ENUM(name) name,
    #define GENERATE_STRING(name) #name,

    enum class Tag {
        FOREACH_TAG_TYPE(GENERATE_ENUM)
    };

    static const char* tagTypeNames[] = {
        FOREACH_TAG_TYPE(GENERATE_STRING)
    };

    inline const char* toString(Tag type) {
        return tagTypeNames[static_cast<int>(type)];
    }


    #define FOREACH_E_TYPE(TYPE) \
    TYPE(None) \
    TYPE(Fire)   \
    TYPE(Ice)    \
    TYPE(Lightning) \
    TYPE(Arcane)

    enum class eType {
        FOREACH_E_TYPE(GENERATE_ENUM)
    };

    static const char* eTypeNames[] = {
        FOREACH_E_TYPE(GENERATE_STRING)
    };

    inline const char* toString(eType type) {
        return eTypeNames[static_cast<int>(type)];
    }

    #define FOREACH_STAT_TYPE(TYPE) \
    TYPE(None) \
    TYPE(HP)   \
    TYPE(Damage)    \
    TYPE(Speed) \
    TYPE(Brz) \
    TYPE(Per)

    enum class Stat {
        FOREACH_STAT_TYPE(GENERATE_ENUM)
    };

    static const char* StatNames[] = {
        FOREACH_STAT_TYPE(GENERATE_STRING)
    };

    inline const char* toString(Stat type) {
        return StatNames[static_cast<int>(type)];
    }
}