#pragma once

#include <rendering/model.hpp>
#include <core/scriptable.hpp>
#include <util/util.hpp>
#include <rendering/single.hpp>
#include <duel/duel_object.hpp>

namespace Golden
{
    class Spell;
    class SpellForm : virtual public Single, virtual public Scriptable, virtual public D_Object
    {
    public:
        vec3 dir = vec3(0,0,0);
        float damage = 0.0f;
        float baseDamage = 0.0f;
        float speed = 0.0f;

        //Spell* spell;

        SpellForm(Model&& _model) {
            model = make<Model>(std::move(_model));
        }
        SpellForm() {}
        ~SpellForm() {}
        
        void addDamage(Tag element, float amt,g_ptr<D_Object> spell)
        {
            baseDamage = amt;
            addTag(element);
            addTag(Tag::Damaging);

            float d = amt;

            std::vector<g_ptr<Effect>> spellEffects = spell->
            filterEffects(getType::any,list<Tag>{element},spell->
                getEffects(getType::all,list<Stat>{Stat::Damage}));
            for(auto e : spellEffects)
            {
                switch(e->opp)
                {
                    case 1: d+=e->by; break;
                    case 2: d*=e->by; break;
                    case 3: d-=e->by; break;
                    case 4: d/=e->by; break;
                    default: break;
                }
            }
            damage = d;
        }

        void addMovment(float _speed,g_ptr<D_Object> spell)
        {
            addTag(Tag::Moving);
            addTag(Tag::Projectile);

            setPhysicsState(P_State::ACTIVE);

            std::vector<g_ptr<Effect>> spellEffects = spell->
            filterEffects(getType::any,list<Tag>{Tag::Moving},spell->
                getEffects(getType::all,list<Stat>{Stat::Speed}));
            float d = _speed;
            for(auto e : spellEffects)
            {
                switch(e->opp)
                {
                    case 1: d+=e->by; break;
                    case 2: d*=e->by; break;
                    case 3: d-=e->by; break;
                    case 4: d/=e->by; break;
                    default: break;
                }
            }
            speed = d;
        }

        void addMovment(float _speed,vec3 direction,g_ptr<D_Object> spell)
        {
            dir = direction;
            addMovment(_speed,spell);
        }

        SpellForm& moveAnim(const vec3& delta) override;

        SpellForm& scaleAnim(float x,float y,float z);

        void remove() override {Single::remove();}
    };
    
}