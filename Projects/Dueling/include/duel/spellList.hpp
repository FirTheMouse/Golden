#pragma once

#include <duel/agent.hpp>
#include <duel/spellForm.hpp>
#include <duel/spell.hpp>
#include <duel/duelManager.hpp>
#include <duel/types.hpp>

namespace Golden
{

using spellFactory = std::function<g_ptr<Spell>(g_ptr<Agent>, g_ptr<Duel>)>;
static map<std::string,spellFactory> spellMap;

struct SpellRegistrar {
    SpellRegistrar(sType type, spellFactory func) {
        spellMap.put(toString(type),func);
    }
};

g_ptr<Spell> createSpell(sType type, g_ptr<Agent> caster, g_ptr<Duel> duel);


static std::string MROOT = "../Projects/Dueling/models/";

void templateSpell(g_ptr<Agent> caster,g_ptr<Duel> duel);

g_ptr<Spell> templateProjectileSpell(g_ptr<Agent> caster,g_ptr<Duel> duel);

void collisonOne(ScriptContext& ctx, g_ptr<Spell> spell,g_ptr<SpellForm> form,g_ptr<Duel> duel);

g_ptr<Spell> mageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel);


g_ptr<Spell> homingMageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel);


g_ptr<Spell> arcingMageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel);


g_ptr<Spell> shield(g_ptr<Agent> caster,g_ptr<Duel> duel);

g_ptr<Spell> empowerArcana(g_ptr<Agent> caster,g_ptr<Duel> duel);

g_ptr<Spell> hurtBubble(g_ptr<Agent> caster,g_ptr<Duel> duel);

};