#pragma once
#include <duel/types.hpp>
#include <util/util.hpp>
#include <core/object.hpp>
#include <vector>
#include <duel/effect.hpp>

namespace Golden
{

enum class getType {
    all = 0,
    any = 1
};

class D_Object : virtual public Object {
private:
    std::vector<Tag> tags;
    std::vector<g_ptr<Effect>> effects;
    map<Tag,Effect> effectMap;
public:
    bool valid = true;

    virtual void addTag(Tag tag)
        {
            tags.push_back(tag);
        }

    virtual bool hasTag(Tag tag)
        {
            for(auto t : tags)
            if(t==tag) return true;
            return false;
        }

    virtual void removeTag(Tag tag)
        {
            tags.erase(tags.begin(),std::remove_if(tags.begin(),tags.end(),[tag](Tag t){
                return t==tag;
            }));
        }

    virtual void addEffect(g_ptr<Effect> effect)
        {
            effects.push_back(effect);
        }

        //Unsafe, needs to be modified and fixed in the future to delete the only real effect
    virtual void removeEffect(g_ptr<Effect> effect)
        {
            effects.erase(effects.begin(),std::remove_if(effects.begin(),effects.end(),[effect](g_ptr<Effect> e){
                return (e->type==effect->type&&e->ticksActive==e->ticksActive);
            }));
        }

    virtual void stepEffects(ScriptContext& ctx)
    {
        std::vector<g_ptr<Effect>> currentEffects = effects;

        for (const auto& e : currentEffects) {
            //print("looping ",toString(e->type));
            if (e->ticksActive >= e->duration) {
                e->run("onExpire",ctx);
                effects.erase(std::remove(effects.begin(), effects.end(), e), effects.end());
            }
            else
            {
                e->run("onTick",ctx);
            }
        }

        for(auto e : effects)
        {
            e->ticksActive = e->ticksActive+1;
        }
    }

    std::vector<g_ptr<Effect>> getEffects(getType getting,list<Tag> check)
        {
            std::vector<g_ptr<Effect>>  toReturn;
            for(g_ptr<Effect> e : effects)
            {
                bool hasAllTags = true;
                for (Tag& required : check) {
                    if (!e->hasTag(required)) {
                        hasAllTags = false;
                        if(getting == getType::all) break;
                    }
                    else if(getting == getType::any)
                    {
                        toReturn.push_back(e);
                        break;
                    }
                }
                if (hasAllTags) toReturn.push_back(e);
            }
            return toReturn;
        }

    std::vector<g_ptr<Effect>> getEffects(getType getting,list<Stat> check)
        {
            std::vector<g_ptr<Effect>>  toReturn;
            for(g_ptr<Effect> e : effects)
            {
                bool hasAllTags = true;
                for (Stat& required : check) {
                    if (e->mods != required) {
                        hasAllTags = false;
                        if(getting == getType::all) break;
                    }
                    else if(getting == getType::any)
                    {
                        toReturn.push_back(e);
                        break;
                    }
                }
                if (hasAllTags) toReturn.push_back(e);
            }
            return toReturn;
        }

    std::vector<g_ptr<Effect>> filterEffects(getType getting,list<Tag> check,std::vector<g_ptr<Effect>> toFilter)
        {
            std::vector<g_ptr<Effect>>  toReturn;
            for(g_ptr<Effect> e : toFilter)
            {
                bool hasAllTags = true;
                for (Tag& required : check) {
                    if (!e->hasTag(required)) {
                        hasAllTags = false;
                        if(getting == getType::all) break;
                    }
                    else if(getting == getType::any)
                    {
                        toReturn.push_back(e);
                        hasAllTags=false;
                        break;
                    }
                }
                if (hasAllTags) toReturn.push_back(e);
            }
            return toReturn;
        }
};

}