#include<util/util.hpp>
#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<gui/text.hpp>
#include<util/color.hpp>
#include<util/meshBuilder.hpp>


int main() {
    using namespace Golden;
    using namespace helper;

    std::string MROOT = "../Projects/Gears/assets/models/";

    Window window = Window(1280, 768, "Gears 0.2");
    auto scene = make<Scene>(window,2);
    scene->camera.toOrbit();
    scene->camera.lock = true;
    Data d = make_config(scene,K);
    load_gui(scene, "Gears", "gearsgui.fab");

    auto box = make<Single>(make<Model>(MROOT+"testcar.glb"));
    box->set<float>("speed",0);
    box->set<int>("stars",0);
    scene->add(box);

    auto floor = make<Single>(makeBox(vec3(500.0f,0.1f,500.0f),Color::GREY));
    scene->add(floor);

    list<g_ptr<Single>> stars;
    Script<> make_star("make_star",[scene](ScriptContext& ctx){
            auto star = make<Single>(makeBox(vec3(5,5,5),Color::RED));
            scene->add(star);
            star->addScript("onCollect",[scene,star](ScriptContext& ctx){
                g_ptr<Single> car = ctx.get<g_ptr<Single>>("car");
                car->inc("stars",1);
                star->recycle();
            });
        ctx.set<g_ptr<Object>>("toReturn",star);
    });
    scene->define("star",make_star);

    for(int i=0;i<12;i++)
    {
        auto star = scene->create<Single>("star");
        star->setPosition(vec3(randf(-250,250),3,randf(-250,250)));
        stars << star;
    }

    S_Tool s_tool;
    box->addScript("onDrive",[scene,box,s_tool,stars](ScriptContext& ctx){
        scene->tickEnvironment(1100);

        for(int i =0;i<stars.length();i++)
        {
        auto star = stars.get(i);
        if(!star->isActive()) continue;
        if(star->getWorldBounds().intersects(box->getWorldBounds()))
        {
            ScriptContext collect_ctx;
            collect_ctx.set<g_ptr<Single>>("car",box);
            star->run("onCollect",collect_ctx);
            auto star_counter = scene->getSlot("star_counter")[0];
            text::setText(std::to_string(box->get<int>("stars")), star_counter);
        }

        }

        float speed = box->get<float>("speed");
        int rev = 1;
        if(box->model->currentAnimation!="CarAction"&&speed>0)
        {
            box->model->playAnimation("CarAction");
        }
        else if(box->model->currentAnimation!="ReverseAction"&&speed<0)
        {
            box->model->playAnimation("ReverseAction");
            rev = -1;
        }

        if(held(D)) {
            float turnRate = 1.5f / (1.0f + std::abs(speed) * 0.2f);
            box->rotate(turnRate * -rev, vec3(0,1,0));
        }
        if(held(A)) {
            float turnRate = 1.5f / (1.0f + std::abs(speed) * 0.2f);
            box->rotate(turnRate * rev, vec3(0,1,0));
        }
        box->model->updateAnimation(s_tool.tpf*(std::abs(speed)/3));
        box->move(box->facing()*(s_tool.tpf*-speed));
    });

    float prev_speed = 0;
    start::run(window,d,[&]{

        vec3 targetPos = box->markerPos("race_cam_pos");
        vec3 currentPos = scene->camera.getPosition();
        float smoothing = 0.95f;
        
        scene->camera.setPosition(currentPos + (targetPos - currentPos) * (1.0f - smoothing));
        float car_speed = box->get<float>("speed");
        scene->camera.setTarget(box->getPosition().addY((std::abs(car_speed)/2)+2));

        if(car_speed!=prev_speed) {
            auto speedometer = scene->getSlot("speed_display")[0];
            text::setText(std::to_string((int)std::abs(car_speed*10)), speedometer);
        }

        if(held(W)) {
            if(car_speed<6)
                box->inc<float>("speed",0.03f);
            box->run("onDrive");
        }
        else if(held(S))
        {
            if(car_speed>-6)
                box->inc<float>("speed",-0.03f);
            box->run("onDrive");
        }
        else if(car_speed>0.08) {
            box->inc<float>("speed",-0.05f);
            box->run("onDrive");
        }
        else if(car_speed<-0.08) {
            box->inc<float>("speed",0.05f);
            box->run("onDrive");
        }
        else if(car_speed!=0.0) {
            box->set<float>("speed",0);
        }
        prev_speed = car_speed;
        s_tool.tick();
    });

    return 0;
}