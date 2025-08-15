#pragma once

#include <rendering/model.hpp>
#include <core/scriptable.hpp>
#include <util/util.hpp>
#include <rendering/single.hpp>
#include <duel/spellForm.hpp>
#include <duel/duel_object.hpp>



namespace Golden
{
    class Agent;
    class Spell : virtual public Single, virtual public Scriptable, virtual public D_Object
    {
    public:
        using Single::Single;

        g_ptr<Agent> caster;
        g_ptr<Agent> target;
        sType type = sType::None;
        eType e_type = eType::None;
        bool hasSpellForm = true;
        bool marked = false;

        keylist<int,ScriptContext> eventBuffer;

        std::vector<g_ptr<SpellForm>> spellForms;
        std::vector<g_ptr<SpellForm>> spellFormsToRemove;
        Spell() {}
        Spell(sType _type) {type = _type;
        addScript("removeLater",[this](ScriptContext& ctx){
            if(ctx.has("remove"))
            {
            g_ptr<SpellForm> toRemove = ctx.get<g_ptr<SpellForm>>("remove");
            if(toRemove)
                {
                removeLater(toRemove);
                }
            }
        });
        }
        ~Spell() {}

        

        void stepEvents(int round)
        {
            if(eventBuffer.length()>0)
            for(int i = eventBuffer.length()-1;i>=0;i--)
            {
                if(eventBuffer[i].key<=round)
                {
                    run(eventBuffer[i].value.get<std::string>("eventType"),
                        eventBuffer[i].value);
                    eventBuffer.removeAt(i);
                }
            }
        }

        void moveProjectile(g_ptr<SpellForm> projectile)
        {
            //projectile->moveAnim(projectile->dir.mult(projectile->speed));
           projectile->setLinearVelocity(projectile->dir * projectile->speed);
        }

        void removeLater(g_ptr<SpellForm> toRemove)
        {
            spellFormsToRemove.push_back(toRemove);
        }
        void addSpellForm(g_ptr<SpellForm> form)
        {
            spellForms.push_back(form);
            //form->spell = this;
        }
        
        void removeSpellForm(g_ptr<SpellForm> toRemove);

        void remove() override {Single::remove();}
    };
}