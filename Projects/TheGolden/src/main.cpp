#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>
#include<core/numGrid.hpp>
using namespace Golden;
using namespace helper;

const std::string AROOT = root()+"/Projects/TheGolden/assets/";
const std::string IROOT = AROOT+"images/";
const std::string MROOT = AROOT+"models/";

g_ptr<Scene> scene = nullptr;
g_ptr<NumGrid> grid = nullptr;
ivec2 win(2560,1536);

list<int> addToGrid(g_ptr<Single> q) {
     return grid->addToGrid(q->ID,q->getWorldBounds());
}
list<int> removeFromGrid(g_ptr<Single> q) {
     return grid->removeFromGrid(q->ID,q->getWorldBounds());
}

int main() {


     Window window = Window(win.x()/2, win.y()/2, "Golden 0.0.5");

     scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     scene->camera.toOrbit();
     scene->tickEnvironment(1100);

     scene->enableInstancing();

     g_ptr<Physics> phys = make<Physics>(scene);
     grid = make<NumGrid>(0.5f,1000.0f);


     g_ptr<Model> a_model = make<Model>(MROOT+"agents/WhiskersLOD3.glb");
     g_ptr<Model> b_model = make<Model>(makeTestBox(1.0f));
     g_ptr<Model> c_model = make<Model>(makeTestBox(0.3f));
     g_ptr<Model> grass_model = make<Model>(MROOT+"products/grass.glb");
     c_model->setColor({0,0,1,1});
     a_model->localBounds.transform(0.6f);

     //I am very, very, very much going to remove the pointless script system in scene's pooling and simplify things.
     scene->define("agent",Script<>("make_agent",[a_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(a_model);
          scene->add(q);
          q->opt_idx_cache = addToGrid(q);
          q->joint = [q](){
               for(auto i : q->opt_idx_cache) {grid->cells[i].erase(q->ID);}
               q->updateTransform(false);
               q->opt_idx_cache = addToGrid(q);
               return false;
          };
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));


     vec3 goal = scene->getMousePos();

     //Make path stones
     // for(int i=0;i<600;i++) {
     //      g_ptr<Single> box = make<Single>(b_model);
     //      scene->add(box);
     //      int f = i;
     //      box->dtype = "path";
     //      vec3 pos(randf(-100,100),0.5,randf(-100,100));
     //      box->setPosition(pos);
     //      box->setScale({1,0.5,1});
     //      box->setPhysicsState(P_State::GHOST);
     //      box->opt_ints << randi(1,10);
     //      box->opt_floats << 0.2f;
     // }

     // //Make grass stalks
     // for(int i=0;i<600;i++) {
     //      g_ptr<Single> box = make<Single>(grass_model);
     //      scene->add(box);
     //      int f = i;
     //      box->dtype = "grass";
     //      float s = randf(3,8);
     //      vec3 pos(randf(-100,100),0,randf(-100,100));
     //      box->setPosition(pos);
     //      box->setScale({1,s,1});
     //      box->setPhysicsState(P_State::GHOST);
     //      box->opt_ints << randi(1,10);
     //      box->opt_floats << -1.0f;
     // }


     float maze_size = 50.0f;
     float wall_thickness = 1.0f;
     float wall_height = 3.0f;
     int grid_rows = 10;
     int grid_cols = 10;
     float cell_size = maze_size / grid_rows;

     // Helper to place wall segment
     auto place_wall = [&](vec3 pos, vec3 scale) {
          g_ptr<Single> wall = make<Single>(b_model);
          scene->add(wall);
          wall->dtype = "wall";
          wall->setPosition(pos);
          wall->setScale(scale);
          wall->setPhysicsState(P_State::FREE);
          addToGrid(wall);
          wall->opt_floats << 0.0f; // Strong repulsion
     };

     // Create outer boundary
     for(int i = 0; i <= grid_cols; i++) {
     float x = -maze_size/2 + i * cell_size;
     // Top wall
     place_wall(vec3(x, wall_height/2, -maze_size/2), 
                    vec3(wall_thickness, wall_height, wall_thickness));
     // Bottom wall
     place_wall(vec3(x, wall_height/2, maze_size/2), 
                    vec3(wall_thickness, wall_height, wall_thickness));
     }

     for(int i = 0; i <= grid_rows; i++) {
     float z = -maze_size/2 + i * cell_size;
     // Left wall
     place_wall(vec3(-maze_size/2, wall_height/2, z), 
                    vec3(wall_thickness, wall_height, wall_thickness));
     // Right wall
     place_wall(vec3(maze_size/2, wall_height/2, z), 
                    vec3(wall_thickness, wall_height, wall_thickness));
     }

     // Create internal maze walls (simple grid with random gaps)
     for(int row = 1; row < grid_rows; row++) {
     for(int col = 1; col < grid_cols; col++) {
          float x = -maze_size/2 + col * cell_size;
          float z = -maze_size/2 + row * cell_size;
          
          // Randomly place walls (70% chance)
          if(randf(0, 1) > 0.3f) {
               // Horizontal wall segment
               place_wall(vec3(x, wall_height/2, z), 
                         vec3(cell_size, wall_height, wall_thickness));
          }
          if(randf(0, 1) > 0.3f) {
               // Vertical wall segment
               place_wall(vec3(x, wall_height/2, z), 
                         vec3(wall_thickness, wall_height, cell_size));
          }
     }
     }

     list<g_ptr<Single>> agents;
     list<vec3> goals;
     list<list<g_ptr<Single>>> crumbs;
     list<list<g_ptr<Single>>> debug;
     scene->define("crumb",Script<>("make_crumb",[c_model,phys](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(c_model);
          scene->add(q);
          q->scale(0.5f);
          q->setPhysicsState(P_State::GHOST);
          q->opt_ints.clear();
          q->opt_floats.clear();
          q->opt_vec_3 = vec3(0,0,0);

          phys->treeBuilt3d = false;

          q->opt_ints << randi(1,10); // [0]
          q->opt_ints << 0; // [1] = ITR id
          q->opt_ints << 0; // [2] = Accumulator
          q->opt_ints << 0; // [3] = Accumulator 2
          q->opt_floats << 0.0f; // [0] = attraction
          q->opt_floats << 0.999; // [1] = decay
          q->physicsJoint = [q](){
               q->opt_floats[0] *= q->opt_floats[1]; // Decay per frame
               float attraction = q->opt_floats[0];
               q->setScale({(float)(1*attraction>0?attraction:1),(float)(1*attraction>0?1:-attraction),(float)(1*attraction>0?attraction:1)});


               if(q->opt_floats[0] < 0.01f && q->opt_floats[0] > -0.01f) scene->recycle(q);
               return true;
          };
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     int width = 10;
     int amt = 3;
     for(int i=0;i<amt;i++) {
          g_ptr<Single> agent = scene->create<Single>("agent");
          int f = i;
          vec3 pos((float)(f%width),0,(float)(f/width)+50);
          agent->setPosition(pos);
          agent->setPhysicsState(P_State::FREE);
          agent->opt_ints << randi(1,10); // [0]
          agent->opt_ints << i; // [1] = ITR id
          agent->opt_ints << 0; // [2] = Accumulator
          agent->opt_ints << 0; // [3] = Accumulator 2
          agent->opt_floats << -0.0f; // [0] = attraction
          agent->opt_floats << randf(4,6); // [1] = speed
          agent->opt_vec3_2 = vec3(0,0,0); //Progress
          agents << agent;

          vec3 goal_pos((float)(f%width),0,(float)(f/width)-30.0f);
          goals << goal_pos;

          g_ptr<Single> marker = scene->create<Single>("crumb");
          marker->setPosition(goal_pos);
          marker->setPhysicsState(P_State::NONE);
          marker->opt_floats[0] = 20.0f; //Set attraction
          marker->opt_floats[1] = 1.0f;
          crumbs << list<g_ptr<Single>>{marker};
          float agent_hue = (float(i) / float(amt)) * 360.0f;
          list<g_ptr<Single>> d_boxes;
          for(int j=0;j<3;j++) {
               g_ptr<Single> dbox = make<Single>(make<Model>(makeTestBox(0.5f)));
               scene->add(dbox);
               // Each box type gets different saturation/value
               float saturation = 1.0f;
               float value = 1.0f - (float(j) * 0.3f); // Boxes get darker: 1.0, 0.7, 0.4
               
               // Convert HSV to RGB
               float h = agent_hue / 60.0f;
               float c = value * saturation;
               float x = c * (1.0f - abs(fmod(h, 2.0f) - 1.0f));
               float m = value - c;
               
               vec3 rgb;
               if(h < 1) rgb = vec3(c, x, 0);
               else if(h < 2) rgb = vec3(x, c, 0);
               else if(h < 3) rgb = vec3(0, c, x);
               else if(h < 4) rgb = vec3(0, x, c);
               else if(h < 5) rgb = vec3(x, 0, c);
               else rgb = vec3(c, 0, x);
               
               rgb += vec3(m, m, m);
               dbox->setColor({rgb.x(), rgb.y(), rgb.z(), 1.0f});
               if(j==0)
                    dbox->scale({1,0.5,1});
               else if(j==1)
                    dbox->scale({0.5,2,0.5});
               else if(j==2)
                    dbox->scale({0.5,1,2});


               dbox->setPosition(agent->getPosition());
               dbox->setPhysicsState(P_State::NONE);
               d_boxes << dbox;
          }
          debug << d_boxes;

          // In your agent setup, replace the physicsJoint with:
          agent->physicsJoint = [agent, &goals, &crumbs, &debug, phys]() {
               int id = agent->opt_ints[1];
               g_ptr<Single> goal_crumb = crumbs[id][0];
               vec3 goal = goal_crumb->getPosition();
               float goal_dist = agent->distance(goal_crumb);
               
               // Goal reached behavior
               if(goal_dist <= 1.0f) {
               if(agent->opt_ints[2] < 650) {
                    if(goal_crumb->isActive()) {
                         scene->deactivate(goal_crumb);
                         agent->setLinearVelocity(vec3(0,0,0));
                    }
                    if(agent->opt_ints[2] % 100 == 0)
                         agent->faceTo(agent->getPosition() + vec3(randf(-5,5), 0, randf(-5,5)));
                    agent->opt_ints[2]++;
                    return true;
               } else {
                    vec3 new_goal = agent->getPosition() + agent->facing() * randf(10, 100);
                    scene->reactivate(goal_crumb);
                    crumbs[id][0]->setPosition(new_goal);
                    agent->opt_ints[2] = 0;
               }
               }
          
               float desired_speed = agent->opt_floats[1];
               
               // A* pathfinding
               auto isWalkable = [](int idx) {
               // for(int id : grid->cells[idx]) {
               //      if(scene->singles[id]->dtype == "wall") return false;
               // }
               // return true;
               return grid->cells[idx].empty();
               };
               
               // Recompute path every 60 frames or if we don't have one
               list<int>& path = agent->opt_idx_cache_2; // Reuse another cache field for path
               if(agent->opt_ints[3] % 60 == 0 || path.empty()) {
               path = grid->findPath(
                    grid->toIndex(agent->getPosition()),
                    grid->toIndex(goal),
                    isWalkable
               );
               }
               agent->opt_ints[3]++;
               
               if(!path.empty()) {
               // Get next waypoint
               vec3 waypoint = grid->indexToLoc(path[0]);
               float waypoint_dist = agent->getPosition().distance(waypoint);
               
               // Reached waypoint, advance to next
               if(waypoint_dist < grid->cellSize) {
                    path.removeAt(0);
                    if(!path.empty()) {
                         waypoint = grid->indexToLoc(path[0]);
                    }
               }
               
               // Move toward current waypoint
               vec3 direction = (waypoint - agent->getPosition()).normalized().nY();
               agent->faceTo(agent->getPosition() + direction);
               agent->setLinearVelocity(direction * desired_speed);
               
               // Debug: show waypoint
               debug[id][0]->setPosition(waypoint.addY(2.0f));
               }
               
               return true;
          };

          // agent->physicsJoint = [agent, &goals, &crumbs, &debug, phys]() {
          //      int& frame = agent->opt_ints[3];
          //      frame++;
          //      int interval = 30;
          //      if(frame % interval == 0) {

          //           int id = agent->opt_ints[1];
          //           list<g_ptr<Single>>& my_crumbs = crumbs[id];
          //           list<g_ptr<Single>>& my_debug = debug[id];
          //           g_ptr<Single> goal_crumb = crumbs[id][0];
          //           vec3 goal = goal_crumb->getPosition();
          //           float goal_dist = agent->distance(goal_crumb);
                    
          //           // Goal reached behavior
          //           if(goal_dist <= 1.0f) {
          //           if(agent->opt_ints[2] < 650) {
          //                if(goal_crumb->isActive()) {
          //                     scene->recycle(goal_crumb);
          //                     agent->setLinearVelocity(vec3(0,0,0));
          //                }
          //                if(agent->opt_ints[2] % 100 == 0)
          //                     agent->faceTo(agent->getPosition() + vec3(randf(-5,5), 0, randf(-5,5)));
          //                agent->opt_ints[2]++;
          //                return true;
          //           } else {
          //                // Pick new random goal
          //                vec3 new_goal = agent->getPosition() + agent->facing() * randf(10, 100);
          //                crumbs[id][0] = scene->create<Single>("crumb");
          //                crumbs[id][0]->setPhysicsState(P_State::NONE);
          //                crumbs[id][0]->setPosition(new_goal);
          //                agent->opt_ints[2] = 0;
          //           }
          //           }

          //           float desired_speed = agent->opt_floats[1];
          //           float repel_radius = 8.0f;
                    
          //           // Query nearby objects
          //           BoundingBox query_bounds = agent->getWorldBounds();
          //           query_bounds.expand(BoundingBox(
          //               agent->getPosition() - vec3(repel_radius),
          //               agent->getPosition() + vec3(repel_radius)
          //           ));
          //           list<g_ptr<S_Object>> queryResults;
          //           phys->queryTree(phys->treeRoot3d, query_bounds, queryResults);
          //           list<g_ptr<Single>> nearby;
          //           for(auto q : queryResults) {
          //               if(g_ptr<Single> obj = g_dynamic_pointer_cast<Single>(q)) {
          //                   if(obj->opt_floats.length() >= 1)
          //                       nearby << obj;
          //               }
          //           }
                    
          //           for(auto c : my_crumbs) {
          //               if(std::abs(c->opt_floats[0]) >= 10.0f) {
          //                   nearby << c;
          //               }
          //           }
                    
          //           // Compute field from attractors/repulsors
          //           vec3 net_force(0, 0, 0);
          //           for(auto obj : nearby) {
          //               if(obj == agent) continue;
          //               if(obj->opt_floats[0] == 0) continue;
                        
          //               float strength = obj->opt_floats[0];
          //               vec3 to_obj = agent->direction(obj).nY();
          //               float dist = agent->distance(obj);
                        
          //               vec3 direction = (strength > 0) ? to_obj : to_obj*-1;
          //               float influence = abs(strength) / (dist * dist + 0.1f);
          //               net_force += direction * influence;
          //           }
                    
          //           // If surrounded by obstacles, switch to gap-finding
          //           int obstacle_count = queryResults.length();
          //           if(obstacle_count > 3) {
          //               vec3 forward = agent->facing().nY().normalized();
          //               vec3 right = vec3(forward.z(), 0, -forward.x());
                        
          //               float angle_step = 360.0f / std::min(obstacle_count * 2, 24); // Finer search when more crowded
          //               int num_samples = std::min(obstacle_count * 2, 12);
                        
          //               vec3 chosen_direction = forward;
          //               float best_openness = 0.0f;
                        
          //               for(int i = 0; i < num_samples; i++) {
          //                   float angle_deg = i * angle_step;
          //                   float angle_rad = angle_deg * 3.14159f / 180.0f;
                            
          //                   vec3 dir = (forward * cos(angle_rad) + right * sin(angle_rad)).normalized();
          //                   float hit_dist = phys->raycast(agent->getPosition(), dir, 10.0f);
                            
          //                   if(hit_dist > best_openness) {
          //                       best_openness = hit_dist;
          //                       chosen_direction = dir;
          //                   }
          //               }
                        
          //               net_force = chosen_direction;
          //           }
                    
          //           my_debug[0]->setPosition((agent->getPosition() + net_force).addY(5.0f));
                    
          //           // if(frame%600==0) {
          //           //      agent->opt_vec3_2 = vec3(0,0,0);
          //           // }
          //           if(agent->opt_vec3_2.length()<12.0f) {
          //                net_force += vec3(randf(-1, 1), 0, randf(-1, 1)) * 2.0f;
          //           }
          //           // print("PROGRESS: ",agent->opt_vec3_2.length());
          //           my_debug[1]->setPosition((agent->getPosition()+net_force).addY(5.0f));
                    
          //           // Apply movement
          //           if(net_force.length() > 0.01f) {
          //                agent->faceTo(agent->getPosition() + net_force);
          //                vec3 final_move = net_force.normalized() * desired_speed;
          //                agent->setLinearVelocity(final_move);
          //                agent->opt_vec3_2 += final_move;
          //           }
          //      }
          //      return true;
          // };

          // agent->physicsJoint = [agent, scene, &goals, &crumbs, &debug, phys]() {
          //      int id = agent->opt_ints[1];
          //      list<g_ptr<Single>>& my_crumbs = crumbs[id];
          //      list<g_ptr<Single>>& my_debug = debug[id];
          //      g_ptr<Single> goal_crumb = my_crumbs[0];
          //      vec3 goal = goal_crumb->getPosition();
          //      float goal_dist = agent->distance(goal_crumb);
          //      if(goal_dist<=1.0f) {
          //           if(agent->opt_ints[2]<650) {
          //                if(goal_crumb->isActive()) {
          //                     scene->recycle(goal_crumb);
          //                     agent->setLinearVelocity(vec3(0,0,0));
          //                }
          //                if(agent->opt_ints[2]%100==0)
          //                     agent->faceTo(agent->getPosition()+vec3(randf(-5,5),0,randf(-5,5)));
          //                agent->opt_ints[2]++;
          //                return true;
          //           } else {
          //                vec3 new_goal = agent->facing()*randf(10,100);
          //                my_crumbs[0] = scene->create<Single>("crumb");
          //                my_crumbs[0]->setPhysicsState(P_State::NONE);
          //                my_crumbs[0]->opt_floats[0] = 100.0f; //Set attraction
          //                my_crumbs[0]->opt_floats[1] = 1.0f;
          //                my_crumbs[0]->setPosition(new_goal);
          //                agent->opt_ints[2]=0;
          //           }
          //      }

          //      float desired_speed = agent->opt_floats[1];
          //      agent->opt_ints[3]++;
          //      int interval = 30;
          //      if(agent->opt_ints[3] % interval == 0) {
          //           vec3 last_pos = agent->opt_vec_3;
          //           float progress = agent->getPosition().distance(last_pos);
          //           agent->opt_vec_3 = agent->getPosition(); //Reset
          //           // print("PROG: ",progress);
          //           // if(progress<0.4f) {
          //           //      g_ptr<Single> frustrated = scene->create<Single>("crumb");
          //           //      frustrated->setPosition(agent->getPosition());
          //           //      frustrated->opt_floats[0] = -20.0f; // Repulsive!
          //           // } else {
          //           //      g_ptr<Single> trail_crumb = scene->create<Single>("crumb");
          //           //      trail_crumb->setPosition(agent->getPosition());
          //           //      trail_crumb->opt_floats[0] = 1.0f; // Mild initial attraction
          //           //      // my_crumbs << trail_crumb;
          //           // }
                
          //           float repel_radius = 8.0f;
          //           BoundingBox query_bounds = agent->getWorldBounds();
          //           query_bounds.expand(BoundingBox(
          //                agent->position - vec3(repel_radius),
          //                agent->position + vec3(repel_radius)
          //           ));
          //           list<g_ptr<S_Object>> queryResults;
          //           phys->queryTree(phys->treeRoot3d, query_bounds, queryResults);
          //           list<g_ptr<Single>> nearby;
          //           for(auto q : queryResults) {
          //                if(g_ptr<Single> obj = g_dynamic_pointer_cast<Single>(q)) 
          //                     nearby << obj;
          //           }

          //           for(auto c : my_crumbs) {
          //                if(c->opt_floats[0]>=10.0f) { //Only distantly remember attractive crumbs (and possibly repulsive as well)
          //                     nearby << c;
          //                }
          //           }
                    
          //           // Sort by attractiveness/distance priority
          //           nearby.sort([agent](g_ptr<Single> a, g_ptr<Single> b) {
          //                float a_priority = abs(a->opt_floats[0]) / std::max(agent->distance(a), 0.1f);
          //                float b_priority = abs(b->opt_floats[0]) / std::max(agent->distance(b), 0.1f);
          //                return a_priority > b_priority;
          //           });
               
          //           // First pass: accumulate desired direction from attractors only
          //           vec3 desired_direction(0, 0, 0);
          //           float attractor_weight = 0.0f;
               
          //           for(auto obj : nearby) {
          //                if(obj == agent) continue;
          //                float score = obj->opt_floats[0];
          //                if(score <= 0) continue; // Only attractors
                         
          //                vec3 to_obj = agent->direction(obj).nY();
          //                float dist = agent->distance(obj);
          //                float influence = score / std::max(dist, 0.1f); // Distance-weighted attraction
                         
          //                desired_direction += to_obj * influence;
          //                attractor_weight += influence;
          //           }
                    
          //           if(attractor_weight > 0.01f) {
          //                desired_direction = desired_direction.normalized() * std::min(attractor_weight, 5.0f); // Keep some magnitude
          //           }

          //           my_debug[0]->setPosition((agent->getPosition()+desired_direction).addY(5.0f));

          //           vec3 current_velocity = agent->getVelocity().position;
          //           float actual_speed = current_velocity.length();
          //           float speed_ratio = actual_speed / std::max(desired_speed, 0.1f);
         
          //           bool placed_rating = false;
          //           for(auto obj : nearby) {
          //                if(obj == agent) continue;
          //               // if(obj->dtype != "crumb") continue; // Only rate crumbs
                         
          //                float dist = agent->distance(obj);
          //                if(dist > 3.0f) continue; // Only rate nearby crumbs
          //                placed_rating = true;
                         
          //                float score = obj->opt_floats[0];
          //                vec3 to_crumb = agent->direction(obj).nY();
                         
          //                // Are we moving toward this crumb?
          //                float alignment = current_velocity.length() > 0.1f ? 
          //                     current_velocity.normalized().dot(to_crumb) : 0.0f;
                         
          //                if(alignment > 0.5f) { // Moving toward it
          //                     if(speed_ratio > 0.7f) {
          //                          obj->opt_floats[0] = std::min(score + 1.0f, 100.0f);
          //                     } else {
          //                          obj->opt_floats[0] = std::max(score - 1.0f, -100.0f);
          //                     }
          //                }
          //                }

          //           if(!placed_rating) {  // No nearby crumbs - place new one
          //                g_ptr<Single> new_crumb = scene->create<Single>("crumb");
          //                new_crumb->setPosition(agent->getPosition());
          //                new_crumb->opt_floats[0] = (speed_ratio > 0.7f) ? 10.0f : -10.0f;
          //           }



          //           //print(id,": Desired direction: ",desired_direction.to_string());
               
          //           // Second pass: let repulsors deflect from desired direction
          //           vec3 net_force = desired_direction;
          //           float force_budget = 1.0f;
                    
          //           for(auto obj : nearby) {
          //                if(obj == agent) continue;
          //                float score = obj->opt_floats[0];
          //                if(score >= 0) continue; // Only repulsors
                         
          //                // Distance to surface, not center!
          //                BoundingBox objBounds = obj->getWorldBounds();
          //                float dist = objBounds.distance(agent->getPosition());
          //                dist = std::max(dist, 0.1f); // Avoid division by zero
                         
          //                // Direction away from closest point on surface
          //                vec3 closest = objBounds.closestPoint(agent->getPosition());
          //                vec3 away_from_obj = (agent->getPosition() - closest).normalized().nY();
                         
          //                float repulsion_strength = abs(score) / (dist * dist);
                         
          //                net_force += away_from_obj * repulsion_strength * force_budget;
          //                force_budget *= 0.8f;
          //           }

          //           if(net_force.length() > 5.0f) {
          //                net_force = net_force.normalized() * 5.0f;
          //           }

          //           my_debug[1]->setPosition((agent->getPosition()+net_force).addY(5.0f));

          //           // print(id,": Net force: ",net_force.to_string());

          //           // Add exploration noise to net_force BEFORE normalization
          //           float exploration_strength = (speed_ratio < 0.5f) ? 1.0f : 0.3f;
          //           vec3 exploration(randf(-1.0f, 1.0f), 0, randf(-1.0f, 1.0f));
          //           vec3 environmental_force = (net_force + exploration * exploration_strength);
          //           //my_debug[2]->setPosition((agent->getPosition()+environmental_force).addY(5.0f));
          //           // Only proceed if we have a force
          //           if(environmental_force.length() > 0.01f) {
          //                environmental_force = environmental_force.normalized();
                         
          //                // Blend with momentum (if we're already moving)
          //                vec3 final_direction;
          //                if(current_velocity.length() > 0.1f) {
          //                     final_direction = (current_velocity.normalized() * 0.7f + environmental_force * 0.3f).normalized();
          //                } else {
          //                     final_direction = environmental_force;
          //                }
          //                //print(id,": Final direction: ",net_force.to_string());
                         
          //                vec3 velocity = final_direction * desired_speed;
          //                agent->faceTo(agent->getPosition() + velocity);
          //                agent->setLinearVelocity(velocity);
          //           }
          //      }
               
          //      return true;
          // };
     }
     vec3 cam_pos(width/2,50,(amt*1.2)/width);
     scene->camera.setPosition(cam_pos);

     // g_ptr<Single> test_origin = make<Single>(make<Model>(makeTestBox(1.0f)));
     // scene->add(test_origin);
     // test_origin->setPosition(vec3(0, 0.5, 0));
     // test_origin->setPhysicsState(P_State::NONE);
     // test_origin->setColor({1, 0, 0, 1}); // Red marker

     list<g_ptr<Single>> ray_markers;
     for(int i = 0; i < 1000; i++) {
          g_ptr<Single> marker = make<Single>(c_model);
          scene->add(marker);
          marker->setPhysicsState(P_State::NONE);
          marker->scale({0.5f,12,0.5f});
          ray_markers << marker;
          scene->deactivate(marker);
     }

     g_ptr<Thread> run_thread = make<Thread>();
     run_thread->run([&](ScriptContext& ctx){
          phys->updatePhysics();
     },0.008f);
     run_thread->logSPS = true;

     S_Tool s_tool;
     s_tool.log_fps = true;
     start::run(window,d,[&]{
          s_tool.tick();
          goal = scene->getMousePos();

          int used_markers = 0;
          for(int i=0;i<grid->cells.length();i++) {
               if(!grid->cells[i].empty()) {
                    for(auto idx : grid->cells[i]) {
                         ray_markers[used_markers++]->setPosition(grid->indexToLoc(i));
                    }
               }
          }
          for(int i=0;i<ray_markers.length();i++) {
               if(i<=used_markers) {
                    scene->reactivate(ray_markers[i]);
               } else if(ray_markers[i]->isActive()) {
                    scene->deactivate(ray_markers[i]);
               }
          }

          // list<vec3> test_dirs = {
          //      vec3(1,0,0), vec3(1,0,1).normalized(), vec3(0,0,1), 
          //      vec3(-1,0,1).normalized(), vec3(-1,0,0), vec3(-1,0,-1).normalized(),
          //      vec3(0,0,-1), vec3(1,0,-1).normalized()
          //  };
           
          //  vec3 test_pos = test_origin->getPosition();
          //  for(int i = 0; i < test_dirs.length(); i++) {
          //      float hit_dist = phys->raycast(test_pos, test_dirs[i], 10.0f);
          //      vec3 hit_point = test_pos + test_dirs[i] * hit_dist;
          //      ray_markers[i]->setPosition(hit_point);
          //  }

     });
     return 0;
}