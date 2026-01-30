#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>
using namespace Golden;
using namespace helper;

const std::string AROOT = root()+"/Projects/TheGolden/assets/";
const std::string IROOT = AROOT+"images/";
const std::string MROOT = AROOT+"models/";

g_ptr<Scene> scene = nullptr;
ivec2 win(2560,1536);

int main() {


     Window window = Window(win.x()/2, win.y()/2, "Golden 0.0.1");

     g_ptr<Scene> scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     scene->camera.toIso();
     scene->tickEnvironment(1100);

     scene->enableInstancing();

     g_ptr<Physics> phys = make<Physics>(scene);


     g_ptr<Model> a_model = make<Model>(MROOT+"agents/WhiskersLOD3.glb");
     g_ptr<Model> b_model = make<Model>(makeTestBox(1.0f));
     g_ptr<Model> c_model = make<Model>(makeTestBox(0.3f));
     c_model->setColor({0,0,1,1});
     a_model->localBounds.transform(1.0f);
     //I am very, very, very much going to remove the pointless script system in scene's pooling and simplify things.
     scene->define("agent",Script<>("make_agent",[a_model,scene](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(a_model);
          scene->add(q);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));

     vec3 goal = scene->getMousePos();

     for(int i=0;i<600;i++) {
          g_ptr<Single> box = make<Single>(b_model);
          scene->add(box);
          int f = i;
          vec3 pos(randf(-100,100),0,randf(-100,100));
          box->setPosition(pos);
          box->setPhysicsState(P_State::GHOST);
          box->opt_ints << randi(1,10);
          box->opt_floats << 0.2f;
     }

     list<g_ptr<Single>> agents;
     list<vec3> goals;
     list<g_ptr<Single>> markers;
     list<g_ptr<Single>> pointers;
     int width = 10;
     int amt = 1000;
     for(int i=0;i<amt;i++) {
          g_ptr<Single> agent = scene->create<Single>("agent");
          int f = i;
          vec3 pos((float)(f%width),0,(float)(f/width));
          agent->setPosition(pos);
          agent->setPhysicsState(P_State::GHOST);
          agent->opt_ints << randi(1,10); // [0]
          agent->opt_ints << i; // [1] = ITR id
          agent->opt_floats << -0.1f;
          agents << agent;

          vec3 goal_pos((float)(f%width),0,(float)(f/width)+100.0f);
          goals << goal_pos;

          g_ptr<Single> marker = make<Single>(c_model);
          marker->dtype = "marker";
          scene->add(marker);
          marker->setPosition(goal_pos);
          marker->setPhysicsState(P_State::NONE);
          marker->opt_floats << 0.0f;
          markers << marker;

          g_ptr<Single> pointer = make<Single>(c_model);
          pointer->dtype = "pointer";
          scene->add(pointer);
          pointer->setPhysicsState(P_State::NONE);
          pointer->scale({1,0.1,0.5});
          pointer->opt_floats << 0.0f;
          pointers << pointer;
          agent->children << pointer;
          pointer->parent = agent;
          pointer->physicsJoint = [pointer](){
               pointer->setPosition(pointer->parent->getPosition().addY(2.0f));
               return true;
          };
          // pointer->physicsJoint = [agent,pointer](){
          //      pointer->setPosition(agent->getPosition().addY(2.0f));
          //      return true;
          // };
          
          // agent->physicsJoint = [agent,&goal](){
          //      agent->setLinearVelocity(agent->getPosition().direction(goal)*agent->opt_ints[0]);
          //      return true;
          // };
                    
               agent->physicsJoint = [agent, scene, &goals, &markers, phys]() {
                    int id = agent->opt_ints[1];
                    vec3 goal = goals[id];
                    float goal_dist = agent->getPosition().distance(goal);
                    if(goal_dist<=2.0f) {
                         vec3 new_goal(randf(-100,100),0,randf(-100,100));
                         markers[id]->setPosition(new_goal);
                         goals[id] = new_goal;
                    }
                    vec3 net_force(0, 0, 0);
                    float repel_radius = 8.0f;
                    BoundingBox query_bounds = agent->getWorldBounds();
                    query_bounds.expand(BoundingBox(
                         agent->position - vec3(repel_radius),
                         agent->position + vec3(repel_radius)
                    ));
                    list<g_ptr<S_Object>> nearby;
                    phys->queryTree(phys->treeRoot3d, query_bounds, nearby);
                    vec3 goal_dir = agent->getPosition().direction(goal).normalized();
    
                    for(auto sobj : nearby) {
                        if(g_ptr<Single> obj = g_dynamic_pointer_cast<Single>(sobj)) {
                            if(obj == agent) continue;
                            float score = obj->opt_floats[0];
                            if(score==0) continue;
                            
                            vec3 to_obj = agent->direction(obj).nY().normalized();
                            
                            // Alignment: 1.0 if toward goal, -1.0 if away from goal, 0.0 if perpendicular
                            float alignment = to_obj.dot(goal_dir);
                            
                            // Weight the force by alignment
                            // Positive scores: only attractive if aligned with goal
                            // Negative scores: more repulsive if blocking the goal path
                            float weight = score > 0 ? std::max(0.0f, alignment) : std::min(0.0f, alignment);
                            
                            net_force += to_obj * score * abs(weight);
                        }
                    }
                    
                    vec3 goal_force = agent->getPosition().direction(goal) * 2.0f;
                    agent->setLinearVelocity(net_force + goal_force);
                    return true;
               };
     }
     vec3 cam_pos(width/2,50,(amt*1.2)/width);
     scene->camera.setPosition(cam_pos);

     g_ptr<Thread> run_thread = make<Thread>();
     run_thread->run([&](ScriptContext& ctx){
          phys->updatePhysics();
          // for(int i=0;i<agents.length();i++) {
          //      auto pointer = pointers[i];
          //      auto agent = agents[i];
          //      scene->transforms[pointer->ID][3] = scene->transforms[agent->ID][3];
          //      //pointer->setPosition(agent->getPosition().addY(2.0f));
          // }
     },0.008f);
     run_thread->logSPS = true;

     S_Tool s_tool;
     s_tool.log_fps = true;
     start::run(window,d,[&]{
          s_tool.tick();
          goal = scene->getMousePos();

     });
     return 0;
}