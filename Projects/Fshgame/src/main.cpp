#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<util/color.hpp>
#include<util/string_generator.hpp>
#include<util/logger.hpp>
#include<core/physics.hpp>

using namespace Golden;

g_ptr<Scene> scene;
g_ptr<Font> font;
ivec2 win(2560,1536);

int main()  {
    using namespace helper;

    std::string MROOT = root()+"/Projects/Fshgame/assets/models/";
    std::string IROOT = root()+"/Projects/Fshgame/assets/images/";
    Window window = Window(win.x()/2, win.y()/2, "GUI Testing");
    scene = make<Scene>(window,2);
    Data d = make_config(scene,K);
    scene->tickEnvironment(0);
    font = make<Font>(root()+"/Engine/assets/fonts/source_code.ttf",50);
    list<g_ptr<Quad>> boxes;

    scene->enableInstancing();
    Physics phys(scene);

    int TESTING = 1;
    if(TESTING == 0) {
        g_ptr<Quad> test = text::makeText(
        "There is no single elected official or leader of the Republic, instead power is distributed to three bodies:"
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        ,font,scene,vec2(50,40),0.8f);
        boxes << test;
    } else if (TESTING==1) {
        g_ptr<Geom> geom = make<Geom>();
        for(int i=0;i<100;i++) {
            g_ptr<Quad> box = make<Quad>(geom);
            scene->add(box);
            box->setColor(i%3==0?Color::RED:i%3==1?Color::BLUE:Color::GREEN);
            box->scale({10,10});
            box->setPosition({randf(0,win.x()),randf(0,win.y())});
            box->setLinearVelocity({randf(-40,40),randf(-40,40)});
            box->setPhysicsState(P_State::ACTIVE);
            boxes << box;
        }
        phys.quadCollisonMethod = Physics::NAIVE;
    } else if (TESTING==2) {
        g_ptr<Quad> box = scene->makeImageQuad(IROOT+"plank.png",10);
        box->setPosition({randf(0,win.x()),randf(0,win.y())});
        boxes << box;
    } else if (TESTING==3) {
        g_ptr<Quad> base = scene->makeImageQuad(IROOT+"plank.png",10);
        base->setPosition({randf(0,win.x()),randf(0,win.y())});
        boxes << base;
        for(int i=0;i<10;i++) {
            g_ptr<Quad> box = make<Quad>(base->getGeom());
            scene->add(box);
            box->scale(base->getScale());
            box->setData(base->getData());
            box->setPosition({randf(0,win.x()),randf(0,win.y())});
            boxes << box;
        }
    } else if(TESTING==4) {
        g_ptr<Quad> box = make<Quad>();
        scene->add(box);
        box->scale({50,50});
        box->setColor(Color::RED);
        box->setPosition({500,500});
        box->setPhysicsState(P_State::ACTIVE);
        boxes << box;

        g_ptr<Quad> line = make<Quad>();
        scene->add(line);
        line->scale({100,1});
        line->setPosition({500,200});
        line->setPhysicsState(P_State::FREE);
        boxes << line;
    }
    
    S_Tool s_tool;
    s_tool.log_fps = true;
    double m = 0;
    start::run(window,d,[&]{
        // m = std::sin(s_tool.frame)*100;
        // //test->setPosition(vec2(m,m));
        //    box->move(vec2(m,m));
        if(TESTING==2) {
            phys.updatePhysics();
        } 
        else if(TESTING==1) {
            // for(auto b : boxes) {
            //     vec2 mov(randf(-10,10),randf(-10,10));
            //     vec2 n_pos = b->getCenter()+mov;
            //     if(n_pos.x()>win.x()) mov.setX(-800);
            //     if(n_pos.x()<=0) mov.setX(800);
            //     if(n_pos.y()>win.y()) mov.setY(-800);
            //     if(n_pos.y()<=0) mov.setY(800);
            //     b->move(mov);
            // }
            phys.updatePhysics();
            for(auto b : boxes) {
                Velocity& mov = b->getVelocity();
                vec2 n_pos = b->getCenter();
                int amt = 400;
                if(n_pos.x()>win.x()) mov.position.setX(randf(-amt,amt));
                if(n_pos.x()<=0) mov.position.setX(randf(-amt,amt));
                if(n_pos.y()>win.y()) mov.position.setY(-randf(-amt,amt));
                if(n_pos.y()<=0) mov.position.setY(randf(-amt,amt));
            }

        } 
        else if(TESTING==4) {
            phys.updatePhysics();
            g_ptr<Quad> line = boxes[1];
            g_ptr<Quad> box = boxes[0];
        }

    if(held(MOUSE_LEFT)) {
        if(TESTING==2) {
            boxes[0]->setLinearVelocity(boxes[0]->direction(scene->mousePos2d())/10);
        } 
        else if(TESTING==4) {

        } 
        else {
            boxes[0]->setCenter(scene->mousePos2d());
        }
    }
       s_tool.tick();
    });

    return 0;
}