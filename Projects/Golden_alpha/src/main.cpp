#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <chrono>
#include <rendering/scene.hpp>
#include <rendering/single.hpp>
#include <rendering/instanced.hpp>
#include <rendering/model.hpp>
#include <core/scriptable.hpp>
#include <util/meshBuilder.hpp>
#include <core/simulation.hpp>
#include <grid.hpp>
#include <generator.hpp>
#include <core/pathfinding.hpp>
#include <grid_pathfinder.hpp>
#include <cell_object.hpp>

using namespace Golden;

glm::vec3 getWorldMousePosition(Scene& scene) {
    Window& window = scene.window;
    Camera& camera = scene.camera;
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
    

    glm::vec3 worldNear = glm::unProject(winNear, view, projection, viewport);
    glm::vec3 worldFar  = glm::unProject(winFar,  view, projection, viewport);

    // Ray from near to far
    glm::vec3 rayOrigin = worldNear;
    glm::vec3 rayDir = glm::normalize(worldFar - worldNear);

    // Intersect with y=0 plane
    if (fabs(rayDir.y) < 1e-6) return glm::vec3(0); // Parallel, fail

    float t = -rayOrigin.y / rayDir.y;
    return rayOrigin + rayDir * t;
}

class Mouse : virtual public Single, virtual public Scriptable, virtual public C_Object  {
public:
    using Single::Single;
    Mouse () {}
    ~Mouse() = default;

    float speed = 1.4f;
    float size = 0.8f;
    std::vector<int> path{0};
    Grid_Pathfinder* pather;

    void remove() override {
        C_Object::remove();
    }

};

class MouseI : virtual public Instanced, virtual public Scriptable {
public:
    using Instanced::Instanced;
    MouseI () {}
    ~MouseI() = default;

};

int main()
{
    Sim& sim = Sim::get();
    Window window = Window(1280, 768, "Golden 0.1.3");
    glfwSwapInterval(1); //Vsync 1=on 0=off
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 0;
    }
    glEnable(GL_DEPTH_TEST);

    auto scenePtr = std::make_shared<Scene>(window,1);
    Scene& scene = *scenePtr;

    // auto Snow = make<Mouse>(Model("../models/agents/Snow.glb"));
    // scene.add(Snow);
    // Sim::get().addActive<Mouse>(Snow);
    // Snow->addScript("move",[Snow]{
    //     if(!Snow->inAnimation)
    //     {
    //     Snow->moveTo(vec3(30.0f,0,0),2.5f);
    //     }
    // });

   // std::shared_ptr<Model> mouseModel = std::make_shared<Model>(Model("../models/agents/WhiskersLOD2.glb"));
    // scene.setupInstance("mouse",mouseModel);
    // for(int i=0;i<1000;i++)
    // {
    // auto agent = make<MouseI>();
    // agent->type = "mouse";
    // agent->setModel(mouseModel);
    // scene.add(agent);
    // agent->setPosition(vec3(randf(-15.0f,15.0f),0,randf(-15.0f,15.0f)));
    // Sim::get().addActive<MouseI>(agent);
    // agent->addScript("move",[agent]{
    //     agent->moveAnim(vec3(2.5f,0,0));
    // });}


    //Sim::get().queueMainThreadCall([Snow]{Snow->move(0.1f,0,0);});
    // auto marker = make<Single>(Model("../models/products/berry.glb"));
    // scene.add(marker);
    // auto marker2 = make<Single>(Model("../models/products/sundial.glb"));
    // scene.add(marker2);
    Grid grid(
        0.4f,
        400,
        Model(makeTerrainGrid(400,0.4f))
    );

    std::shared_ptr<Single> gridSingle = std::make_shared<Single>();
    gridSingle->setModel(grid.model);
    scene.add(gridSingle);

    std::shared_ptr<Model> grassModel = std::make_shared<Model>(Model("../models/products/grass1.glb"));
    genInstancesFert(scene,grid,40000,grassModel,199.0f,"grass");

    genInstancesFert(scene,grid,2000,std::make_shared<Model>(Model("../models/products/clover.glb")),199.0f,"clover");

    // std::shared_ptr<Model> stoneModel = std::make_shared<Model>(Model("../models/products/stone.glb"));
    // genInstances(scene,grid,1000,stoneModel,199.0f,"stone");

    // auto starting = std::chrono::high_resolution_clock::now();
    //Grid_Pathfinder pather(grid);
    // for(int n : findPath(
    //     grid.getCell(vec3(1.0f,0,0)).index,
    //     grid.getCell(vec3(0,0,-10.0f)).index,
    //     pather))
    // {
    //    // std::cout << grid.cells[n].x << "," << grid.cells[n].z << std::endl;
    //     grid.updateCellColorFast(grid.cells[n],glm::vec4(1,0,0,1));
    // }
    // auto ending = std::chrono::high_resolution_clock::now();
    // auto totalTime = std::chrono::duration<float>(ending - starting);
    // std::cout << totalTime.count() << std::endl;

    //std::cout << grid.getCell(vec3(0,0,0)).index << std::endl;
    //std::cout << grid.getCell(vec3(0,0,-10.0f)).index << std::endl;

    Grid_Pathfinder pather(grid);

    // auto Snow = make<Mouse>(Model("../models/agents/Snow.glb"));
    // Snow->type = "mouse";
    // scene.add(Snow);
    // Sim::get().addActive<Mouse>(Snow);
    // Snow->grid = &grid;
    // Snow->pather = &pather;

    for(int i=0;i<10;i++)
    {
    auto agent = make<Mouse>(Model("../models/agents/WhiskersLOD2.glb"));
    agent->type = "mouse";
    scene.add(agent);
    agent->setPosition(vec3(randf(-15.0f,15.0f),0,randf(-15.0f,15.0f)));
    Sim::get().addActive<Mouse>(agent);
    agent->grid = &grid;
    agent->pather = &pather;
    agent->addScript("move",[agent]{
        agent->moveToAnim(agent->getPosition().addX(agent->speed),agent->speed);
    });
    // agent->grid->scanCells(agent->getPosition(),100.0f);
    // agent->data.add<int>("index",agent->grid->findType("grass",agent->grid->toIdx
    //         (agent->getPosition()),[agent](const std::unordered_set<int>& set, int point) 
    //         {
    //         int furthest = 0;
    //         int fDist = 0;
    //         for (int i : set) {
    //             int dist = agent->grid->cellDistance(i,point);
    //             if (dist>fDist&&dist<=200) {furthest = i; fDist = dist;} 
    //             else if(dist==fDist&&randi(0,10)>5) furthest=i; 
    //         }
    //         return furthest;
    //         }));

    // agent->addScript("move",[agent]{
    //     agent->grid->scanCells(agent->getPosition(),24.0f);
    //     //agent->updateCells(0.4f);
    //     int index = agent->data.get<int>("index");
    //     if(index!=0)
    //     {
    //     if(agent->grid->cellDistance(index,agent->grid->toIdx(agent->getPosition()))>1)
    //     {
    //      //agent->moveToAnim(agent->grid->toLoc(index),1.4f);
    //       if(agent->path.size()>1)
    //       {
    //         agent->moveToAnim(agent->grid->stepPath(agent->path,agent->speed),agent->speed);
    //       }
    //       else
    //       {
    //         std::vector<int> path{0};
    //         std::unordered_set<int> exclude;
    //         while(path.size()<2)
    //         {
    //         index = agent->grid->findType("grass",agent->grid->toIdx
    //             (agent->getPosition()),[agent,exclude](const std::unordered_set<int>& set, int point) 
    //             {
    //                 int closest = 0;
    //                 int cDist = INFINITY;
    //                 for (int i : set) {
    //                     if(exclude.contains(i)) continue;
    //                     int dist = agent->grid->cellDistance(i,point);
    //                     if (dist<cDist) {closest = i; cDist = dist;} 
    //                     else if(dist==cDist&&randi(0,10)>5) closest=i; 
    //                 }
    //                 return closest;
    //             });
    //             if(index!=0) 
    //             {
    //                 agent->data.set<int>("index",index);
    //                 path=findPath(agent->grid->toIdx(agent->getPosition()),index,*agent->pather,agent->size);
    //                 if(path.size()<2) exclude.insert(index);
    //                 else break;
    //             }
    //             else break;
    //         }
    //         if(path.size()<2)
    //         {
    //             std::cout << "no viable paths at all" << std::endl;
    //         }
    //         else {agent->path=path;}
    //       }
    //     }
    //     else
    //     {
    //         std::cout << "Goal reached" << std::endl;
    //     }
    //     }
    //     agent->updateCells(agent->size);
    // });
    }


    float tpf = 0.1; float frametime = 0; int frames = 0;
    int frame = 0;
    auto last = std::chrono::high_resolution_clock::now();
    bool flag = true;
    bool tickDay = false;
    int chosenScene = 1;

        vec3 lastclickpos(0,0,0);

    scene.tickEnvironment(1000);
    Sim::get().startSimulation();
    Sim::get().setScene(scenePtr);
    bool gridCleaned = false;

    while (!window.shouldClose()) {
        if(Input::get().keyPressed(KeyCode::K)) break;
        float currentFrame = glfwGetTime();
        Sim::get().tick(tpf);
        Sim::get().flushMainThreadCalls();
        //Sim::get_T().flushMove(tpf,Sim::get().tps,Sim::get().lst);
        Sim::get_T().flushMove(tpf,Sim::get().tps);


        // if(Sim::get().slice%3==0&&!gridCleaned) {
        // grid.scanCells(vec3(0,0,0),-1);
        // grid.typeCache.clear();
        // gridCleaned=true;
        // }
        // else if(Sim::get().slice%3!=0) gridCleaned=false;

    scene.updateScene(tpf);
    if(Input::get().keyPressed(F)) 
    {
        vec3 clickpos = getWorldMousePosition(scene);
        // grid.scanCells(clickpos,8.0f);
        // int index = grid.findType("grass",grid.toIdx
        // (clickpos),[grid](const std::unordered_set<int>& set, int point) 
        // {
        // int closest = 0;
        // int cDist = INFINITY;
        // for (int i : set) {
        //     int dist = grid.cellDistance(i,point);
        //     if (dist<cDist) {closest = i; cDist = dist;} 
        //     else if(dist==cDist&&randi(0,10)>5) closest=i; 
        // }
        // return closest;
        // });

        // if(index!=0)
        for(Cell* n : grid.cellsAroundUltraFast(clickpos,2.0f))
        {
            if(n->solid)
            grid.updateCellColorFast(*n,glm::vec4(1,0,0,1));
            else
             grid.updateCellColorFast(*n,glm::vec4(0,0,0,1));
        }
    }

    // if(Input::get().keyPressed(MOUSE_LEFT)) 
    // {
    //     flag = false;
    //     vec3 clickpos = getWorldMousePosition(scene);  
    //     if(Snow->path.size()>1) {Snow->moveTo(grid.stepPath(Snow->path,Snow->speed),Snow->speed);}
    //     else
    //     {
    //         Snow->path=findPath(Snow->grid->toIdx(Snow->getPosition()),grid.toIdx(clickpos),*Snow->pather);
    //         for(int n : Snow->path)
    //         {
    //             grid.updateCellColorFast(grid.cells[n],glm::vec4(1,0,0,1));
    //         }
    //         lastclickpos.addX(-lastclickpos.x());
    //     }
    // }

    // if(Input::get().keyPressed(MOUSE_LEFT)&&flag) 
    // {
    //     flag = false;
    //     vec3 clickpos = getWorldMousePosition(scene);  
    //     if(lastclickpos.x()==0) {lastclickpos = clickpos;}
    //     else
    //     {
    //         // for(int n : findPath(grid.toIdx(lastclickpos),grid.toIdx(clickpos),*Snow->pather))
    //         // {
    //         //     grid.updateCellColorFast(grid.cells[n],glm::vec4(1,0,0,1));
    //         // }
    //         int index = grid.cellBetween(grid.toIdx(lastclickpos),grid.toIdx(clickpos),4.0f);
    //         grid.updateCellColorFast(grid.cells[index],glm::vec4(1,0,0,1));
    //         lastclickpos.addX(-lastclickpos.x());
    //     }
    // }

        auto end = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(end - last);
        tpf = delta.count(); last = end; frames++; 
        frametime+=tpf; frame++;
        if(frametime>=1) {
            flag=true;
            std::cout << frames << " FPS" << std::endl;
            frametime=0;
            frames=0;
        }

        window.swapBuffers();
        window.pollEvents();
    }
    Sim::get().stopSimulation();
    glfwTerminate();
    return 0;
}