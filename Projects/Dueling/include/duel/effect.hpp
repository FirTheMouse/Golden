#pragma once
#include <duel/types.hpp>
#include <util/util.hpp>
#include <core/object.hpp>
#include <vector>
#include <core/scriptable.hpp>

namespace Golden
{

#define FOREACH_EFFECT_TYPE(TYPE) \
TYPE(None) \
TYPE(Burning)   \
TYPE(Shield)    \
TYPE(HomingMageBolt) \
TYPE(ArcaneEmpower)

#define GENERATE_ENUM(name) name,
#define GENERATE_STRING(name) #name,

enum class effect {
    FOREACH_EFFECT_TYPE(GENERATE_ENUM)
};

static const char* effectTypeNames[] = {
    FOREACH_EFFECT_TYPE(GENERATE_STRING)
};

inline const char* toString(effect type) {
    return effectTypeNames[static_cast<int>(type)];
}

class Effect : virtual public Object, virtual public Scriptable {
private:

public:
    Effect() {}
    ~Effect() {}

    std::vector<Tag> tags;
    int started = 0;
    int ticksActive = 0;
    int duration = 0;
    effect type = effect::None;
    Stat mods = Stat::None;
    float by = 0.0f;
    int opp = 0;
    //0 = No opperation
    //1 = Addition
    //2 = Multiplication
    //3 = Subtraction
    //4 = Division

    bool hasTag(Tag tag)
    {
        for(auto t : tags)
        if(t==tag) return true;
        return false;
    }

    void addTag(Tag tag)
    {
        tags.push_back(tag);
    }
};


// void effectBurning()
// {
//     Effect e;
//     e.type = effect::Burning;
// }

// void effectArcaneEmpower()
// {
//     Effect e;
//     e.type = effect::ArcaneEmpower;
//     e.addTag(Tag::Arcane);
// }

}