#pragma once

#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <rendering/model.hpp>
#include <rendering/single.hpp>
#include <core/scriptable.hpp>
#include <util/meshBuilder.hpp>
#include <fstream>
#include <string>
#include <queue>
#include <duel/spell.hpp>
#include <duel/agent.hpp>
#include <duel/gui_object.hpp>

namespace Golden
{

struct gui_event
{
    float start;
    float end;
    std::function<void()> event;

    gui_event(float _start, float _end, std::function<void()> _event) :
    start(_start), end(_end), event(_event) {}
};

class Duel : virtual public Scene
{
public:
    using Scene::Scene;

    int round = 0;
    int turn = 0;
    int action = 0;
    int tick = 0;

    float sceneTime = 0.0f;

    std::vector<g_ptr<Agent>> agents;
    std::vector<g_ptr<Spell>> spells;


    void tickSet()
    {
        //0:0:0 - 0:0:1 - 0:0:2 - 0:0:3
        //0:0:1 - 0:0:2 - 0:0:3 - 0:1:0
        //print("Round: ",round," Turn: ",turn, " Action: ",action);
        ScriptContext ctx;
        g_ptr<Agent> agent = agents.at(turn);
        if(action>=agent->setSize)
        {
            agent->run("onTurnEnd",ctx);
            turn++;
            action=0;
            if(turn>=agents.size())
            {
                round++;
                turn = 0;
                for(auto n : agents) n->run("onRoundStart",ctx);
                for(auto n : spells) n->run("onRoundStart",ctx);
            }
            agent = agents.at(turn);
            agent->run("onTurnStart",ctx);
        }
        agent->run("onAction",ctx);
        tickAll(ctx);
        action++;
    }

    struct colllisonRequest
    {
        g_ptr<SpellForm> form;
        ScriptContext ctx;
        colllisonRequest( g_ptr<SpellForm> _form,ScriptContext& _ctx) :
        form(_form), ctx(_ctx) {}
        ~colllisonRequest(){}

        void collide()
        {form->run("onCollide",ctx);}
    };


    void tickAll(ScriptContext& ctx)
    {
        tick++;
        std::vector<g_ptr<Spell>> toRemove;
        // std::deque<colllisonRequest> toCollide = checkCollisons();
        // for (auto& c : toCollide) {
        //     c.collide();
        // }
        // toCollide.clear(); 

        for(auto a : agents) {
            a->run("onTick",ctx);
            ScriptContext effectCtx;
            effectCtx.set<g_ptr<D_Object>>("agent",a);
            a->stepEffects(effectCtx);
        }
        for(auto s : spells) {
            if(s->spellFormsToRemove.size()>0) {
            for(int i = s->spellFormsToRemove.size()-1;i>=0;i--)
            {
                g_ptr<SpellForm> form = s->spellFormsToRemove[i];
                s->removeSpellForm(form);
            }
            }
           
            if(s->spellForms.empty()&&s->hasSpellForm)
            {
                if(s->marked)
                {
                toRemove.push_back(s);
                continue;
                }
                else {s->marked=true;}
            }
            else if(!s->spellForms.empty()&&s->marked) {s->marked = false;}
            s->stepEvents(round);
            s->run("onTick",ctx);
        }

        for(auto& r : toRemove)
        {
            removeSpell(r);
        }

    }

    void registerEvent(g_ptr<Spell> spell,const std::string& type, int time,ScriptContext& eventCtx)
    {
        eventCtx.set<std::string>("eventType",type);
        spell->eventBuffer.put(round+time,eventCtx);
    }

    //Yes, I know this is inefficent, and yes, I know I have a grid system for this
    //But it's a small game, so ineffiency is fine
    std::deque<colllisonRequest>  checkCollisons()
    {
        std::deque<colllisonRequest> toCollide;
        for(auto s : spells)
        {
            if(s->hasTag(Tag::NoCollison)) continue;
            for(auto p : s->spellForms)
            {
                if(p->hasTag(Tag::PassiveCollison)) continue;
               
                for(auto s2 : spells)
                {
                    if(s2==s) continue;
                    if(s2->caster==s->caster) continue;
                    for(auto p2 : s2->spellForms)
                    {
                        // if(!p2->hasTag(Tag::Solid)) continue;
                        // if(p->getWorldBounds().intersects(p2->getWorldBounds()))
                        // {
                        //     ScriptContext collison;
                        //     collison.set<g_ptr<SpellForm>>("from",p);
                        //     collison.set<g_ptr<SpellForm>>("with",p2);
                        //     vec3 at;
                        //     vec3 dir = p->direction(p2);
                        //     int steps = p->distance(p2)/0.3f;
                        //     for(int i=0;i<steps;i++)
                        //     {
                        //         at = p->getPosition()+(dir.mult(0.3f*i));
                        //         if(p2->getWorldBounds().contains(at.toGlm())) 
                        //         {break;}
                        //     }
                        //     collison.set<vec3>("at",at);
                        //     collison.set<g_ptr<Spell>>("withSpell",s2);
                        //     collison.set<std::string>("collideType","spellForm");
                        //     toCollide.emplace_back(p, collison);
                        //     goto next_p;
                        // }

                        if(p2->hasTag(Tag::NoCollison)) continue;
                        if(p->speed<=0.1)
                        {
                            if(p->getWorldBounds().intersects(p2->getWorldBounds()))
                            {
                                ScriptContext collison;
                                collison.set<g_ptr<SpellForm>>("from",p);
                                collison.set<g_ptr<SpellForm>>("with",p2);
                                vec3 at;
                                vec3 dir = p->direction(p2);
                                int steps = p->distance(p2)/0.3f;
                                for(int i=0;i<steps;i++)
                                {
                                    at = p->getPosition()+(dir.mult(0.3f*i));
                                    if(p2->getWorldBounds().contains(at.toGlm())) 
                                    {break;}
                                }
                                collison.set<vec3>("at",at);
                                collison.set<g_ptr<Spell>>("withSpell",s2);
                                collison.set<std::string>("collideType","spellForm");
                                toCollide.emplace_back(p, collison);
                                if(p->hasTag(Tag::SingleCollide)&&!p->hasTag(Tag::NoCollison)) 
                                    p->addTag(Tag::NoCollison);
                                if(p2->hasTag(Tag::SingleCollide)&&!p2->hasTag(Tag::NoCollison)) 
                                    p2->addTag(Tag::NoCollison);
                                goto next_p;
                            }
                        }
                        else
                        {
                            bool collided = false;
                            vec3 at;
                            vec3 dir = p->direction(p2);
                            int steps = (p->speed)/0.2f;
                            for(int i=0;i<steps;i++)
                            {
                                at = p->getPosition()+(dir.mult(0.1f*i));
                                if(p2->getWorldBounds().contains(at.toGlm())) 
                                {collided = true; break;}
                            }
                            if(collided)
                            {
                            ScriptContext collison;
                            collison.set<g_ptr<SpellForm>>("from",p);
                            collison.set<g_ptr<SpellForm>>("with",p2);
                            collison.set<vec3>("at",at);
                            collison.set<std::string>("collideType","spellForm");
                            collison.set<g_ptr<Spell>>("withSpell",s2);
                            toCollide.emplace_back(p, collison);
                            if(p->hasTag(Tag::SingleCollide)&&!p->hasTag(Tag::NoCollison)) 
                                p->addTag(Tag::NoCollison);
                            if(p2->hasTag(Tag::SingleCollide)&&!p2->hasTag(Tag::NoCollison)) 
                                p2->addTag(Tag::NoCollison);
                            goto next_p;
                            }
                        }
                    }
                }

                for(auto a : agents)
                {
                    if(a==s->caster) continue;
                    if(p->speed>8.0f)
                    {
                    bool collided = false;
                    vec3 at;
                    vec3 dir = p->direction(a);
                    int steps = (p->speed/1.2f)/0.3f;
                    for(int i=0;i<steps;i++)
                    {
                        at = p->getPosition()+(dir.mult(0.3f*i));
                        if(a->getWorldBounds().contains(at.toGlm())) 
                        {collided = true; break;}
                    }
                    if(collided)
                    {
                    ScriptContext collison;
                    collison.set<g_ptr<SpellForm>>("from",p);
                    collison.set<g_ptr<Agent>>("with",a);
                    collison.set<vec3>("at",at);
                    collison.set<std::string>("collideType","agent");
                    toCollide.emplace_back(p, collison);
                    if(p->hasTag(Tag::Projectile)) p->addTag(Tag::NoCollison);
                    goto next_p;
                    }
                    }
                    else if(p->getWorldBounds().intersects(a->getWorldBounds()))
                    {
                        ScriptContext collison;
                        collison.set<g_ptr<SpellForm>>("from",p);
                        collison.set<g_ptr<Agent>>("with",a);
                        vec3 at;
                        vec3 dir = p->direction(a);
                        int steps = p->distance(a)/0.3f;
                        for(int i=0;i<steps;i++)
                        {
                            at = p->getPosition()+(dir.mult(0.3f*i));
                            if(a->getWorldBounds().contains(at.toGlm())) 
                            {break;}
                        }
                        collison.set<vec3>("at",at);
                        collison.set<std::string>("collideType","agent");
                        toCollide.emplace_back(p, collison);
                        if(p->hasTag(Tag::Projectile)) p->addTag(Tag::NoCollison);
                        goto next_p;
                    }

                    // if(a==s->caster) continue;
                    // bool collided = false;
                    // vec3 at;
                    // vec3 dir = p->direction(a);
                    // int steps = (p->speed/1.2f)/0.3f;
                    // for(int i=0;i<steps;i++)
                    // {
                    //     at = p->getPosition()+(dir.mult(0.3f*i));
                    //     if(a->getWorldBounds().contains(at.toGlm())) 
                    //     {collided = true; break;}
                    // }
                    // if(collided)
                    // {
                    // ScriptContext collison;
                    // collison.set<g_ptr<SpellForm>>("from",p);
                    // collison.set<g_ptr<Agent>>("with",a);
                    // collison.set<vec3>("at",at);
                    // collison.set<std::string>("collideType","agent");
                    // toCollide.emplace_back(p, collison);
                    // goto next_p;
                    // }
                }

                next_p:;
            }
        }
        return toCollide;
    }

    void addAgent(g_ptr<Agent> agent)
    {
        agents.push_back(agent);
    }

    void castSpell(g_ptr<Agent> caster,g_ptr<Spell> spell)
    {
        spell->caster = caster;
        spells.push_back(spell);
        caster->spells.push_back(spell);
        ScriptContext castContext; 
        spell->run("onCast",castContext);
    }

    void castSpell(g_ptr<Agent> caster,g_ptr<Spell> spell,ScriptContext& ctx)
    {
        spell->caster = caster;
        spells.push_back(spell);
        caster->spells.push_back(spell);
        spell->run("onCast",ctx);
    }

    void removeSpell(g_ptr<Spell> toRemove)
    {
        if(toRemove->spellForms.size()>0)
        for(int i = toRemove->spellForms.size()-1;i>=0;i--)
        {
            toRemove->removeSpellForm(toRemove->spellForms[i]);
        }
        std::vector<g_ptr<Spell>>& casterSpells = toRemove->caster->spells;
        //print("Removing ",toRemove->caster->name," ",toString(toRemove->type));
        casterSpells.erase(std::remove(casterSpells.begin(), 
        casterSpells.end(), toRemove),casterSpells.end());

       // print(toRemove->caster->name," ",casterSpells.size()," ",spells.size());
        // for(auto s : casterSpells)
        // {
        //     print("   ",toString(s->type));
        // }
        

        spells.erase(std::remove(spells.begin(), 
        spells.end(), toRemove),spells.end());
    }


    std::vector<g_ptr<G_Object>> selectable;

    bool rayIntersectsAABB(const glm::vec3& origin, const glm::vec3& dir, const BoundingBox& box, float& outT) {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i) {
            if (fabs(dir[i]) < 1e-6f) {
                if (origin[i] < box.min[i] || origin[i] > box.max[i]) return false;
            } else {
                float invD = 1.0f / dir[i];
                float t0 = (box.min[i] - origin[i]) * invD;
                float t1 = (box.max[i] - origin[i]) * invD;
                if (t0 > t1) std::swap(t0, t1);
                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);
                if (tMax < tMin) return false;
            }
        }

    outT = tMin;
    return true;
    }

    g_ptr<G_Object>  nearestElement() {
        int windowWidth = window.width;
        int windowHeight = window.height;
        double xpos, ypos;
        glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);

        // Flip y for OpenGL
        float glY = windowHeight - float(ypos);

        // Get view/proj
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();
        glm::ivec4 viewport(0, 0, windowWidth, windowHeight);

        // Unproject near/far points
        glm::vec3 winNear(float(xpos), glY, 0.0f);   // depth = 0
        glm::vec3 winFar(float(xpos), glY, 1.0f);    // depth = 1
        glm::vec3 rayOrigin = glm::unProject(winNear, view, projection, viewport);
        glm::vec3 rayEnd    = glm::unProject(winFar, view, projection, viewport);
        glm::vec3 rayDir    = glm::normalize(rayEnd - rayOrigin);

        g_ptr<G_Object> closestHit = nullptr;
        float closestT = std::numeric_limits<float>::max();
        float lastVolume = 0.0f;

        for (auto it = selectable.begin(); it != selectable.end(); ) {
            if (!(*it)) {
                it = selectable.erase(it);
            } else {
                ++it;
            }
        }

        for (auto obj : selectable) {
            if(!obj->valid) continue;
           // std::cout << "Checking obj " << obj.get() << " (" << obj->UUID << "), ID: " << obj->ID << "\n";
            BoundingBox worldBox = obj->getWorldBounds();
            float t;
            if (rayIntersectsAABB(rayOrigin, rayDir, worldBox, t)) {
                if (t > 0.0f) {
                    float volume = worldBox.volume();
                    bool isCloser = t < closestT - 0.001f;
                    bool isTie = fabs(t - closestT) < 0.001f && volume > lastVolume;

                    if (isCloser || isTie) {
                        closestT = t;
                        lastVolume = volume;
                        closestHit = obj;
                    }
                }
            }
        }

        return closestHit;
    }

    std::vector<gui_event> events;
    void queueEvent(float end,std::function<void()> event)
    {
        events.emplace_back(sceneTime,end,event);
    }

    void updateGuiScene(float tpf)
    {
        sceneTime+=tpf;
        for (std::vector<gui_event>::iterator it = events.begin() ; it != events.end(); ) {
        if ((sceneTime-it->start)>=it->end) {
            it->event();
            it = events.erase(it);
        } else {
            ++it;
        }
        }

    }

    void addClickEffect(g_ptr<Button> button)
    {
        button->addScript("onClick",[button,this](ScriptContext& ctx)
        {
        button->setPosition(button->getPosition().addZ(button->weight));
        queueEvent(button->deadTime+0.1f,[button]
            {
            button->setPosition(button->getPosition().addZ(-button->weight));
            });
        });
    }
};

}