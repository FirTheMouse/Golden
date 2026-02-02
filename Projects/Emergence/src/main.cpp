#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>


using namespace Golden;
using namespace helper;

g_ptr<Scene> scene;
ivec2 win(2560,1536);

vec3 gridToTorus(int i, int width, int height, float mR, float mr) {
     int col = i % width;
     int row = i / width;
     float u = (col / (float)width) * 2.0f * M_PI;
     float v = (row / (float)height) * 2.0f * M_PI;
     vec3 pos;
     pos.x((mR + mr * cos(v)) * cos(u));
     pos.y(mr * sin(v)); 
     pos.z((mR + mr * cos(v)) * sin(u));
     
     return pos;
 }

int main() {



     std::string MROOT = root()+"/Projects/Emergence/assets/models/";
     std::string IROOT = root()+"/Projects/Emergence/assets/images/";
     std::string SROOT = root()+"/Projects/Emergence/src/";
     Window window = Window(win.x()/2, win.y()/2, "Emergence 0.3");
     scene = make<Scene>(window,1);
     Data d = make_config(scene,K);
     scene->tickEnvironment(1000);
     scene->background_color = vec4(0,0,0,1);
     scene->camera.toIso();

     // Eigen::run_mnist(scene,1000);

     g_ptr<Text> twig = nullptr;

     list<g_ptr<Quad>> squares;
     list<g_ptr<Single>> boxes;

     scene->enableInstancing();
   


     int grid_size = 400;
     int width = grid_size;
     int height = grid_size;
     list<int> grid;
     for(int i=0;i<(width*height);i++) {
          grid << randi(0,1);
     }


     scene->camera.setPosition(vec3(50,50,50));


     // for(int i = 0; i < grid.length(); i++) {
     //     int x = i % width;
     //     int y = i / width;
     //     int dx = x - width/2;
     //     int dy = y - height/2;
     //     int r = 4; //radius
     //     if(dx*dx + dy*dy <= r*r) {
     //         grid[i] = 16;  // inside blob
     //     } else {
     //         grid[i] = 0;  // outside blob
     //     }
     // }

     bool toggle = true;
     bool shuffle_neighbours = true;

     // ORDERS
          int max_order = 3;
          list<std::function<void(list<int>&)>> order_funcs;
          int order = 0; list<std::string> order_types;

          order_funcs << [](list<int>& iterate_order){
               //Do nothing, forward order
          };
          order_types << "FORWARD";

          order_funcs << [](list<int>& iterate_order){
               list<int> n_ran;
               for(int i=iterate_order.length()-1;i>=0;i--) {
                    n_ran << iterate_order[i];
               }
               iterate_order = n_ran;
          };
          order_types << "REVERSE";

          order_funcs << [](list<int>& iterate_order){
               iterate_order.shuffle();
          };
          order_types << "RANDOM";

          order_funcs << [](list<int>& iterate_order){
               list<int> first_ran;
               for(int i=iterate_order.length()-1;i>=iterate_order.length()/2;i--) {
               first_ran << iterate_order[i];
               }
               list<int> second_ran;
               for(int i=0;i<=iterate_order.length()/2;i++) {
               second_ran << iterate_order[i];
               }
               first_ran << second_ran;
               iterate_order = first_ran;
          };
          order_types << "SPLIT";

     //BIASES
          int max_bias = 4;
          int bias = 0; list<std::string> bias_types;
          list<std::function<void(list<int>&)>> bias_funcs;

          bias_funcs << [](list<int>& flips){
               //Do nothing, no bias
          };
          bias_types << "NONE";

          bias_funcs << [&](list<int>& flips){
               flips.sort([&](int a, int b) {return grid[a] >= grid[b];});
          };
          bias_types << "TOWARDS ENERGY AND EQUAL";

          bias_funcs << [&](list<int>& flips){
               flips.sort([&](int a, int b) {return grid[a] > grid[b];});
          };
          bias_types << "TOWARDS ENERGY";

          bias_funcs << [&](list<int>& flips){
               flips.sort([&](int a, int b) {return grid[a] == grid[b];});
          };
          bias_types << "TOWARDS EQUAL";

          bias_funcs << [&](list<int>& flips){
               flips.sort([&](int a, int b) {return grid[a] < grid[b];});
          };
          bias_types << "AWAY FROM ENERGY";

     //FREEDOMS
          int max_freedom = 2;
          int freedom = 0; list<std::string> freedom_types;
          list<std::function<void(int,list<int>&)>> freedom_funcs;

          freedom_funcs << [&](int i, list<int>& neighbours){
               int row = i / width;
               int col = i % width;
               
               // Wrap vertically and horizontally (torus topology)
               int up = ((row - 1 + height) % height) * width + col;
               int down = ((row + 1) % height) * width + col;
               int left = row * width + ((col - 1 + width) % width);
               int right = row * width + ((col + 1) % width);
               
               neighbours << up << down << left << right;
               
               // Diagonal wrapping
               int up_left = ((row - 1 + height) % height) * width + ((col - 1 + width) % width);
               int up_right = ((row - 1 + height) % height) * width + ((col + 1) % width);
               int down_left = ((row + 1) % height) * width + ((col - 1 + width) % width);
               int down_right = ((row + 1) % height) * width + ((col + 1) % width);
               
               neighbours << up_left << up_right << down_left << down_right;
          };
          freedom_types << "TORODIAL";

          freedom_funcs << [&](int i, list<int>& neighbours){
               int row = i / width;
               int col = i % width;
               if(row > 0) neighbours << i - width;      // up
               if(row < height-1) neighbours << i + width; // down
               if(col > 0) neighbours << i - 1;          // left
               if(col < width-1) neighbours << i + 1;    // right

               if(row > 0 && col > 0) neighbours << i - width - 1;
               if(row > 0 && col < width-1) neighbours << i - width + 1;
               if(row < height-1 && col > 0) neighbours << i + width - 1;
               if(row < height-1 && col < width-1) neighbours << i + width + 1;
          };
          freedom_types << "8 DEGREES";

          freedom_funcs << [&](int i, list<int>& neighbours){
               int row = i / width;
               int col = i % width;
               if(row > 0) neighbours << i - width;      // up
               if(row < height-1) neighbours << i + width; // down
               if(col > 0) neighbours << i - 1;          // left
               if(col < width-1) neighbours << i + 1;    // right
          };
          freedom_types << "4 DEGREES";

     auto run = [&](){
          list<list<int>> desired_flips(width*height);
          list<int> iterate_order;
          for(int i=0;i<grid.length();i++) {
               iterate_order << i;
          }

          order_funcs[order](iterate_order); //Order

          for(int i : iterate_order) {
               int value = grid[i];
               list<int> neighbours;
               if(value>0) {
                    freedom_funcs[freedom](i,neighbours);
               }

               if(shuffle_neighbours)
                    neighbours.shuffle();

               if(desired_flips.length()<i)
                    desired_flips[i] = neighbours;
               else
                    desired_flips << neighbours;
          }

          for(int i : iterate_order) {
               list<int> flips = desired_flips[i];
               bias_funcs[bias](flips);
               for(int to : flips) {
                    //Incremental
                         if(grid[i]>0) {
                              grid[i] = grid[i]-1;
                              grid[to] = grid[to]+1;
                         }                         
               }
          }
     };

     vec4 red = vec4(1,0,0,1);
     vec4 black = vec4(0,0,0,1);

     bool render2d = true;
     bool oneD2d = false;
     bool torus_mode = true;
     bool enable_color = true;

     float tile_x = (win.x())/width;
     float tile_y = (win.y())/height;

     g_ptr<Geom> geom = make<Geom>();
     for(int i=0;i<(width*height);i++) {
          auto q = make<Quad>(geom);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          q->scale({tile_x,tile_y});
          q->setColor(black);
          if(!oneD2d)
               q->setPosition(vec2((q->ID%width)*tile_x,(q->ID/width)*tile_y));
          else
               q->setPosition(vec2(q->ID*tile_x,0));
          squares << q;
     }

     int cam_type = 0;
     float s_scale = 0.5f;
     float mR_factor = 1.5f;
     float mr_factor = 4.0f;

     float mR = (width * s_scale) / (mR_factor * M_PI); 
     float mr = (height * s_scale) / (mr_factor * M_PI);

     g_ptr<Model> model = make<Model>(makeTestBox(s_scale-0.01f)); 
     for(int i=0;i<(width*height);i++) {
          auto s = make<Single>(model);
          scene->add(s);
          s->setPhysicsState(P_State::NONE);
          s->setColor({1,0,0,1});
          if(torus_mode) {
               s->setPosition(gridToTorus(i, width, height, mR, mr));
               } else {
               s->setPosition(vec3((s->ID%width)*s_scale, 0, (s->ID/width)*s_scale));
               }
          boxes << s;
     }

     if(render2d) {
          for(auto b : boxes) scene->active[b->ID] = false;
          for(auto s : squares) scene->quadActive[s->ID] = true;
     } else {
          for(auto b : boxes) scene->active[b->ID] = true;
          for(auto s : squares) scene->quadActive[s->ID] = false;
     }

     //Log::Line l; l.start();

     g_ptr<Thread> run_thread = make<Thread>();
     run_thread->run([&](ScriptContext& ctx){
          run();
     },0.016f);


     list<list<int>> for_csv;
     list<int> record_indexes;
     //Give random record indexes
     for(int i=0;i<3;i++) {
          int idx = randi(0,(width*height)-1);
          int depth = 0;
          while(record_indexes.has(idx)&&depth<1000) {
               idx = randi(0,(width*height)-1);
               depth++;
          }
          if(depth>=999) print("WARNING TRYING TO RECORD MORE INDEXES THAN POSITIONS IN THE GRID!");
          record_indexes << idx;
          for_csv << list<int>();
     }


     int display_threshold = 2;
     int current_tick = 0;
     vec2 a_mov;
     float sensitivity = 8.0f;
     S_Tool s_tool;
     s_tool.log_fps = true;
     start::run(window,d,[&]{
          s_tool.tick();
          //l.end(); l.start();
          // run();
          // print("Time 1: ",l.end()/1000000,"ms "); l.start();
          int total = 0;
          int highest_val = 0;
          for(int i=0;i<grid.length();i++) {
                    if(i>=grid.length()) break;
                    if(render2d) {
                         if(i>=scene->quadActive.length()) break;
                         scene->quadActive[i] = grid[i]>=display_threshold;
                    } else {
                         if(i>=scene->active.length()) break;
                         scene->active[i] = grid[i]>=display_threshold;
                    }
                    if(grid[i]>=display_threshold) {
                         vec4 color = black;
                         if(grid[i]>=display_threshold) {
                              color = vec4(0,0,((float)grid[i]+0.2f)/4,1);
                              if(grid[i]>3) 
                                   color = vec4(((float)grid[i]-3)/4,0,0,1);
                              if(grid[i]>10) 
                                   color = vec4(1,1,0,1);
                         }
                         if(display_threshold==1&&grid[i]==display_threshold) {
                              color = vec4(0.0f,0.3f,0.0f,1);
                         }
                         if(grid[i]>highest_val) highest_val = grid[i];

                         if(render2d) {
                              scene->guiColor[i] = color;
                         }
                         else {
                              if(torus_mode) {
                                   // For torus, move along the normal direction
                                   vec3 base_pos = gridToTorus(i, width, height, mR, mr);
                                   
                                   // Calculate normal (points outward from torus surface)
                                   int col = i % width;
                                   int row = i / width;
                                   float u = (col / (float)width) * 2.0f * M_PI;
                                   float v = (row / (float)height) * 2.0f * M_PI;
                                   
                                   vec3 normal;
                                   normal.x(cos(v) * cos(u));
                                   normal.y(sin(v));
                                   normal.z(cos(v) * sin(u));
                                   normal = normal.normalized();
                                   
                                   scene->transforms[i][3] = vec4(base_pos + normal * (grid[i] * 0.1f), 1.0f).toGlm();
                              }
                              else {
                                   scene->transforms[i][3].y = grid[i]*3;
                              }
                         }
                    } 
                    total+=grid[i];

               int record_idx = record_indexes.find(i);
               if(record_idx!=-1) {
                    for_csv[record_idx] << grid[i];
               }

               //if(i%width==0) print(); printnl(grid[i]);
               //squares[r*grid.length()+c]->setColor(grid[c][r]==1?vec4(1,0,0,1):vec4(0,0,0,0));
          }
          current_tick++;

          if(render2d) {
               vec2 mov = input_2d_keys(sensitivity)*-1;
               a_mov += mov;
               if(mov.length()!=0)
                    for(auto b : squares) {
                         if(!oneD2d)
                              b->setPosition(vec2((b->ID%width)*tile_x, (b->ID/width)*tile_y) + a_mov);
                         else
                              b->setPosition(vec2(b->ID*tile_x,0) + a_mov);
                    }
          }
           

          //KEYBINDS:
          //F = Increase display threshold (hold shift to decrease)
          //R = Reset
          //Up/Down = shift through orders, hold shift to shift through freedoms
          //Left/Right = shift through biases
          //E = Speed up the simulation (hold shift to slow it down)
          //Q = Zoom in (hold shift to zoom out)
          //WASD = Pan
          //H = Print highest value
          //C = Print data and write to the CSV
          //T = Current confqiuration
          //M = if 3d, change mode
          //X = switch between 2d and 3d
          //Z = color
          //I and O = Changing torus factors

          //I = changing mR factors
          if(pressed(I)||pressed(O)) {
               if(pressed(I)) {
                    if(held(LSHIFT))
                         mR_factor+=0.05f;
                    else
                         mR_factor-=0.05f;
                    mR = (width * s_scale) / (mR_factor * M_PI); 
                    print("mR factor: ",mR_factor);
               }
               if(pressed(O)) {
                    if(held(LSHIFT))
                         mr_factor+=0.2f;
                    else
                         mr_factor-=0.2f;  
                    mr = (height * s_scale) / (mr_factor * M_PI);
                    print("mr factor: ",mr_factor);
               }
          }
          if(pressed(P)) {
               oneD2d = !oneD2d;
          }

          if(pressed(Z)) {
               enable_color = !enable_color;
               if(enable_color) {
                    print("Enabling colors");
               } else {
                    print("Disabling colors");
                    for(auto s : squares) s->setColor(vec4(0,0,0.6f,1));
               }
          }


          if(pressed(E)) {
               if(held(LSHIFT)) {
                    run_thread->setSpeed(run_thread->getSpeed()/2); 
               } else {
                    if(run_thread->getSpeed()==0) 
                         run_thread->setSpeed(1.0f);
                    run_thread->setSpeed(run_thread->getSpeed()*2); 
               }
               print("Thread speed to: ",run_thread->getSpeed());
          }
          if(pressed(Q)) {
               if(render2d) {
                    vec2 view_center = (a_mov*-1) + vec2(win.x()/2, win.y()/2);
                    vec2 grid_pos_before = view_center / vec2(tile_x, tile_y);
                    
                    if(held(LSHIFT)) {
                    tile_x = tile_x/2;
                    tile_y = tile_y/2;
                    sensitivity/=2;
                    } else {
                    tile_x = tile_x*2;
                    tile_y = tile_y*2;
                    sensitivity*=2;
                    }
     
                    vec2 grid_pos_after = grid_pos_before * vec2(tile_x, tile_y);
                    a_mov = a_mov + (view_center - grid_pos_after);
                    
                    // Reposition all quads
                    for(int i=0; i<(width*height); i++) {
                    if(i<squares.length()) {
                         auto q = squares[i];
                         q->scale({tile_x, tile_y});
                         if(!oneD2d)
                              q->setPosition(vec2((q->ID%width)*tile_x, (q->ID/width)*tile_y) + a_mov);
                         else
                              q->setPosition(vec2(q->ID*tile_x,0) + a_mov);
                    }
                    }
                }
                else {
                    if(held(LSHIFT)) {
                         scene->camera.setPosition(vec3(0,50,0));
                    }
                    else {
                         cam_type++;
                         if(cam_type==0)
                              scene->camera.toIso();
                         else if(cam_type==1)
                              scene->camera.toOrbit();
                         else {
                              cam_type = 0;
                              scene->camera.toIso();
                         }
                    }
                }
          }
          if(pressed(M)) { 
               if(!render2d) {
                   torus_mode = !torus_mode;
                   for(int i = 0; i < boxes.length(); i++) {
                       if(torus_mode) {
                         boxes[i]->setPosition(gridToTorus(i, width, height, mR, mr));
                       } else {
                           boxes[i]->setPosition(vec3((i%width)*s_scale, 0, (i/width)*s_scale));
                       }
                   }
                   
                   print(torus_mode ? "TORUS MODE" : "FLAT GRID MODE");
               }
          }
          if(pressed(X)) {
               render2d = !render2d;
               if(render2d) {
                    for(auto b : boxes) scene->active[b->ID] = false;
                    for(auto s : squares) scene->quadActive[s->ID] = true;
               } else {
                    for(auto b : boxes) scene->active[b->ID] = true;
                    for(auto s : squares) scene->quadActive[s->ID] = false;
               }
          }
          if(pressed(H)) {
               print("Highest value: ",highest_val);
          }
          if(pressed(T)) {
               print("---------------------------------");
               print("Current tick: ",current_tick," total: ",total);
               print("ORDER: ",order_types[order]);
               print("BIAS: ",bias_types[bias]);
               print("FREEDOM: ",freedom_types[freedom]);
               print("SHUFFLE NEIGHBORS: ",shuffle_neighbours?"TRUE":"FALSE");
               print("---------------------------------");
          }
          if(pressed(C)) {
               std::string csv = "";
               
               int max_len = 0;
               for(auto& row : for_csv) {
                    max_len = std::max(max_len, (int)row.length());
               }
               
               for(int col = 0; col < max_len; col++) {
                    for(int row = 0; row < for_csv.length(); row++) {
                         if(col < for_csv[row].length()) {
                              csv.append(std::to_string(for_csv[row][col]));
                         }
                         if(row < for_csv.length() - 1) {
                              csv.append(",");
                         }
                    }
                    csv.append("\n");
               }
               
               print("------------CSV-----------");
               print(csv);
               writeFile(IROOT+"data.csv",csv);
          }
          if(pressed(F)) {
               if(held(LSHIFT)) {
                    if(display_threshold>1)
                         display_threshold--;
               }
               else {
                    if(display_threshold<=12)
                         display_threshold++;
               }
          }
          if(pressed(R)) {
               // for(int i=0;i<(width*height);i++) {
               //      grid = randi(0,1);
               // }

               for(int i = 0; i < grid.length(); i++) {
               int x = i % width;
               int y = i / width;
               int dx = x - width/2;
               int dy = y - height/2;
               int r = 4; //radius
               if(dx*dx + dy*dy <= r*r) {
                    grid[i] = 16;  // inside blob
               } else {
                    grid[i] = 0;  // outside blob
               }
               }
          }
          if(pressed(LEFT)||pressed(RIGHT)) {
               if(pressed(LEFT)) {
                    if(bias<=0) 
                         bias = max_bias;
                    else bias--;
               }
               if(pressed(RIGHT)) {
                    if(bias>=max_bias) 
                         bias = 0;
                    else bias++;
               }
               print("BIAS: ",bias_types[bias]);
          }
          if((pressed(UP)||pressed(DOWN))&&!held(LSHIFT)) {
               if(pressed(DOWN)) {
                    if (order<=0) {
                         order = max_order;
                    } else {
                         order--;
                    }
               }
               if(pressed(UP)) {
                    if(order>=max_order) {
                         order = 0;
                    } else {
                         order++;
                    }
               }
               print("ORDER: ",order_types[order]);
          }
          if((pressed(UP)||pressed(DOWN))&&held(LSHIFT)) {
               max_freedom = 2; //Total ammount of switch cases
               if(pressed(DOWN)) {
                    if (freedom<=0) {
                         freedom = max_freedom;
                    } else {
                         freedom--;
                    }
               }
               if(pressed(UP)) {
                    if(freedom>=max_freedom) {
                         freedom = 0;
                    } else {
                         freedom++;
                    }
               }
               print("FREEDOM: ",freedom_types[freedom]);
          }

          if(pressed(NUM_1)) {
               freedom = 0; //Torodial
               order = 0; //Forward
               bias = 1; //Towards energy
               shuffle_neighbours = true;
          }
          if(pressed(NUM_2)) {
               freedom = 0; //Torodial
               order = 2; //Random
               bias = 1; //Towards energy
               shuffle_neighbours = true;
          }
          if(pressed(NUM_3)) {
               freedom = 0; //Torodial
               order = 0; //Forward
               bias = 1; //Towards energy
               shuffle_neighbours = false;
          }

          if(pressed(U)) {
               shuffle_neighbours = !shuffle_neighbours;
               if(shuffle_neighbours) print("SHUFFLING NEIGHBOURS");
               else print("DISABLED NEIGHBOUR SHUFFLING");
          }


          //print("Time 2: ",l.end()/1000000,"ms "); l.start();
     } );
}