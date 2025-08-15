#include <duel/spellList.hpp>
#include<duel/turnManager.hpp>

namespace Golden
{

#define AUTO_REGISTER_SPELL(type, func) \
    static SpellRegistrar _reg_##func(type, func)

g_ptr<Spell> createSpell(sType type, g_ptr<Agent> caster, g_ptr<Duel> duel) {
    if (spellMap.hasKey(toString(type))) {
        return spellMap.get(toString(type))(caster, duel);
    } else {
        std::cerr << "Unknown spell type!" << std::endl;
        return nullptr;
    }
}

void templateSpell(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::None);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
        //Cast behaviour
        
        });

        spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
        for(auto p : spell->spellForms)
        {
            p->run("onTick",ctx);
        }
        
        });

        spell->addScript("onCollide",[caster,spell,duel](ScriptContext& ctx){
        std::string collideType = ctx.get<std::string>("collideType");
        g_ptr<SpellForm> from = ctx.get<g_ptr<SpellForm>>("from");
        vec3 at = ctx.get<vec3>("at");

        if(collideType=="spellForm")
        {
            g_ptr<SpellForm> with = ctx.get<g_ptr<SpellForm>>("with");
            g_ptr<Spell> withSpell= ctx.get<g_ptr<Spell>>("withSpell");
        }
        else if(collideType=="agent")
        {
            g_ptr<Agent> with = ctx.get<g_ptr<Agent>>("with");
        }

        
    });
}

g_ptr<Spell> templateProjectileSpell(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
std::string ROOT = MROOT;
auto spell = make<Spell>(sType::None);
spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
    for(int i=0;i<1;i++)
    {
        auto projectile = make<SpellForm>(Model(ROOT+"magebolt.glb"));
        duel->add(projectile);
        projectile->speed=randf(8.0f,10.0f);
        projectile->setPosition(caster->castPoint());
        spell->spellForms.push_back(projectile);

    projectile->addScript("onTick",[caster,spell,duel,projectile](ScriptContext& ctx){
        spell->moveProjectile(projectile);
        
    });

    projectile->addScript("onCollide",[caster,spell,duel,projectile](ScriptContext& ctx){
        std::string collideType = ctx.get<std::string>("collideType");
        vec3 at = ctx.get<vec3>("at");

        if(collideType=="spellForm")
        {
            g_ptr<SpellForm> with = ctx.get<g_ptr<SpellForm>>("with");
            g_ptr<Spell> withSpell= ctx.get<g_ptr<Spell>>("withSpell");
        }
        else if(collideType=="agent")
        {
            g_ptr<Agent> with = ctx.get<g_ptr<Agent>>("with");
        }
    });
    }
    });

    spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
    for(auto p : spell->spellForms)
    {
        p->run("onTick",ctx);
    }
    
    });

    spell->addScript("onCollide",[caster,spell,duel](ScriptContext& ctx){
    std::string collideType = ctx.get<std::string>("collideType");
    g_ptr<SpellForm> from = ctx.get<g_ptr<SpellForm>>("from");
    vec3 at = ctx.get<vec3>("at");

    if(collideType=="spellForm")
    {
        g_ptr<SpellForm> with = ctx.get<g_ptr<SpellForm>>("with");
        g_ptr<Spell> withSpell= ctx.get<g_ptr<Spell>>("withSpell");
    }
    else if(collideType=="agent")
    {
        g_ptr<Agent> with = ctx.get<g_ptr<Agent>>("with");
    }

    
});

return spell;
}

void collisonOne(ScriptContext& ctx, g_ptr<Spell> spell,g_ptr<SpellForm> form,g_ptr<Duel> duel)
{
    g_ptr<S_Object> with = ctx.get<g_ptr<S_Object>>("with");
    ctx.set<g_ptr<SpellForm>>("from",form);
    ctx.set<vec3>("at",form->getPosition());
    if(auto otherForm = g_dynamic_pointer_cast<SpellForm>(with))
    {
        //print(otherForm->spell->caster->name);
        if(!otherForm->hasTag(Tag::DeferCollison))
        {
            form->damage = form->damage-=std::abs((otherForm->damage)/4.0f);
            if(form->damage<=0.1f)
            {
               
            duel->registerEvent(spell,"pop",0,ctx);
            spell->removeLater(form);
            }
            else {
                float s = form->damage+1.0f;
                // form->scale(s,s,s);
                form->setScaleVelocity(vec3(-0.1,-0.1,-0.1));
            }
        }
        else 
        {
            // ScriptContext collison;
            // collison.set<g_ptr<SpellForm>>("from",with);
            // collison.set<g_ptr<SpellForm>>("with",from);
            // collison.set<vec3>("at",at);
            // collison.set<std::string>("collideType","spellForm");
            // collison.set<g_ptr<Spell>>("withSpell",spell);
            // with->run("onCollide",collison);
        }
    }
    else if(auto otherAgent = g_dynamic_pointer_cast<Agent>(with))
    {
        print("Agent!");
        if(otherAgent==spell->caster) return;
        print("Collide!");
        ScriptContext nctx;
        nctx.set<float>("damage",form->damage);
        //nctx.set<vec3>("push",mageBolt->dir.mult(mageBolt->speed*0.3f));
        otherAgent->run("onHit",nctx);
        form->run("applyEffect",ctx);
        duel->registerEvent(spell,"pop",0,ctx);
        //spell->run("pop",ctx);
        spell->removeLater(form);
    }
}

g_ptr<Spell> mageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::MageBolt);
    spell->addTag(Tag::Projectile);
    spell->addTag(Tag::Arcane);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
    for(int i=0;i<1;i++) {
            auto mageBolt = make<SpellForm>(Model(ROOT+"magebolt.glb"));
            duel->add(mageBolt);
            mageBolt->addDamage(Tag::Arcane,0.7f,caster);
            float s = mageBolt->damage+1.0f;
            mageBolt->addMovment(randf(18.0f,20.0f)/(s/2),caster);
            mageBolt->scale(s,s,s);
            mageBolt->setPosition(caster->castPoint());
            spell->addSpellForm(mageBolt);
        mageBolt->addScript("onTick",[caster,spell,duel,mageBolt](ScriptContext& ctx){
            if(!spell->target)
            {
                g_ptr<Agent> closest;
                float closestDist = INFINITY;
                for(auto n : duel->agents)
                {
                    if(n==spell->caster) continue;
                    float dist = n->getPosition().distance(mageBolt->getPosition());
                    if(dist<=closestDist){closest = n; closestDist = dist;}
                }
                if(closestDist>=100) spell->removeLater(mageBolt);
                if(closest)
                {
                float aProx = 8.0f/caster->accuracy;
                vec3 aPoint = vec3(closest->getWorldBounds().getCenter())
                .addZ(randf(-aProx,aProx))
                .addY(randf(-aProx/3,aProx/3));

                mageBolt->dir=mageBolt->getPosition()
                .direction(aPoint);
                mageBolt->faceTo(aPoint);
                spell->target = closest;
                }
            } 
            else if(mageBolt->distance(spell->target)>=100) spell->removeLater(mageBolt);

            spell->moveProjectile(mageBolt);
        });
        
        mageBolt->addScript("onCollide",[caster,spell,duel,mageBolt](ScriptContext& ctx){
            collisonOne(ctx,spell,mageBolt,duel);
        });
        ScriptContext effectCtx;
        effectCtx.set<g_ptr<Spell>>("spell",spell);
        effectCtx.set<g_ptr<SpellForm>>("form",mageBolt);
        effectCtx.set<g_ptr<Agent>>("caster",caster);
        effectCtx.set<g_ptr<Duel>>("duel",duel);
        // for(auto e : caster->getEffects(list<Tag>{Tag::Arcane,Tag::Damaging}))
        // {
        //     e.run("onCast",effectCtx);
        // }
    }  
    });

    spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
       for(auto p : spell->spellForms)
       {
        p->run("onTick",ctx);
       }
       
    });

    spell->addScript("pop",[caster,spell,duel,ROOT](ScriptContext& ctx){
        g_ptr<SpellForm> from = ctx.get<g_ptr<SpellForm>>("from");
        vec3 at = ctx.get<vec3>("at");
        auto pop = make<SpellForm>(Model(ROOT+"pop.glb"));
        duel->add(pop);
        pop->setPosition(at);
        spell->addSpellForm(pop);
        pop->addTag(Tag::NoCollison);
        // ScriptContext eventCtx;
        // eventCtx.set<g_ptr<SpellForm>>("remove",pop);
        // duel->registerEvent(spell,"removeSpellForm",0,eventCtx);
        float scale = from->damage+randf(1.0f,2.0f);
        pop->data.add<float>("size",scale);
        pop->scaleAnim(scale*5,scale*5,scale*5);
        spell->removeLater(pop);
        pop->addScript("onTick",[caster,spell,duel,pop](ScriptContext& ctx){
            float s = pop->data.get<float>("size");
            pop->scaleAnim(s,s,s);
            pop->data.set<float>("size",s+8.0f);
            
        });    
    });

    return spell;
}
AUTO_REGISTER_SPELL(sType::MageBolt, mageBolt);

g_ptr<Spell> fireBolt(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::FireBolt);
    spell->addTag(Tag::Projectile);
    spell->addTag(Tag::Fire);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
    for(int i=0;i<1;i++) {
            auto fireBolt = make<SpellForm>(Model(ROOT+"firebolt.glb"));
            duel->add(fireBolt);
            fireBolt->addDamage(Tag::Fire,0.7f,caster);
            float s = fireBolt->damage+1.0f;
            fireBolt->addMovment(randf(8.0f,10.0f)/(s/2),caster);
            fireBolt->scale(s,s,s);
            fireBolt->setPosition(caster->castPoint());
            spell->addSpellForm(fireBolt);
        fireBolt->addScript("onTick",[caster,spell,duel,fireBolt](ScriptContext& ctx){
            if(!spell->target)
            {
                g_ptr<Agent> closest;
                float closestDist = INFINITY;
                for(auto n : duel->agents)
                {
                    if(n==spell->caster) continue;
                    float dist = n->getPosition().distance(fireBolt->getPosition());
                    if(dist<=closestDist){closest = n; closestDist = dist;}
                }
                if(closestDist>=100) spell->removeLater(fireBolt);
                if(closest)
                {
                float aProx = 8.0f/caster->accuracy;
                vec3 aPoint = vec3(closest->getWorldBounds().getCenter())
                .addZ(randf(-aProx,aProx))
                .addY(randf(-aProx/3,aProx/3));

                fireBolt->dir=fireBolt->getPosition()
                .direction(aPoint);
                fireBolt->faceTo(aPoint);
                spell->target = closest;
                }
            } 
            else if(fireBolt->distance(spell->target)>=100) spell->removeLater(fireBolt);

            spell->moveProjectile(fireBolt);
        });
        
        fireBolt->addScript("onCollide",[caster,spell,duel,fireBolt](ScriptContext& ctx){
            collisonOne(ctx,spell,fireBolt,duel);
        });

        fireBolt->addScript("applyEffect",[caster,spell,duel](ScriptContext& ctx){
                g_ptr<Agent> with = ctx.get<g_ptr<Agent>>("with");
                auto e = make<Effect>();
                e->type = effect::Burning;
                e->duration = 3;
                e->addTag(Tag::Fire);
                e->addScript("onTick",[with](ScriptContext& ctx){
                    ScriptContext nctx;
                    nctx.set<float>("damage",0.3f);
                    with->run("onHit",nctx);
                });
                with->addEffect(e);
            });
        }  
    });

    

    spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
       for(auto p : spell->spellForms)
       {
        p->run("onTick",ctx);
       }
       
    });

    spell->addScript("pop",[caster,spell,duel,ROOT](ScriptContext& ctx){
        g_ptr<SpellForm> from = ctx.get<g_ptr<SpellForm>>("from");
        vec3 at = ctx.get<vec3>("at");
        auto pop = make<SpellForm>(Model(ROOT+"pop.glb"));
        duel->add(pop);
        pop->setPosition(at);
        spell->addSpellForm(pop);
        pop->addTag(Tag::NoCollison);
        float scale = from->damage+randf(1.0f,2.0f);
        pop->data.add<float>("size",scale);
        pop->scaleAnim(scale*5,scale*5,scale*5);
        spell->removeLater(pop);
        pop->addScript("onTick",[caster,spell,duel,pop](ScriptContext& ctx){
            float s = pop->data.get<float>("size");
            pop->scaleAnim(s,s,s);
            pop->data.set<float>("size",s+8.0f);
            
        });
        
    });

    return spell;
}
AUTO_REGISTER_SPELL(sType::FireBolt, fireBolt);

g_ptr<Spell> homingMageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = mageBolt(caster,duel);
    spell->type = sType::HomingMageBolt;
    spell->replaceScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
        for(int i=0;i<1;i++) {
                auto mageBolt = make<SpellForm>(Model(ROOT+"pop.glb"));
                duel->add(mageBolt);
                mageBolt->addDamage(Tag::Arcane,0.5f,spell);
                float s = mageBolt->damage+1.0f;
                mageBolt->addMovment(randf(4.0f,6.0f)/(s/2),caster);
                mageBolt->scale(s,s,s);
                mageBolt->setPosition(caster->castPoint());
                spell->spellForms.push_back(mageBolt);
            mageBolt->addScript("onTick",[caster,spell,duel,mageBolt](ScriptContext& ctx){
                g_ptr<Agent> closest;
                float closestDist = INFINITY;
                for(auto n : duel->agents)
                {
                    if(n==spell->caster) continue;
                    float dist = n->getPosition().distance(mageBolt->getPosition());
                    if(dist<=closestDist){closest = n; closestDist = dist;}
                }
                if(closest)
                {
                float aProx = std::max(std::pow(mageBolt->distance(closest) / 8.0f, 2.0f), 0.1f);
                if(mageBolt->speed>7.0f&&aProx<8.0f) mageBolt->speed=7.0f;
                vec3 aPoint = vec3(closest->getWorldBounds().getCenter())
                .addZ(randf(-aProx,aProx))
                .addY(randf(-aProx/3,aProx/3));

                mageBolt->dir=mageBolt->getPosition()
                .direction(aPoint);
                mageBolt->faceTo(aPoint);
                spell->target = closest;
                }
                spell->moveProjectile(mageBolt);
                if(closestDist>=100) spell->removeLater(mageBolt);
            });
            
            mageBolt->addScript("onCollide",[caster,spell,duel,mageBolt](ScriptContext& ctx){
                collisonOne(ctx,spell,mageBolt,duel);
            });
    
        }  });
    return spell;
}
AUTO_REGISTER_SPELL(sType::HomingMageBolt, homingMageBolt);

g_ptr<Spell> arcingMageBolt(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = mageBolt(caster,duel);
    spell->type = sType::ArcingMageBolt;
    spell->replaceScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
        for(int i=0;i<1;i++) {
                auto mageBolt = make<SpellForm>(Model(ROOT+"mageBolt.glb"));
                duel->add(mageBolt);
                mageBolt->addDamage(Tag::Arcane,0.5f,spell);
                float s = mageBolt->damage+1.0f;
                mageBolt->addMovment(randf(10.0f,12.0f)/(s/2),caster);
                mageBolt->scale(s,s,s);
                mageBolt->setPosition(caster->castPoint());
                spell->spellForms.push_back(mageBolt);
            mageBolt->addScript("onTick",[caster,spell,duel,mageBolt](ScriptContext& ctx){
                if(!spell->target)
                {
                    g_ptr<Agent> closest;
                    float closestDist = INFINITY;
                    for(auto n : duel->agents)
                    {
                        if(n==spell->caster) continue;
                        float dist = n->getPosition().distance(mageBolt->getPosition());
                        if(dist<=closestDist){closest = n; closestDist = dist;}
                    }
                    if(closestDist>=100) spell->removeLater(mageBolt);
                    if(closest)
                    {
                    float aProx = 20.0f;
                    float sn = randf(-1,1)>=0 ? -1 : 1;
                    vec3 aPoint = vec3(closest->getWorldBounds().getCenter())
                    .addZ(sn*aProx)
                    .addY(3.0f)
                    .addX(-(caster->distance(closest)/4));
                    mageBolt->data.set<vec3>("aPoint",aPoint);
                    mageBolt->dir=mageBolt->getPosition()
                    .direction(aPoint);
                    mageBolt->faceTo(aPoint);
                    spell->target = closest;
                    }
                }
                else
                {
                    vec3 aPoint = mageBolt->data.get<vec3>("aPoint");
                    if(mageBolt->getPosition().distance(aPoint)<14.0f)
                    {
                        vec3 center = vec3(spell->target->getWorldBounds().getCenter());
                        mageBolt->dir = mageBolt->dir=mageBolt->getPosition()
                        .direction(center);
                        mageBolt->faceTo(center);
                        mageBolt->speed = mageBolt->speed*1.5f;
                    }
                   if(mageBolt->distance(spell->target)>=100) spell->removeLater(mageBolt);
                }
                
                spell->moveProjectile(mageBolt);
            });
            
            mageBolt->addScript("onCollide",[caster,spell,duel,mageBolt](ScriptContext& ctx){
               collisonOne(ctx,spell,mageBolt,duel);
            });
    
        }  });
    return spell;
}
AUTO_REGISTER_SPELL(sType::ArcingMageBolt, arcingMageBolt);

g_ptr<Spell> shield(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::Shield);
    spell->addTag(Tag::Abjuration);
    spell->addTag(Tag::Arcane);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
            auto shield = make<SpellForm>(Model(ROOT+"shield.glb"));
            duel->add(shield);
            float s = caster->model->localBounds.max.y/1.3f;
            shield->scale(s,s,s);
            //shield->addTag(Tag::PassiveCollison);
            //shield->addTag(Tag::DeferCollison);
            //shield->addTag(Tag::SingleCollide);
            shield->setPosition(caster->castPoint());
            // auto marker = make<Single>(Model(makeBox(
            //     shield->getWorldBounds().max.x,
            //     shield->getWorldBounds().max.y,
            //     shield->getWorldBounds().max.z,
            //     glm::vec4(1,0,0,1)
            // )));
            // duel->add(marker);
            // marker->setPosition(vec3(shield->getWorldBounds().getCenter()));

            shield->faceTo(vec3(0,0,0));
            spell->spellForms.push_back(shield);
        shield->addScript("onCollide",[caster,spell,duel,shield](ScriptContext& ctx){
            g_ptr<S_Object> with = ctx.get<g_ptr<S_Object>>("with");
            ctx.set<g_ptr<SpellForm>>("from",shield);
            ctx.set<vec3>("at",shield->getPosition());
            if(auto otherForm = g_dynamic_pointer_cast<SpellForm>(with))
            {
                    ScriptContext nctx = ScriptContext();
                    otherForm->run("onHit",nctx);
                    shield->run("shatter",ctx);
                    spell->removeLater(shield);
                    bool invert = true;
                    bool remove = false;
                    if(invert)
                    {
                        otherForm->dir = otherForm->dir.mult(-1);
                        otherForm->faceTo(otherForm->dir.mult(otherForm->speed));
                        // withSpell->target = withSpell->caster;
                        // withSpell->caster = caster;
                        otherForm->speed = otherForm->speed+1.0f;
                        otherForm->damage = otherForm->damage+1.0f;
                    }
        
                    if(remove)
                    {
                        //otherSpell->removeLater(otherForm);
                    }
            }
            else if(auto otherAgent = g_dynamic_pointer_cast<Agent>(with))
            {
                
            }
        });

        shield->addScript("onTick",[caster,spell,duel,shield](ScriptContext& ctx){
            g_ptr<SpellForm> closest;
            float closestDist = INFINITY;
            for(auto s : duel->spells)
            {
                if(s==spell) continue;
                for(auto n : s->spellForms)
                {
                if(!s->hasTag(Tag::Damaging)) continue;

                float dist = n->getPosition().distance(shield->getPosition());
                if(dist<=closestDist&&(n->getPosition().x()-shield->getPosition().x())<3.0f) continue;
                if(dist<=closestDist){closest = n; closestDist = dist;}
                }
            }
            if(closest)
            {
            shield->dir=shield->getPosition()
            .direction(closest->getPosition());
            shield->faceTo(closest->getPosition());
            }
        });

        shield->addScript("shatter",[caster,spell,duel,shield,ROOT](ScriptContext& ctx){
            float s = 4.0f;
            if(ctx.has("with")){
            g_ptr<S_Object> with = ctx.get<g_ptr<S_Object>>("with");
            if(auto otherForm = g_dynamic_pointer_cast<SpellForm>(with))
                s+=otherForm->damage;
            }
            vec3 at = ctx.get<vec3>("at");
            int numShards = 12;
            float radius = 1.0f;
            for(int i=0;i<numShards;i++)
            {
                float angle = (float(i) / numShards) * glm::two_pi<float>(); 
                auto shard = make<SpellForm>(Model(ROOT+"shard.glb"));
                duel->add(shard);
                spell->addSpellForm(shard);
                shard->addTag(Tag::NoCollison);
                shard->setPosition(at);
                shard->faceTo(at);
                shard->dir = vec3(0.0f, cos(angle), sin(angle));
                shard->speed = s;
                ScriptContext nctx;
                nctx.set<g_ptr<SpellForm>>("remove",shard);
                duel->registerEvent(spell,"removeLater",1,nctx);

                shard->addScript("onTick",[caster,spell,duel,shard](ScriptContext& ctx){
                    spell->moveProjectile(shard);
                    //shard->speed = 50.0f;
                    //shard->scaleAnim(4.2f,4.2f,4.2f);
                    //shard->faceTo(vec3(0,randf(-20.0f,20.0f),0));
                    
                });
            }
            
        });

        
     });

    spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
       for(auto p : spell->spellForms)
       {
            p->run("onTick",ctx);
       }
       
    });

    return spell;
}
AUTO_REGISTER_SPELL(sType::Shield, shield);

g_ptr<Spell> empowerArcana(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::EmpowerArcana);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
            auto projectile = make<SpellForm>(Model(ROOT+"magepop.glb"));
            duel->add(projectile);
            projectile->setPosition(caster->castPoint());
            spell->addSpellForm(projectile);
            projectile->addTag(Tag::NoCollison);
            ScriptContext nctx;
            nctx.set<g_ptr<SpellForm>>("remove",projectile);
            duel->registerEvent(spell,"removeLater",2,nctx);
           // duel->registerEvent(spell,"unEmpower",2,nctx);
            projectile->data.set<bool>("scaling",false);
            auto e = make<Effect>();
            e->type = effect::ArcaneEmpower;
            e->mods = Stat::Damage;
            e->by = 2.0f;
            e->opp = 2;
            e->duration = 1;
            e->addTag(Tag::Arcane);
            e->addTag(Tag::Damaging);

            auto e2 = make<Effect>();
            e2->type = effect::ArcaneEmpower;
            e2->mods = Stat::Speed;
            e2->by = 2.0f;
            e2->opp = 2;
            e2->duration = 1;
            e2->addTag(Tag::Arcane);
            e2->addTag(Tag::Moving);
            caster->addEffect(e2);
        projectile->addScript("onTick",[caster,spell,duel,projectile](ScriptContext& ctx){
            float a = 8;
            if(projectile->data.get<bool>("scaling")) a = 0.1f;
            projectile->scaleAnim(a,a,a);
            projectile->data.set<bool>("scaling",true);
            
        });
    });

     spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
        for(auto p : spell->spellForms)
        {
         p->run("onTick",ctx);
        }
        
     });
    return spell;
}
AUTO_REGISTER_SPELL(sType::EmpowerArcana, empowerArcana);

g_ptr<Spell> hurtBubble(g_ptr<Agent> caster,g_ptr<Duel> duel)
{
    std::string ROOT = MROOT;
    auto spell = make<Spell>(sType::HurtBubble);
    spell->addScript("onCast",[caster,spell,duel,ROOT](ScriptContext& ctx){
        for(int i=0;i<1;i++)
        {
            float d = ctx.get<float>("damage")+1.0f;
            auto projectile = make<SpellForm>(Model(ROOT+"hurt.glb"));
            duel->add(projectile);
            projectile->addTag(Tag::NoCollison);
            projectile->addMovment(randf(2.0f,3.0f)/(d/2),vec3(0,1,0),caster);
            float mX = caster->model->localBounds.max.x/2;
            float mZ = caster->model->localBounds.max.z/2;
            projectile->setPosition(caster->getPosition().addY(caster->getWorldBounds().max.y)
            .addX(randf(-mX,mX))
            .addZ(randf(-mZ,mZ)));
            spell->addSpellForm(projectile);
            int duration = 4;
            projectile->data.set<float>("scale",d/4); //caster->hp/30
            projectile->data.set<float>("scaleFactor",0.0f); //<- Could add a bit of animation with this
            ScriptContext removeCtx;
            removeCtx.set<g_ptr<SpellForm>>("remove",projectile);
            duel->registerEvent(spell,"removeLater",duration-1,removeCtx);

        projectile->addScript("onTick",[caster,spell,duel,projectile](ScriptContext& ctx){
            float s = projectile->data.get<float>("scale");
            float f = projectile->data.get<float>("scaleFactor");
            projectile->data.set<float>("scale",s-=f);
            if((s-=f)>0) projectile->scale(s-=f);

            spell->moveProjectile(projectile);
        });
     }});

     spell->addScript("onTick",[caster,spell,duel](ScriptContext& ctx){
        for(auto p : spell->spellForms) { p->run("onTick",ctx); }
     });        
    
    return spell;
}
AUTO_REGISTER_SPELL(sType::HurtBubble, hurtBubble);

}