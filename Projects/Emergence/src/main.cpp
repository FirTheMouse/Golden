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
     Window window = Window(win.x()/2, win.y()/2, "Emergence 0.2");
     scene = make<Scene>(window,1);
     Data d = make_config(scene,K);
     scene->tickEnvironment(1000);
     scene->background_color = vec4(0,0,0,1);
     scene->camera.toIso();

     g_ptr<Text> twig = nullptr;

     list<g_ptr<Quad>> squares;
     list<g_ptr<Single>> boxes;

     scene->enableInstancing();
     Physics phys(scene);


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
     //     int r = 3; //radius
     //     if(dx*dx + dy*dy <= r*r) {
     //         grid[i] = 8;  // inside blob
     //     } else {
     //         grid[i] = 0;  // outside blob
     //     }
     // }

     bool toggle = true;
     int order = 0;
     int bias = 0;
     int max_order = 2;
     int max_bias = 2;
     bool shuffle_neighbours = true;

     std::function<void(list<int>&)> order_func = [](list<int>& iterate_order){
          //Do nothing, forward order
     };
     std::function<void(list<int>&)> bias_func = [](list<int>& flips){
          //Do nothing, no bias
     };
     std::function<void(int,list<int>&)> direction_func = [](int i, list<int>& neighbours){
          //Do nothing, forward order
     };

     auto run = [&](){
          list<list<int>> desired_flips(width*height);
          list<int> iterate_order;
          for(int i=0;i<grid.length();i++) {
               iterate_order << i;
          }

          order_func(iterate_order); //Order

          
          //Priority order
               //iterate_order.sort([&](int a, int b) {return grid[a] > grid[b];}); //Towards high energy
               //iterate_order.sort([&](int a, int b) {return grid[a] < grid[b];}); //Towards low energy

          //Sanity check
               // for(int i=0;i<10;i++) {
               //     printnl(grid[iterate_order[i]],":");
               // }
               // print();

          for(int i : iterate_order) {
               int value = grid[i];
               list<int> neighbours;
               if(value>0) {
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
               }

               if(shuffle_neighbours)
                    neighbours.shuffle();
               desired_flips[i] = neighbours;
          }

               for(int i : iterate_order) {
                    list<int> flips = desired_flips[i];
                    bias_func(flips);
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
     vec4 black = vec4(0,0,0,0);

     #define enable_quads 1
     #define enable_singles 0


     #if enable_quads
          float tile_x = (win.x())/width;
          float tile_y = (win.y())/height;

          g_ptr<Geom> geom = make<Geom>();
          // Script<> make_char("make_char",[&](ScriptContext& ctx){
          //     auto q = make<Quad>(geom);
          //     scene->add(q);
          //     float tile_x = (win.x())/width;
          //     float tile_y = (win.y())/height;
          //     q->setPhysicsState(P_State::NONE);
          //     q->scale({tile_x,tile_y});
          //     q->setColor(black);
          //     q->setPosition(vec2((q->ID%width)*tile_x,(q->ID/width)*tile_y));
          //     ctx.set<g_ptr<Object>>("toReturn",q);
          // });
          //scene->define("squares",make_char);

          for(int i=0;i<(width*height);i++) {
               //auto q = scene->create<Quad>("squares");
               auto q = make<Quad>(geom);
               scene->add(q);
               q->setPhysicsState(P_State::NONE);
               q->scale({tile_x,tile_y});
               q->setColor(black);
               q->setPosition(vec2((q->ID%width)*tile_x,(q->ID/width)*tile_y));
               squares << q;
          }
     #endif

     #if enable_singles
          int cam_type = 0;
          bool torus_mode = false;
          float s_scale = 0.5f;
          float mR = (width * s_scale) / (2.0f * M_PI); 
          float mr = (height * s_scale) / (3.0f * M_PI);
          g_ptr<Model> model = make<Model>(makeTestBox(s_scale-0.01f)); 
          for(int i=0;i<(width*height);i++) {
               // auto q = scene->create<Quad>("squares");
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
     #endif

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

                    #if enable_quads
                         if(i>=scene->quadActive.length()) break;
                         scene->quadActive[i] = grid[i]>=display_threshold;
                    #endif
                    #if enable_singles
                         if(i>=scene->active.length()) break;
                         scene->active[i] = grid[i]>=display_threshold;
                    #endif
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
                         #if enable_quads
                              scene->guiData[i] = color;
                         #endif
                         #if enable_singles
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
                         #endif
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

          #if enable_quads
               vec2 mov = input_2d_keys(sensitivity)*-1;
               a_mov += mov;
               if(mov.length()!=0)
                    for(auto b : squares) {
                         b->setPosition(vec2((b->ID%width)*tile_x, (b->ID/width)*tile_y) + a_mov);
                    }
          #endif
           

          //KEYBINDS:
          //F = Increase display threshold (hold shift to decrease)
          //R = Reset
          //Up/Down = shift through orders
          //Left/Right = shift through biases
          //E = Speed up the simulation (hold shift to slow it down)
          //Q = Zoom in (hold shift to zoom out)
          //WASD = Pan
          //H = Print highest value
          //C = Print data and write to the CSV
          //T = Current tick and total units
          //M = if 3d, change mode


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
               #if enable_quads
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
                         q->setColor(black);
                         q->setPosition(vec2((q->ID%width)*tile_x, (q->ID/width)*tile_y) + a_mov);
                    }
                    }
                #endif
                #if enable_singles
                    if(held(LSHIFT)) {
                         scene->camera.setPosition(vec3(0,50,0));
                    }
                    else {
                         cam_type++;
                         if(cam_type>=2) cam_type=0;
                         if(cam_type==0)
                              scene->camera.toIso();
                         else if(cam_type==1)
                              scene->camera.toOrbit();
                         else if(cam_type==2)
                              scene->camera.toFly();
                    }
                #endif
          }
          if(pressed(M)) { 
               #if enable_singles
                   torus_mode = !torus_mode;
                   for(int i = 0; i < boxes.length(); i++) {
                       if(torus_mode) {
                         boxes[i]->setPosition(gridToTorus(i, width, height, mR, mr));
                       } else {
                           boxes[i]->setPosition(vec3((i%width)*s_scale, 0, (i/width)*s_scale));
                       }
                   }
                   
                   print(torus_mode ? "TORUS MODE" : "FLAT GRID MODE");
               #endif
           }
          if(pressed(H)) {
               print("Highest value: ",highest_val);
          }
          if(held(T)) {
               print("Current tick: ",current_tick," total: ",total);
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
               for(int i=0;i<(width*height);i++) {
                    grid = randi(0,1);
               }
          }
          if(pressed(G)) {
               // for(int i = 0; i < grid.length(); i++) {
               //     int x = i % width;
               //     int y = i / width;
               //     int dx = x - width/2;   
               //     int dy = y - height/2;
               //     int r = 8; //radius
               //     if((dx*dx + dy*dy) <= r*r) {
               //         grid[i] = 8;  // inside blob
               //     } 
               // }
          }

          if(pressed(LEFT)||pressed(RIGHT)) {
               max_bias = 2; //Total ammount of switch cases
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
               switch(bias) {
                    case 0:
                         bias_func = [&](list<int>& flips){
                              //Do nothing, no bias
                         };
                         print("NO BIAS");
                    break;
                    case 1:
                         bias_func = [&](list<int>& flips){
                              flips.sort([&](int a, int b) {return grid[a] >= grid[b];});
                         };
                         print("BIAS TOWARDS ENERGY");
                    break;
                    case 2:
                         bias_func = [&](list<int>& flips){
                              flips.sort([&](int a, int b) {return grid[a] < grid[b];});
                         };
                         print("BIAS AWAY FROM ENERGY");
                    break;
                    default:
                         break;
               }
          }
          if(pressed(UP)||pressed(DOWN)) {
               max_order = 4; //Total ammount of switch cases
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
               switch(order) {
                    case 0:
                         order_func = [](list<int>& iterate_order){
                              //Do nothing, forward order
                         };
                         print("FORWARD ORDER");
                    break;
                    case 1:
                         order_func = [](list<int>& iterate_order){
                              list<int> n_ran;
                              for(int i=iterate_order.length()-1;i>=0;i--) {
                                   n_ran << iterate_order[i];
                              }
                              iterate_order = n_ran;
                         };
                         print("REVERSE ORDER");
                    break;
                    case 2:
                         order_func = [](list<int>& iterate_order){
                              iterate_order.shuffle();
                         };
                         print("RANDOM ORDER");
                    break;
                    case 3:
                         order_func = [&](list<int>& iterate_order){
                              iterate_order.sort([&](int a, int b) {return grid[a] > grid[b];}); //Towards high energy
                         };
                         print("PRIORITY ORDER");
                    break;
                    case 4:
                         order_func = [](list<int>& iterate_order){
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
                         print("SPLIT ORDER");
                    break;
                    default:
                         break;
               }
          }

          if(pressed(U)) {
               shuffle_neighbours = !shuffle_neighbours;
               if(shuffle_neighbours) print("SHUFFLING NEIGHBOURS");
               else print("DISABLED NEIGHBOUR SHUFFLING");
          }
          //print("Time 2: ",l.end()/1000000,"ms "); l.start();
     } );
}