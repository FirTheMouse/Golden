#pragma once

#include <rendering/model.hpp>
#include <core/scriptable.hpp>
#include <util/util.hpp>
#include <rendering/single.hpp>
#include <duel/spell.hpp>
#include <duel/duel_object.hpp>

namespace Golden
{
    class Agent : virtual public Single, virtual public Scriptable, virtual public D_Object
    {
    private:
    public:
        float hp = 100;
        std::string name;
        int setSize = 3;
        sType spellList[3];
        float damageMod = 1.0f;
        float accuracy = 1.0f;
        map<eType,float> eMods;
        vec3 castPos;

        std::vector<g_ptr<Spell>> spells;

        Agent(std::string _name,Model&& _model) {
            name = _name;
            //setModel(_model);
            model = make<Model>(std::move(_model));
            vec3 pos = vec3(0,0,0);
            pos = pos
            .addX(model->localBounds.max.x/0.4f)
            .addY(model->localBounds.max.y/1.6f)
            .addZ(1.2f);
            castPos = pos;
        }
        ~Agent() {}

        vec3 castPoint() {return castPos+getPosition();}

        int numCasts(sType type)
        {
            int counter = 0;
            for(auto s : spells)
            {
                if(s->type==type)
                counter++;
            }
            return counter;
        }


    };
}