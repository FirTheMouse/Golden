#include<util/util.hpp>
#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<gui/text.hpp>


int main() {
    using namespace Golden;
    using namespace helper;

    std::string IROOT = "../Projects/FlappyBird/assets/images/";
    std::string GROOT = "../Projects/FlappyBird/assets/gui/";

    Window window = Window(1280, 768, "Flappy Bird");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);
    load_gui(scene, "FlappyBird", "flappygui.fab");

    list<g_ptr<Quad>> pipes;
    for(int i=1;i<6;i++)
    {
    auto pipe = scene->makeImageQuad(IROOT+"vertPipe.png");
    float h = randf(300,1400);
    if(i==1) h=1400;
    pipe->setPosition(vec2(i*500,h));
    pipe->scale(vec2(400,400+(1400-h)));
    pipes << pipe;
    }
    auto bird = scene->makeImageQuad(IROOT+"largeBird.png");
    bird->set<float>("jump",-8);
    bird->set<int>("score",0);
    bird->setPosition(vec2(500,768));

    start::run(window,d,[&]{ 
        if(held(SPACE)) {
            float jump = bird->inc<float>("jump",0.1);
            if(jump<=0)
            {
            bird->move(vec2(0,jump));
            }
            else {
                bird->move(vec2(0,1));
            }
        }
        else {
            if(bird->get<float>("jump")>-8)
                bird->inc<float>("jump",-2);
            bird->move(vec2(0,5));
        }

        for(auto pipe : pipes)
        {
            pipe->move(vec2(-2,0));
            if(pipe->pointInQuad(bird->getCenter()))
            {
                scene->deactivate(bird);
            }
            else if(bird->getCenter().x()==pipe->getCenter().x())
            {
                bird->inc<int>("score",1);
                text::setText(std::to_string(bird->get<int>("score")),scene->getSlot("score_counter")[0]);
            }
            if(pipe->getPosition().x()<=-400)
            {
                float h = randf(300,1400);
                pipe->setPosition(vec2(2460,h));
                pipe->scale(vec2(400,400+(1400-h)));
            }
        }
    });
    return 0;
}