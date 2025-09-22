#include<core/helper.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<core/thread.hpp>
#include<util/string_generator.hpp>

using namespace helper;
using namespace Golden;

int loop = -1;
int level = 0;
bool in_loop = false;
float l_time = 0.0f;
g_ptr<Scene> scene;
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
    obj->setPhysicsState(P_State::ACTIVE);
    if(obj->dtype=="floor") {
        obj->setPhysicsState(P_State::PASSIVE);
    }
    obj->setLinearVelocity(vec3(0,0,0));
    if(obj->has("func"))
        by_func.getOrPut(obj->get<std::string>("func"),list<g_ptr<Single>>{}).push(obj);
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
    basic_add(obj,start);
    surfaces << obj;
    return obj;
}

g_ptr<Single> add_grabbable(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    basic_add(obj,start);
    grabable << obj;
    grabable_pos << start;
    return obj;
}

vec3 g_pos_of(int c_id) {
   int time = (l_time*100)/2;
   vec3 f = clonesTowards[c_id][time];
   vec3 p = clonePoses[c_id][time];
   return p.addY(0.4f)+f.mult(1.8f);
   //return clones[c_id]->getPosition().addY(0.4f)+clones[c_id]->facing()+(clones[c_id]->facing().addY(clonesTowards[c_id][(l_time*100)/2].y()).mult(0.05f));
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
    for(auto p : slots_within(from,min)) {
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
    obj->setPosition(g_pos_of(c_id));
    float s = 4.0f;
    int time = (l_time*100)/2;
    if(shift_down[c_id][time]) {
        s = 14.0f;
    }
    vec3 f = clonesTowards[c_id][time];
    obj->faceTowards(f.mult(-1),vec3(0,1,0));
    obj->setLinearVelocity(f.mult(s).addY(f.y()*(s*1.5f)));
}


void update_physics() {
    //print(scene->active.length());
    for(int i=0;i<scene->active.length();i++) {
        if(scene->active[i]) {
            if(scene->physicsStates[i]==P_State::PASSIVE) continue;
            g_ptr<Single> obj = scene->singles[i];
            glm::mat4& transform = scene->transforms.get(i,"physics::141");
            Velocity& velocity = scene->velocities.get(i,"physics::142");
            float drag = 0.99f;

            velocity.position.addY(-4.8 * 0.016f); //Gravity
            if(scene->singles[i]->inc<int>("cooldown",0)>0) {
                scene->singles[i]->inc<int>("cooldown",-1);
                scene->physicsStates[i] = P_State::NONE;
            } else {
                scene->physicsStates[i] = P_State::ACTIVE;
            }

            int g_id = grabable.find(obj);
            if(g_id!=-1&&grabbed.find(obj)==-1) {
                auto slot = in_slot(obj);
                if(slot==nullptr) {
                    slot = closest_slot(obj,0.8f);
                    if(slot) {
                        obj->setPosition(slot->pos());
                        velocity.position = vec3(0,0,0);
                        slot->obj = obj;
                        obj->set<g_ptr<_slot>>("in_slot",slot);
                        continue;
                    }
                } else {
                    obj->setPosition(slot->pos());
                    velocity.position = vec3(0,0,0);
                    continue;
                }
            }

            int c_id = clones.find(scene->singles[i]);
            for(int s=0;s<scene->active.length();s++) {
                if(s==i) continue;
                if(scene->physicsStates[s]==P_State::NONE) continue;
                //if(clones.find(scene->singles[s])!=-1) continue;
                if(c_id!=-1) {
                    if(grabable.find(scene->singles[s])!=-1) continue;
                }
                if(scene->active[s]) {
                    if(scene->singles[i]->getWorldBounds().intersects(scene->singles[s]->getWorldBounds())) {

                        vec3 thisCenter = scene->singles[i]->getPosition();
                        vec3 otherCenter = scene->singles[s]->getPosition();
                        
                        auto thisBounds = scene->singles[i]->getWorldBounds();
                        auto otherBounds = scene->singles[s]->getWorldBounds();
                        
                        vec3 overlap = vec3(
                            std::min(thisBounds.max.x, otherBounds.max.x) - std::max(thisBounds.min.x, otherBounds.min.x),
                            std::min(thisBounds.max.y, otherBounds.max.y) - std::max(thisBounds.min.y, otherBounds.min.y),
                            std::min(thisBounds.max.z, otherBounds.max.z) - std::max(thisBounds.min.z, otherBounds.min.z)
                        );
                        
                        vec3 normal;
                        if(overlap.x() < overlap.y() && overlap.x() < overlap.z()) {
                            normal = vec3(thisCenter.x() > otherCenter.x() ? 1 : -1, 0, 0);
                        } else if(overlap.y() < overlap.z()) {
                            normal = vec3(0, thisCenter.y() > otherCenter.y() ? 1 : -1, 0);
                        } else {
                            normal = vec3(0, 0, thisCenter.z() > otherCenter.z() ? 1 : -1);
                        }
                        
                        vec3 currentVel = velocity.position;
                        float velocityAlongNormal = currentVel.dot(normal);
                        
                        if(velocityAlongNormal < 0) {
                            velocity.position = currentVel - (normal * velocityAlongNormal);
                            drag = 0.78f;
                        }

                        //velocity.position = vec3(0,0,0);
                    }
                }
            }

            velocity.position = velocity.position * drag; //Drag

            if(glm::length(velocity.position.toGlm()) < 0.001f) {
                velocity.position = vec3(0,0,0);
            }
            float velocityScale = 1.0f;
            glm::vec3 pos = glm::vec3(transform[3]);
            pos += vec3(velocity.position * 0.016f * velocityScale).toGlm();
            transform[3] = glm::vec4(pos, 1.0f);
            glm::vec3 rotVel = vec3(velocity.rotation * 0.016f * velocityScale).toGlm();
            if (glm::length(rotVel) > 0.0001f) {
                glm::mat4 rotationDelta = glm::mat4(1.0f);
                rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                transform = transform * rotationDelta; // Apply rotation in object space
            }
            glm::vec3 scaleChange = vec3(velocity.scale * 0.016f * velocityScale).toGlm();
            if (glm::length(scaleChange) > 0.0001f) {
                glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                transform = transform * scaleDelta; // Apply after rotation
            }

           

            
        }
    }

    if(by_func.hasKey("cut"))
    for(auto c : by_func.get("cut")) {
        if(c->getPhysicsState()==P_State::NONE) continue;
        for(int g=0;g<clones.length();g++) {
            if(g==grabbed.find(c)) continue;
            if(dead[g]) continue;
            if(clones[g]->getWorldBounds().intersects(c->getWorldBounds())) {
                kill(g);
            }
        }
    }
}

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
    //printnl(c_id," attempt at a grab at "); g_pos_of(c_id).print();
    if(grabbed[c_id]) {
        drop(c_id);
        grabbed[c_id] = nullptr;
    }
    else {
        g_ptr<Single> closest = closest_grababble(g_pos_of(c_id),2.0f);
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

void manage_interactions(int c_id) {
    auto obj = grabbed[c_id];
    if(!obj) return;
    if(!obj->isActive()) return;
    if(obj->inc<int>("cooldown",0)>0) return;

    std::string func = obj->get<std::string>("func");
    std::string g_name = obj->dtype;
    auto surface = closest_surface(obj->getPosition());

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
                    by_func.get("be_cut").erase(c);
                    scene->recycle(c,c_name);
                    grabbed[c_id]->set<int>("cooldown",30);
                    break;
                }
            }
        }
    }
    if(func=="hold_water") {
        if(grabbed[c_id]->facing().y()>0.9f) {
            for(auto s : slots(grabbed[c_id])) {
                if(s->obj) {
                    s->obj->move(vec3(0,-1.0f,0));
                    remove_from_slot(s->obj);
                }
            }
        } 
        else if(surface) {
            if(surface->get<std::string>("func")=="water_source") {
                for(auto s : slots(obj)) {
                    if(s->size==-1&&!s->obj) {
                        add_grabbable("water",obj->getPosition().addY(0.5f));
                    } 
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
        player->setPhysicsState(P_State::ACTIVE);
        player->setLinearVelocity(vec3(0,0,0));
    }
    list<std::string> poses{"white"};
    std::string chose = poses[randi(0,poses.length()-1)];
    is_mouse = chose=="whiskers";
    player = scene->create<Single>("player_one_"+chose);
    player->setPhysicsState(P_State::NONE);
    grabbed << nullptr;
    dead << false;
    clonePoses << list<vec3>{};
    clonesTowards << list<vec3>{};
    performed_grab << list<bool>{};
    shift_down << list<bool>{};
    clones << player;
}

void kill(int c_id) {
    dead[c_id] = true;
    clones[c_id]->move(0,2,0);
    clones[c_id]->rotate(90.0f,vec3(1,0,0));
    clones[c_id]->setLinearVelocity(vec3(0,10,0));
    if(c_id==loop) reloop();
}

list<_origin> get_level(int l) {
    list<_origin> result;
    switch(l) {
        case 0:

        break;
        default:
result << _origin("floor",vec3(0,-1,0),false);
result << _origin("column",vec3(0,0,-5),false);
result << _origin("table",vec3(0,-0.5f,-2.5f),false);
result << _origin("spout",vec3(10,0,0),false);
result << _origin("knife",vec3(8,0,-2.5f),true);
result << _origin("tomato",vec3(-8,0,0),true);
result << _origin("tomato",vec3(-6,0,0),true);
result << _origin("tomato",vec3(-4,0,0),true);
result << _origin("tomato",vec3(-2,0,0),true);
result << _origin("pot",vec3(6,0,0),true);
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
    text::TextEditor text(scene);
    Data d = make_config(scene,K);
    load_gui(scene, "ThymeLoop", "thymegui.fab");
    start::define_objects(scene,"ThymeLoop");

    auto l1 = make<Light>(Light(glm::vec3(0,10,0),glm::vec4(300,300,300,1)));
    scene->lights.push_back(l1);

    starting = get_level(-1);
    
    reloop(true);

    // g_ptr<Single> box = make<Single>(makeTestBox(0.1f));
    // scene->add(box);
    S_Tool s_tool;
    int cam_mode = 1;
    start::run(window,d,[&]{
        if(pressed(N)&&!text.editing) text.scan(scene->getSlot("timer")[0]);
        text.tick(0.016f);

        if(pressed(NUM_1)) {
            window.lock_mouse();
            scene->camera.speedMod = 0.01f;
            scene->camera.toFirstPerson();
            cam_mode = 1;
        } 
        if(pressed(NUM_2)) {
            window.unlock_mouse();
            scene->camera.speedMod = 0.01f;
            scene->camera.toIso();
            cam_mode = 2;
            if(in_loop) in_loop = false;
        }

        if(pressed(SPACE)) {
            if(in_loop) {
                player->setLinearVelocity(vec3(0,is_mouse?10:4,0));
            } else
                in_loop = true;
        }

        if((pressed(R)&&in_loop)||l_time>=500.0f+(loop%5)) {
            kill(loop);
        }

        if(pressed(E)) {
            if(grabbed[loop]) {
                if(grabbed[loop]->get<std::string>("func")=="hold_water") {
                    add_grabbable("water",grabbed[loop]->getPosition().addY(0.5f));
                }
            }
        }

        if(cam_mode==1) {
            player->setLinearVelocity(dir_fps(is_mouse?1.5f:3.5f,player).setY(player->getVelocity().position.y()));
            vec3 f = scene->camera.front;
            player->faceTowards(vec3(f.x(),0,f.z()),vec3(0,1,0));
            vec3 camPos = player->getPosition().addY(0.7f)+player->facing().mult(0.8f);
            scene->camera.setPosition(player->markerPos("camera_pos"));   
        }

        if(in_loop) {
            //Looping for the clones and updating the loop for the player
            update_physics();
            int time = ((l_time*100)/2);
            for(int i = 0;i<clonePoses.length();i++) {
                if(i==clonePoses.length()-1) {
                    clonePoses[i].push(player->getPosition());
                    clonesTowards[i].push(scene->camera.front);
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
                    vec3 cf = clonesTowards[i][time];
                    clones[i]->faceTowards(vec3(cf.x(),0,cf.z()),vec3(0,1,0));
                }

                if(performed_grab[i][time]) {
                    grab(i);
                }

                if(grabbed[i]) {
                    //grabbed[i]->setPosition(g_pos_of(i).addY(-0.1f));
                    grabbed[i]->setPosition(clones[i]->markerPos("grip_pos"));
                    if(!hitting_surface(i)) {
                        vec3 cf = clonesTowards[i][time];
                        grabbed[i]->faceTowards(vec3(-cf.x(),-cf.y(),-cf.z()),vec3(0,1,0));
                    }
                }

               manage_interactions(i);
            }

            if(pressed(C)) {
                print("----------------");
            }    
            //box->setPosition(g_pos_of(loop));
            l_time+=0.02f;
            auto timer = scene->getSlot("timer")[0];
            text::setText(std::to_string((int)l_time), timer);
        }
        //s_tool.tick();
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