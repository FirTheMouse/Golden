#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <duel/turnManager.hpp>
#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <rendering/model.hpp>
#include <rendering/single.hpp>
#include <core/scriptable.hpp>
#include <util/meshBuilder.hpp>
#include <fstream>
#include <string>
#include <duel/spellList.hpp>
#include <gui/text.hpp>
#include <extension/physics.hpp>

using namespace Golden;

vec3 input2D()
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    Input& input = Input::get();

    if(input.keyPressed(UP)) z-=0.3f;
    if(input.keyPressed(DOWN)) z+=0.3f;
    if(input.keyPressed(LEFT)) x-=0.3f;
    if(input.keyPressed(RIGHT)) x+=0.3f;
    return vec3(x,y,z);
}

void SimSpeedControl()
{
    static bool deBounceSpace = false;
    if(Input::get().keyPressed(SPACE)&&!deBounceSpace) {T_Sim::get().setSpeed(0); P_Sim::get().setSpeed(0); deBounceSpace=true;}
    if(!Input::get().keyPressed(SPACE)&&deBounceSpace) {deBounceSpace = false;}
    if(Input::get().keyPressed(NUM_1)) T_Sim::get().setSpeed(0.3f);
    if(Input::get().keyPressed(NUM_2)) T_Sim::get().setSpeed(0.1f);
    if(Input::get().keyPressed(NUM_3)) T_Sim::get().setSpeed(0.03f);
    if(Input::get().keyPressed(NUM_4)) T_Sim::get().setSpeed(0.016f);
    if(Input::get().keyPressed(NUM_5)) T_Sim::get().setSpeed(0.001f);
}

static std::string MROOT = "../Projects/Dueling/models/";


bool shouldShield(g_ptr<Agent> agent, g_ptr<Duel> duel)
{
    return false;
    bool toShield = false;
    for(auto s : duel->spells)
    {
        if(s->caster==agent) continue;
        for(auto p : s->spellForms)
        {
            if(!s->hasTag(Tag::Damaging)&&s->hasTag(Tag::NoCollison)) continue;
            if(p->distance(agent)<=p->speed*4)
            {
                toShield = true;
                break;
            }
        }
    }
    if(agent->numCasts(sType::Shield)>=1) toShield = false;

    return toShield;
}

void AIzero(g_ptr<Agent> agent, g_ptr<Duel> duel, ScriptContext& ctx)
{
    bool toShield = shouldShield(agent,duel);
    int i = duel->action;
    if(toShield) duel->castSpell(agent,shield(agent,duel));
    else {
        duel->castSpell(agent,createSpell(agent->spellList[i],agent,duel));
    }
}

void AIone(g_ptr<Agent> agent, g_ptr<Duel> duel, ScriptContext& ctx)
{
    bool toShield = shouldShield(agent,duel);
    
    switch(duel->action)
    {
        case 0:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
          duel->castSpell(agent,arcingMageBolt(agent,duel));
        }
        break;

        case 1:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
         duel->castSpell(agent,homingMageBolt(agent,duel));
        }
        break;

        case 2:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
          duel->castSpell(agent,homingMageBolt(agent,duel));
        }
        break;

        default:
        duel->castSpell(agent,empowerArcana(agent,duel));
        break;
    }
}

void AItwo(g_ptr<Agent> agent, g_ptr<Duel> duel, ScriptContext& ctx)
{
    bool toShield = shouldShield(agent,duel);
    
    switch(duel->action)
    {
        case 0:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
          duel->castSpell(agent,empowerArcana(agent,duel));
        }
        break;

        case 1:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
         duel->castSpell(agent,arcingMageBolt(agent,duel));
        }
        break;

        case 2:
        if(toShield) {
            duel->castSpell(agent,shield(agent,duel));
        }
        else {
          duel->castSpell(agent,arcingMageBolt(agent,duel));
        }
        break;

        default:
        duel->castSpell(agent,empowerArcana(agent,duel));
        break;
    }
}

void onHit(g_ptr<Agent> agent, g_ptr<Duel> duel, ScriptContext& ctx)
{
    std::cout << "Ouch!" << std::endl;
    if(ctx.has("pushForce")) {agent->move(vec3(ctx.get<float>("pushForce"),0,0));}
    if(ctx.has("push")) {agent->move(ctx.get<vec3>("push"));}
    if(ctx.has("damage")) {
        if(ctx.get<float>("damage")>=agent->hp)
        {
            agent->hp=1;
        }
        else {agent->hp-=ctx.get<float>("damage");}
        duel->castSpell(agent,hurtBubble(agent,duel),ctx);
    }
}

struct sb{
    std::string model;
    sType type;
    sb(std::string m,sType s) : model(m), type(s) {}
};

int main()
{
    print("starting");
    std::string ROOT = "../Projects/Dueling/models/";
    Window window = Window(1280, 768, "Dueling 0.0.3");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 0;
    }
    glfwSwapInterval(0); //Vsync 1=on 0=off
    glEnable(GL_DEPTH_TEST);

    glm::vec4 BLUE = glm::vec4(0,0,1,1);
    glm::vec4 RED = glm::vec4(1,0,0,1);
    glm::vec4 BLACK = glm::vec4(0,0,0,1);

    Input& input = Input::get();


    g_ptr<Duel> duel = make<Duel>(window,1);
    duel->camera.setPosition(glm::vec3(0.0f, 50.0f, 60.0f));
    duel->tickEnvironment(1700);
    duel->setupShadows();
    GDM::get().setDuel(duel);

    auto l1 = make<Light>(Light(glm::vec3(0,18,0),glm::vec4(600,600,600,1)));
    duel->lights.push_back(l1);

    auto strip = make<Single>(Model(ROOT+"strip.glb"));
    duel->add(strip);


    auto Cliff = make<Agent>("Cliff",Model(ROOT+"fox.glb"));
    duel->add(Cliff);
    Cliff->setPosition(vec3(-40,0,0));
    Cliff->faceTo(vec3(0,0,8));
    Cliff->addScript("onAction",[Cliff,duel](ScriptContext& ctx){
        AIzero(Cliff,duel,ctx);
    });
    Cliff->addScript("onHit",[Cliff,duel](ScriptContext& ctx){
        onHit(Cliff,duel,ctx);
    });
    duel->addAgent(Cliff);

    auto Pine = make<Agent>("Pine",Model(ROOT+"weasel.glb"));
    duel->add(Pine);
    Pine->setPosition(vec3(40,0,0));
    Pine->faceTo(vec3(0,0,0));
    Pine->castPos = vec3(0,0,0).addY(Pine->model->localBounds.max.y/1.3f).addX(-3.0f);
    Pine->addScript("onAction",[Pine,duel](ScriptContext& ctx){
        AIzero(Pine,duel,ctx);
        
    });
    Pine->addScript("onHit",[Pine,duel](ScriptContext& ctx){
        onHit(Pine,duel,ctx);
    });
    Pine->addScript("onRoundStart",[Pine,duel](ScriptContext& ctx){
    });
    Pine->addScript("onTick",[Pine,duel](ScriptContext& ctx){
        if(shouldShield(Pine,duel)) duel->castSpell(Pine,shield(Pine,duel));
    });
    duel->addAgent(Pine);

    Pine->accuracy = 1.0f;
    Pine->spellList[0] = sType::ArcingMageBolt;
    Pine->spellList[1] = sType::MageBolt;
    Pine->spellList[2] = sType::MageBolt;


    Cliff->accuracy = 8.0f;
    Cliff->spellList[0] = sType::ArcingMageBolt;
    Cliff->spellList[1] = sType::MageBolt;
    Cliff->spellList[2] = sType::MageBolt;

    
    auto l2 = make<Light>(Light(glm::vec3(-40,10,28),glm::vec4(100,100,100,1)));
    duel->lights.push_back(l2);

    auto l3 = make<Light>(Light(glm::vec3(38,10,28),glm::vec4(100,100,100,1)));
    duel->lights.push_back(l3);

    sb ts[4] {
        sb("magebolt.glb",sType::MageBolt),
        sb("pop.glb",sType::HomingMageBolt),
        sb("firebolt.glb",sType::FireBolt),
        sb("magepop.glb",sType::EmpowerArcana)
    };
    for(int c=0;c<3;c++)
    {
        for(int i=0;i<4;i++)
        {
        auto m = make<Button>("spellButton",4.0f);
        m->setModel(Model(ROOT+ts[i].model));
        duel->add(m);
        m->scale(3.0f);
        duel->addClickEffect(m);
        m->addScript("onClick",[Cliff,i,ts,c](ScriptContext& ctx){
            Cliff->spellList[c] = ts[i].type;
        });
        m->setPosition(vec3(-48+(i*5),0,18+(c*6)));
        duel->selectable.push_back(m);
        }
    }

    for(int c=0;c<3;c++)
    {
        for(int i=0;i<4;i++)
        {
        auto m = make<Button>("spellButton",4.0f);
        m->setModel(Model(ROOT+ts[i].model));
        duel->add(m);
        m->scale(3.0f);
        duel->addClickEffect(m);
        m->addScript("onClick",[Pine,i,ts,c](ScriptContext& ctx){
            Pine->spellList[c] = ts[i].type;
        });
        m->setPosition(vec3(30+(i*5),0,18+(c*6)));
        duel->selectable.push_back(m);
        }
    }

    // TextBox text(duel);
    // text.setText(list<char>{'1','.','0'});
    // text.setPosition(vec3(0,3,0));

    //GDM::get().startSimulation();
    T_Sim::get().setScene(duel);
    P_Sim::get().setScene(duel);
    T_Sim::get().startSimulation();
    P_Sim::get().startSimulation();

    float tpf = 0.1; float frametime = 0; int frame = 0;
    auto last = std::chrono::high_resolution_clock::now();
    float pause = 0.0f; int frames = 0;

    while (!window.shouldClose()) {
        if(Input::get().keyPressed(KeyCode::K)) break;
        if(pause>0) pause -=tpf;
        //GDM::get().get_T().flushMove(tpf,GDM::get().tps);
        //GDM::get().tick(tpf);
        //GDM::get().flushMainThreadCalls();
        T_Sim::get().flushTasks();
        P_Sim::get().flushTasks();
        SimSpeedControl();

        if(Input::get().keyPressed(R))
        {
            ScriptContext boom = ScriptContext();
            boom.data.add<float>("pushForce",10.0f);
            Pine->run("onHit",boom);
        }

        ScriptContext guiCtx;
        auto g = duel->nearestElement();
        if(g)
        {
            if(input.keyPressed(MOUSE_LEFT))
            {
            if(g->isActive())
            {
                g->run("onClick",guiCtx);
                g->valid = false;
                duel->queueEvent(g->deadTime,[g]
                {
                g->valid = true;
                });
            }
            }
        }
        duel->updateScene(tpf);
        duel->updateGuiScene(tpf);

        auto end = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(end - last);
        tpf = delta.count(); last = end; frame++; 
        frametime+=tpf; frames++;
        if(frametime>=1) {
            //std::cout << frames << " FPS" << std::endl;
            frametime=0;
            frames=0;
        }
        window.swapBuffers();
        window.pollEvents();
    }

        //GDM::get().stopSimulation();

    
    glfwTerminate();
    return 0;
}
