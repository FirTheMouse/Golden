#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath> 
#include <rendering/model.hpp>
#include <rendering/renderer.hpp>
#include <rendering/camera.hpp>
#include <rendering/scene.hpp>
#include <util/util.hpp>
#include <core/input.hpp>
#include <util/meshBuilder.hpp>
#include <core/simulation.hpp>
#include <grid.hpp>

auto randf = [] (float min, float max) -> float {
    return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
};
auto randi = [] (int min, int max) -> int {
    return min + static_cast<int>(rand()) / static_cast<int>(RAND_MAX) * (max - min);
};
std::string ROOT = "/Users/jc52156/Desktop/ConnersCode24-25/Golden/";

void genGrass(Scene& scene)
{
 const int instances = 80000; 
    std::vector<glm::mat4> instanceTransforms;
    std::vector<shared_ptr<Object>> objects;
    objects.reserve(instances);
    instanceTransforms.reserve(instances);

shared_ptr<Model> grass = make_shared<Model>(Model("../models/products/grass.glb"));
grass->instance=true;
scene.setupInstance("grass",grass);

for (int i = 0; i < instances; ++i) {

    std::shared_ptr<Object> grassBlade = std::make_shared<Object>();
    grassBlade->id=i;
    grassBlade->type="grass";
    grassBlade->model=grass;
    grassBlade->instance = true;
    grassBlade->hp = 30;
    float x = randf(-199.0f, 199.0f);   // Spread randomly in X
    float z = randf(-199.0f, 199.0f);   // Spread randomly in Z
    float y = 0.0f;
    while(scene.grid.getCell(vec3(x,y,z)).fertility<0.3f)
    {
        x = randf(-199.0f, 199.0f); 
        z = randf(-199.0f, 199.0f); 
    }
    scene.add(grassBlade);
    scene.grid.getCell(vec3(x,y,z)).add(grassBlade);
    //objects.push_back(grassBlade);
    // for(Cell c : grid.cellsAround(vec3(x,y,z),6.0f))
    // {
    //     c.fertility-=0.1f;
    // }
   //for(auto c : grid.getCell(vec3(x,y,z)).objects) cout << c->id << endl;

    float angle = randf(0.0f, 6.2831853f); // Random rotation (0 to 2*PI)
    float scale = randf(0.5f, 1.2f);       // Random overall scale
    float scaleY = randf(4.5f, 15.5f);      // Random height scale

    glm::mat4 transforms = glm::mat4(1.0f);
    transforms = glm::translate(transforms, glm::vec3(x, y, z));
    transforms = glm::rotate(transforms, angle, glm::vec3(0, 1, 0));
    transforms = glm::scale(transforms, glm::vec3(scale, scaleY, scale));
    //instanceTransforms.push_back(transforms);
    grassBlade->transform(transforms);
}
//for(auto o : objects) scene.add(o);
//grass->setupInstancing(instanceTransforms);
//grass->instanceTransforms=instanceTransforms;
}

void genInstances(Scene& scene,int instances,shared_ptr<Model> model,float radius,std::string type)
{
    std::vector<glm::mat4> instanceTransforms;
    instanceTransforms.reserve(instances);

    model->instance=true;
    scene.setupInstance(type,model);

    for (int i = 0; i < instances; ++i) {
        std::shared_ptr<Object> object = std::make_shared<Object>();
        object->id=i;
        object->type=type;
        object->model=model;
        object->instance = true;
        float x = randf(-radius, radius); 
        float z = randf(-radius, radius);
        float y = 0.0f;
        scene.add(object);
        scene.grid.getCell(vec3(x,y,z)).add(object);

        float angle = randf(0.0f, 6.2831853f);
        float scale = randf(0.8f, 1.2f); 

        glm::mat4 transforms = glm::mat4(1.0f);
        transforms = glm::translate(transforms, glm::vec3(x, y, z));
        transforms = glm::rotate(transforms, angle, glm::vec3(0, 1, 0));
        transforms = glm::scale(transforms, glm::vec3(scale, scale, scale));

        instanceTransforms.push_back(transforms);
    }
    model->setupInstancing(instanceTransforms);
}

void genInstancesV(Scene& scene,int instances,shared_ptr<Model> model,std::string type)
{
    std::vector<glm::mat4> instanceTransforms;
    instanceTransforms.reserve(instances);

    model->instance=true;

    for (int i = 0; i < instances; ++i) {
        std::shared_ptr<Object> object = std::make_shared<Object>();
        object->id=i;
        object->type=type;
        object->model=model;
        object->instance = true;
        float x = randf(-199.0f, 199.0f); 
        float z = randf(-199.0f, 199.0f);
        float y = 0.0f;
        while(scene.grid.getCell(vec3(x,y,z)).fertility<0.3f)
        {
            x = randf(-199.0f, 199.0f); 
            z = randf(-199.0f, 199.0f); 
        }
        scene.grid.getCell(vec3(x,y,z)).add(object);

        float angle = randf(0.0f, 6.2831853f);
        float scale = randf(0.3f, 1.6f); 

        glm::mat4 transforms = glm::mat4(1.0f);
        transforms = glm::translate(transforms, glm::vec3(x, y, z));
        transforms = glm::rotate(transforms, angle, glm::vec3(0, 1, 0));
        transforms = glm::scale(transforms, glm::vec3(scale, scale*2.0f, scale));

        instanceTransforms.push_back(transforms);
    }
    model->setupInstancing(instanceTransforms);
    scene.setupInstance(type,model);
}

void ants(Scene& scene,size_t amt)
{
    shared_ptr<Model> antsModel = make_shared<Model>(Model("../models/agents/WhiskersLOD1.glb"));
    antsModel->instance=true;
    scene.setupInstance("ant",antsModel);
    std::vector<glm::mat4> antTransforms;
    for(size_t i=0;i<amt;i++)
    {
        std::shared_ptr<Ant> ant = std::make_shared<Ant>();
        ant->model = antsModel;
        ant->id = i;
        ant->scene = &scene;
        ant->instance = true;
        float cid = ((float)i)/amt;
        ant->data.add<glm::vec4>("color",glm::vec4(
        i<amt/3?cid/3:cid,
        i<amt/2?cid/2:cid,
        i<amt/2?cid/2:cid,
        1.0f));

        Sim::get().addEntity<Ant>(ant);

        //  ant->actions.push(action<>("cut",[ant]{
        //     Grid& grid = ant->scene->grid;
        //     grid.scanCells(ant->getPosition(),28.0f);
        //     int index = ant->scene->grid.findType("food",grid.toIdx(ant->getPosition()), [ant](const std::unordered_set<int>& set, int point) -> int {
        //     int closest = 0;
        //     int cDist = 10000;
        //     for (int i : set) {
        //         int dist = ant->scene->grid.cellDistance(i,point);
        //         if (dist<cDist) {closest = i; cDist = dist;} else if(dist==cDist&&randi(0,10)>5) closest=i; 
        //     }
        //     return closest;
        //     });
        //     if(index!=0) {
        //     if(grid.cellDistance(index,grid.toIdx(ant->getPosition()))>0)
        //     ant->moveTo(grid.cells[index].getPosition(),0.4f).faceTo(grid.cells[index].getPosition());
        //     else 
        //     {
        //     for(auto o : grid.cells[index].getType("food")) {
        //         if(!o->locked)
        //         {
        //             if(o->hp<=0)
        //             {
        //             o->locked=true;
        //             Sim::get().queueMainThreadCall([o](){o->cleanUp();});
        //             }
        //             else o->hp--;
        //             }
        //         }
        //     }
        //     }
        // }));

        if(i<=amt/5)
        {
        ant->actions.push(action<>("cut",[ant]{
            Grid& grid = ant->scene->grid;
            grid.scanCells(ant->getPosition(),28.0f);
            int index = ant->scene->grid.findType("grass",grid.toIdx(ant->getPosition()), [ant](const std::unordered_set<int>& set, int point) -> int {
            int closest = 0;
            int cDist = 10000;
            for (int i : set) {
                int dist = ant->scene->grid.cellDistance(i,point);
                if (dist<cDist) {closest = i; cDist = dist;} else if(dist==cDist&&randi(0,10)>5) closest=i; 
            }
            return closest;
            });
            if(index!=0) {
            if(grid.cellDistance(index,grid.toIdx(ant->getPosition()))>0)
            ant->moveTo(grid.cells[index].getPosition(),0.4f).faceTo(grid.cells[index].getPosition());
            else 
            {
            for(auto o : grid.cells[index].getType("grass")) {
                if(!o->locked)
                {
                    if(o->hp<=0)
                    {
                    o->locked=true;
                    //Sim::get().queueMainThreadCall([o](){o->cleanUp();});
                    Sim::get().queueMainThreadCall([o,ant](){
                        shared_ptr<Object> cut = std::make_shared<Object>();
                        cut->type = "cut";
                        cut->instance = true;
                        ant->scene->add(cut);
                        glm::mat4 original = o->getTransform();
                        glm::vec3 pos = glm::vec3(original[3]);
                        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
                        glm::mat4 transformed = rotation * original;
                        transformed = glm::scale(transformed, glm::vec3(1.0f, 0.7f, 1.0f));
                        transformed[3] = glm::vec4(pos, 1.0f);
                        cut->transform(transformed);
                        cut->setPosition(ant->getPosition());
                        o->cleanUp(); 
                        cut->updateCells();
                        });
                    }
                    else o->hp--;
                    }
                }
            }
            }
        }));
        }
        else {
            ant->actions.push(action<>("transport",[ant]{
            Grid& grid = ant->scene->grid;
            if(ant->has.size()<=0)
            {
                grid.scanCells(ant->getPosition(),28.0f);
                int index = ant->scene->grid.findType("cut",grid.toIdx(ant->getPosition()), [ant](const std::unordered_set<int>& set, int point) -> int {
                int closest = 0;
                int cDist = 10000;
                for (int i : set) {
                    int dist = ant->scene->grid.cellDistance(i,point);
                    int c =0;
                     for(auto o : ant->scene->grid.cells[i].getType("cut"))
                     {
                        if(o->locked) c++;
                     }
                     if(c<=0)
                     {
                     if (dist<cDist) {closest = i; cDist = dist;}
                     else if(dist<cDist+10&&randi(0,10)>4) {closest=i; cDist = dist;}
                     }
                }
                return closest;
                });
                if(index!=0) {
                if(grid.cellDistance(index,grid.toIdx(ant->getPosition()))>1)
                ant->moveTo(grid.cells[index].getPosition(),0.4f).faceTo(grid.cells[index].getPosition());
                else 
                {
                //cout << ant->uid << " arrived: " << grid.cells[index].getType("cut").size() << endl;
                for(auto o : grid.cells[index].getType("cut")) {
                        ant->has.push_back(o->shared_from_this());
                    }
                }
               // if(ant->has.size()>0) cout << ant->uid << " Proceeding" << endl;
                }
            }
            else
            {
                if(vec3(0,0,0).distance(ant->getPosition())>1.2f)
                {
                    ant->moveTo(vec3(0,0,0),0.4f).faceTo(vec3(0,0,0));
                      for(auto o : ant->has) {
                        o->setPosition(ant->getPosition().addY(1.8f));
                      }
                }
                else {
                    for(auto o : ant->has) {
                        o->setPosition(ant->getPosition());
                        o->locked=true;
                      }
                    ant->has.clear();
                }
            }
        }));
        }

        float x = randf(-20.0f, 20.0f); 
        float z = randf(-20.0f, 20.0f);
        float y = 0.0f;
        scene.add(ant);
        scene.grid.getCell(vec3(x,y,z)).add(ant);

        float scale = randf(0.6f, 1.4f); 

        glm::mat4 transforms = glm::mat4(1.0f);
        transforms = glm::translate(transforms, glm::vec3(x, y, z));
        transforms = glm::scale(transforms, glm::vec3(scale, scale, scale));

        // antTransforms.push_back(transforms);
        scene.add(ant);
        ant->transform(transforms);
    }
   // antsModel->setupInstancing(antTransforms);
    //scene.setupInstance("ant",antsModel);
}

// void mice(Scene& scene,size_t amt)
// {
//     for(size_t i=0;i<amt;i++)
//     {
//         std::shared_ptr<Mouse> mouse = std::make_shared<Mouse>();
//         mouse->setModel(Model(ROOT+"/models/agents/WhiskersLOD2.glb"));
//         mouse->scene = &scene;
//         scene.add(1,mouse->model);
//         Sim::get().addEntity<Mouse>(mouse);

//         float x = randf(-20.0f, 20.0f); 
//         float z = randf(-20.0f, 20.0f);
//         float y = 0.0f;
//         scene.grid.getCell(vec3(x,y,z)).add(mouse); 
//         mouse->setPosition(vec3(x,y,z));
//     }
// }

// void crowd(const Scene& scene){
//     shared_ptr<Model> agentsModel = make_shared<Model>(Model(ROOT+"models/agents/WhiskersLOD1.glb"));
//     agentsModel->instance=true;
//     std::vector<glm::mat4> agentInstanceTransforms;
//     for(int i=0;i<9000;i++)
//     {
//         Object agent;
//         agent.name = "Crowd";
//         agent.type = "agent";
//         agent.model = agentsModel;
//         agent.id = i;
//         agent.instance = true;

//         float x = randf(-30.0f, 30.0f);   // Spread randomly in X
//         float z = randf(-30.0f, 30.0f);   // Spread randomly in Z
//         float y = 0.0f;
//         scene.grid.getCell(vec3(x,y,z)).objects.push_back(make_shared<Object>(agent));

//         glm::mat4 transforms = glm::mat4(1.0f);
//         transforms = glm::translate(transforms, glm::vec3(x, y, z));
//         agentInstanceTransforms.push_back(transforms);
//     }
//     agentsModel->setupInstancing(agentInstanceTransforms);
//     //scene.add(INSTANCE_MODEL,agentsModel);

//     shared_ptr<Model> testModel = make_shared<Model>(Model(makeBox(0.5f,1.0f,0.5f,glm::vec4(1.0f,0.0f,0.0f,1.0f))));
//     testModel->instance=true;
//     std::vector<glm::mat4> testInstanceTransforms;
//     for(int i=0;i<200000;i++)
//     {
//         float x = randf(-200.0f, 200.0f);
//         float z = randf(-200.0f, 200.0f); 
//         float y = 0.0f;

//         glm::mat4 transforms = glm::mat4(1.0f);
//         transforms = glm::translate(transforms, glm::vec3(x, y, z));
//         testInstanceTransforms.push_back(transforms);
//     }
//     testModel->setupInstancing(testInstanceTransforms);
//     scene.add(INSTANCE_MODEL,testModel);
// }