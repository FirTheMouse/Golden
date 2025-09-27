#include<core/helper.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<core/thread.hpp>
#include<util/string_generator.hpp>
#include<util/logger.hpp>
#include<core/physics.hpp>

using namespace helper;
using namespace Golden;

int loop = -1;
int level = 0;
bool in_loop = false;
float l_time = 0.0f;
g_ptr<Scene> scene;
g_ptr<Physics> physics;
g_ptr<Single> player;
bool is_mouse = false;

map<std::string,list<g_ptr<Single>>> by_func;
list<g_ptr<Single>> grabable;
list<vec3> grabable_pos;

list<g_ptr<Single>> surfaces;

list<g_ptr<Single>> grabbed;
list<list<bool>> performed_grab;
list<list<bool>> shift_down;
list<bool> dead;
list<g_ptr<Single>> clones;
list<vec3> grabPoses;
list<list<vec3>> clonePoses;
list<list<vec3>> clonesTowards;
list<list<vec3>> clonesScales;

class _slot : public Object {
public:
    _slot() {}
    _slot(int _s,std::string _name,g_ptr<Single> _in) : size(_s),name(_name),in(_in) {}
    int size;
    std::string name;
    g_ptr<Single> in = nullptr;
    g_ptr<Single> obj = nullptr;

    vec3 pos() {
        // print("Getting pos of ",name," from ",in->get<std::string>("name"));
        // for(auto s : in->markers()) print("   ",s);
        return in->markerPos(name);;
    }
};
struct _origin {
    _origin() {}
    _origin(std::string _type, vec3 _pos,bool _grabbable) : type(_type),pos(_pos),grabbable(_grabbable) {}
    std::string type;
    vec3 pos;
    bool grabbable;
};
list<_origin> starting;

void kill(int c_id);

g_ptr<Single> add_object(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    obj->setPosition(start);
    return obj;

}

list<g_ptr<_slot>> slots(g_ptr<Single> obj) {
    if(obj->has("slot_list"))
        return obj->get<list<g_ptr<_slot>>>("slot_list");
    else 
        return list<g_ptr<_slot>>{};
}
g_ptr<_slot> in_slot(g_ptr<Single> obj) {
    return  obj->get<g_ptr<_slot>>("in_slot");
}
void remove_from_slot(g_ptr<Single> obj) {
    auto slot = in_slot(obj);
    if(slot) {
        slot->obj = nullptr;
        obj->set<g_ptr<_slot>>("in_slot",nullptr);
    }
}

void basic_add(g_ptr<Single> obj, vec3 start) {
    obj->setPosition(start);
    obj->setLinearVelocity(vec3(0,0,0));
    if(obj->has("func"))
        by_func.getOrPut(obj->get<std::string>("func"),list<g_ptr<Single>>{}).push(obj);
    if(obj->has("color")) {
        vec4 col =obj->get<vec4>("color");
        obj->model->setColor(obj->get<vec4>("color").toGlm());
        obj->set<vec4>("mix_color",col);
    }
    obj->set<g_ptr<_slot>>("in_slot",nullptr);
    if(obj->has("slots")) {
        int s_num = obj->get<int>("slots");
        list<g_ptr<_slot>> sub; 
        list<int> sizes = obj->get<list<int>>("slot_size");
        list<std::string> names = obj->markers();
        for(int i = 0;i<s_num;i++) {
            sub << make<_slot>(sizes[i],names[i],obj);
        }
        obj->set<list<g_ptr<_slot>>>("slot_list",sub);
    }
}

g_ptr<Single> add_surface(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    obj->setPhysicsState(P_State::PASSIVE);
    obj->getLayer().setLayer(2);
    obj->getLayer().addCollision({1,3});
    basic_add(obj,start);
    surfaces << obj;
    return obj;
}

g_ptr<Single> add_grabbable(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    obj->setPhysicsState(P_State::ACTIVE);
    obj->getLayer().setLayer(1);
    obj->getLayer().addCollision({1,2,3});
    basic_add(obj,start);
    grabable << obj;
    grabable_pos << start;
    return obj;
}

void basic_remove(g_ptr<Single> obj) {
    if(obj->has("func"))
        by_func.get(obj->get<std::string>("func")).erase(obj);
    remove_from_slot(obj);
    obj->recycle();
}

void remove_grabbable(g_ptr<Single> obj) {
    grabable.erase(obj);
    basic_remove(obj);
}

vec3 g_pos_of(int c_id) {
   int time = (l_time*100)/2;
   vec3 f = clonesTowards[c_id][time];
   vec3 p = clones[c_id]->markerPos("camera_pos");
   vec3 rayStart = p;
   vec3 rayDir = f.normalized();
   
   return rayStart + rayDir * 1.0f;
}

g_ptr<Single> at_hand(int c_id) {
    int time = (l_time*100)/2;
    vec3 f = clonesTowards[c_id][time];
    vec3 p = clones[c_id]->markerPos("camera_pos");
    vec3 rayStart = p;
    vec3 rayDir = f.normalized();
    g_ptr<Single> closest = nullptr;
    float closestDistance = 999.0f;
    
    for(auto s : grabable) {
        if(!s->isActive()) continue;
        if(s==grabbed[c_id]) continue;
        vec3 toSphere = s->getPosition() - rayStart;
        float projLength = toSphere.dot(rayDir);
        if(projLength > 0 && projLength < 2.5f) {
            vec3 closestPoint = rayStart + rayDir * projLength;
            if(closestPoint.distance(s->getPosition()) < 1.0f) {
                if(projLength < closestDistance) {
                    closestDistance = projLength;
                    closest = s;
                }
            }
        }
    }

    if(closest) {
        while(closest->has("in_slot")) {
            auto slot = in_slot(closest);
            if(slot) {
                if(grabable.find(slot->in)!=-1)
                    closest = slot->in;
                else 
                    break;
            } else
                break;
        }
    }
    return closest;
 }

vec3 get_velocitiy(int c_id,int timeframe) {
    if(clonesTowards[c_id].length()>timeframe) {
        int time = ((l_time*100)/2);
        if(time >= timeframe) {
            vec3 current = clonesTowards[c_id][time];
            vec3 start = clonesTowards[c_id][time-timeframe];
            return current - start;
        }
    }
    return vec3(0,0,0);
}

g_ptr<Single> closest_surface(vec3 to,float min = 2.0f) {
    float min_dist = min;
    g_ptr<Single> closest = nullptr;
    for(auto s : surfaces) {
        if(!s->isActive()) continue;
        float dist = s->getPosition().distance(to);
        if(dist<min_dist) {
            min_dist = dist;
            closest = s;
        }
    }
    return closest;
}

vec3 closest_point(vec3 from,g_ptr<Single> surface,float min = 2.0f) {
    float min_dist = min;
    vec3 closest = vec3(0,0,0);
    for(auto s : surface->markers()) {
        float dist = from.distance(surface->markerPos(s));
        if(dist<min_dist) {
            min_dist = dist;
            closest = surface->markerPos(s);
        }
    }
    return closest;
}

list<g_ptr<_slot>> slots_within(g_ptr<Single> from,float min = 2.0f) {
    list<g_ptr<_slot>> result;
    for(auto s : surfaces) {
        if(s==from) continue;
        if(s->has("slot_list")) {
            if(s->distance(from)<=min) result << slots(s);
        }
    }
    for(auto g : grabable) {
        if(g==from) continue;
        if(g->has("slot_list")) {
            if(g->distance(from)<=min) result << slots(g);
        }
    }
    return result;
}

g_ptr<_slot> closest_slot(g_ptr<Single> from,float min = 2.0f) {
    list<g_ptr<_slot>> options;
    int size = from->get<int>("size");
    for(auto p : slots_within(from,min+1.0f)) {
        if(p->obj) continue; //If occupied
        if(p->size!=size) continue;
        options << p;
    }

    float min_dist = min;
    g_ptr<_slot> closest = nullptr;
    for(auto o : options) {
        float dist = o->pos().distance(from->getPosition());
        if(dist<=min_dist) {
            min_dist = dist;
            closest = o;
        }
    }
    return closest;
}


void drop(int c_id) {
    auto obj = grabbed[c_id];
    obj->set<int>("cooldown",50);
    // auto surface = closest_surface(obj->getPosition());
    // if(surface) {
    //     obj->setPosition(closest_point(obj->getPosition(),surface));
    // }
    float s = 1.0f;
    int time = (l_time*100)/2;
    if(shift_down[c_id][time]) {
        vec3 f = clonesTowards[c_id][time];
        obj->setPosition(clones[c_id]->markerPos("camera_pos")+f.mult(1.8f));
        s = 8.0f;
    }
    vec3 f = clonesTowards[c_id][time];
    obj->faceTowards(f.mult(-1),vec3(0,1,0));
    obj->setLinearVelocity(f.mult(s).addY(f.y()*(s*1.5f)));
}

//     if(by_func.hasKey("cut"))
//     for(auto c : by_func.get("cut")) {
//         if(c->getPhysicsState()==P_State::NONE) continue;
//         for(int g=0;g<clones.length();g++) {
//             if(g==grabbed.find(c)) continue;
//             if(dead[g]) continue;
//             if(clones[g]->getWorldBounds().intersects(c->getWorldBounds())) {
//                 kill(g);
//             }
//         }
//     }
// }

g_ptr<Single> closest_grababble(vec3 to,float min = 2.0f) {
    float min_dist = min;
    g_ptr<Single> closest = nullptr;
    for(auto g : grabable) {
        if(!g->isActive()) continue;
        float dist = g->getPosition().distance(to);
        if(dist<min_dist) {
            min_dist = dist;
            closest = g;
        }
    }
    return closest;
}

void grab(int c_id) {
    if(grabbed[c_id]) {
        drop(c_id);
        grabbed[c_id] = nullptr;
    }
    else {
        g_ptr<Single> closest = at_hand(c_id);
        if(!closest) 
            closest = closest_grababble(g_pos_of(c_id),0.8f);

        if(closest) {
            int p_id = grabbed.find(closest);
            if(p_id!=-1) grabbed[p_id] = nullptr;
            grabbed[c_id] = closest;
            remove_from_slot(closest);
        }
    }
}

bool hitting_surface(int c_id) {
    if(!grabbed[c_id]) return false;
    if(!grabbed[c_id]->isActive()) return false;

    for(auto s : surfaces) {
        if(!s->isActive()) continue;
        if(s==grabbed[c_id]) continue;
        if(grabbed[c_id]->getWorldBounds().expand(3).intersects(s->getWorldBounds())) {
            return true;
        }
    }
    return false;
}

vec4 mix_color(vec4 base, vec4 pigment, float strength) {
    return vec4(
        base.x() * (1.0f - strength * (1.0f - pigment.x())),
        base.y() * (1.0f - strength * (1.0f - pigment.y())), 
        base.z() * (1.0f - strength * (1.0f - pigment.z())),
        base.w()
    );
}

void manage_interactions(int c_id) {
    auto obj = grabbed[c_id];
    if(!obj) return;
    if(!obj->isActive()) return;
    if(obj->inc<int>("cooldown",0)>0) return;

    std::string func = obj->has("func")?obj->get<std::string>("func"):"";
    std::string g_name = obj->dtype;
    auto surface = closest_surface(obj->getPosition());

    if(grabbed[c_id]->has("slots"))
    if(grabbed[c_id]->facing().y()>0.9f) {
        for(auto s : slots(grabbed[c_id])) {
            if(s->obj) {
                s->obj->move(vec3(0,-1.0f,0));
                remove_from_slot(s->obj);
            }
        }
    } 

    if(func=="cut") {
        if(by_func.hasKey("be_cut"))
        for(auto c : by_func.get("be_cut")) {
            if(!c->isActive()) continue;
            std::string c_name = c->dtype;
            if(get_velocitiy(c_id,3).y()<-0.3f) {
                if(grabbed[c_id]->getWorldBounds().intersects(c->getWorldBounds())) {
                    vec3 cutterRight = grabbed[c_id]->right();
                    int numPieces = c->get<int>("func_num");
                    for(int i = 0; i < numPieces; i++) {
                        vec3 perpOffset = ((i % 2 == 0) ? cutterRight.mult(-1) : cutterRight).mult(0.2f);
                        float alongCutOffset = (i - (numPieces - 1) / 2.0f) * 0.2f;
                        vec3 parallelOffset = grabbed[c_id]->facing().nY() * alongCutOffset;
                        vec3 spawnPos = c->getPosition() + perpOffset + parallelOffset;
                        auto slice = add_grabbable(c_name+"_cut", spawnPos);
                    }
                    remove_grabbable(c);
                    grabbed[c_id]->set<int>("cooldown",30);
                    break;
                }
            }
        }
    }
    if(func=="hold_water") {
        if(surface) {
            if(surface->has("func"))
            if(surface->get<std::string>("func")=="water_source") {
                for(auto s : slots(obj)) {
                    if(s->size==-1&&!s->obj) {
                        add_grabbable("water",obj->getPosition().addY(0.5f));
                    } 
                }
            }
        }
    }
    if(func=="stir") {
        g_ptr<Single> h = at_hand(c_id); 
        if(h) {
        if(h->dtype=="pot") {
            // if(!slots(h)[0]->obj) print("NO WATER");
            vec3 potCenter = h->getPosition();
            int time = (l_time*100)/2;
            vec3 lookDir = clonesTowards[c_id][time];
            float angle = lookDir.y() * 6.28f;
            float radius = 0.2f;
            vec3 stirPos = potCenter + vec3(
                cos(angle) * radius,
                0.2f,
                sin(angle) * radius
            );
            obj->setPosition(stirPos);
            if(get_velocitiy(c_id,3).length()>=0.1f)
                if(h->inc<int>("stir",1)>60) {
                    auto liquid = slots(h)[0]->obj;
                    vec4 blend(0,0,0,0);
                    int count = 0;
                    for(auto s : slots(liquid)) {
                        if(s->obj) {
                            if(s->obj->has("mix_color")) {
                                blend = blend+s->obj->get<vec4>("mix_color");
                                if(s->obj->inc<int>("shrink",1)<6) {
                                    s->obj->scale(0.8f);
                                } else {
                                    s->obj->set<int>("shrink",0);
                                    remove_grabbable(s->obj);
                                }
                                count++;
                            }
                        }   
                    } 
                    if(count>0) {
                        blend = blend/count;
                        if(liquid&&liquid->has("mix_color")) {
                            vec4 currentColor = liquid->get<vec4>("mix_color");
                            //float r = (float)(count)/(slots(liquid).length()*3);
                            vec4 mixed = mix_color(currentColor,blend,0.2f);
                            liquid->model->setColor(mixed.toGlm());
                            liquid->set<vec4>("mix_color", mixed);
                        }
                    }
                    h->set<int>("stir",0);
                }
        }
        }
    }
}

void reloop(bool start = false) {
    in_loop = false;
    l_time = 0;
    loop++;
    for(int i = 0;i<grabable.length();i++) {
        if(grabable[i]->isActive())
            grabable[i]->recycle();
    }
    for(int i = 0;i<surfaces.length();i++) {
        if(surfaces[i]->isActive())
            surfaces[i]->recycle();
    }
    by_func.clear();
    grabable_pos.clear();
    grabable.clear();
    surfaces.clear();
    for(auto s : starting) {
        if(s.grabbable)
            add_grabbable(s.type,s.pos);
        else
            add_surface(s.type,s.pos);
    } 
    for(int i = 0;i<grabbed.length();i++) {
        grabbed[i] = nullptr;
        dead[i] = false;
    }
    if(!start) {
        player->getLayer().addCollision(1);
        player->setPhysicsState(P_State::ACTIVE);
        player->setLinearVelocity(vec3(0,0,0));
    }
    list<std::string> poses{"white"};
    std::string chose = poses[randi(0,poses.length()-1)];
    is_mouse = chose=="whiskers";
    player = scene->create<Single>("player_one_"+chose);
    player->setPhysicsState(P_State::ACTIVE);
    player->getLayer().setLayer(3);
    player->getLayer().addCollision({2,3});
    grabbed << nullptr;
    dead << false;
    clonePoses << list<vec3>{};
    clonesTowards << list<vec3>{};
    clonesScales << list<vec3>{};
    performed_grab << list<bool>{};
    shift_down << list<bool>{};
    clones << player;
}

void kill(int c_id) {
    dead[c_id] = true;
    clones[c_id]->move(0,2,0);
    add_grabbable("blood",clones[c_id]->getPosition());
    clones[c_id]->rotate(90.0f,clones[c_id]->right());
    clones[c_id]->setLinearVelocity(vec3(0,10,0));
    if(c_id==loop) reloop();
}

list<_origin> get_level(int l) {
    list<_origin> result;
    result << _origin("floor",vec3(0,-1,0),false);
    switch(l) {
        case 0: //Tomato Tamato
result << _origin("cutting_board",vec3(0,0,-2.5f),true);
result << _origin("knife",vec3(1,0,-2.5f),true);
result << _origin("plate",vec3(-1.5f,0,-2.5f),true);
result << _origin("tomato",vec3(-8,0,0),true);
result << _origin("tomato",vec3(-6,0,0),true);
result << _origin("tomato",vec3(-4,0,0),true);
        break;
        case 1:

        break;
        default:
result << _origin("column",vec3(0,0,-5),false);
result << _origin("heater",vec3(0,0,-2.5f),false);
// result << _origin("cutting_board",vec3(0,1.5f,-2.5f),true);
result << _origin("spout",vec3(10,0,0),false);
result << _origin("knife",vec3(8,0,-2.5f),true);
result << _origin("tomato",vec3(-8,0,0),true);
result << _origin("tomato",vec3(-6,0,0),true);
result << _origin("tomato",vec3(-4,0,0),true);
result << _origin("potato",vec3(-2,0,0),true);

result << _origin("hotcore",vec3(-3,0,0),true);
result << _origin("hotcore",vec3(-3,0,1),true);
result << _origin("hotcore",vec3(-3,0,2),true);

result << _origin("pot",vec3(6,0,0),true);
result << _origin("spoon",vec3(2,0,0),true);
        break;
    }
    return result;
}

int main() {

    std::string MROOT = "../Projects/ThymeLoop/assets/models/";

    Window window = Window(1280, 768, "ThymeLoop 0.4");
    scene = make<Scene>(window,1);
    scene->tickEnvironment(1000);
    scene->setupShadows();
    scene->camera.speedMod = 0.01f;
    scene->camera.toFirstPerson();
    window.lock_mouse();
    physics = make<Physics>(scene);
    physics->thread->setSpeed(0.016f);
    text::TextEditor text(scene);
    Data d = make_config(scene,K);
    load_gui(scene, "ThymeLoop", "thymegui.fab");
    start::define_objects(scene,"ThymeLoop");

    auto l1 = make<Light>(Light(glm::vec3(0,10,0),glm::vec4(300,300,300,1)));
    scene->lights.push_back(l1);

    starting = get_level(-1);
    

    // auto o =add_grabbable("tomato");
    // o->inc<int>("t",5);
    // auto b = add_grabbable("tomato");
    // auto c = add_grabbable("tomato");
    // o->recycle();
    // b->recycle();
    // c->recycle();
    // auto h = add_grabbable("tomato");
    // print(h->inc<int>("t",0));

    // for(int i = 1;i<5;i++) {
    //     add_grabbable("tomato")->setPosition(vec3(i,i,i));
    //     //vec3(randf(-10,10),randf(-10,10),randf(-10,10)
    // }

    // auto a = add_grabbable("tomato");
    // auto b = add_grabbable("tomato");

    // double b_time = log::time_function(1,[](int i){
    //     physics->buildTree();
    // });
    // print("Intial build: ",b_time/1000000," ms");

    // double a_time;
    // double a_time = log::time_function(1,[](int i){
    //     physics->updatePhysics();
    // });
    // print("Physics update: ",a_time/1000000," ms");


    // BoundingBox queryBounds = a->getWorldBounds();
    // list<g_ptr<Single>> results;

    // a_time = log::time_function(1,[&queryBounds,&results](int i){
    //     physics->queryTree(physics->treeRoot, queryBounds, results);
    // });
    // print("Query: ",a_time/1000000," ms");
    // print("Found ", results.size(), " potential collisions for tomato A");


    // a->setPosition(vec3(2, 2.5f, 2)); 
    // physics->printTree(physics->treeRoot);
    // double c_time = log::time_function(1,[](int i){
    //     physics->buildTree();
    // });
    // print("Tree update: ",c_time/1000000," ms");
    // physics->printTree(physics->treeRoot);

    // results.clear();
    // a_time = log::time_function(1,[&queryBounds,&results](int i){
    //     physics->queryTree(physics->treeRoot, queryBounds, results);
    // });
    // print("Query: ",a_time/1000000," ms");
    // print("Found ", results.size(), " potential collisions for tomato A");


    // a_time = log::time_function(1,[&queryBounds,&results](int i){
    //     physics->generateCollisionPairs();
    // });
    // print("Tree: ",a_time/1000000," ms");

    // a_time = log::time_function(1,[&queryBounds,&results](int i){
    //    physics->generateCollisionPairsNaive();
    // });
    // print("Naive: ",a_time/1000000," ms");

    reloop(true);

    // g_ptr<Single> box = make<Single>(makeTestBox(0.5f));
    // scene->add(box);
    // box->setPosition(vec3(-3,0,0));
    // box->setPhysicsState(P_State::PASSIVE);
    S_Tool s_tool;
    int cam_mode = 1;
    start::run(window,d,[&]{
        if(pressed(N)&&!text.editing) text.scan(scene->getSlot("timer")[0]);
        text.tick(s_tool.tpf);

        if(pressed(NUM_1)) {
            window.lock_mouse();
            scene->camera.speedMod = 0.01f;
            scene->camera.toFirstPerson();
            cam_mode = 1;
        } 
        if(pressed(NUM_2)) {
            window.unlock_mouse();
            scene->camera.speedMod = 0.01f;
            scene->camera.toOrbit();
            cam_mode = 2;
            if(in_loop) in_loop = false;
        }

        if(pressed(SPACE)) {
            if(in_loop) {
                if(player->getVelocity().position.y()==0)
                    player->setLinearVelocity(vec3(0,is_mouse?10:4,0));
            } else
                in_loop = true;
        }

        if((pressed(R)&&in_loop)||l_time>=500.0f+(loop%5)) {
            kill(loop);
        }

        if(held(E)) {
            float factor;
            if(held(LSHIFT)) {
                factor = 0.99f;
            } else {
                factor = 1.01f;
            }

            vec3 scale = player->getScale();
            float height = player->getWorldBounds().extent().y();
            float ag = scale.length()<0.2f?0.9f:1.0f;
            float move = (factor - ag) * (height / 2);
            player->scale(factor);
            player->move(0, move, 0);
        }

        if(cam_mode==1) {
            player->setLinearVelocity(dir_fps(is_mouse?1.5f:3.5f,player).setY(player->getVelocity().position.y()));
            vec3 f = scene->camera.front;
            player->faceTowards(vec3(f.x(),0,f.z()),vec3(0,1,0));
            vec3 camPos = player->getPosition().addY(0.7f)+player->facing().mult(0.8f);
            scene->camera.setPosition(player->markerPos("camera_pos"));   
        }

        if(in_loop) {
            
           // physics->printTree(physics->treeRoot);
            double p_time = log::time_function(1,[](int i){
                physics->updatePhysics();
            });
            //print("Phys update: ",p_time/1000000," ms");

            int time = ((l_time*100)/2);
            for(int i = 0;i<clonePoses.length();i++) {
                if(i==clonePoses.length()-1) {
                    clonePoses[i].push(player->getPosition());
                    clonesTowards[i].push(scene->camera.front);
                    clonesScales[i].push(player->getScale());
                    if(pressed(MOUSE_LEFT)) performed_grab[i] << true;
                    else performed_grab[i] << false;

                    if(held(LSHIFT)) shift_down[i] << true;
                    else shift_down[i] << false;
                } else {
                    if(dead[i]) {
                        continue;
                    }
                    if((clonePoses[i].length()<=time)) {
                        kill(i);
                        continue;
                    }
                    clones[i]->setPosition(clonePoses[i][time]);
                    clones[i]->setScale(clonesScales[i][time]);
                    vec3 cf = clonesTowards[i][time];
                    clones[i]->faceTowards(vec3(cf.x(),0,cf.z()),vec3(0,1,0));
                }

                if(performed_grab[i][time]) {
                    grab(i);
                }

                if(grabbed[i]) {
                    grabbed[i]->setPosition(clones[i]->markerPos("grip_pos"));
                    vec3 cf = clonesTowards[i][time];
                    grabbed[i]->faceTowards(vec3(-cf.x(),-cf.y(),-cf.z()),vec3(0,1,0));
                }

               manage_interactions(i);
            }


            if(pressed(C)) {
                print("----------------");
            }    

        //    g_ptr<Single> h = at_hand(0);
        //    if(h) {
        //     box->setPosition(h->getPosition().addY(0.1f));
        //    }
            l_time+=0.02f;
            auto timer = scene->getSlot("timer")[0];
            text::setText(std::to_string((int)l_time), timer);
        }
        s_tool.tick();
    });

    // vec3 input = input_2d_arrows(1.0f);
    // player->move(input);
    // if(input.length()!=0) {
    //     player->faceTowards(input_2d_arrows(1.0f),vec3(0,1,0));
    //     box->setPosition(player->getPosition()+player->facing());
    // }

    window.unlock_mouse();
    return 0;
}