
#include<util/meshBuilder.hpp>
#include<rendering/scene.hpp>
#include<util/color.hpp>
#include<extension/physics.hpp>
#include<hel/hel_sim.hpp>
#include<hel/hel_scene.hpp>
#include<hel/hel_object.hpp>
#include<core/helper.hpp>

using namespace Golden;
using namespace helper;

// using g_list = g_ptr<qd_list<g_ptr<H_Object>>>;
// using g_listM = qd_list<g_ptr<H_Object>>;

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


int main()
{
    std::string ROOT = "../Projects/Hel/storage/";
    std::string GROOT = "../Projects/Hel/assets/gui/";
    std::string MROOT = "../Projects/Hel/models/";

    print("starting");
    
    Window window = Window(1280, 768, "Hel 0.1.0",0);
    glfwSetInputMode((GLFWwindow*)window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    g_ptr<Hel> hel = make<Hel>(window,2);
    hel->tickEnvironment(1100);
    hel->setupShadows();
    hel->camera.speedMod = 0.01f;
    hel->camera.toFirstPerson();
    S_Tool s_tool;

    load_gui(hel,"Hel","helgui.fab");
    check_loaded({"ammo_counter","health_bar"},hel);

    
    auto ammo_counter =hel->getSlot("ammo_counter")[0];


    auto l1 = make<Light>(Light(glm::vec3(0,18,0),glm::vec4(600,300,300,1)));
    hel->lights.push_back(l1);

    hel->set<g_ptr<Model>>("bullet_model", make<Model>(MROOT+"bullet.glb"));
    Script<> make_bullet("make_bullet",[hel](ScriptContext& ctx){
        auto new_model = make<Model>();
            new_model->copy(*hel->get<g_ptr<Model>>("bullet_model"));
            auto proj = make<H_Object>(new_model);
            proj->set<str>("dtype","proj");
            hel->add(proj);
            proj->setPhysicsState(P_State::ACTIVE);
        ctx.set<g_ptr<Object>>("toReturn",proj);
    });
    hel->define("bullet",make_bullet);


    list<g_ptr<H_Object>> initial_objects1;
    for(int i = 0; i < 10; i++) {
        auto obj = hel->create<H_Object>("bullet");
        initial_objects1 << obj;
    }
    
    for(auto& obj : initial_objects1) {
        hel->recycle(obj, "bullet");
    }

    hel->set<g_ptr<Model>>("demon_model", make<Model>(MROOT+"demon.glb"));
    Script<> make_demon("make_demon",[hel](ScriptContext& ctx){
        auto new_model = make<Model>();
        new_model->copy(*hel->get<g_ptr<Model>>("demon_model"));
        auto demon = make<H_Object>(new_model);
        demon->set<str>("dtype","demon");
        hel->add(demon);
        demon->setPhysicsState(P_State::ACTIVE);
        demon->addScript("onTick",[hel,demon](ScriptContext& ctx){
            if(!demon->isActive()) return;
            if(auto g = hel->closestType(demon->getPosition(),"human"))
            {
                demon->setLinearVelocity(demon->direction(g).mult(4.0f).nY());
                demon->faceTo(g->getPosition());  
                ctx.set<int>("damage",1);
                if(demon->distance(g)<=2.0f) g->run("onHit",ctx);   
            }
        });
        ctx.set<g_ptr<Object>>("toReturn",demon);
    });
    hel->define("demon",make_demon);


    list<g_ptr<H_Object>> initial_objects;
    for(int i = 0; i < 10; i++) {
        auto obj = hel->create<H_Object>("demon");
        initial_objects << obj;
    }
    
    for(auto& obj : initial_objects) {
        hel->recycle(obj, "demon");
    }



    auto one = make<H_Object>(make<Model>(MROOT+"one.glb"));
    one->set<str>("dtype","human");
    one->set<int>("hp",500);
    hel->add(one);
    one->setPhysicsState(P_State::ACTIVE);
    one->setPosition(vec3(0,1.22f,0));
    one->addScript("onTick",[hel,one](ScriptContext& ctx){

    });
    one->addScript("onCollide",[hel,one](ScriptContext& ctx){
        g_ptr<S_Object> s = ctx.get<g_ptr<S_Object>>("with");
    });

    one->addScript("onHit",[hel,one](ScriptContext& ctx){
        auto g = hel->getSlot("health_bar")[0];
        g->setPosition(g->getPosition().setY(1536-one->inc("hp",-ctx.get<int>("damage"))));
    });
    auto hbar = hel->getSlot("health_bar")[0];
    hbar->setPosition(hbar->getPosition().setY(1536-one->inc("hp",0)));

    auto gun = make<H_Object>(make<Model>(MROOT+"testgun.glb"));
    gun->set<str>("dtype","weapon");
    gun->set<int>("max_ammo",8);
    hel->add(gun);
    gun->setPhysicsState(P_State::NONE);
    gun->inc("ammo",gun->get<int>("max_ammo"));
    gun->addScript("onFire",[gun,hel](ScriptContext& ctx){
        if(gun->get<int>("ammo")>0)
        {
        auto g =hel->getSlot("ammo_counter")[0];
        text::setTextBefore(std::to_string(gun->inc<int>("ammo",-1)),'/',g);
        text::setTextAfter(std::to_string(gun->get<int>("max_ammo")),'/',g);
        }
    });

    gun->addScript("onTick",[gun,hel,one,s_tool](ScriptContext& ctx){
        //&&gun->inc<float>("cooldown",-s_tool.tpf)<=0.0f
        // if(pressed(MOUSE_LEFT)&&gun->get<int>("ammo")>0)
        // {
        //     print("fire!");
        //     gun->set<float>("cooldown",0.1f);
        //     ScriptContext ctx;
        //     gun->run("onFire",ctx);
        //     auto proj = hel->create<H_Object>("bullet");
        //     proj->setPosition(gun->markerPos("fire_point"));
        //     proj->setLinearVelocity(hel->camera.front.mult(100.0f));
        //     proj->faceTowards(hel->camera.front);

        //     proj->addScript("onCollide",[hel,proj](ScriptContext& ctx){
        //         g_ptr<S_Object> s = ctx.get<g_ptr<S_Object>>("with");
        //         if(auto h = g_dynamic_pointer_cast<H_Object>(s))
        //         {
        //         if(s->get<str>("dtype")=="demon")
        //         {
        //             // P_Sim::get().queueTask([hel,h,proj]{
        //             //     hel->removeObject(h);
        //             //     hel->removeObject(proj);
        //             // });
        //             hel->recycle(proj,"bullet");
        //         }
        //         }
        //     });
        // }
    });

    auto g =hel->getSlot("ammo_counter")[0];
    text::setTextBefore(std::to_string(gun->inc<int>("ammo",0)),'/',ammo_counter);
    text::setTextAfter(std::to_string(gun->get<int>("max_ammo")),'/',ammo_counter);

    auto main = make<H_Object>(make<Model>(MROOT+"main.glb"));
    hel->add(main);

    T_Sim::get().setScene(hel);
    P_Sim::get().setScene(hel);

    T_Sim::get().logSPS = true;
    T_Sim::get().setSpeed(0.016f);

    T_Sim::get().startSimulation();
    P_Sim::get().startSimulation();



    float pause = 0.0f;
    float reloadBreak = 0.0f;
    float spawn1 = 0.0f;
    float spawn2 = 0.0f;
    

    while (!window.shouldClose()) {
        if(Input::get().keyJustPressed(KeyCode::K)) { 
             glfwSetInputMode((GLFWwindow*)window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
             T_Sim::get().stopSimulation();
             P_Sim::get().stopSimulation();
             glfwTerminate();
             return 0;}
        T_Sim::get().flushTasks();
        SimSpeedControl();
        move_fps(12.0f,one);
        pause -= s_tool.tpf;
        reloadBreak -= s_tool.tpf;
        spawn1 -= s_tool.tpf;
        spawn2 -= s_tool.tpf;

        if(Input::get().keyPressed(KeyCode::R)&&reloadBreak<=0.0f)
        {
            reloadBreak = 4.0f;
            gun->set<int>("ammo",gun->get<int>("max_ammo"));

            text::setTextBefore(std::to_string(gun->get<int>("ammo")),'/',ammo_counter);
        }


        if(pressed(MOUSE_LEFT)&&gun->get<int>("ammo")>0)
        {
            gun->set<float>("cooldown",0.1f);
            ScriptContext ctx;
            gun->run("onFire",ctx);
            auto proj = hel->create<H_Object>("bullet");
            proj->setPosition(gun->markerPos("fire_point"));
            proj->setLinearVelocity(hel->camera.front.mult(100.0f));
            proj->faceTowards(hel->camera.front,vec3(0,1,0));

            proj->addScript("onCollide",[hel,proj](ScriptContext& ctx){
                g_ptr<S_Object> s = ctx.get<g_ptr<S_Object>>("with");
                if(auto h = g_dynamic_pointer_cast<H_Object>(s))
                {
                if(s->get<str>("dtype")=="demon")
                {
                    print("collided");

                    hel->deactivate(proj);
                    hel->deactivate(h);

                    hel->recycle(proj,"bullet");
                    hel->recycle(h,"demon");
                    // print("Recycled demon with ID ",h->ID);
                }
                }
            });
        }

        

        if(spawn1<=0)
        {
        spawn1 = 4.3f;
        auto demon = hel->create<H_Object>("demon");
        // print("created demon with ID ",demon->ID);
        demon->setPosition(main->markerPos("spawn_1").addY(0.00f));
        }

        if(spawn2<=0)
        {
        spawn2 = 5.3f;
        auto demon = hel->create<H_Object>("demon");
        // print("created demon with ID ",demon->ID);
        demon->setPosition(main->markerPos("spawn_2"));
        }

        hel->updateScene(1.0f);
        hel->advanceSlots();

        hel->camera.setPosition(one->markerPos("camera_point"));
        vec3 f = hel->camera.front;
        one->faceTowards(vec3(f.x(),0,f.z()),vec3(0,1,0));
        gun->setPosition(one->markerPos("grip_point"));
        gun->faceTowards(hel->camera.front,vec3(0,1,0));

        s_tool.tick();

        window.swapBuffers();
        window.pollEvents();
    }


    return 0;
}