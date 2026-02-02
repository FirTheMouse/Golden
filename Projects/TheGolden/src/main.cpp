#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>
#include<util/ml_util.hpp>
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

// void benchmark_raycasts() {
//      print("\n========== RAYCAST BENCHMARK ==========\n");
     
//      // Setup test environment
//      grid = make<NumGrid>(0.5f, 1000.0f);
//      g_ptr<Physics> phys = make<Physics>();
//      Window window = Window(win.x()/2, win.y()/2, "Golden 0.0.5");
//      g_ptr<Scene> test_scene = make<Scene>(window, 2);
//      phys->scene = test_scene;
     
//      // Create maze walls in both structures
//      list<g_ptr<Single>> walls;
//      g_ptr<Model> wall_model = make<Model>(makeTestBox(1.0f));
     
//      for(int i = 0; i < 500; i++) {
//          g_ptr<Single> wall = make<Single>(wall_model);
//          test_scene->add(wall);
//          vec3 pos(randf(-50, 50), 1.5f, randf(-50, 50));
//          wall->setPosition(pos);
//          wall->setPhysicsState(P_State::FREE);
//          grid->addToGrid(wall->ID, wall->getWorldBounds());
//          walls << wall;
//      }
     
//      // Build tree once for static geometry
//      phys->buildTree3d();
     
//      // Test positions and directions
//      vec3 origins[10];
//      vec3 directions[10];
//      for(int i = 0; i < 10; i++) {
//          origins[i] = vec3(randf(-40, 40), 1.0f, randf(-40, 40));
//          directions[i] = vec3(randf(-1, 1), 0, randf(-1, 1)).normalized();
//      }

//      int hits_tree = 0, hits_grid = 0;
//      for(int i = 0; i < 10; i++) {
//      float tree_dist = phys->raycast(origins[i], directions[i], 10.0f);
//      float grid_dist = grid->raycast(origins[i], directions[i], 10.0f);
     
//      if(tree_dist < 10.0f) hits_tree++;
//      if(grid_dist < 10.0f) hits_grid++;
     
//      print("Ray ", i, " - Tree: ", tree_dist, " Grid: ", grid_dist);
//      }
//      print("Tree hits: ", hits_tree, " Grid hits: ", hits_grid);
     
//      Log::rig r;
     
//      // Tree raycast
//      r.add_process("tree_raycast", [&](int i) {
//          int idx = i % 10;
//          phys->raycast(origins[idx], directions[idx], 10.0f);
//      });
     
//      // Grid raycast
//      r.add_process("grid_raycast", [&](int i) {
//          int idx = i % 10;
//          grid->raycast(origins[idx], directions[idx], 10.0f);
//      });
     
//      // Grid raycast with exclusion
//      r.add_process("grid_raycast_with_exclusion", [&](int i) {
//          int idx = i % 10;
//          grid->raycast(origins[idx], directions[idx], 10.0f, 0); // Exclude ID 0
//      });
     
//      // Tree raycast (fresh rebuild each time - worst case)
//      r.add_process("tree_rebuild_raycast", [&](int i) {
//           if(i<3) {
//                phys->treeBuilt3d = false;
//                phys->buildTree3d();
//           }
//          int idx = i % 10;
//          phys->raycast(origins[idx], directions[idx], 10.0f);
//      }, 1); // Separate table
     
//      r.add_comparison("grid_raycast", "tree_raycast");
//      r.add_comparison("grid_raycast_with_exclusion", "grid_raycast");
//      r.add_comparison("tree_rebuild_raycast", "tree_raycast");
     
//      r.run(1000, true, 1000); // 1000 iterations, warm cache, 1000 operations each
//  }
 

void make_maze(g_ptr<Model> wall_model) {
     float maze_size = 50.0f;
     float wall_thickness = 1.0f;
     float wall_height = 1.0f;
     int grid_rows = 10;
     int grid_cols = 10;
     float cell_size = maze_size / grid_rows;

     // Helper to place wall segment
     auto place_wall = [&](vec3 pos, vec3 scale) {
          g_ptr<Single> wall = make<Single>(wall_model);
          scene->add(wall);
          wall->dtype = "wall";
          wall->setPosition(pos);
          wall->setScale(scale);
          wall->setPhysicsState(P_State::FREE);
          addToGrid(wall);
          wall->opt_floats << -2.0f; 
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
}

#define GTIMERS 1
#define CRUMB_ROWS 32  // <- Tweak this to test different sizes

struct crumb {
     g_ptr<Single> form = nullptr;
     float mat[CRUMB_ROWS][10];

     inline float* data() const {
          return (float*)&mat[0][0];
     }
     
     inline int rows() const {
          return CRUMB_ROWS;
     }
};

static float compute_trace(const float* matrix) {
     float trace = 0.0f;
     for(int i = 0; i < CRUMB_ROWS; i++) {
         trace += matrix[i * 10 + i];
     }
     return trace;
}

static void matmul(const float* eval, const float* against, float* result) {
     cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                CRUMB_ROWS, 10, 10,
                1.0f,
                eval, 10,
                against, 10,
                0.0f,
                result, 10);
}

static float evaluate_elementwise(const crumb& agent, const crumb& item) {
    float score = 0.0f;
    
    for(int i = 0; i < CRUMB_ROWS; i++) {
        for(int j = 0; j < 10; j++) {
            score += agent.mat[i][j] * item.mat[i][j];
        }
    }
    
    return score;
}

static float evaluate(const crumb& agent, const crumb& item) {
    return evaluate_elementwise(agent, item);
}

static void apply_mask(crumb& target, const crumb& mask) {
    for(int i = 0; i < CRUMB_ROWS; i++) {
        for(int j = 0; j < 10; j++) {
            target.mat[i][j] *= mask.mat[i][j];
        }
    }
}

static float evaluate_matmul(const crumb& eval, const crumb& against, crumb& result) {
    matmul(eval.data(), against.data(), result.data());
    return compute_trace(result.data());
}

static float evaluate_matmul(const crumb& eval, const crumb& against) {
    crumb result{};
    return evaluate_matmul(eval, against, result);
}

struct CategoryNode : public Object {
     std::string label;
     float archetype[CRUMB_ROWS][10];
     
     // Tree structure
     g_ptr<CategoryNode> parent;
     list<g_ptr<CategoryNode>> children;
     
     // Instances (only for leaf nodes)
     list<int> instance_indices;
     
     // Metadata
     float confidence = 1.0f;
     int observation_count = 0;
     
     CategoryNode(std::string _label) : label(_label) {
         // Zero archetype
         for(int i = 0; i < CRUMB_ROWS; i++)
             for(int j = 0; j < 10; j++)
                 archetype[i][j] = 0.0f;
     }
     
     float* get_archetype() {
         return &archetype[0][0];
     }
};

class crumb_manager : public Object {
public:
     crumb_manager() {
          places_root = make<CategoryNode>("places");
          items_root = make<CategoryNode>("items");
          agents_root = make<CategoryNode>("agents");
          circles_root = make<CategoryNode>("circles");
     };
     ~crumb_manager() {};

     list<crumb> crumbs;

     // Priority lists - reordered frequently, cheap to rebuild
     list<int> building_priority;
     list<int> agent_priority;

     g_ptr<CategoryNode> places_root = nullptr;
     g_ptr<CategoryNode> items_root = nullptr;
     g_ptr<CategoryNode> agents_root = nullptr;
     g_ptr<CategoryNode> circles_root = nullptr;

     void addCrumb(crumb& n_crumb) {crumbs << n_crumb;}

     // Find best matching category for a crumb
     g_ptr<CategoryNode> find_best_category(const crumb& crumb_obj, g_ptr<CategoryNode> root) {
         g_ptr<CategoryNode> current = root;
         
         while(!current->children.empty()) {
             float best_match = -1e9f;
             g_ptr<CategoryNode> best_child = nullptr;
             
             for(auto child : current->children) {
                 float match = evaluate_against_archetype(crumb_obj, child);
                 if(match > best_match) {
                     best_match = match;
                     best_child = child;
                 }
             }
             
             // Only descend if child is significantly better
             if(best_match > 0.5f) {  // Threshold
                 current = best_child;
             } else {
                 break;
             }
         }
         
         return current;
     }
     
     // Evaluate crumb against category archetype
     float evaluate_against_archetype(const crumb& crumb_obj, g_ptr<CategoryNode> category) {
         float score = 0.0f;
         float* archetype = category->get_archetype();
         
         for(int i = 0; i < CRUMB_ROWS; i++) {
             for(int j = 0; j < 10; j++) {
                 score += crumb_obj.mat[i][j] * archetype[i * 10 + j];
             }
         }
         
         return score;
     }
     
     // Update category archetype with new instance
     void update_archetype(g_ptr<CategoryNode> category, const crumb& instance) {
         category->observation_count++;
         float weight = 1.0f / category->observation_count;  // Running average
         
         float* archetype = category->get_archetype();
         
         for(int i = 0; i < CRUMB_ROWS; i++) {
             for(int j = 0; j < 10; j++) {
                 archetype[i * 10 + j] = 
                     archetype[i * 10 + j] * (1.0f - weight) + 
                     instance.mat[i][j] * weight;
             }
         }
     }
     
     // Add instance to category
     void add_to_category(int crumb_idx, g_ptr<CategoryNode> category) {
         category->instance_indices << crumb_idx;
     }
     
     // Create new emergent category
     g_ptr<CategoryNode> create_category(const crumb& seed, g_ptr<CategoryNode> parent, std::string label) {
         g_ptr<CategoryNode> new_cat = make<CategoryNode>(label);
         
         // Copy seed as initial archetype
         float* archetype = new_cat->get_archetype();
         for(int i = 0; i < CRUMB_ROWS; i++) {
             for(int j = 0; j < 10; j++) {
                 archetype[i * 10 + j] = seed.mat[i][j];
             }
         }
         
         new_cat->parent = parent;
         new_cat->observation_count = 1;
         parent->children << new_cat;
         
         return new_cat;
     }
     
     // Check if category should split (low coherence)
     void check_coherence(g_ptr<CategoryNode> category) {
         if(category->instance_indices.length() < 5) return;  // Too few to split
         
         // Calculate average similarity to archetype
         float total_similarity = 0.0f;
         
         for(int idx : category->instance_indices) {
             float similarity = evaluate_against_archetype(crumbs[idx], category);
             total_similarity += similarity;
         }
         
         category->confidence = total_similarity / category->instance_indices.length();
         
         if(category->confidence < 0.6f) {
             // Category is incoherent - should split
             // TODO: implement split logic
         }
     }
};

void test_crumbs() {
     // Create a crumb manager
     g_ptr<crumb_manager> manager = make<crumb_manager>();
     
     // Create agent state
     crumb agent_state;
     
     // Zero everything
     for(int i = 0; i < CRUMB_ROWS; i++) {
         for(int j = 0; j < 10; j++) {
             agent_state.mat[i][j] = 0.0f;
         }
     }
     
     // Row 4 = NEED_HUNGER
     agent_state.mat[4][0] = 0.0f;  // quantity (not used)
     agent_state.mat[4][1] = 5.0f;  // affinity (wants to reduce hunger)
     agent_state.mat[4][2] = 8.0f;  // current state (VERY HUNGRY)
     agent_state.mat[4][7] = 9.0f;  // BITE satisfaction (biting helps hunger a lot)
     
     // Row 10 = MATERIAL_BERRY affinity
     agent_state.mat[10][0] = 0.0f;  // doesn't have any
     agent_state.mat[10][1] = 7.0f;  // likes berries
     agent_state.mat[10][7] = 8.0f;  // biting berries is good
     
     // Create THREE test items - SAME ROW LAYOUT AS AGENT
     crumb berry;
     crumb stone;
     crumb wood;
     
     // Zero everything
     for(int i = 0; i < CRUMB_ROWS; i++) {
         for(int j = 0; j < 10; j++) {
             berry.mat[i][j] = 0.0f;
             stone.mat[i][j] = 0.0f;
             wood.mat[i][j] = 0.0f;
         }
     }
     
     // Berry - row 4 = how it satisfies NEED_HUNGER
     berry.mat[4][7] = 10.0f;  // bite satisfaction (GREAT for biting when hungry)
     
     // Berry - row 10 = MATERIAL_BERRY properties
     berry.mat[10][0] = 1.0f;   // quantity exists
     berry.mat[10][1] = 8.0f;   // affinity (tasty)
     berry.mat[10][7] = 10.0f;  // bite satisfaction
     
     // Stone - bad for hunger
     stone.mat[4][7] = 1.0f;   // terrible for biting when hungry
     stone.mat[10][0] = 1.0f;
     stone.mat[10][1] = 2.0f;
     stone.mat[10][7] = 1.0f;
     
     // Wood - medium
     wood.mat[4][7] = 3.0f;
     wood.mat[10][0] = 1.0f;
     wood.mat[10][1] = 4.0f;
     wood.mat[10][7] = 3.0f;
     
     print("\n=== BASIC EVALUATION TEST ===");
     
     // Evaluate with element-wise
     float berry_score = evaluate_elementwise(agent_state, berry);
     float stone_score = evaluate_elementwise(agent_state, stone);
     float wood_score = evaluate_elementwise(agent_state, wood);
     
     print("Berry score: ", berry_score);
     print("Stone score: ", stone_score);
     print("Wood score: ", wood_score);
     
     print("\n=== CATEGORY TEST ===");
     
     // Add items to manager
     manager->addCrumb(berry);
     manager->addCrumb(stone);
     manager->addCrumb(wood);
     
     // Create "food" category with berry as seed
     g_ptr<CategoryNode> food_category = manager->create_category(
         berry, 
         manager->agents_root,
         "food"
     );
     manager->add_to_category(0, food_category);  // Berry at index 0
     
     print("Created 'food' category with berry as archetype");
     
     // Test categorization of stone and wood
     float stone_vs_food = manager->evaluate_against_archetype(stone, food_category);
     float wood_vs_food = manager->evaluate_against_archetype(wood, food_category);
     
     print("Stone match to 'food': ", stone_vs_food);
     print("Wood match to 'food': ", wood_vs_food);
     print("(Berry would match ~", berry_score, " since it IS the archetype)");
     
     // Update archetype with wood (medium food)
     manager->update_archetype(food_category, wood);
     manager->add_to_category(2, food_category);
     
     print("\nAdded wood to 'food' category, archetype updated");
     
     // Re-check stone match
     float stone_vs_updated_food = manager->evaluate_against_archetype(stone, food_category);
     print("Stone match to updated 'food': ", stone_vs_updated_food);
     print("(Should be slightly higher as archetype diluted)");
     
     // Check coherence
     manager->check_coherence(food_category);
     print("\nFood category confidence: ", food_category->confidence);
     print("Observations: ", food_category->observation_count);
     
     print("\n=== HIERARCHY TEST ===");
     
     // Create "edible_plants" subcategory under food
     g_ptr<CategoryNode> plants_category = manager->create_category(
         berry,
         food_category,
         "edible_plants"
     );
     
     print("Created 'edible_plants' as child of 'food'");
     
     // Find best category for berry
     g_ptr<CategoryNode> berry_best = manager->find_best_category(berry, manager->agents_root);
     print("Best category for berry: ", berry_best->label);
     print("(Should descend to 'edible_plants' if hierarchy working)");
}

int main() {
     Window window = Window(win.x()/2, win.y()/2, "Golden 0.0.6");

     scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     scene->camera.toIso();
     scene->tickEnvironment(1100);

     scene->enableInstancing();

     int amt = 10000;
     float agents_per_unit = 0.3f;
     float total_area = amt / agents_per_unit;
     float side_length = std::sqrt(total_area);
     
     g_ptr<Physics> phys = make<Physics>(scene);
     float grid_size = std::max(100.0f,total_area*0.05f);
     grid = make<NumGrid>(0.5f,grid_size);

     static bool use_grid = true; 

     auto terrain = make<Single>(makeBox({grid_size,0.1f,grid_size},{0.7f,0.4f,0.2f,1}));
     scene->add(terrain);
     terrain->setPhysicsState(P_State::NONE);
     terrain->setPosition({0,-0.2f,0});


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
          q->opt_vec_3_3 = q->getPosition();
          q->joint = [q](){
               //ALWAYS REBUILD
                    // for(auto i : q->opt_idx_cache) {grid->cells[i].erase(q->ID);}
                    // q->updateTransform(false);
                    // q->opt_idx_cache = addToGrid(q);

               //STATEFUL REBUILD
                    // q->updateTransform(false);
                    // list<int> new_cells = grid->cellsAround(q->getWorldBounds());
                    // if(new_cells != q->opt_idx_cache) {
                    // for(auto i : q->opt_idx_cache) {
                    //      grid->cells[i].erase(q->ID);
                    // }
                    // for(auto i : new_cells) {
                    //      grid->cells[i].push(q->ID);
                    // }
                    // q->opt_idx_cache = new_cells;
                    // }
               //TRANFORM REBUILD
                    q->updateTransform(false);
     
                    vec3 current_pos = q->getPosition();
                    float moved = current_pos.distance(q->opt_vec_3_3);
                    
                    // Only update grid if moved more than half a cell
                    if(moved > grid->cellSize * 0.5f) {
                    for(auto i : q->opt_idx_cache) {grid->cells[i].erase(q->ID);}
                         q->opt_idx_cache = grid->addToGrid(q->ID, q->getWorldBounds());
                         q->opt_vec_3_3 = current_pos;
                    }
               return false;
          };
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));

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
     
     int grass_count = (int)(side_length * side_length * 0.02f); // 2% grass coverage
     for(int i = 0; i < grass_count; i++) {
         g_ptr<Single> box = make<Single>(grass_model);
         scene->add(box);
         box->dtype = "grass";
         float s = randf(3, 8);
         vec3 pos(randf(-side_length/2, side_length/2), 0, randf(-side_length/2, side_length/2));
         box->setPosition(pos);
         box->setScale({1.3, s, 1.3});
         box->setPhysicsState(P_State::FREE);
         box->opt_ints << randi(1, 10);
         box->opt_floats << -1.0f;
         addToGrid(box);
     }
     

     scene->define("crumb",Script<>("make_crumb",[c_model,phys](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(c_model);
          scene->add(q);
          q->scale(0.5f);
          q->setPhysicsState(P_State::GHOST);
          q->opt_ints = list<int>(4,0);
          q->opt_floats = list<float>(2,0);
          q->opt_vec_3 = vec3(0,0,0);

          //If you want to update a crumb each frame
          // q->physicsJoint = [q](){

          //      return true;
          // };
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     //make_maze(b_model);

     list<g_ptr<Single>> agents;
     list<g_ptr<crumb_manager>> crumb_managers;
     list<vec3> goals;
     list<list<g_ptr<Single>>> crumbs;
     list<list<g_ptr<Single>>> debug;

     int width = (int)std::sqrt(amt);
     float spacing = side_length / width;

     for(int i=0;i<amt;i++) {
          g_ptr<Single> agent = scene->create<Single>("agent");
          int f = i;
          int row = i / width;
          int col = i % width;
          vec3 base_pos(
              (col - width/2) * spacing,
              0,
              (row - width/2) * spacing
          );
          vec3 jitter(randf(-spacing*0.3f, spacing*0.3f), 0, randf(-spacing*0.3f, spacing*0.3f));
          agent->setPosition(base_pos + jitter);
          agent->setPhysicsState(P_State::FREE);
          agent->opt_ints << randi(1,10); // [0]
          agent->opt_ints << i; // [1] = ITR id
          agent->opt_ints << 0; // [2] = Accumulator
          agent->opt_ints << 0; // [3] = Accumulator 2
          agent->opt_ints << 0; // [4] = Accumulator 3
          agent->opt_ints << 0; // [5] = Accumulator 4
          agent->opt_floats << -0.4f; // [0] = attraction
          agent->opt_floats << randf(4,6); // [1] = speed
          agent->opt_vec3_2 = vec3(0,0,0); //Progress
          agents << agent;

          g_ptr<Single> marker = scene->create<Single>("crumb");
          marker->setPosition(vec3(
               randf(-side_length/2, side_length/2), 
               0, 
               randf(-side_length/2, side_length/2)
           ));
          marker->setPhysicsState(P_State::NONE);
          marker->opt_floats[0] = 20.0f; //Set attraction
          marker->opt_floats[1] = 1.0f;
          crumbs << list<g_ptr<Single>>{marker};

          g_ptr<crumb_manager> memory = make<crumb_manager>();

           // Create 350 memories total (all uniform crumbs now)
          for(int c = 0; c < 350; c++) {
               crumb mem;
               
               // Fill with random data for benchmark
               for(int r = 0; r < CRUMB_ROWS; r++) {
               for(int col = 0; col < 10; col++) {
                    mem.mat[r][col] = randf(-1, 1);
               }
               }
               
               // Embed position in first 3 rows, column 0
               mem.mat[0][0] = randf(-side_length/2, side_length/2);
               mem.mat[1][0] = 0.0f;
               mem.mat[2][0] = randf(-side_length/2, side_length/2);
               
               memory->addCrumb(mem);
          }
               
          crumb_managers << memory;

          float agent_hue = (float(i) / float(amt)) * 360.0f;
          // list<g_ptr<Single>> d_boxes;
          // for(int j=0;j<3;j++) {
          //      g_ptr<Single> dbox = make<Single>(make<Model>(makeTestBox(0.5f)));
          //      scene->add(dbox);
          //      // Each box type gets different saturation/value
          //      float saturation = 1.0f;
          //      float value = 1.0f - (float(j) * 0.3f); // Boxes get darker: 1.0, 0.7, 0.4
               
          //      // Convert HSV to RGB
          //      float h = agent_hue / 60.0f;
          //      float c = value * saturation;
          //      float x = c * (1.0f - abs(fmod(h, 2.0f) - 1.0f));
          //      float m = value - c;
               
          //      vec3 rgb;
          //      if(h < 1) rgb = vec3(c, x, 0);
          //      else if(h < 2) rgb = vec3(x, c, 0);
          //      else if(h < 3) rgb = vec3(0, c, x);
          //      else if(h < 4) rgb = vec3(0, x, c);
          //      else if(h < 5) rgb = vec3(x, 0, c);
          //      else rgb = vec3(c, 0, x);
               
          //      rgb += vec3(m, m, m);
          //      dbox->setColor({rgb.x(), rgb.y(), rgb.z(), 1.0f});
          //      if(j==0)
          //           dbox->scale({1,0.5,1});
          //      else if(j==1)
          //           dbox->scale({0.5,2,0.5});
          //      else if(j==2)
          //           dbox->scale({0.5,1,2});


          //      dbox->setPosition(agent->getPosition());
          //      dbox->setPhysicsState(P_State::NONE);
          //      d_boxes << dbox;
          // }
          // debug << d_boxes;



          agent->physicsJoint = [agent, &agents, &crumbs, &debug, phys, side_length]() {
               #if GTIMERS
               Log::Line overall;
               Log::Line l;
               list<double>& timers = agent->timers;
               list<std::string>& timer_labels = agent->timer_labels;
               timers.clear(); timer_labels.clear();
               overall.start();
               #endif
               //Always setup getters for sketchpad properties because chances are they'll change later
               //Plus it helps with clarity to name them
               int id = agent->opt_ints[1];
               list<g_ptr<Single>>& my_crumbs = crumbs[id];
               list<g_ptr<Single>>& my_debug = debug[id];
               g_ptr<Single> goal_crumb = my_crumbs[0];
               int& spin_accum = agent->opt_ints[2];
               int& on_frame = agent->opt_ints[3];

               //This is just for testing
               vec3 goal = goal_crumb->getPosition();
               if(agent->distance(goal_crumb) <= 1.0f) {
               if(spin_accum < 5) {
                    if(goal_crumb->isActive()) {
                         scene->deactivate(goal_crumb);
                         agent->setLinearVelocity(vec3(0,0,0));
                    }
                    if(spin_accum % 2 == 0)
                         agent->faceTo(agent->getPosition() + vec3(randf(-5,5), 0, randf(-5,5)));
                         spin_accum++;
                    return true;
               } else {
                    // vec3 new_goal = agent->getPosition() + agent->facing() * randf(10, 100);
                    vec3 new_goal(
                         randf(-side_length/2, side_length/2), 
                         0, 
                         randf(-side_length/2, side_length/2)
                    );
                    scene->reactivate(goal_crumb);
                    goal_crumb->setPosition(new_goal);
                    spin_accum = 0;
               }
               }
               //Primary getters and incrementers, opt_ints[3] is the agent-local frame accumulator (may be global later) 
               float desired_speed = agent->opt_floats[1];  
               on_frame++; 
               //The path can always be grabbed, and used to install directions in situations beyond A*, it's the only continous movment method that isn't velocity depentent.
               list<int>& path = agent->opt_idx_cache_2;
               bool enable_astar = false;
               int pathing_interval = 30; 
               //A* Unit
                    if(enable_astar && (on_frame % (pathing_interval*20) == id % (pathing_interval*20)  || path.empty())) {
                         path = grid->findPath(
                              grid->toIndex(agent->getPosition()),
                              grid->toIndex(goal),
                              [agent](int idx) {
                                   if(grid->cells[idx].empty()) return true;
                                   if(grid->cells[idx].length() == 1 && grid->cells[idx].has(agent->ID)) return true;
                                   return false; // Occupied by walls or other agents
                              }
                         );
                         if(path.empty()) {
                              print("UNABLE TO FIND A PATH!");
                              // vec3 new_goal = vec3(randf(-100,100),0,randf(-100,100));
                              // scene->reactivate(goal_crumb);
                              // crumbs[id][0]->setPosition(new_goal);
                              return true;
                         }
                    }

               vec3 goal_loc = agent->opt_vec_3_4;

               if(!path.empty()) {
                    #if GTIMERS
                    l.start();
                    #endif
                    vec3 waypoint = grid->indexToLoc(path[0]);
                    float waypoint_dist = agent->getPosition().distance(waypoint);
                    if(waypoint_dist < grid->cellSize) {
                         path.removeAt(0);
                         if(!path.empty()) {
                              waypoint = grid->indexToLoc(path[0]);
                         }
                    }
                    vec3 direction = (waypoint - agent->getPosition()).normalized().nY();
                    agent->faceTo(agent->getPosition() + direction);
                    agent->setLinearVelocity(direction * desired_speed);
                    #if GTIMERS
                    timers << l.end(); timer_labels << "navigate_path";
                    #endif
               }
               else if(on_frame % pathing_interval == id % pathing_interval) {
                    vec3 net_force(0,0,0);
                    vec3 current_velocity = agent->getVelocity().position;
                    float actual_speed = current_velocity.length();
                    float speed_ratio = actual_speed / std::max(desired_speed, 0.1f);
                    #if GTIMERS
                    l.start();
                    #endif
                    
                    // Tunable parameters
                    int num_rays = 24; // Start conservative, tune up to find budget
                    float ray_distance = 10.0f;
                    float cone_width = 180.0f; // Degrees of forward arc to sample
                    
                    // Get goal direction for biasing
                    vec3 to_goal = agent->direction(goal_crumb).nY();
                    vec3 forward = to_goal.length() > 0.01f ? to_goal.normalized() : agent->facing().nY();
                    vec3 right = vec3(forward.z(), 0, -forward.x()); // Perpendicular
                    
                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_setup"; l.start();
                    #endif
                    
                    // Cast rays in a forward-biased cone
                    float best_openness = 0.0f;
                    vec3 best_direction = forward;
                    
                    float start_angle = -cone_width / 2.0f;
                    float angle_step = cone_width / (num_rays - 1);
                    
                    for(int i = 0; i < num_rays; i++) {
                    float angle_deg = start_angle + i * angle_step;
                    float angle_rad = angle_deg * 3.14159f / 180.0f;
                    
                    // Direction in the sampling cone
                    vec3 ray_dir = (forward * cos(angle_rad) + right * sin(angle_rad)).normalized();
                    
                    // Raycast to find openness in this direction
                    float hit_dist = grid->raycast(
                         agent->getPosition(),
                         ray_dir,
                         ray_distance,
                         agent->ID
                    );
                    
                    // Weight by distance + bias toward goal
                    float goal_alignment = ray_dir.dot(to_goal.normalized());
                    float score = hit_dist * (1.0f + goal_alignment * 0.5f); // 50% bias toward goal
                    
                    if(score > best_openness) {
                         best_openness = score;
                         best_direction = ray_dir;
                    }
                    }
                    
                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_sampling"; l.start();
                    #endif
                    
                    // If we found decent openness, move that direction
                    if(best_openness > 1.0f) { // At least 1 unit of clearance
                    net_force = best_direction * std::min(best_openness / ray_distance, 1.0f);
                    } else {
                    // Too enclosed - add random exploration
                    net_force = vec3(randf(-1, 1), 0, randf(-1, 1));
                    }
                    
                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_postprocess"; l.start();
                    #endif
                         //my_debug[1]->setPosition((agent->getPosition()+net_force).addY(5.0f));
     
                         // print(id,": Net force: ",net_force.to_string());
     
                         // float exploration_strength = (speed_ratio < 0.5f) ? 1.0f : 0.3f;
                         // vec3 exploration(randf(-1.0f, 1.0f), 0, randf(-1.0f, 1.0f));
                         // vec3 environmental_force = (net_force + exploration * exploration_strength);

                    vec3 environmental_force = net_force; //Just trying no exploration for now
                    if(environmental_force.length() > 0.01f) {
                         environmental_force = environmental_force.normalized();
                         
                         // Blend with momentum (if we're already moving)
                         vec3 final_direction;
                         if(current_velocity.length() > 0.1f) {
                              final_direction = (current_velocity.normalized() * 0.7f + environmental_force * 0.3f).normalized();
                         } else {
                              final_direction = environmental_force;
                         }
                         //print(id,": Final direction: ",net_force.to_string());
                         
                         vec3 velocity = final_direction * desired_speed;
                         agent->faceTo(agent->getPosition() + velocity);
                         agent->setLinearVelocity(velocity);
                    }
                    #if GTIMERS
                    timers << l.end(); timer_labels << "velocity_application";
                    #endif
               }

               #if GTIMERS
               timers << overall.end(); timer_labels << "overall";
               #endif
               
               return true;
          };

          agent->threadUpdate = [agent, &agents, &crumb_managers](){
               #if GTIMERS
               Log::Line overall, l;
               list<double>& timers = agent->timers2;
               list<std::string>& timer_labels = agent->timer_labels2;
               timers.clear(); timer_labels.clear();
               overall.start();
               #endif
               
               int id = agent->opt_ints[1];
               g_ptr<crumb_manager> memory = crumb_managers[id];
               int& on_frame = agent->opt_ints[4];
               on_frame++;
               
               // === CONFIGURABLE TEST PARAMETERS ===
               static const int EVAL_INTERVAL = 1200;
               static const int TOP_N = 20;  // Top candidates to re-evaluate
               
               if(on_frame % EVAL_INTERVAL == id % EVAL_INTERVAL) {
                   
                   // ===== PHASE 1: INITIALIZE AGENT STATE =====
                   #if GTIMERS
                   l.start();
                   #endif
                   
                   crumb agent_state;
                   for(int i = 0; i < CRUMB_ROWS; i++) {
                       for(int j = 0; j < 10; j++) {
                           agent_state.mat[i][j] = randf(-1, 1);
                       }
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase1_init"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 2: BROAD EVALUATION (PASS 1) =====
                   // Evaluate all memories for general attraction
                   
                   list<float> all_scores;
                   list<int> all_indices;

                   for(int i = 0; i < memory->crumbs.length()/8; i++) {
                       float score = randf(0,10.0f); //evaluate(agent_state, memory->crumbs[i]);
                       all_scores << score;
                       all_indices << i;
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase2_broad_eval"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 3: TOP SELECTION =====
                   // Find top N candidates for detailed evaluation
                   
                   struct ScoredCrumb {
                       float score;
                       int index;
                   };
                   
                   list<ScoredCrumb> all_crumbs;
                   for(int i = 0; i < all_scores.length(); i++) {
                       all_crumbs << ScoredCrumb{all_scores[i], all_indices[i]};
                   }
                   
                   // Sort descending by score
                   std::sort(all_crumbs.begin(), all_crumbs.end(), 
                       [](const ScoredCrumb& a, const ScoredCrumb& b) {
                           return a.score > b.score;
                       }
                   );
                   
                   // Take top N
                   list<ScoredCrumb> top_crumbs;
                   for(int i = 0; i < TOP_N && i < all_crumbs.length(); i++) {
                       top_crumbs << all_crumbs[i];
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase3_top_selection"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 4: ACTION EVALUATION (PASS 2) =====
                   // For top candidates, evaluate specific actions
                   
                   struct ActionScore {
                       int crumb_index;
                       int action_col;  // Which action column (3-9)
                       float score;
                   };
                   
                   list<ActionScore> action_scores;
                   
                   for(int i = 0; i < top_crumbs.length(); i++) {
                       // Simulate checking each action column
                       for(int action_col = 3; action_col < 10; action_col++) {
                           // Extract specific row × column interaction
                           // Assuming row 4 = dominant need for this test
                           float agent_desire = agent_state.mat[4][action_col];
                           float crumb_suitability = memory->crumbs[top_crumbs[i].index].mat[4][action_col];
                           
                           float score = agent_desire * crumb_suitability;
                           action_scores << ActionScore{top_crumbs[i].index, action_col, score};
                       }
                   }
                   
                   // Find best action
                   float best_action_score = -1e9f;
                   int best_action_idx = -1;
                   for(int i = 0; i < action_scores.length(); i++) {
                       if(action_scores[i].score > best_action_score) {
                           best_action_score = action_scores[i].score;
                           best_action_idx = i;
                       }
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase4_action_eval"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 5: STATE UPDATES (MASKING) =====
                   // Apply decay mask
                   
                   crumb decay_mask;
                   for(int i = 0; i < CRUMB_ROWS; i++) {
                       for(int j = 0; j < 10; j++) {
                           decay_mask.mat[i][j] = 1.0f;  // Identity
                       }
                   }
                   
                   // Decay certain stats
                   decay_mask.mat[4][2] = 0.99f;  // NEED current state decays
                   decay_mask.mat[5][2] = 0.99f;
                   
                   apply_mask(agent_state, decay_mask);
                   
                   // Apply cascade mask (hunger affects happiness)
                   crumb cascade_mask;
                   for(int i = 0; i < CRUMB_ROWS; i++) {
                       for(int j = 0; j < 10; j++) {
                           cascade_mask.mat[i][j] = 1.0f;
                       }
                   }
                   
                   // If hungry (row 4), reduce happiness (row 6)
                   float hunger_level = agent_state.mat[4][2];
                   cascade_mask.mat[6][2] = 1.0f + (hunger_level * -0.05f);
                   
                   apply_mask(agent_state, cascade_mask);
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase5_state_update"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 6: GOAL EVALUATION (PASS 3) =====
                   // Check progress toward active goals
                   
                   // Simulate 5 active goals
                   static list<crumb> goals;
                   if(goals.empty()) {
                       for(int i = 0; i < 5; i++) {
                           crumb goal;
                           for(int r = 0; r < CRUMB_ROWS; r++) {
                               for(int c = 0; c < 10; c++) {
                                   goal.mat[r][c] = randf(-1, 1);
                               }
                           }
                           goals << goal;
                       }
                   }
                   
                   list<float> goal_scores;
                   for(int i = 0; i < goals.length(); i++) {
                       float score = evaluate(agent_state, goals[i]);
                       goal_scores << score;
                   }
                   
                   // Find closest goal
                   float best_goal_score = -1e9f;
                   int best_goal_idx = -1;
                   for(int i = 0; i < goal_scores.length(); i++) {
                       if(goal_scores[i] > best_goal_score) {
                           best_goal_score = goal_scores[i];
                           best_goal_idx = i;
                       }
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase6_goal_eval"; l.start();
                   #endif
                   
                   
                   // ===== PHASE 7: SOCIAL CHECKS =====
                   // Re-evaluate nearby agents for social interactions
                   
                   // Simulate checking 10 nearby agents for gossip/interaction
                   list<float> social_scores;
                   int social_check_count = std::min(10, (int)memory->crumbs.length());
                   
                   for(int i = 0; i < social_check_count; i++) {
                       float score = evaluate(agent_state, memory->crumbs[i]);
                       social_scores << score;
                   }
                   
                   #if GTIMERS
                   timers << l.end(); timer_labels << "phase7_social_check";
                   #endif
                   
                   // Store results for physics thread to use
                   agent->opt_floats[0] = best_action_score;
                   agent->opt_ints[5] = best_action_idx;
               }
               
               #if GTIMERS
               timers << overall.end(); timer_labels << "overall";
               #endif
          };
     }
     vec3 cam_pos(0, side_length * 0.8f, side_length * 0.8f);
     scene->camera.setPosition(cam_pos);
     scene->camera.setTarget(vec3(0, 0, 0));


     // g_ptr<Single> test_origin = make<Single>(make<Model>(makeTestBox(1.0f)));
     // scene->add(test_origin);
     // test_origin->setPosition(vec3(0, 0.5, 0));
     // test_origin->setPhysicsState(P_State::NONE);
     // test_origin->setColor({1, 0, 0, 1}); // Red marker

     // list<g_ptr<Single>> ray_markers;
     // for(int i = 0; i < 10000; i++) {
     //      g_ptr<Single> marker = make<Single>(c_model);
     //      scene->add(marker);
     //      marker->setPhysicsState(P_State::NONE);
     //      marker->scale({0.5f,12,0.5f});
     //      ray_markers << marker;
     //      scene->deactivate(marker);
     // }

     // list<g_ptr<Single>> walls;
     // int on_wall = 0;
     // for(int i=0;i<3;i++) {
     //      auto w = make<Single>(b_model);
     //      scene->add(w);
     //      w->setPhysicsState(P_State::PASSIVE);
     //      walls << w;   w->opt_idx_cache = addToGrid(w);
     //      w->joint = [w](){
     //           for(auto i : w->opt_idx_cache) {grid->cells[i].erase(w->ID);}
     //           w->updateTransform(false);
     //           w->opt_idx_cache = addToGrid(w);
     //           return false;
     //      };
     // }

     S_Tool phys_logger;
     #if GTIMERS
     phys_logger.log = [agents,phys](){
          map<std::string,double> times;
          for(auto a : agents) {
               for(int i=0;i<a->timers.length();i++) {
                    times.getOrPut(a->timer_labels[i],0) += a->timers[i];
               }
          }
          print("------------\n AGENT JOINT TIMES");
          for(auto e : times.entrySet()) {
               print(e.key,": ",ftime(e.value));
          }
     };
     #endif
     g_ptr<Thread> run_thread = make<Thread>();
     run_thread->name = "Physics";
     run_thread->run([phys,&phys_logger,&agents](ScriptContext& ctx){
          phys_logger.tick();
          if(use_grid&&phys->collisonMethod!=Physics::GRID) {
               phys->collisonMethod = Physics::GRID;
               phys->grid = grid;
          } else if(!use_grid&&phys->collisonMethod!=Physics::AABB) {
               phys->collisonMethod = Physics::AABB;
               phys->treeBuilt3d = false;
          }
          phys->updatePhysics();
     },0.008f);
     run_thread->logSPS = true;

     S_Tool update_logger;
     #if GTIMERS
     update_logger.log = [agents](){
          map<std::string,double> times;
          for(auto a : agents) {
               for(int i=0;i<a->timers2.length();i++) {
                    times.getOrPut(a->timer_labels2[i],0) += a->timers2[i];
               }
          }
          print("------------\n AGENT UPDATE TIMES");
          for(auto e : times.entrySet()) {
               print(e.key,": ",ftime(e.value));
          }
     };
     #endif
     g_ptr<Thread> update_thread = make<Thread>();
     update_thread->name = "Update";
     update_thread->run([&update_logger,&agents](ScriptContext& ctx){
          update_logger.tick();
          for(auto a : agents) {
               if(a->threadUpdate&&a->isActive())
                    a->threadUpdate();
          }
     },0.00001f);
     update_thread->logSPS = true;

     S_Tool s_tool;
     s_tool.log_fps = true;
     start::run(window,d,[&]{
          s_tool.tick();
          if(pressed(Q)) {
               scene->camera.setTarget(agents[0]->getPosition()+vec3(0,20,20));
          }
          vec3 mousepos = scene->getMousePos();

          // if(pressed(E)) {
          //      if(on_wall++>=walls.length()-1) {
          //           on_wall = 0;
          //      }
          // }

          // static vec3 drag_start_pos;
          // static bool dragging = false;
          // if(held(MOUSE_LEFT)) {
          //     if(!walls.empty()) {
          //         g_ptr<Single> wall = walls[on_wall];
          //         if(held(LSHIFT)) {
          //             wall->setPosition(mousepos);
          //         } else {
          //             if(!dragging) {
          //                 drag_start_pos = wall->getPosition();
          //                 dragging = true;
          //             }
                      
          //             float dist = drag_start_pos.distance(mousepos);
          //             wall->faceTo(mousepos);
          //             wall->setScale({1, 1, dist});
                      
          //             // Position at start, then offset by half the distance
          //             vec3 offset = wall->facing() * (dist / 2.0f);
          //             wall->setPosition(drag_start_pos + offset);
          //         }
          //     }
          // } else {
          //     dragging = false;
          // }
          // int used_markers = 0;
          // for(int i=0;i<grid->cells.length();i++) {
          //      if(!grid->cells[i].empty()) {
          //           ray_markers[used_markers++]->setPosition(grid->indexToLoc(i));
          //      }
          // }
          // for(int i=0;i<ray_markers.length();i++) {
          //      if(i<=used_markers) {
          //           scene->reactivate(ray_markers[i]);
          //      } else if(ray_markers[i]->isActive()) {
          //           scene->deactivate(ray_markers[i]);
          //      }
          // }

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


//Field based pathfinding unit:
     // #if GTIMERS
     // l.start();
     // #endif


     // //Could use the vector sum tracker instead, expeirment with approaches
     //      vec3 last_pos = agent->opt_vec_3;
     //      float progress = agent->getPosition().distance(last_pos);
     //      agent->opt_vec_3 = agent->getPosition(); 
          
     //      float repel_radius = 8.0f;
     //      BoundingBox query_bounds = agent->getWorldBounds();
     //      query_bounds.expand(BoundingBox(
     //           agent->position - vec3(repel_radius),
     //           agent->position + vec3(repel_radius)
     //      ));

     // #if GTIMERS
     // timers << l.end(); timer_labels << "query_setup"; l.start();
     // #endif

     // list<g_ptr<Single>> nearby;
     // if(!use_grid) {
     // list<g_ptr<S_Object>> queryResults;
     //                     phys->queryTree(phys->treeRoot3d, query_bounds, queryResults);
     //                     #if GTIMERS
     //                     timers << l.end(); timer_labels << "tree_query"; l.start();
     //                     #endif
     //                     for(auto q : queryResults) {
     //                          if(g_ptr<Single> obj = g_dynamic_pointer_cast<Single>(q)) 
     //                               nearby << obj;
     //                     }
     //                     #if GTIMERS
     //                     timers << l.end(); timer_labels << "query_convert"; l.start();
     //                     #endif                         
     //                } else {
     //                     list<int> around = grid->cellsAround(query_bounds);
     //                     #if GTIMERS
     //                     timers << l.end(); timer_labels << "grid_query"; l.start();
     //                     #endif
     //                     static list<int> seen_flags;
     //                     seen_flags = list<int>(scene->singles.length(), -1);
     //                     int current_agent_id = agent->ID;
     //                     for(auto cell : around) {
     //                     for(auto i : grid->cells[cell]) {
     //                          if(seen_flags[i] != current_agent_id) {
     //                               seen_flags[i] = current_agent_id;
     //                               if(i < scene->singles.length() && scene->active[i]) {
     //                                    nearby << scene->singles[i];
     //                               }
     //                          }
     //                     }
     //                     }
     //                     #if GTIMERS
     //                     timers << l.end(); timer_labels << "unpack_cells"; l.start();
     //                     #endif
     //                }

     //                     for(auto c : my_crumbs) {
     //                          if(c->opt_floats[0]>=10.0f) { //Only distantly remember attractive crumbs (and possibly repulsive as well)
     //                               nearby << c;
     //                          }
     //                     }
     //                     // First pass: accumulate desired direction from attractors only
     //                     vec3 desired_direction(0, 0, 0);
     //                     float attractor_weight = 0.0f;
     //                     for(auto obj : nearby) {
     //                          if(obj == agent) continue;
     //                          float score = obj->opt_floats[0];
     //                          if(score <= 0) continue; // Only attractors
                              
     //                          vec3 to_obj = agent->direction(obj).nY();
     //                          float dist = agent->distance(obj);
     //                          float influence = score / std::max(dist, 0.1f); // Distance-weighted attraction
                              
     //                          desired_direction += to_obj * influence;
     //                          attractor_weight += influence;
     //                     }
                         
     //                     if(attractor_weight > 0.01f) {
     //                          desired_direction = desired_direction.normalized() * std::min(attractor_weight, 5.0f); // Keep some magnitude
     //                     }

     //                #if GTIMERS
     //                timers << l.end(); timer_labels << "attractor_pass"; l.start();
     //                #endif
     
     //                     //my_debug[0]->setPosition((agent->getPosition()+desired_direction).addY(5.0f));
     //                     //print(id,": Desired direction: ",desired_direction.to_string());
                    
     //                     // Second pass: let repulsors deflect from desired direction
     //                     net_force = desired_direction;
     //                     float force_budget = 1.0f;
                         
     //                     for(auto obj : nearby) {
     //                          if(obj == agent) continue;
     //                          float score = obj->opt_floats[0];
     //                          if(score >= 0) continue; // Only repulsors
                              
     //                          // Distance to surface, not center!
     //                          BoundingBox objBounds = obj->getWorldBounds();
     //                          float dist = objBounds.distance(agent->getPosition());
     //                          dist = std::max(dist, 0.1f); // Avoid division by zero
                              
     //                          // Direction away from closest point on surface
     //                          vec3 closest = objBounds.closestPoint(agent->getPosition());
     //                          vec3 away_from_obj = (agent->getPosition() - closest).normalized().nY();
                              
     //                          float repulsion_strength = abs(score) / (dist * dist);
                              
     //                          net_force += away_from_obj * repulsion_strength * force_budget;
     //                          force_budget *= 0.8f;
     //                     }
     
     //                     if(net_force.length() > 5.0f) {
     //                          net_force = net_force.normalized() * 5.0f;
     //                     }

     //                #if GTIMERS
     //                timers << l.end(); timer_labels << "repulsor_pass"; l.start();
     //                #endif


//Multi-modal pathfinding joint
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


//A* pathfinding joint

       // agent->physicsJoint = [agent, &goals, &crumbs, &debug, phys]() {
          //      int id = agent->opt_ints[1];
          //      g_ptr<Single> goal_crumb = crumbs[id][0];
          //      vec3 goal = goal_crumb->getPosition();
          //      float goal_dist = agent->distance(goal_crumb);
               
          //      // Goal reached behavior
          //      if(goal_dist <= 1.0f) {
          //      if(agent->opt_ints[2] < 650) {
          //           if(goal_crumb->isActive()) {
          //                scene->deactivate(goal_crumb);
          //                agent->setLinearVelocity(vec3(0,0,0));
          //           }
          //           if(agent->opt_ints[2] % 100 == 0)
          //                agent->faceTo(agent->getPosition() + vec3(randf(-5,5), 0, randf(-5,5)));
          //           agent->opt_ints[2]++;
          //           return true;
          //      } else {
          //           vec3 new_goal = agent->getPosition() + agent->facing() * randf(10, 100);
          //           scene->reactivate(goal_crumb);
          //           crumbs[id][0]->setPosition(new_goal);
          //           agent->opt_ints[2] = 0;
          //      }
          //      }
          
          //      float desired_speed = agent->opt_floats[1];
               
          //      // A* pathfinding
          //      auto isWalkable = [agent](int idx) {
          //           if(grid->cells[idx].empty()) return true;
          //           if(grid->cells[idx].length() == 1 && grid->cells[idx].has(agent->ID)) return true;
          //           return false; // Occupied by walls or other agents
          //       };
               
          //      list<int>& path = agent->opt_idx_cache_2;
          //      if(agent->opt_ints[3] % 300 == id % 300  || path.empty()) {
          //           path = grid->findPath(
          //                grid->toIndex(agent->getPosition()),
          //                grid->toIndex(goal),
          //                isWalkable
          //           );
          //           if(path.empty()) {
          //                vec3 new_goal = vec3(randf(-100,100),0,randf(-100,100));
          //                scene->reactivate(goal_crumb);
          //                crumbs[id][0]->setPosition(new_goal);
          //                return true;
          //           }
          //      }
          //     // debug[id][1]->setPosition(goal.addY(2.0f));
          //      agent->opt_ints[3]++;
               
          //      if(!path.empty()) {
          //           // Get next waypoint
          //           vec3 waypoint = grid->indexToLoc(path[0]);
          //           float waypoint_dist = agent->getPosition().distance(waypoint);
                    
          //           // Reached waypoint, advance to next
          //           if(waypoint_dist < grid->cellSize) {
          //                path.removeAt(0);
          //                if(!path.empty()) {
          //                     waypoint = grid->indexToLoc(path[0]);
          //                }
          //           }
                    
          //           // Move toward current waypoint
          //           vec3 direction = (waypoint - agent->getPosition()).normalized().nY();
          //           agent->faceTo(agent->getPosition() + direction);
          //           agent->setLinearVelocity(direction * desired_speed);
                    
          //           // Debug: show waypoint
          //          //debug[id][0]->setPosition(waypoint.addY(2.0f));
          //      }
               
          //      return true;
          // };



//Crumb based

          // scene->define("crumb",Script<>("make_crumb",[c_model,phys](ScriptContext& ctx){
          //      g_ptr<Single> q = make<Single>(c_model);
          //      scene->add(q);
          //      q->scale(0.5f);
          //      q->setPhysicsState(P_State::GHOST);
          //      q->opt_ints = list<int>(4,0);
          //      q->opt_floats = list<float>(2,0);
          //      q->opt_vec_3 = vec3(0,0,0);

          //      //If you want to update a crumb each frame
          //      // q->physicsJoint = [q](){

          //      //      return true;
          //      // };
          //      ctx.set<g_ptr<Object>>("toReturn",q);
          // }));

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