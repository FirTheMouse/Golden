#include<core/helper.hpp>
#include<core/grid.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<util/files.hpp>
#include<core/thread.hpp>

using namespace Golden;

struct bill {
    bill() {}
    bill(vec3 _target,bool _complete,int _target_id) : target(_target), complete(_complete), target_id(_target_id) {}

    vec3 target;
    bool complete; 
    int target_id;
};

int main()  {
    using namespace helper;

    std::string MROOT = root()+"/Projects/Spaced/assets/models/";

    Window window = Window(1280, 768, "Spaced 0.0.1");
    auto scene = make<Scene>(window,2);
    scene->camera.toIso();
    Data d = make_config(scene,K);
    // load_gui(scene, "Spaced", "spacedgui.fab");

    float cellSize = 1.0f;
    float mapSize = 100.0f;
    list<g_ptr<Grid>> levels;
    for(int i=0;i<4;i++)
    {
        auto grid = make<Grid>(cellSize,mapSize);
        grid->y_level = i*2;
        levels << grid;
    }

    g_ptr<Grid> level = levels[0];
    start::define_objects(scene,"Spaced",level);
    // for(int i=0;i<2;i++)
    // {
    //     scene->create<Single>("floor_metal");
    //     scene->create<Single>("half_floor_metal");
    //     scene->create<Single>("wall_metal");
    // }

    g_ptr<Single> selected = nullptr;
    g_ptr<Single> selected_last = nullptr;

    
    // auto box = scene->create<Single>("floor_metal");
    // box->setPosition({0,-4,-6});

    //Mesh box_mesh = makeTestBox(1.0f);
    g_ptr<Model> box_model = make<Model>(makeTestBox(0.5f));

    // g_ptr<Single> box2 = make<Single>(box_model);
    // scene->add(box2);
    // box2->setPosition({5,0,0});
    
    // g_ptr<Single> box3 = make<Single>(box_model);
    // scene->add(box3);
    // box3->setPosition({10,0,0});

    g_ptr<Model> snow = make<Model>(root()+"/models/agents/Snow.glb");
    g_ptr<Model> whiskers = make<Model>(root()+"/models/agents/Whiskers.glb");
    g_ptr<Model> whiskers_1 = make<Model>(root()+"/models/agents/WhiskersLOD1.glb");
    g_ptr<Model> whiskers_2 = make<Model>(root()+"/models/agents/WhiskersLOD2.glb");
    g_ptr<Model> whiskers_3 = make<Model>(root()+"/models/agents/WhiskersLOD3.glb");

    scene->enableInstancing();
    
    int row = 0;
    int fac = 80;
    int total = 10000;
    list<bill> bills;
    list<int> apple_ids;
    list<int> mouse_ids; 
    for(int i=0;i<total;i++) {
        //g_ptr<Single> n_box = scene->create<Single>("floor_metal");
        g_ptr<Single> n_box = make<Single>(whiskers_3);
        scene->add(n_box);
        if(i%(total/fac)) row++;
        n_box->setPosition({(float)(row%(total/fac)*-2),-5,(float)((i%row)*-2)});
        if(i==300) selected = n_box;
        mouse_ids << n_box->ID;
    }

    row = 0;
    map<int,int> claimed;
    for(int i=0;i<total;i++) {
        g_ptr<Single> n_box = make<Single>(box_model);
        scene->add(n_box);
        if(i%(total/fac)) row++;
        n_box->setPosition({(float)(row%(total/fac)*-2),-5,(float)((i%row)*-2)});
        apple_ids << n_box->ID;
        claimed.put(n_box->ID,-1);
    }

    list<int> copy_list = apple_ids;
    for(auto i : mouse_ids) { 
        int rand_i = randi(0,copy_list.length()-1);
        int id = copy_list[rand_i];
        copy_list.removeAt(rand_i);
        bills << bill(vec3(scene->transforms[id][3]),false,id);
    }

    // auto Fir = make<Single>(make<Model>(root()+"/models/agents/Snow.glb"));
    // scene->add(Fir);
    bool moving = false;


    g_ptr<Thread> thread = make<Thread>();
    thread->run([&](ScriptContext& ctx){
        if(moving) {
            for(auto i : mouse_ids) {
                if(i>=bills.length()) continue;
                if(vec3(scene->transforms[i][3]).distance(bills[i].target)<1.0f) {
                    bills[i].target = {0,-5,0};
                    claimed.set(bills[i].target_id,i);
                } else {
                    //faceMatrixTo(scene->transforms[i],bills[i].target);
                    translateMatrix(scene->transforms[i],vec3(scene->transforms[i][3]).direction(bills[i].target));
                }
                //translateMatrix(scene->transforms[i],{randf(-0.1,0.1),0,randf(-0.1,0.1)});
            }
            for(auto i : apple_ids) {
                int id = claimed.get(i);
                if(id!=-1) {
                    scene->transforms[i] = glm::translate(glm::mat4(1.0f),glm::vec3(scene->transforms[id][3])+glm::vec3(0,2,0)); //(vec3(scene->transforms[id][3])+vec3(0,2,0)).toGlm();
                }
            }
        }
    },0.002f);
    thread->start();
    
    S_Tool tool;
    tool.log_fps = true;
    start::run(window,d,[&]{
        tool.tick();
        
        if(held(H)) scene->cullSinglesSimplePoint();

        if(pressed(F)) {
            // list<glm::mat4> ntr = {a,box->getTransform()};
            // box_model->transformInstances(ntr);
                scene->models[selected->ID]->instance = !scene->models[selected->ID]->instance;
        }

        if(pressed(SPACE)) moving = !moving;
   
        if(held(E)) {
            if(held(LSHIFT))
                thread->setSpeed(thread->getSpeed()+0.001f);
            else if(thread->getSpeed()>0.001f)
                thread->setSpeed(thread->getSpeed()-0.001f);
        }

        if(pressed(R)) {
            bills.clear();
            row = 0;
            for(int i=0;i<total;i++) {
                g_ptr<Single> n_box = scene->singles[mouse_ids[i]];
                if(i%(total/fac)) row++;
                n_box->setPosition({(float)(row%(total/fac)*-2),-5,(float)((i%row)*-2)});
            }
        
            row = 0;
            for(int i=0;i<total;i++) {
                g_ptr<Single> n_box = scene->singles[apple_ids[i]];
                if(i%(total/fac)) row++;
                n_box->setPosition({(float)(row%(total/fac)*-2),-5,(float)((i%row)*-2)});
                claimed.set(n_box->ID,-1);
            }
        
            copy_list = apple_ids;
            for(auto i : mouse_ids) { 
                int rand_i = randi(0,copy_list.length()-1);
                int id = copy_list[rand_i];
                copy_list.removeAt(rand_i);
                bills << bill(vec3(scene->transforms[id][3]),false,id);
            }
        }

        // if(pressed(N)) {
        //     if(held(LSHIFT))
        //         box2->setColor(Color::BLUE);
        //     else
        //         box2->setColor(Color::RED);
        // }

        // if(held(G)) {
        //     box2->setPosition(scene->getMousePos());
        // }
        if(held(T)) {
            selected->setPosition(scene->getMousePos());
        }

        // if(pressed(NUM_1)) {
        //     level = levels[0];
        // }
        // else if(pressed(NUM_2)) {
        //     level = levels[1];
        // }
        // else if(pressed(NUM_3)) {
        //     level = levels[2];
        // }
        // else if(pressed(NUM_4)) {
        //     level = levels[3];
        // }

        // if(pressed(MOUSE_LEFT)) {
        //    if(selected)
        //    {
        //     selected = nullptr;
        //    }
        //    else {
        //         auto clickedCell = level->getCell(scene->getMousePos(level->y_level));
        //         if(!clickedCell->empty()) {
        //             if(auto g = g_dynamic_pointer_cast<Single>(clickedCell->list::get(0,"main_click_selection"))) {
        //                 selected = g;
        //                 selected_last = g;
        //             }
        //         }
        //     }
        // }

        // if(pressed(F)&&selected_last)
        // {
        //     scene->create<Single>(selected_last->dtype);
        // }

        // if(selected)
        // {
        //     if(pressed(G)) print(selected->get<int>("price"));
        //     selected->setPosition(level->snapToGrid(scene->getMousePos(level->y_level)));

        //     list<Cell> myCells = level->cellsAround(selected->getPosition(),((selected->model->localBounds.getSize().x-1)/2));
        //     list<Cell> oldCells = selected->get<list<Cell>>("_cells");
        //     for(auto cell : oldCells)
        //     {
        //         cell->erase(selected);
        //     }
        //     for(auto cell : myCells)
        //     {
        //         cell->push(selected);
        //     }
        //     selected->set<list<Cell>>("_cells",myCells);
        // }
    });

    return 0;
}