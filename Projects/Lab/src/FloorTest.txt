#include<util/util.hpp>
#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<gui/text.hpp>
#include<util/color.hpp>
#include<util/meshBuilder.hpp>
#include<core/thread.hpp>
#include<core/grid.hpp>

using namespace Golden;


int main() {
    using namespace Golden;
    using namespace helper;

    std::string MROOT = "../Projects/Lab/assets/models/";

    Window window = Window(1280, 768, "Floor Test");
    auto scene = make<Scene>(window,1);
    Data d = make_config(scene,K);

    float numThreads = 0;
    list<g_ptr<Thread>> threads;
    std::atomic<int> thread_counter{0};
    float baseline_performance = 0;  // Store the 1-thread result
    float efficiency = 0;
    std::chrono::steady_clock::time_point last = std::chrono::high_resolution_clock::now();

    float mapSize = 100.0f;
    int cube_amt = 500;
    list<g_ptr<Single>> cubes;
    for(int i=0;i<cube_amt;i++)
    {
        auto cube = make<Single>(makeTestBox(1.0f));
        scene->add(cube);
        cube->dtype=std::to_string(i);
        float color = cube_amt/i;
        cube->setPosition(vec3(randf(-mapSize/2,mapSize/2),0,randf(-mapSize/2,mapSize/2)));
        cubes << cube;
        cube->setColor(glm::vec4(color/2,color,color/2,1));
        cube->hide();
    }

    list<g_ptr<Grid>> grids;
    bool start = true;
    start::run(window,d,[&]{


        auto current_time = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration<float>(current_time - last).count()>5||start) {
            start=false;
            print("Iterations complete on ",numThreads,": ",thread_counter.load());
            if(numThreads == 1) {
                baseline_performance = thread_counter.load();
                efficiency = 1.0f;
            } else if(baseline_performance > 0) {
                float expected_performance = baseline_performance * numThreads;
                efficiency = (float)thread_counter.load() / expected_performance;
            }
            print("Efficiency: ", efficiency * 100, "%");
            thread_counter.store(0);
            if(numThreads<8) numThreads++;
            print("Testing ",numThreads," threads");
            if(!threads.empty())
            {
                for(auto t : threads)
                {
                    t->end();
                }
                threads.clear();
            }
            
            list<list<g_ptr<Single>>> parts;
            for(int i=0;i<numThreads;i++)
            {
                parts.push(list<g_ptr<Single>>());
            }
            for(int c=0;c<cubes.length();c++)
            {
                parts[c%(int)numThreads] << cubes[c];
            }
           
            grids.clear();
            for(int t=0;t<numThreads;t++) {
                grids << make<Grid>(1.0f,mapSize);
            }

            for(int t=0;t<numThreads;t++)
            {   
                auto screw_logic = make<Thread>(std::to_string(t));
                list<g_ptr<Single>> screw_cubes = parts[t];
                screw_logic->set<int>("screws",0);
                auto grid = grids[t];
                std::string cellID = "cell"+std::to_string(numThreads);

                screw_logic->run([scene,screw_logic,&thread_counter,grid,screw_cubes,cellID,t,&grids](ScriptContext& ctx){

                    float oxygen = randf(0,10);
                    if(t==0) {
                        for(int c=1;c<grids.length();c++)
                        {
                           if(grids[c]->cells[0]->Object::has("oxygen")) {
                            oxygen += grids[c]->cells[0]->Object::get<float>("oxygen");
                           }

                        }
                    }
                    grid->cells[0]->Object::set<float>("oxygen",oxygen);

                    for(int i =0;i<screw_cubes.length();i++)
                    {
                        if(!screw_cubes[i]->has(cellID))
                        {
                            Cell cell = grid->getCell(screw_cubes[i]->getPosition());
                            cell->push_if(screw_cubes[i]);
                            screw_cubes[i]->set<Cell>(cellID,cell);
                        }

                        vec3 currentPos = screw_cubes[i]->getPosition();
                        vec3 movement = vec3(randi(-1,1),0,randi(-1,1)).mult(randf(0.5f,1.5f));
                        vec3 newPos = currentPos + movement;
                        
                        float halfMap = grid->mapSize / 2.0f;
                        if(newPos.x() < -halfMap || newPos.x() > halfMap) movement.setX(-movement.x());
                        if(newPos.z() < -halfMap || newPos.z() > halfMap) movement.setZ(-movement.z());
                        
                        screw_cubes[i]->move(movement);
                        Cell newCell = grid->getCell(screw_cubes[i]->getPosition());
                        Cell oldCell = screw_cubes[i]->get<Cell>(cellID);
                        if(newCell!=oldCell)
                        {
                            oldCell->erase(screw_cubes[i]);
                            newCell->push_if(screw_cubes[i]);
                            screw_cubes[i]->set<Cell>(cellID,newCell);
                        }

                        list<g_ptr<Single>> neighbors;
                        for(Cell cell : grid->cellsAround(screw_cubes[i]->getPosition(),2)) {
                            for(int j=0;j<cell->length();j++) {
                                    g_ptr<Object> obj = cell->list::get(j, "screw_1");
                                    if(auto g = g_dynamic_pointer_cast<Single>(obj))
                                    {
                                    neighbors << g;
                                    }
                            }
                        }
                        screw_cubes[i]->set<list<g_ptr<Single>>>("neighbors",neighbors);

                    }
                    thread_counter.fetch_add(1); 
                }, 0.000000001f);
                threads << screw_logic;
            }

            for(auto t : threads) {
                t->start();
            }
            last = std::chrono::high_resolution_clock::now();
        }

    });

    return 0;
}


  // float numThreads = 1;
    // list<g_ptr<Thread>> threads;

    // auto shared_obj = make<Object>();
    // std::atomic<int> thread_counter{0};
    // list<std::atomic<int>> per_thread_counter;

    // for(int i=0;i<numThreads;i++)
    // {
    //     auto screw_logic = make<Thread>(std::to_string(i));
    //     screw_logic->set<int>("screws",0);
    //     screw_logic->run([scene,screw_logic,shared_obj,&thread_counter](ScriptContext& ctx){
    //         thread_counter.fetch_add(1); 
    //     }, 0.000000001f);
    //     screw_logic->set<int>("its",0);
    //     threads << screw_logic;
    // }

        // if(pressed(SPACE)) {
        //     for(auto t : threads)
        //     {
        //     if(t->runningTurn)
        //         t->pause();
        //     else 
        //         t->start();
        //     }
        // }

              // if(pressed(Q))
        // {
        //     auto delta = std::chrono::duration<float>(current_time - last);
        //     float secs = delta.count();
            
        //     int its = 0;
        //     for(auto t : threads) {
        //         its += t->get<int>("its");
        //     }
            
        //     float iterations_per_sec = (its - last_its) / secs;
            
        //     last = current_time;
        //     last_its = its;
        //     total_time += secs;

        //     print("Iterations: ",its," over ",secs," seconds \n","Iterations a second: ",iterations_per_sec);
        //     print("total time: ",total_time);
        //     float count = thread_counter;
        //     print("Thread counts a sec:   ",count/total_time);
        // }

