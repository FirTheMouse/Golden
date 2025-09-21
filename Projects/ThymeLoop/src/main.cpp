#include<core/helper.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<core/thread.hpp>

using namespace helper;
using namespace Golden;

int loop = 0;
bool in_loop = false;
float l_time = 0.0f;
g_ptr<Scene> scene;
g_ptr<Single> player;

map<std::string,list<g_ptr<Single>>> by_func;
list<g_ptr<Single>> grabable;
list<vec3> grabable_pos;

list<g_ptr<Single>> surfaces;

list<g_ptr<Single>> grabbed;
list<list<bool>> performed_grab;
list<g_ptr<Single>> clones;
list<vec3> grabPoses;
list<list<vec3>> clonePoses;
list<list<vec3>> clonesTowards;

struct _origin {
    _origin() {}
    _origin(std::string _type, vec3 _pos) : type(_type),pos(_pos) {}
    std::string type;
    vec3 pos;
};
list<_origin> starting;

void update_physics() {
    //print(scene->active.length());
    for(int i=0;i<scene->active.length();i++) {
        if(scene->active[i]) {
            glm::mat4& transform = scene->transforms.get(i,"physics::141");
            Velocity& velocity = scene->velocities.get(i,"physics::142");
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
}

g_ptr<Single> add_object(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    obj->setPosition(start);
    return obj;
}

g_ptr<Single> add_surface(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    obj->setPosition(start);
    surfaces << obj;
    return obj;
}

g_ptr<Single> add_grabbable(const std::string& type,vec3 start = vec3(0,0,0)) {
    auto obj = scene->create<Single>(type);
    grabable << obj;
    obj->setPosition(start);
    grabable_pos << start;
    by_func.getOrPut(obj->get<std::string>("func"),list<g_ptr<Single>>{}).push(obj);
    if(!in_loop&&loop==0) {
        starting.push(_origin(type,start));
    }
    return obj;
}

vec3 g_pos_of(int c_id) {
   return clones[c_id]->getPosition().addY(0.4f)+clones[c_id]->facing()+(clones[c_id]->facing().addY(clonesTowards[c_id][(l_time*100)/2].y()).mult(0.05f));
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

void drop(g_ptr<Single> obj) {
    auto surface = closest_surface(obj->getPosition());
    if(surface) {
        obj->setPosition(closest_point(obj->getPosition(),surface));
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
        drop(grabbed[c_id]);
        grabbed[c_id] = nullptr;
    }
    else {
        g_ptr<Single> closest = closest_grababble(g_pos_of(c_id),2.0f);
        if(closest) {
            int p_id = grabbed.find(closest);
            if(p_id!=-1) grabbed[p_id] = nullptr;
            grabbed[c_id] = closest;
        }
    }
}

void manage_cut() {
    for(auto g : by_func.get("cut")) {
        if(!g->isActive()) continue;
        std::string g_name = g->get<std::string>("name");
        for(auto c : by_func.get("be_cut")) {
            if(!c->isActive()) continue;
            std::string c_name = c->get<std::string>("name");
            if(g->getWorldBounds().intersects(c->getWorldBounds())) {
                vec3 cutterRight = g->right();
                int numPieces = c->get<int>("func_num");
                for(int i = 0; i < numPieces; i++) {
                    vec3 perpOffset = ((i % 2 == 0) ? cutterRight.mult(-1) : cutterRight).mult(0.3f);
                    float alongCutOffset = (i - (numPieces - 1) / 2.0f) * 0.3f;
                    vec3 parallelOffset = g->facing().nY() * alongCutOffset;
                    vec3 spawnPos = c->getPosition() + perpOffset + parallelOffset;
                    auto slice = add_grabbable(c_name+"_cut", spawnPos);
                    print("Created slice ", i, " at ", spawnPos.x(), ",", spawnPos.y(), ",", spawnPos.z(), 
                          " ID=", slice->ID, " active=", slice->isActive());
                }
                scene->recycle(c,c_name);
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
    by_func.clear();
    grabable_pos.clear();
    grabable.clear();
    for(auto s : starting) {
        add_grabbable(s.type,s.pos);
    } 
    for(int i = 0;i<grabbed.length();i++) {
        grabbed[i] = nullptr;
    }
    if(!start)
        player->setLinearVelocity(vec3(0,0,0));
    player = scene->create<Single>("player");
    player->model->setColor(glm::vec4(loop,loop,loop,loop));
    grabbed << nullptr;
    clonePoses << list<vec3>{};
    clonesTowards << list<vec3>{};
    performed_grab << list<bool>{};
    clones << player;
}


int main() {

    std::string MROOT = "../Projects/ThymeLoop/assets/models/";

    Window window = Window(1280, 768, "ThymeLoop 0.1");
    scene = make<Scene>(window,1);
    scene->tickEnvironment(1100);
    scene->setupShadows();
    scene->camera.speedMod = 0.01f;
    scene->camera.toFirstPerson();
    glfwSetInputMode((GLFWwindow*)window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    Data d = make_config(scene,K);
    load_gui(scene, "ThymeLoop", "thymegui.fab");
    start::define_objects(scene,"ThymeLoop");

    Script<> make_player("make_player",[](ScriptContext& ctx){
        auto player = make<Single>(makeBox(1.0f,2.0f,1.0f,Color::RED));
        scene->add(player);
        ctx.set<g_ptr<Object>>("toReturn",player);
    });
    scene->define("player",make_player);

    // auto phys_thread = make<Thread>();
    // phys_thread->run([&](ScriptContext& ctx){
    //     update_physics();
    // },0.016);
    // phys_thread->start();

    g_ptr<Single> ground = make<Single>(makeBox(50.0f,0.1f,50.0f,Color::GREY));
    scene->add(ground);
    ground->move(0,-1.0f,0);

    g_ptr<Single> marker  = make<Single>(makeBox(1.0f,3.0f,1.0f,Color::GREY));
    scene->add(marker);
    marker->setPosition(vec3(0,0.5f,-4.0f));

    add_grabbable("knife");
    add_surface("table");
    add_grabbable("tomato",vec3(4,0,0));
    
    reloop(true);

    start::run(window,d,[&]{
        if(pressed(SPACE)) {
            in_loop = true;
        }

        if(pressed(R)&&in_loop) {
            reloop();
        }

        if(pressed(C)) {
            print("----------------");
        }

        move_fps(5.0f,player);
        vec3 f = scene->camera.front;
        player->faceTowards(vec3(f.x(),0,f.z()),vec3(0,1,0));
        vec3 camPos = player->getPosition().addY(0.7f)+player->facing().mult(0.8f);
        scene->camera.setPosition(camPos);   

        if(in_loop) {
            //Looping for the clones and updating the loop for the player
            update_physics();
            int time = ((l_time*100)/2);
            for(int i = 0;i<clonePoses.length();i++) {
                if(i==clonePoses.length()-1) {
                    //if(clonePoses[loop].length()>time) continue;
                    clonePoses[i].push(player->getPosition());
                    clonesTowards[i].push(scene->camera.front);
                    if(pressed(MOUSE_LEFT)) performed_grab[i] << true;
                    else performed_grab[i] << false;
                } else {
                    if((clonePoses[i].length()<=time)) continue;
                    clones[i]->setPosition(clonePoses[i][time]);
                    vec3 cf = clonesTowards[i][time];
                    clones[i]->faceTowards(vec3(cf.x(),0,cf.z()),vec3(0,1,0));
                }

                if(performed_grab[i][time]) {
                    grab(i);
                }

                if(grabbed[i]) {
                    grabbed[i]->setPosition(g_pos_of(i).addY(-0.1f));
                    vec3 cf = clonesTowards[i][time];
                    grabbed[i]->faceTowards(vec3(-cf.x(),-cf.y(),-cf.z()),vec3(0,1,0));
                }
            }
            manage_cut();

            l_time+=0.02f;
            auto timer = scene->getSlot("timer")[0];
            //text::setText(std::to_string((int)l_time), timer);
        }
    });

    // vec3 input = input_2d_arrows(1.0f);
    // player->move(input);
    // if(input.length()!=0) {
    //     player->faceTowards(input_2d_arrows(1.0f),vec3(0,1,0));
    //     box->setPosition(player->getPosition()+player->facing());
    // }

    glfwSetInputMode((GLFWwindow*)window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    return 0;
}