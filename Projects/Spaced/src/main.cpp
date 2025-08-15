#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<core/grid.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<util/files.hpp>

using namespace Golden;

int main()  {
    using namespace helper;

    std::string MROOT = "../Projects/Spaced/assets/models/";

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
    for(int i=0;i<2;i++)
    {
        scene->create<Single>("floor_metal");
        scene->create<Single>("half_floor_metal");
        scene->create<Single>("wall_metal");
    }

    g_ptr<Single> selected = nullptr;
    g_ptr<Single> selected_last = nullptr;


    start::run(window,d,[&]{

        if(pressed(NUM_1)) {
            level = levels[0];
        }
        else if(pressed(NUM_2)) {
            level = levels[1];
        }
        else if(pressed(NUM_3)) {
            level = levels[2];
        }
        else if(pressed(NUM_4)) {
            level = levels[3];
        }

        if(pressed(MOUSE_LEFT)) {
           if(selected)
           {
            selected = nullptr;
           }
           else {
                auto clickedCell = level->getCell(scene->getMousePos(level->y_level));
                if(!clickedCell->empty()) {
                    if(auto g = g_dynamic_pointer_cast<Single>(clickedCell->list::get(0,"main_click_selection"))) {
                        selected = g;
                        selected_last = g;
                    }
                }
            }
        }

        if(pressed(F)&&selected_last)
        {
            scene->create<Single>(selected_last->dtype);
        }

        if(selected)
        {
            if(pressed(G)) print(selected->get<int>("price"));
            selected->setPosition(level->snapToGrid(scene->getMousePos(level->y_level)));

            list<Cell> myCells = level->cellsAround(selected->getPosition(),((selected->model->localBounds.getSize().x-1)/2));
            list<Cell> oldCells = selected->get<list<Cell>>("_cells");
            for(auto cell : oldCells)
            {
                cell->erase(selected);
            }
            for(auto cell : myCells)
            {
                cell->push(selected);
            }
            selected->set<list<Cell>>("_cells",myCells);
        }
    });

    return 0;
}