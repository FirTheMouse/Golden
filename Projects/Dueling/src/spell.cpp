#include <duel/spell.hpp>
#include <duel/turnManager.hpp>


namespace Golden
{

 void Spell::removeSpellForm(g_ptr<SpellForm> toRemove)
        {
           // if(toRemove->ANIMID>0) GDM::get_T().removeAnimation(toRemove->ANIMID);
            spellForms.erase(std::remove(spellForms.begin(), 
            spellForms.end(), toRemove),spellForms.end());
            toRemove->remove();
        }

}