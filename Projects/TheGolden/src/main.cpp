#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>
#include<util/ml_core.hpp>
#include<util/logger.hpp>
#include<set>
using namespace Golden;
using namespace helper;

const std::string AROOT = root()+"/Projects/TheGolden/assets/";
const std::string IROOT = AROOT+"images/";
const std::string MROOT = AROOT+"models/";

g_ptr<Scene> scene = nullptr;
g_ptr<NumGrid> grid = nullptr;
g_ptr<Geom> guiGeom = nullptr;
g_ptr<Font> font = nullptr;
ivec2 win(2560,1536);

list<int> addToGrid(g_ptr<Single> q) {
     return grid->addToGrid(q->ID,q->getWorldBounds());
}
list<int> removeFromGrid(g_ptr<Single> q) {
     return grid->removeFromGrid(q->ID,q->getWorldBounds());
}
 
g_ptr<Single> draw_line(vec3 start, vec3 end, std::string color, float height) {
     g_ptr<Single> line = scene->create<Single>(color);
     
     float dist = start.distance(end);
     line->setScale({0.1f, 0.1f, dist});
     
     // Set position first
     vec3 midpoint = (start + end) * 0.5f;
     midpoint.y(height); // Set height
     line->setPosition(midpoint);
     
     // Then orient toward end
     line->faceTo(end);
     
     return line;
 }
 list<g_ptr<Single>> maze_walls;
 void make_maze() {
    if(!maze_walls.empty()) {
        for(auto w : maze_walls) {
            removeFromGrid(w);
            scene->recycle(w);
        }
        maze_walls.clear();
    }

     float maze_size = 50.0f;
     float wall_thickness = 1.0f;
     float wall_height = 3.0f;
     int grid_rows = 10;
     int grid_cols = 10;
     float cell_size = maze_size / grid_rows;
 
     // Helper to place wall segment
     auto place_wall = [&](vec3 pos, vec3 scale) {
         g_ptr<Single> wall = scene->create<Single>("wall");
         wall->getLayer().setLayer(1);
         wall->getLayer().setCollision(0);
         wall->setPosition(pos);
         wall->setScale(scale);
         wall->setPhysicsState(P_State::PASSIVE);
         addToGrid(wall);
         maze_walls << wall;
     };
 
     // Maze grid: track which walls exist
     // For each cell, track: [north, east, south, west] walls
     struct Cell {
         bool visited = false;
         bool walls[4] = {true, true, true, true}; // N, E, S, W
     };
     
     Cell cells[10][10];
     
     // Recursive backtracking maze generation
     std::function<void(int, int)> carve_path = [&](int row, int col) {
         cells[row][col].visited = true;
         
         // Randomized directions
         int dirs[4] = {0, 1, 2, 3}; // N, E, S, W
         for(int i = 0; i < 4; i++) {
             int j = randi(i, 3);
             std::swap(dirs[i], dirs[j]);
         }
         
         for(int d = 0; d < 4; d++) {
             int next_row = row;
             int next_col = col;
             
             switch(dirs[d]) {
                 case 0: next_row--; break; // North
                 case 1: next_col++; break; // East
                 case 2: next_row++; break; // South
                 case 3: next_col--; break; // West
             }
             
             // Check bounds
             if(next_row < 0 || next_row >= grid_rows || 
                next_col < 0 || next_col >= grid_cols) continue;
             
             if(!cells[next_row][next_col].visited) {
                 // Remove walls between current and next
                 cells[row][col].walls[dirs[d]] = false;
                 cells[next_row][next_col].walls[(dirs[d] + 2) % 4] = false;
                 carve_path(next_row, next_col);
             }
         }
     };
     
     // Start from top-left corner
     carve_path(0, 0);
     
     // Clear center 2x2 area
     int center_row = grid_rows / 2;
     int center_col = grid_cols / 2;
     for(int r = center_row - 1; r <= center_row; r++) {
         for(int c = center_col - 1; c <= center_col; c++) {
             if(r >= 0 && r < grid_rows && c >= 0 && c < grid_cols) {
                 cells[r][c].walls[0] = false; // North
                 cells[r][c].walls[1] = false; // East
                 cells[r][c].walls[2] = false; // South
                 cells[r][c].walls[3] = false; // West
             }
         }
     }
     
     // Create entrance at edge (remove north wall of cell 0,0)
    //  cells[0][0].walls[0] = false;
     
     // Build walls from cell data
     for(int row = 0; row < grid_rows; row++) {
         for(int col = 0; col < grid_cols; col++) {
             float x = -maze_size/2 + col * cell_size;
             float z = -maze_size/2 + row * cell_size;
             
             // North wall
             if(cells[row][col].walls[0] && row > 0) {
                 place_wall(vec3(x + cell_size/2, wall_height/2, z), 
                           vec3(cell_size, wall_height, wall_thickness));
             }
             
             // East wall
             if(cells[row][col].walls[1] && col < grid_cols - 1) {
                 place_wall(vec3(x + cell_size, wall_height/2, z + cell_size/2), 
                           vec3(wall_thickness, wall_height, cell_size));
             }
             
             // South wall
             if(cells[row][col].walls[2] && row < grid_rows - 1) {
                 place_wall(vec3(x + cell_size/2, wall_height/2, z + cell_size), 
                           vec3(cell_size, wall_height, wall_thickness));
             }
             
             // West wall
             if(cells[row][col].walls[3] && col > 0) {
                 place_wall(vec3(x, wall_height/2, z + cell_size/2), 
                           vec3(wall_thickness, wall_height, cell_size));
             }
         }
     }
     
     // Outer boundary walls (to enclose maze)
     // North boundary
     for(int col = 0; col < grid_cols; col++) {
         float x = -maze_size/2 + col * cell_size;
         if(cells[0][col].walls[0]) { // Only if not entrance
             place_wall(vec3(x + cell_size/2, wall_height/2, -maze_size/2), 
                       vec3(cell_size, wall_height, wall_thickness));
         }
     }
     
     // South boundary
     for(int col = 0; col < grid_cols; col++) {
         float x = -maze_size/2 + col * cell_size;
         place_wall(vec3(x + cell_size/2, wall_height/2, maze_size/2), 
                   vec3(cell_size, wall_height, wall_thickness));
     }
     
     // West boundary
     for(int row = 0; row < grid_rows; row++) {
         float z = -maze_size/2 + row * cell_size;
         place_wall(vec3(-maze_size/2, wall_height/2, z + cell_size/2), 
                   vec3(wall_thickness, wall_height, cell_size));
     }
     
     // East boundary
     for(int row = 0; row < grid_rows; row++) {
         float z = -maze_size/2 + row * cell_size;
         place_wall(vec3(maze_size/2, wall_height/2, z + cell_size/2), 
                   vec3(wall_thickness, wall_height, cell_size));
     }
 }
#define GTIMERS 0
#define VISPATHS 1
#define TRIALS 1
// #define CRUMB_ROWS 40 
// // Packed ints with start / count so we can be more dynamic!
// const int META  = (0 << 16) | 1; 
// const int IS    = (1 << 16) | 10;
// const int DOES  = (11 << 16) | 10;
// const int WANTS = (21 << 16) | 10;
// const int HAS   = (31 << 16) | 9;
// const int ALL = (0 << 16) | CRUMB_ROWS;

#define CRUMB_ROWS 20
// Packed ints with start / count so we can be more dynamic!
const int META  = (0 << 16) | 1; 
const int IS    = (1 << 16) | 5;
const int DOES  = (6 << 16) | 5;
const int WANTS = (11 << 16) | 5;
const int HAS   = (16 << 16) | 5;
const int ALL = (0 << 16) | CRUMB_ROWS;

struct Crumb {
    Crumb() {
        setmat(0);
    }
    Crumb(float fill) {
        setmat(fill);
    }

    int id = -1;
    bool is_grid = false;
    float mat[CRUMB_ROWS][10];

    inline void setmat(float v) {
        for(int r = 0; r < CRUMB_ROWS; r++) {
            for(int c = 0; c < 10; c++) {
                mat[r][c] = v;
            }
        }
     }

    inline float* data() const {
        return (float*)&mat[0][0];
    }
     
    inline int rows() const {
        return CRUMB_ROWS;
    }

    //DEBUG ONLY METHOD
    vec3 getPosition() {
        if(is_grid)
            return grid->indexToLoc(id);
        else
            return vec3(GET(scene->transforms,id)[3]);
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

static float evaluate_matmul(const Crumb& eval, const Crumb& against, Crumb& result) {
     matmul(eval.data(), against.data(), result.data());
     return compute_trace(result.data());
 }
 
 static float evaluate_matmul(const Crumb& eval, const Crumb& against) {
     Crumb result{};
     return evaluate_matmul(eval, against, result);
 }
 

static float evaluate_elementwise(const Crumb& eval, int eval_verb, const Crumb& against, int against_verb) {
     int eval_start = (eval_verb >> 16) & 0xFFFF;
     int eval_count = eval_verb & 0xFFFF;
     int against_start = (against_verb >> 16) & 0xFFFF;
     int against_count = against_verb & 0xFFFF;

     assert(eval_count == against_count); 

     float score = 0.0f;
     const float* eval_ptr = &eval.mat[eval_start][0];
     const float* against_ptr = &against.mat[against_start][0];
     int total = eval_count * 10;
     for(int i = 0; i < total; i++) {
          score += eval_ptr[i] * against_ptr[i];
     }

     return score;
}

static float evaluate(const Crumb& eval, int eval_verb, const Crumb& against, int against_verb) {
    return evaluate_elementwise(eval, eval_verb, against, against_verb);
}

static float evaluate_binary(const Crumb& a, int a_verb, const Crumb& b, int b_verb) {
     int a_start = (a_verb >> 16) & 0xFFFF;
     int a_count = a_verb & 0xFFFF;
     int b_start = (b_verb >> 16) & 0xFFFF;
     int b_count = b_verb & 0xFFFF;
     
     assert(a_count == b_count);
     
     float* a_ptr = (float*)&a.mat[a_start][0];
     float* b_ptr = (float*)&b.mat[b_start][0];
     
     int total_elements = a_count * 10;
     int matching_bits = 0;
     int total_bits = total_elements * 32;
     
     for(int i = 0; i < total_elements; i++) {
         uint32_t a_bits = *(uint32_t*)&a_ptr[i];
         uint32_t b_bits = *(uint32_t*)&b_ptr[i];
         uint32_t xor_result = a_bits ^ b_bits;
         uint32_t matching = ~xor_result;
         matching_bits += __builtin_popcount(matching);
     }
     return (float)matching_bits / (float)total_bits;
 }

template<typename Op>
static void apply_mask(Crumb& target, int target_verb, const Crumb& mask, int mask_verb, Op op) {
    int target_start = (target_verb >> 16) & 0xFFFF;
    int target_count = target_verb & 0xFFFF;
    int mask_start = (mask_verb >> 16) & 0xFFFF;
    int mask_count = mask_verb & 0xFFFF;
    
    assert(target_count == mask_count);
    
    float* target_ptr = &target.mat[target_start][0];
    const float* mask_ptr = &mask.mat[mask_start][0];
    int total = target_count * 10;
    
    for(int i = 0; i < total; i++) {
        target_ptr[i] = op(target_ptr[i], mask_ptr[i]);
    }
}

static void mult_mask(Crumb& t, int tv, const Crumb& m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a * b; });
}

static void div_mask(Crumb& t, int tv, const Crumb& m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a / b; });
}

static void add_mask(Crumb& t, int tv, const Crumb& m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a + b; });
}

static void scale_mask(Crumb& target, int verb, float scalar) {
     int start = (verb >> 16) & 0xFFFF;
     int count = verb & 0xFFFF;
     float* ptr = &target.mat[start][0];
     int total = count * 10;
     
     for(int i = 0; i < total; i++) {
         ptr[i] *= scalar;
     }
 }
 

static Crumb randcrumb() {
     Crumb seed;
     for(int r=0; r<CRUMB_ROWS; r++)
          for(int c=0; c<10; c++)
               seed.mat[r][c] = randf(-1, 1);
     return seed;
 }

struct crumb_store : public q_object {
    list<Crumb> crumbs;
};

struct c_node : public q_object {

    c_node() {
        store = make<crumb_store>();
        cmask.setmat(1.0f);
    }
    c_node(std::string _label) {
        label = _label;
        store = make<crumb_store>();
        cmask.setmat(1.0f);
    }

    std::string label;
    Crumb archetype;
    Crumb cmask;
    
    list<c_node*> parents;
    list<g_ptr<c_node>> children;
    g_ptr<crumb_store> store = nullptr;

    //DEBUG ONLY METHOD
    vec3 getPosition() {
        return archetype.getPosition();
    }

    //DEBUG ONLY METHOD
    int getID() {
        return archetype.id;
    }

    bool operator==(g_ptr<c_node> other) {
        return *this == other.getPtr();
    }
        
    list<Crumb>& get_crumbs() {
        return store->crumbs;
    }

    void set_crumbs(list<Crumb>& n_crumbs) {
        store->crumbs = std::move(n_crumbs);
    }

    list<g_ptr<c_node>> neighbors() {
        list<g_ptr<c_node>> n;
        n << children;
        for(auto p : parents) n << p;
        return n;
    }

    void add_to_category(Crumb& n_crumb) {
        get_crumbs() << n_crumb;
    }
    
    g_ptr<c_node> subnode(const Crumb& seed) {
        g_ptr<c_node> new_cat = make<c_node>();
        new_cat->archetype = std::move(seed);
        new_cat->parents << this;
        children << new_cat;
        
        return new_cat;
    }
    void split(const list<list<int>>& to_split, int verb = ALL) {
        for(int s = 0; s<to_split.length();s++) {
            list<Crumb> crumbs;
            for(int i=0;i<to_split[s].length();i++) {
                    crumbs.push(get_crumbs()[to_split[s][i]]);
            }
            
            Crumb new_arc;
            for(Crumb& c : crumbs) {
                    add_mask(new_arc,verb,c,verb);
            }
            scale_mask(new_arc,verb,1.0f / crumbs.length());

            auto new_cat = subnode(new_arc);
            new_cat->set_crumbs(crumbs);
        }
    }
};

static int total_connections = 0;
static float min_sal = 1.0f;
class nodenet : public Object {
public:
    nodenet() {
        meta_root = make<c_node>("meta");
        is_root = make<c_node>("is");
        does_root = make<c_node>("does");
        wants_root = make<c_node>("wants");
        has_root = make<c_node>("has");

        physics_attention << meta_root;
        cognitive_attention << is_root;

        for(int i = 0; i < CRUMB_ROWS; i++)
        for(int j = 0; j < 10; j++)
            cmask.mat[i][j] = 1.0f;
    };
    ~nodenet() {};

    Crumb cmask;

    g_ptr<c_node> meta_root = nullptr;
    g_ptr<c_node> is_root = nullptr;
    g_ptr<c_node> does_root = nullptr;
    g_ptr<c_node> wants_root = nullptr;
    g_ptr<c_node> has_root = nullptr;

    Crumb current_state;
    int agent_id = -1;

    int physics_focus = 4;
    list<g_ptr<c_node>> physics_attention; 
    int cognitive_focus = 8;
    list<g_ptr<c_node>> cognitive_attention;

    map<int, c_node*> id_to_node; 
    list<g_ptr<Single>> debug;

    void clear_debug() {
        for(auto d : debug) { 
            scene->transforms[d->ID] = glm::mat4(1.0f);
            scene->colors[d->ID] = vec4(1,1,1,1);
            scene->recycle(d); 
        }
        debug.clear();
    }

    
    //Visit should return false to stop, true to expand.
   void walk_graph(g_ptr<c_node> start, 
                std::function<bool(g_ptr<c_node>)> visit,
                std::function<list<g_ptr<c_node>>(g_ptr<c_node>)> expand,
                bool breadth_first = false) {
        std::set<void*> visited;
        list<g_ptr<c_node>> queue;
        queue << start;

        while(!queue.empty()) {
            g_ptr<c_node> current = breadth_first ? queue.first() : queue.last();
            if(breadth_first) 
                queue.removeAt(0);
            else
                queue.pop();

            void* ptr = (void*)current.getPtr();
            if(visited.count(ptr) > 0) continue;
            visited.insert(ptr);

            if(!visit(current)) continue;
            queue << expand(current);
        }
    }

    void profile(g_ptr<c_node> start, std::function<bool(g_ptr<c_node>)> visit) {
        walk_graph(start, visit, [](auto n) { return n->neighbors(); }, false);
    }

    void propagate(g_ptr<c_node> start, int& energy, std::function<bool(g_ptr<c_node>)> visit) {
        walk_graph(start, 
            [&](g_ptr<c_node> node) {
                if(energy <= 0) return false;
                energy--;
                return visit(node);
            }, 
            [](auto n) { return n->neighbors(); },
            true  // BFS
        );
    }

    g_ptr<c_node> has_seen_before_cheat(const Crumb& thing) {
        c_node* fallback = nullptr;
        c_node* ptr = id_to_node.getOrDefault(thing.id,fallback);
        if(ptr)
            return g_ptr<c_node>(ptr);
        else
            return nullptr;
    }
 
    inline float navigation_focus() const {
        return current_state.mat[0][1];
    }

    float spatial_salience(const Crumb& observation) {
        return evaluate(current_state, META, observation, META);
    }

    float desire_salience(const Crumb& observation) {
        return evaluate(current_state, WANTS, observation, IS);
    }

    float salience(const Crumb& observation) {
        float sp_sal = spatial_salience(observation);
        float ds_sal = desire_salience(observation);
        // print("SP_SAL: ",sp_sal);
        // print("DS_SAL: ",ds_sal);
        // print("NAV FOCUS: ",navigation_focus());
        // print("SAL: ",std::lerp(sp_sal,ds_sal,navigation_focus()));
        return std::lerp(ds_sal,sp_sal,navigation_focus());
    }

    bool in_attention(Crumb& thing) {
        for(auto recent : physics_attention)
        { if(thing.id==recent->getID()) return true; }
        return false;
    }

    float relevance(Crumb& thing, float sal) {
        if(sal < min_sal) return 0.0f;
        float min_current_salience = min_sal;
        for(auto attended : physics_attention) {
            float attended_sal = salience(attended->archetype);
            if(attended_sal < min_current_salience) {
                min_current_salience = attended_sal;
            }
            //print("ATTENDED SAL = ",attended_sal);
        }
        // print("THIS SAL: ",sal);
        // print("MIN_CURR_SAL: ",min_current_salience);
        if(min_current_salience==0) return 1.0f; //Just to stop those inf returns
        float relative_score = (sal - min_current_salience * 0.9f) / min_current_salience;
        return std::max(0.0f, relative_score);
    }

    void push_attention_head(g_ptr<c_node> node) {
        int found_id = physics_attention.find(node);
        if(found_id!=-1) {
            physics_attention << physics_attention[found_id];
            physics_attention.removeAt(found_id);
        } else {
            physics_attention << node;
            if(physics_attention.length()>physics_focus) {
                    physics_attention.removeAt(0);
            }
        }
    }

    // g_ptr<c_node> closest_visible_node(const vec3& pos, list<int> exclude_ids = list<int>{}, list<std::string> match_dtype = list<std::string>{}) {
    //     float best_dist = 1000.0f;
    //     g_ptr<c_node> best_node = nullptr;
    //     exclude_ids << agent_id;
    //     for(auto e : id_to_node.entrySet()) {
    //         if(!match_dtype.empty()) {
    //                 bool cont = false;
    //                 for(auto s : match_dtype) {
    //                     if(scene->singles[e.key]->dtype!=s) {
    //                         cont = true; break;
    //                     }
    //                 }
    //                 if(cont) continue;
    //         }

    //         vec3 npos = vec3(scene->transforms[e.key][3]);
    //         float dist = npos.distance(pos);
    //         if(dist<best_dist) {
    //                 exclude_ids << e.key;
    //                 if(grid->can_see(npos,pos,exclude_ids)) {
    //                     best_node = g_ptr<c_node>(e.value);
    //                     best_dist = dist;
    //                 }
    //                 exclude_ids.pop();
    //         }
    //     }
    //     return best_node;
    // }
    // g_ptr<c_node> closest_visible_node(g_ptr<Single> single) {
    //     return closest_visible_node(single->getPosition(),{single->ID},{single->dtype});
    // }

    g_ptr<c_node> observe(Crumb& thing,g_ptr<Single> single) {
        g_ptr<c_node> node = make<c_node>();
        node->archetype = thing;

        int l = physics_attention.length();
        //Do the N last objects of our attention list
        int n = 1;
        for(int i= l<n ? 0: l-n ;i<l;i++) {
             auto recent = physics_attention[i];
             node->parents << recent.getPtr();
             recent->children << node;
             total_connections += 2;
        }

        //Should also add some measure of grid collision detection here too
        // float best_dist = 1000.0f;
        // g_ptr<c_node> best_node = physics_attention.last();
        // vec3 pos = single->getPosition();
        // for(auto n : physics_attention) {
        //      float dist = n->getPosition().distance(pos);
        //      if(dist<best_dist) {
        //           if(grid->can_see(n->getPosition(),pos,{agent_id,single->ID,n->getID()})) {
        //                best_node = n;
        //                best_dist = dist;
        //           }
        //      }
        // }

        // g_ptr<c_node> best_node = closest_visible_node(single);
        // if(!best_node) best_node = physics_attention.last();

        // node->parents << best_node.getPtr();
        // best_node->children << node;
        // total_connections++;

        if(!node) {
            print("OBSERVE WARNING: formless archetpye for category!");
        } else if(single) {
            id_to_node.put(node->getID(),node.getPtr());
        } else {
            //Push to some grid form?
        }
        // push_attention_head(node);
        return node;
    }

    void decrease_novelty(g_ptr<c_node> seen_before) {
        Crumb mask;
        mask.setmat(1.0f); //Initilize to 1.0f
        mask.mat[0][1] = 0.98f;
        mult_mask(seen_before->archetype, META, mask, META);
    }


    g_ptr<c_node> has_seen_before(const Crumb& thing, float focus) {
        if(physics_attention.empty()) return nullptr;
        
        g_ptr<c_node> result = nullptr;
        int energy = (int)(focus * 500);
        //In the future, make it so we can use mulltiple memorable/salient nodes as propagation points for checks like this
        propagate(physics_attention.last(), energy, [&](g_ptr<c_node> node) {
            if(thing.id == node->getID()) {
                result = node;
                return false; // Stop
            }
            return true;
        });
        
        return result;
    }

    list<std::pair<g_ptr<c_node>, float>> imagine(g_ptr<c_node> source,  const Crumb& query, int query_verb, int target_verb, int energy) {
        map<c_node*, float> activations;
        
        propagate(source, energy, [&](g_ptr<c_node> node) {
            float activation = evaluate(query, query_verb, node->archetype, target_verb);
            activations.put(node.getPtr(), activation);
            return true;
        });
        
        list<std::pair<g_ptr<c_node>, float>> results;
        for(auto e : activations.entrySet()) {
            results << std::make_pair(g_ptr<c_node>(e.key), e.value);
        }
        results.sort([](auto a,auto b) {return a.second > b.second; });
        
        return results;
    }


     // Core visualization primitive
     void visualize_nodes(const list<g_ptr<c_node>>& nodes, g_ptr<c_node> highlight = nullptr) {
        std::set<std::pair<int,int>> drawn_edges;
        
        for(auto current : nodes) {
            int current_id = current->getID();
            
            // Node color logic
            std::string color = "green";
            // if(current->archetype.mat[0][1] < 3.0f) color = "white";
            if(current == highlight) color = "black";
            
            // Node marker
            auto marker = scene->create<Single>(color);
            if(current == highlight) {
                marker->setScale(vec3(0.5f,2.0f,0.5f));
            } else if(!current->children.empty()) {
                float cscale = std::min(1.0f, (float)current->children.length() / 2.0f);
                marker->setScale(vec3(cscale,1.0f,cscale));
            } else {
                marker->setScale(vec3(0.3f,2.0f,0.3f));
            }
            marker->setPosition(current->getPosition().addY(
                current == highlight ? 4.0f : 2.0f));

            marker->scale(1.0f);
            debug << marker;
            
            // Child edges (blue)
            for(auto child : current->children) {
                if(child->getID() == -1) continue;
                auto edge = std::make_pair(std::min(current_id, child->getID()), 
                                        std::max(current_id, child->getID()));
                if(drawn_edges.count(edge) == 0) {
                    debug << draw_line(current->getPosition(), child->getPosition(), 
                                    "blue", 1.9f);
                    drawn_edges.insert(edge);
                }
            }
            
            // Parent edges (yellow)
            for(auto parent : current->parents) {
                g_ptr<c_node> p(parent);
                if(p->getID() == -1) continue;
                auto edge = std::make_pair(std::min(current_id, p->getID()), 
                                        std::max(current_id, p->getID()));
                if(drawn_edges.count(edge) == 0) {
                    debug << draw_line(current->getPosition(), p->getPosition(), 
                                    "yellow", 2.2f);
                    drawn_edges.insert(edge);
                }
            }
        }
    }

    // Simplified public methods
    void visualize_structure(g_ptr<c_node> root = nullptr) {
        clear_debug();
        if(!root) root = physics_attention.empty() ? meta_root : physics_attention.last();
        
        list<g_ptr<c_node>> visited_nodes;
        std::set<int> visited_ids;
        
        profile(root, [&](g_ptr<c_node> node) {
            if(visited_ids.count(node->getID()) > 0) return false;
            visited_ids.insert(node->getID());
            visited_nodes << node;
            return true;
        });
        
        visualize_nodes(visited_nodes, root);
        
        // Attention marker
        auto mark = scene->create<Single>("cyan");
        mark->setPosition(physics_attention.last()->getPosition().addY(5.0f));
        mark->setScale({0.5f, 1.5f, 0.5f});
        debug << mark;
    }

    void visualize_registry(g_ptr<c_node> root = nullptr) {
        clear_debug();
        if(!root) root = meta_root;
        
        list<g_ptr<c_node>> all_nodes;
        for(auto entry : id_to_node.entrySet()) {
            all_nodes << g_ptr<c_node>(entry.value);
        }
        
        visualize_nodes(all_nodes, root);
    }
};

g_ptr<Quad> make_button(std::string slot) {
    g_ptr<Quad> q = scene->create<Quad>("gui_element");
    q->scale(vec2(slot.length()*100.0f,40));
    q->setColor(vec4(1,0,0,1));
    q->addSlot("test");
    q->addSlot("otherTest");
    q->setDepth(1.0f);
    q->addScript("onHover",[q](ScriptContext& ctx){
        q->setColor(vec4(0,1,0,1));
    });
    q->addScript("onUnHover",[q](ScriptContext& ctx){
        q->setColor(vec4(1,0,0,1));
    });
    q->addScript("onPress",[q,slot](ScriptContext& ctx){
        print("You just pressed the ",slot," button");
        q->setColor(q->getColor()*vec4(0.5,0.5,0.5,1));
        q->setPosition(vec2(100,0));
    });
    q->addScript("onRelease",[q](ScriptContext& ctx){
        q->setColor(vec4(0,1,0,1));
        q->setPosition(vec2(0,0));
    });
    g_ptr<Text> twig = make<Text>(font,scene);
    auto t = twig->makeText(slot);
    q->addChild(t[0]);
    return q;
}

int main() {



     Window window = Window(win.x()/2, win.y()/2, "Golden 0.0.7");

     scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     scene->camera.toOrbit();
     scene->tickEnvironment(1100);

     scene->enableInstancing();

     guiGeom = make<Geom>();
     font = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",50);

     scene->define("gui_element",[](){
          g_ptr<Quad> q = make<Quad>(guiGeom);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          return q;
     });


     make_button("test");

     int amt = 10;
     float agents_per_unit = 0.3f;
     float total_area = amt / agents_per_unit;
     float side_length = std::sqrt(total_area);
     
     g_ptr<Physics> phys = make<Physics>(scene);
     float grid_size = std::max(100.0f,amt*0.05f);
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
     g_ptr<Model> berry_model = make<Model>(MROOT+"products/berry.glb");
     g_ptr<Model> clover_model = make<Model>(MROOT+"products/clover.glb");
     g_ptr<Model> stone_model = make<Model>(MROOT+"products/stone.glb");
     c_model->setColor({0,0,1,1});
     a_model->localBounds.transform(0.6f);

     g_ptr<Model> red_model = make<Model>(makeBox(1,1,1,{1,0,0,1}));
     scene->define("red",Script<>("make_red",[red_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(red_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> green_model = make<Model>(makeBox(1,1,1,{0,1,0,1}));
     scene->define("green",Script<>("make_green",[green_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(green_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> blue_model = make<Model>(makeBox(1,1,1,{0,0,1,1}));
     scene->define("blue",Script<>("make_blue",[blue_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(blue_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> yellow_model = make<Model>(makeBox(1,1,1,{1,1,0,1}));
     scene->define("yellow",Script<>("make_yellow",[yellow_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(yellow_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> white_model = make<Model>(makeBox(1,1,1,{1,1,1,1}));
     scene->define("white",Script<>("make_white",[white_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(white_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> cyan_model = make<Model>(makeBox(1,1,1,{0,0.5,0.5,1}));
     scene->define("cyan",Script<>("make_cyan",[cyan_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(cyan_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));
     g_ptr<Model> black_model = make<Model>(makeBox(1,1,1,{0,0,0,1}));
     scene->define("black",Script<>("make_black",[black_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(black_model);
          scene->add(q);
          q->setPhysicsState(P_State::NONE);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));

     map<int,Crumb> crumbs;

     //I am very, very, very much going to remove the pointless script system in scene's pooling and simplify things.
     scene->define("agent",Script<>("make_agent",[a_model](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(a_model);
          scene->add(q);
          q->opt_idx_cache = addToGrid(q);
          q->opt_vec_3_3 = q->getPosition();
          q->joint = [q](){
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
     scene->define("crumb",Script<>("make_crumb",[c_model,phys](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(c_model);
          scene->add(q);
          q->scale(0.5f);
          q->setPhysicsState(P_State::NONE);
          q->opt_ints = list<int>(4,0);
          q->opt_floats = list<float>(2,0);
          q->opt_vec_3 = vec3(0,0,0);
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

    // test_imagination();
    // return 0;


    scene->define("wall",[b_model](){
        g_ptr<Single> q = make<Single>(b_model);
        scene->add(q);
        return q;
     });
     make_maze();
     int grass_count = (int)(grid_size * grid_size * 0.00f);
     for(int i = 0; i < grass_count; i++) {
         g_ptr<Single> box = make<Single>(grass_model);
         scene->add(box);
         box->dtype = "grass";
         float s = randf(3, 8);
         vec3 pos(randf(-grid_size/2, grid_size/2), 0, randf(-grid_size/2, grid_size/2));
         box->setPosition(pos);
         box->setScale({1.3, s, 1.3});
         box->setPhysicsState(P_State::PASSIVE);
         box->opt_ints << randi(1, 10);
         box->opt_floats << -1.0f;
         addToGrid(box);
         Crumb mem;
         for(int r = 0; r < CRUMB_ROWS; r++) {
             for(int col = 0; col < 10; col++) {
                 mem.mat[r][col] = 0.0f;
             }
         }
         mem.mat[3][0] = randf(1.0f,1.04f); //Satiety
         mem.mat[3][1] = randf(0.2f,0.6f); //Shelter
         mem.id = box->ID;
         crumbs.put(box->ID,mem);
     }

     // g_ptr<Single> clover = make<Single>(clover_model);
     // scene->add(clover);
     // clover->dtype = "clover";
     // clover->setScale({1,1.2,1});
     // clover->setPosition({0,0,0});
     // clover->setPhysicsState(P_State::PASSIVE);
     // addToGrid(clover);
     // Crumb clover_crumb;
     // clover_crumb.mat[3][0] = 0.5f; //Satiety
     // clover_crumb.mat[3][1] = 1.3f; //Shelter
     // clover_crumb.form = clover;
     // crumbs.put(clover->ID,clover_crumb);

     list<g_ptr<Single>> agents;
     list<vec3> goals;
     list<list<g_ptr<Single>>> debug;
     list<g_ptr<nodenet>> crumb_managers;


     g_ptr<tensor> W_spatial = weight(24, 24, 0.1f);    // Local edge detection
     g_ptr<tensor> W_invariant = weight(24, 10, 0.1f);  // Position-invariant pooling
     g_ptr<tensor> W_grouping = weight(10, 6, 0.1f);    // spatial integration

     g_ptr<Thread> run_thread = make<Thread>();
     g_ptr<Thread> update_thread = make<Thread>();
     
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
          // base_pos.addZ(-35);
          // base_pos.addX(-22);
          vec3 jitter(randf(-spacing*0.3f, spacing*0.3f), 0, randf(-spacing*0.3f, spacing*0.3f));
          agent->setPosition(base_pos + jitter);
          agent->setPhysicsState(P_State::FREE);
          #if TRIALS
            //making it so agents don't interfere with each other
            agent->setPosition(vec3(0,0,0));
            agent->getLayer().setLayer(0);
            agent->getLayer().setCollision(1);
            grid->make_seethru(agent->ID);
          #endif
          agent->opt_ints << randi(1,10); // [0]
          agent->opt_ints << i; // [1] = ITR id
          agent->opt_ints << 0; // [2] = Accumulator
          agent->opt_ints << 0; // [3] = Accumulator 2
          agent->opt_ints << 0; // [4] = Accumulator 3
          agent->opt_ints << 0; // [5] = Accumulator 4
          agent->opt_ints << 0; // [6] = Accumulator 5
          agent->opt_ints << 0; // [7] = Progress
          agent->opt_ints << 1; // [8] = Nav strategy
          agent->opt_floats << -0.4f; // [0] = attraction
          agent->opt_floats << randf(4,6); // [1] = speed
          agent->opt_floats << 1.0f; // [2] = goal focus
          agent->opt_floats << 0.01f; // [3] = Varience threshold

          agent->opt_floats << 0.01f; // [4] = Visual activation threshold
          agent->opt_floats << 3.0f; // [5] = Match threshold
          agent->opt_floats << 1.04f; // [6] = Node growth rate
          agent->opt_floats << 0.7f; // [7] = Signifigance weight
          agent->opt_floats << 10.0f; // [8] = Spatial weight

          agent->opt_vec3_2 = vec3(0,0,0); //Last position
          agents << agent;

          g_ptr<Single> marker = scene->create<Single>("crumb");
          // vec3 possible_pos = vec3(
          //      randf(-side_length/2, side_length/2), 
          //      0, 
          //      randf(-side_length/2, side_length/2)
          // );
          vec3 possible_pos = vec3(-22,0,-35);
          int depth = 0;
          while(!grid->getCell(possible_pos).empty()&&depth<50) {
               possible_pos = vec3(randf(-5,5), 0, randf(-5,5));
               depth++;
          }
          marker->setPosition(possible_pos);
          marker->setPhysicsState(P_State::NONE);
          goals << marker->getPosition();

          g_ptr<nodenet> memory = make<nodenet>();
          memory->agent_id = agent->ID;
          Crumb mem;
          for(int r = 0; r < CRUMB_ROWS; r++) {
               for(int col = 0; col < 10; col++) {
                    mem.mat[r][col] = 0.0f;
               }
          }
          mem.mat[0][0] = 1.0f; //Distance saliance factor
          mem.mat[0][1] = 0.1f; //Navigation focus
          mem.mat[0][2] = 1.0f; //Salience impactor
          mem.mat[3][0] = randf(0.1f,0.3f); //Satiety
          mem.mat[3][1] = randf(0.2f,0.6f); //Shelter
          mem.mat[13][0] = randf(1.0f,1.8f); //Hunger
          mem.mat[13][1] = randf(0.1f,0.2f); //Tierdness
          mem.id = agent->ID;
          crumbs.put(agent->ID,mem);

          Crumb mem_root;
          g_ptr<Single> root_form = scene->create<Single>("crumb");
          root_form->setPosition(base_pos);
          mem_root.id = root_form->ID;
          memory->meta_root->archetype = mem_root;
          memory->id_to_node.put(root_form->ID,memory->meta_root.getPtr());
          memory->current_state = mem;
                         
          crumb_managers << memory;



          agent->physicsJoint = [agent, W_grouping, W_spatial, W_invariant, memory, marker, update_thread, &crumb_managers, &goals, &agents, &crumbs, &debug, phys, side_length]() {
               #if GTIMERS
               Log::Line overall;
               Log::Line l;
               list<double>& timers = agent->timers;
               list<std::string>& timer_labels = agent->timer_labels;
               timers.clear(); timer_labels.clear();
               overall.start();
               agent->opt_ints[4] = 0;
               // agent->opt_ints[5] = 0;
               #endif
               //Always setup getters for sketchpad properties because chances are they'll change later
               //Plus it helps with clarity to name them
               int id = agent->opt_ints[1];
               list<g_ptr<Single>>& my_debug = debug[id];
               g_ptr<Single> goal_crumb = marker;
               int& spin_accum = agent->opt_ints[2];
               int& on_frame = agent->opt_ints[3];
               float& goal_focus = agent->opt_floats[2];
               int& accum5 = agent->opt_ints[5];
               int& frames_since_progress = agent->opt_ints[7];
               int& nav_strat = agent->opt_ints[8];
               float& variance_threshold = agent->opt_floats[3];
               float navigation_focus = memory->navigation_focus();
               //The path can always be grabbed, and used to install directions in situations beyond A*, it's the only continous movment method that isn't velocity depentent.
               list<int>& path = agent->opt_idx_cache_2;
               //This is just for testing
               vec3 goal = goals[id];  //goal_crumb->getPosition();
               

               float moved_recently = agent->getPosition().distance(agent->opt_vec3_2);
               if(moved_recently > 2.0f) {
                    agent->opt_vec3_2 = agent->getPosition();
                    frames_since_progress = 0;
               } else {
                    frames_since_progress++;
               }

               if(agent->distance(marker) <= 10.0f) {
                    goal = marker->getPosition();
               }

               float desired_speed = agent->opt_floats[1];  
               on_frame++; 
               bool enable_astar = false;
               int pathing_interval = 15; 
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
                              // print("UNABLE TO FIND A PATH!");
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

                    if(frames_since_progress>=30) {
                         // list<int> new_path = grid->findPath(
                         //      grid->toIndex(agent->getPosition()),
                         //      grid->toIndex(waypoint),
                         //      [agent](int idx) {
                         //           if(grid->empty(idx)) return true;
                         //           if(grid->has_only(idx,agent->ID)) return true;
                         //           return false; // Occupied by walls or other agents
                         //      }
                         // );
                         // print("Came up with a new path with: ",new_path.length()," nodes");
                         // path << new_path;
                         agent->setPosition(waypoint); //Cheating for now so I can focus on other things
                    } 
                    if(waypoint_dist < grid->cellSize) {
                         path.removeAt(0);
                         if(!path.empty()) {
                              waypoint = grid->indexToLoc(path[0]);
                         } else {
                              // print("PATH COMPLETE!!");
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
                    vec3 current_velocity = agent->getVelocity().position;
                    float actual_speed = current_velocity.length();
                    float speed_ratio = actual_speed / std::max(desired_speed, 0.1f);
                    #if GTIMERS
                    l.start();
                    #endif
                    
                    // Tunable parameters
                    int num_rays = 24; 
                    float ray_distance = 30.0f;
                    float cone_width = 235.0f; 
                    
                    // Get goal direction for biasing
                    vec3 to_goal = agent->getPosition().direction(goal).nY();
                    float goal_distance = agent->getPosition().distance(goal);
                    vec3 forward =  agent->facing().nY(); //to_goal.length() > 0.01f ? to_goal.normalized() : agent->facing().nY();
                    vec3 right = vec3(forward.z(), 0, -forward.x()); // Perpendicular
                                        
                    // Cast rays in a forward-biased cone
                    float best_openness = 0.0f;
                    vec3 best_direction = forward;
                    
                    float start_angle = -cone_width / 2.0f;
                    float angle_step = cone_width / (num_rays - 1);

                    float best_goal_alignment = -1.0f;
                    float best_aligned_clearance = 0.0f;
                    
                    list<std::pair<float,int>> observed_objects;
                    list<g_ptr<Single>> new_crumbs;
                
                    #if VISPATHS
                    list<g_ptr<Single>> raycast_debug;
                    #endif
                    
                    list<float> ray_distances;

                    float left_clearance = 0.0f;
                    float right_clearance = 0.0f;
                    float left_count = 0.0f;
                    float right_count = 0.0f;

                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_setup"; l.start();
                    #endif

                    for(int i = 0; i < num_rays; i++) {
                        float angle_deg = start_angle + i * angle_step;
                        float angle_rad = angle_deg * 3.14159f / 180.0f;
                        
                        vec3 ray_dir = (forward * cos(angle_rad) + right * sin(angle_rad)).normalized();
                        
                        std::pair<float,int> hit_info = grid->raycast(
                            agent->getPosition(),
                            ray_dir,
                            ray_distance,
                            agent->ID
                        );
                        float hit_dist = hit_info.first;
                        int hit_cell = hit_info.second;
                        ray_distances << hit_dist;

                        if(angle_deg < 0) {
                              left_clearance += hit_dist;
                              left_count++;
                         } else {
                              right_clearance += hit_dist;
                              right_count++;
                         }
                        
                        #if VISPATHS
                        vec3 ray_end = agent->getPosition() + ray_dir * hit_dist;
                        raycast_debug << draw_line(agent->getPosition(), ray_end, "white", 0.5f);
                        #endif
                        
                        if(std::abs(angle_deg) < 90.0f) {  // Within ±90° of forward
                              if(hit_dist > best_openness) {
                                   best_openness = hit_dist;
                                   best_direction = ray_dir;
                              }
                              float goal_alignment = ray_dir.dot(to_goal.normalized());
                              if(goal_alignment > best_goal_alignment) {
                                  best_goal_alignment = goal_alignment;
                                  best_aligned_clearance = hit_dist;
                              }
                         }

                        bool curr_hit = (hit_cell >= 0 && !grid->cells[hit_cell].empty());
                        if(curr_hit) {
                            for(int obj_id : grid->cells[hit_cell]) {
                                if(obj_id != agent->ID && 
                                   !observed_objects.has({hit_dist, obj_id})) {
                                    observed_objects << std::make_pair(hit_dist, obj_id);
                                }
                            }
                        }
                    }
                    
                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_sampling"; l.start();
                    #endif

                    auto ray_tensor = make<tensor>(list<int>{1, 24}, ray_distances);
    
                    Pass matmul1 = {MATMUL, W_spatial};
                    auto edges = ray_tensor->forward(matmul1);
                    Pass relu1 = {RELU};
                    edges = edges->forward(relu1);

                    Pass matmul2 = {MATMUL, W_invariant};
                    auto signature = edges->forward(matmul2);
                    Pass relu2 = {RELU};
                    signature = signature->forward(relu2);

                    Pass matmul3 = {MATMUL, W_grouping};
                    auto consolidated = signature->forward(matmul3);
                    Pass relu3 = {RELU};
                    consolidated = consolidated->forward(relu3);
                    
                    float mean = 0.0f;
                    for(int f = 0; f < 6; f++) {
                        mean += consolidated->at({0, f});
                    }
                    mean /= 6.0f;
                    
                    float variance = 0.0f;
                    for(int f = 0; f < 6; f++) {
                        float diff = consolidated->at({0, f}) - mean;
                        variance += diff * diff;
                    }
                    variance /= 6.0f;
                    variance *= 100.0f; //To keep the numbers reasonable

                    int search_energy = 150;
                    float& threshold = agent->opt_floats[4];
                    float& match_threshold = agent->opt_floats[5];
                    float& growth_rate = agent->opt_floats[6];
                    float& sig_weight = agent->opt_floats[7];
                    float& spatial_weight = agent->opt_floats[8];

                    list<g_ptr<c_node>> matching_nodes;

                    // Only process gaps if there's enough spatial structure
                    list<vec3> gaps;
                    list<float> gap_sal;
                    list<g_ptr<tensor>> gap_sig;

                    #if GTIMERS
                    timers << l.end(); timer_labels << "gap_forward_passes"; l.start();
                    #endif

                    if(variance >= variance_threshold) {                    
                        for(int feat = 0; feat < 6; feat++) {
                            float activation = consolidated->at({0, feat});
                            // print(activation,"/",threshold);
                            if(activation > threshold) {
                                // Find the "center of mass" for this feature
                                // Weight each ray by how much it contributed to this feature
                                vec3 weighted_pos(0, 0, 0);
                                float total_weight = 0.0f;
                                
                                for(int i = 0; i < 24; i++) {
                                    // How much does ray i contribute to feature feat?
                                    // This comes from W_grouping weights (signature[i] -> consolidated[feat])
                                    float edge_activation = edges->at({0, i});
                                    if(edge_activation < threshold) continue;
                                    
                                    // Calculate ray position
                                    float angle_deg = start_angle + i * angle_step;
                                    float angle_rad = angle_deg * 3.14159f / 180.0f;
                                    vec3 ray_dir = (forward * cos(angle_rad) + right * sin(angle_rad)).normalized();
                                    vec3 ray_pos = agent->getPosition() + ray_dir * ray_distances[i]/2;
                                    
                                    // Weight by edge activation
                                    weighted_pos = weighted_pos + (ray_pos * edge_activation);
                                    total_weight += edge_activation;
                                }
                                
                                if(total_weight < 0.01f) continue;
                                vec3 gap_pos = weighted_pos / total_weight;

                                #if VISPATHS
                                    auto vismark = scene->create<Single>("white");
                                    vismark->setPosition(gap_pos+vec3(0,6,0));
                                    vismark->setScale({0.5f, 1, 0.5f});
                                    raycast_debug << vismark;
                                #endif
                                
                                if(!grid->empty(gap_pos)) {
                                    continue;
                                }
                                
                                Crumb query;

                                for(int f = 0; f < 6; f++) {
                                    query.mat[0][f] = consolidated->at({0, f});
                                }
                                
                                // Search for matching gap memory
                                g_ptr<c_node> best_match = nullptr;
                                float best_score = 0.0f;
                                
                                memory->propagate(memory->physics_attention.last(), search_energy, 
                                [&](g_ptr<c_node> node) {
                                    if(scene->singles[node->getID()]->dtype != "gap") return true;
                                    
                                    // Inflate during propagation
                                    for(int f = 0; f < 6; f++) {
                                        node->archetype.mat[0][f] *= growth_rate;
                                    }
                                    
                                    float spatial_dist = gap_pos.distance(node->getPosition());
                                    
                                    // Signature similarity
                                    float sig_score = 0.0f;
                                    for(int f = 0; f < 6; f++) {
                                        sig_score += query.mat[0][f] * node->archetype.mat[0][f];
                                    }
                                    
                                    float spatial_score = 1.0f / (1.0f + spatial_dist);
                                    float combined = sig_score * sig_weight + spatial_score * spatial_weight;
                                    
                                    // Collect ALL good matches, not just best
                                    if(combined > match_threshold) {
                                        matching_nodes << node;
                                    }
                                    
                                    return true;
                                });

                            // If ANY matches found, update them all and don't create new
                            if(!matching_nodes.empty()) {
                                for(auto match : matching_nodes) {
                                    for(int f = 0; f < 6; f++) {
                                        float delta = (query.mat[0][f] - match->archetype.mat[0][f]) * 0.1f;
                                        match->archetype.mat[0][f] += delta;
                                    }
                                }
                                memory->push_attention_head(matching_nodes[0]);  // Attend to first match
                            } else {
                                    // Novel gap - create new memory
                                    auto marker = scene->create<Single>("crumb");
                                    marker->setPosition(gap_pos);
                                    marker->hide();
                                    marker->dtype = "gap";
                                    
                                    Crumb gap_crumb = query;
                                    gap_crumb.id = marker->ID;
                                    
                                    crumbs.put(marker->ID, gap_crumb);
                                    memory->push_attention_head(memory->observe(gap_crumb, marker));
                                }
                            }
                        }
                        variance_threshold*=1.05f;
                    } else {
                        variance_threshold*=0.95f;
                    }
                    // variance_threshold = 0.005f;
                    //print(variance,"->",variance_threshold);
                    variance_threshold = std::max(0.0001f, std::min(3.0f, variance_threshold));

                    #if GTIMERS
                    timers << l.end(); timer_labels << "gap_processing"; l.start();
                    #endif

                    //print(gaps.length(),"<- ",variance,"/",variance_threshold);
                    //printnl("]\n");

                    // Cycle through strategies: 0=none, 1=left, 2=forward, 3=right
                    // int gap_strategy = 1;
                    vec3 gap_bias(0, 0, 0);
                    vec3 best_gap(0, 0, 0);
                    // float gap_curiosity = 1.0f;
                    float best_score = -1000.0f;

                    int gidx = -1;
                    for(int i=0;i<gaps.length();i++) {
                        float score = gap_sal[i];
                        vec3 to_gap = (gaps[i] - agent->getPosition()).normalized();
                        if(score > best_score) {
                              best_score = score;
                              best_gap = to_gap;
                              gidx = i;
                         }
                    }
                                
                    float gap_bias_strength = 10.6f;
                    float goal_bias_strength = 0.6f;
                    float wall_bias_strength = 0.4f;

                    gap_bias = best_gap * gap_bias_strength; // Weight for gap bias

                    // Decide movement: beeline if well-aligned and clear, otherwise use best scored direction
                    float beeline_threshold = 0.95f * goal_focus; // Scales with goal focus
                    bool can_beeline = (best_goal_alignment > beeline_threshold && 
                                        best_aligned_clearance >= goal_distance);
                    
                    left_clearance /= left_count;
                    right_clearance /= right_count;
                    vec3 wall_bias = right * ((right_clearance - left_clearance) / ray_distance) * wall_bias_strength;
                    
                    float wall_proximity = 1.0f / (std::min(left_clearance, right_clearance) + 1.0f);
                    float goal_weight = std::lerp(0.5f, 0.0f, wall_proximity);
                    vec3 goal_bias = to_goal * goal_weight * goal_bias_strength;
                    
                    //print("BEST DIR: ",best_direction.to_string(),", WALL_BIAS: ",wall_bias.to_string(),", GOAL_BIAS: ",goal_bias.to_string(),", GAP_BIAS: ",gap_bias.to_string());
                    vec3 environmental_force = can_beeline ? 
                         to_goal.normalized() : 
                         (best_direction + wall_bias + goal_bias + gap_bias).normalized();
                    // print("ENVIRO FORCE: ",environmental_force.to_string());
                    //     (best_openness > 1.0f ? best_direction : vec3(randf(-1, 1), 0, randf(-1, 1)));
                
                    agent->opt_ints[4] += observed_objects.length();
                
                    vec3 familiarity_vector(0, 0, 0);
                    int familiar_count = 0;
                    g_ptr<c_node> most_relevant = nullptr;
                    float highest_relevency = 0.0f;
                    // for(auto info : observed_objects) {
                    //      #if GTIMERS
                    //      l.start();
                    //      #endif
                
                    //      int obj_id = info.second;
                    //      //print("Crumbs has this key: ",obj_id,"? ",crumbs.hasKey(obj_id)?"Yes":"No");
                    //      Crumb obj_crumb = crumbs[obj_id]; //Make a copy
                    //      Crumb* obs = &obj_crumb;
                    //      g_ptr<Single> single = scene->singles[obj_id];

                    //      obs->id = single->ID;
                    //      float dist_to_obj = info.first;
                
                    //      #if GTIMERS
                    //      timers << l.end(); timer_labels << "observe_value_init"; l.start();
                    //      #endif
                    
                    //      g_ptr<c_node> seen_before = memory->has_seen_before_cheat(*obs);

                    //      #if GTIMERS
                    //      timers << l.end(); timer_labels << "has_seen_before"; l.start();
                    //      #endif

                    //      if(seen_before != nullptr) {
                    //           obs = &seen_before->archetype;
                    //           //memory->decrease_novelty(seen_before);
                    //           //print("I've seen this before... novelty is now ",obj_crumb->mat[0][1]);
                    //         //   float novelty = seen_before->archetype.mat[0][1];
                    //         //   if(novelty < 3.0f) { // Familiar
                    //         //       vec3 to_familiar = (obs->getPosition() - agent->getPosition()).normalized();
                    //         //       float familiarity_weight = (3.0f - novelty);
                                  
                    //         //       familiarity_vector = familiarity_vector + (to_familiar * familiarity_weight);
                    //         //       familiar_count++;
                    //         //   }
                    //      } 

                    //      #if GTIMERS
                    //      timers << l.end(); timer_labels << "seen_before_resolve"; l.start();
                    //      #endif

                    //      obs->mat[0][0] = 1.0f / (info.first + 1.0f); // Temporary distance for salience
                    //      float salience = memory->salience(*obs);
                    //      float relevance = memory->relevance(*obs, salience);
                    //      //print("Dist: ",1.0f / (info.first + 1.0f)," actual_dist: ",info.first," Sal: ",salience," Rel: ",relevance);
                    //      obs->mat[0][0] = 0.0f; // Clear temporary distance (don't persist)
                    //      #if GTIMERS
                    //      timers << l.end(); timer_labels << "sal_rel_calcs"; l.start();
                    //      #endif
                    //      if(relevance>0.3f) {
                    //           //Remove the gap Crumb if we used it
                    //           int found_idx = new_crumbs.find(single);
                    //           if(found_idx!=-1) {new_crumbs.removeAt(found_idx);}
                    //           g_ptr<c_node> node = nullptr;
                    //           if(seen_before == nullptr) {
                    //                //print("Look! Something new!");
                    //                //obs->mat[0][1] = 3.0f; //Novelty
                    //                //node = memory->observe(*obs,single);
                    //           } else {
                    //                node = seen_before;
                    //              //  memory->push_attention_head(seen_before);
                    //           }

                    //           if(relevance>highest_relevency) {
                    //                highest_relevency = relevance;
                    //                most_relevant = node;
                    //           }

                    //           #if VISPATHS
                    //                struct ray_debug {
                    //                     vec3 from;
                    //                     vec3 to;
                    //                     bool hit = false;
                    //                };

                    //                vec3 pos = single->getPosition();
                    //                int best = memory->physics_attention.length()-1;
                    //                float best_dist = 100.0f;
                    //                list<ray_debug> visrays;
                    //                for(int j = 0; j<memory->physics_attention.length();j++) {
                    //                     g_ptr<c_node> n = memory->physics_attention[j];
                    //                     float dist = n->getPosition().distance(pos);
                    //                     vec3 dir = n->getPosition().direction(pos);
                    //                     auto cast = grid->raycast(n->getPosition(),dir,dist,{agent->ID,n->getID(),single->ID});
                    //                     if(cast.second==-1&&dist<best_dist&&n->getID()!=single->ID) {
                    //                          best_dist = dist;
                    //                          best = j;
                    //                     }
                    //                     vec3 ray_end = n->getPosition() + dir * cast.first;
                    //                     visrays << ray_debug{n->getPosition(),ray_end,cast.second!=-1};
                    //                     // bool cansee = grid->can_see(n->getPosition(),pos,{id,n->getID()});
                    //                }
                    //                for(int j = 0; j<memory->physics_attention.length();j++) {
                    //                     g_ptr<c_node> n = memory->physics_attention[j];
                    //                     ray_debug d = visrays[j];
                    //                     raycast_debug << draw_line(d.from, d.to, j==best?"black":d.hit?"red":"cyan", j==best?2.8f:0.5f);
                    //                }
                    //           #endif

                    //           if(relevance>0.9f) {
                    //                //print("I wanna go there");
                    //                goals[id] = vec3(scene->transforms[obj_id][3]);
                    //                goal_focus = 1.0f; // Set high goal focus
                    //           }
                    //      } 


                    //      #if GTIMERS
                    //      timers << l.end(); timer_labels << "observe_resolution";
                    //      #endif
                    // }
                    // if(most_relevant) {
                    //      memory->push_attention_head(most_relevant);
                    // }
                    // if(familiar_count > 0) {
                    //      familiarity_vector = familiarity_vector.normalized(); // Average direction
                         
                    //      // Gentle override strength based on focus
                    //      float repulsion_blend = navigation_focus * goal_focus * 0.3f; // Tune this weight
                         
                    //      // Blend away from familiarity into the environmental force
                    //      environmental_force = (environmental_force * (1.0f - repulsion_blend)) + 
                    //                            (familiarity_vector * -repulsion_blend);
                    //      environmental_force = environmental_force.normalized();
                    //  }

                    for(auto r : new_crumbs) {
                        scene->recycle(r,"crumb");
                    }
                    #if VISPATHS
                        memory->visualize_registry(memory->physics_attention.last());
                        memory->debug << raycast_debug;
                    #endif
                    
                    #if GTIMERS
                    timers << l.end(); timer_labels << "raycast_postprocess"; l.start();
                    #endif
                
                    if(environmental_force.length() > 0.01f) {
                         environmental_force = environmental_force.normalized();
                         
                         // Blend with momentum (if we're already moving)
                         vec3 final_direction;
                         if(current_velocity.length() > 0.1f) {
                              final_direction = (current_velocity.normalized() * 0.2f + environmental_force * 0.8f).normalized();
                         } else {
                              final_direction = environmental_force;
                         }
                         
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


          agent->threadUpdate = [agent, i, memory, update_thread, &agents, &crumb_managers, &crumbs](){
               #if GTIMERS
                    Log::Line overall, l;
                    list<double>& timers = agent->timers2;
                    list<std::string>& timer_labels = agent->timer_labels2;
                    timers.clear(); timer_labels.clear();
                    overall.start();
                    l.start();
                    // agent->opt_ints[5] = 0;
               #endif
               
               static thread_local int on_frame = 0;


               int id = agent->opt_ints[1];
               g_ptr<nodenet> memory = crumb_managers[id];
               on_frame++; 
               


               #if GTIMERS
               timers << l.end(); timer_labels << "value setup"; l.start();
               #endif

     
               #if GTIMERS
               timers << l.end(); timer_labels << "thread flush";
               #endif

               int eval_interval = 600;
               if(on_frame % eval_interval == id % eval_interval) {
               
               }

               int recategorize_interval = 600;
               if(on_frame % recategorize_interval == id % recategorize_interval) {
                    #if GTIMERS
                    l.start();
                    #endif

                    
               }
               
               #if GTIMERS
               timers << overall.end(); timer_labels << "overall";
               #endif
          };
     }
     vec3 cam_pos(0, side_length * 0.8f, side_length * 0.8f);
     scene->camera.setPosition(cam_pos);
     scene->camera.setTarget(vec3(0, 0, 0));

     //Reward for the agent completing their goal
    //  g_ptr<Single> berry = make<Single>(berry_model);
    //  scene->add(berry);
    //  berry->dtype = "berry";
    //  berry->setPosition(agents[0]->getPosition()+agents[0]->facing()*2.0f);
    //  berry->setPhysicsState(P_State::PASSIVE);
    //  addToGrid(berry);
    //  // grid->make_seethru(berry->ID);
    //  Crumb berry_crumb;
    //  berry_crumb.mat[3][0] = 1.3f; //Satiety
    //  berry_crumb.mat[3][1] = 0.5f; //Shelter
    //  berry_crumb.id = berry->ID;
    //  crumbs.put(berry->ID,berry_crumb);
    //  //crumb_managers[0]->observe(berry_crumb,berry);

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

     
     list<std::function<void()>> from_phys_to_update;

     map<std::string,double> total_times;
     S_Tool phys_logger;
     #if GTIMERS
     phys_logger.log = [agents,phys,&crumb_managers,&total_times,amt](){
          map<std::string,double> times;
          int accum4 = 0;
          int accum5 = 0;
          for(auto a : agents) {
               for(int i=0;i<a->timers.length();i++) {
                    times.getOrPut(a->timer_labels[i],0) += a->timers[i];
               }
               accum4 += a->opt_ints[4];
               accum5 += a->opt_ints[5];
               a->opt_ints[5] = 0;
          }
          print("------------\n AGENT JOINT TIMES");
          for(auto e : times.entrySet()) {
               print(e.key,": ",ftime(e.value)," (total: ",ftime(total_times.getOrDefault(e.key,0.0)),")");
          }
          for(auto e : total_times.entrySet()) {
               if(!times.hasKey(e.key)) {
                    print("(",e.key,": ",ftime(e.value),")");
               }

          }
          total_times.clear();
        //   print("Total crumbs observed: ",accum4);
        //   print("Evaluated spatial queries since last log: ",accum5);

        //   std::function<int(g_ptr<c_node>)> instances_in_cateogry = [&](g_ptr<c_node> c){
        //        int total = c->store->crumbs.length();
        //        for(auto child : c->children) {
        //             total += instances_in_cateogry(child);
        //        }
        //        return total;
        //   };

        //   int total = 0;
        //   int start_idx = randi(0,amt-1);
        //   for(int i=start_idx;i<start_idx+10;i++) {
        //        if(i>=amt) break;
        //        g_ptr<nodenet> c = crumb_managers[i];
        //        total += instances_in_cateogry(c->meta_root);
        //   }
        //   print("Avg crumbs in memory: ",total/10);
     };
     #endif

    #if TRIALS
        // Best found so far
        float best_fitness = -1000.0f;
        list<float> best_params;

        // Parameter ranges to search
        list<float> threshold_range = {0.005f, 0.01f, 0.02f, 0.05f};
        list<float> match_range = {1.0f, 2.0f, 3.0f, 5.0f, 10.0f};
        list<float> growth_range = {1.01f, 1.02f, 1.04f, 1.06f};
        list<float> sig_weight_range = {0.5f, 0.7f, 0.9f};
        list<float> spatial_weight_range = {5.0f, 10.0f, 20.0f, 50.0f};

        int population_size = 20;
        int generations = 50;
        int current_generation = 0;
        int current_genome = 0;
        int frames_per_genome = 5000;

        // Population stored as list of parameter arrays
        list<list<float>> population;
        list<float> fitness_scores;

        // Initialize population with random genomes
        for(int i = 0; i < population_size; i++) {
            list<float> genome = {
                threshold_range[randi(0, threshold_range.length()-1)],
                match_range[randi(0, match_range.length()-1)],
                growth_range[randi(0, growth_range.length()-1)],
                sig_weight_range[randi(0, sig_weight_range.length()-1)],
                spatial_weight_range[randi(0, spatial_weight_range.length()-1)]
            };
            population << genome;
            fitness_scores << -999999.0f;
        }

        // Genetic operators as lambdas
        auto mutate = [](list<float>& genome, float rate = 0.2f) {
            if(randf(0, 1) < rate) genome[0] *= randf(0.8f, 1.2f);
            if(randf(0, 1) < rate) genome[1] *= randf(0.8f, 1.2f);
            if(randf(0, 1) < rate) genome[2] += randf(-0.01f, 0.01f);
            if(randf(0, 1) < rate) genome[3] += randf(-0.2f, 0.2f);
            if(randf(0, 1) < rate) genome[4] *= randf(0.8f, 1.2f);
            
            // Clamp to valid ranges
            genome[0] = std::max(0.001f, std::min(0.1f, genome[0]));
            genome[1] = std::max(0.5f, std::min(15.0f, genome[1]));
            genome[2] = std::max(1.0f, std::min(1.1f, genome[2]));
            genome[3] = std::max(0.1f, std::min(1.0f, genome[3]));
            genome[4] = std::max(1.0f, std::min(100.0f, genome[4]));
        };

        auto crossover = [](const list<float>& parent1, const list<float>& parent2) {
            list<float> child;
            for(int i = 0; i < parent1.length(); i++) {
                child << (randf(0, 1) < 0.5f ? parent1[i] : parent2[i]);
            }
            return child;
        };

        auto tournament_select = [&](int tournament_size = 3) {
            int best_idx = randi(0, population_size - 1);
            float best_fit = fitness_scores[best_idx];
            
            for(int i = 1; i < tournament_size; i++) {
                int idx = randi(0, population_size - 1);
                if(fitness_scores[idx] > best_fit) {
                    best_fit = fitness_scores[idx];
                    best_idx = idx;
                }
            }
            return best_idx;
        };

        int current_trial = 0;
        int total_trials = 100;
        int frames_per_trial = 5000;
        int frame_count = 0;
    #endif
    run_thread->name = "Physics";
    Log::Line time; time.start();
    phys->gravity = 0.0f;
    run_thread->run([&](ScriptContext& ctx){
          phys_logger.tick();
          if(use_grid&&phys->collisonMethod!=Physics::GRID) {
               phys->collisonMethod = Physics::GRID;
               phys->grid = grid;
          } else if(!use_grid&&phys->collisonMethod!=Physics::AABB) {
               phys->collisonMethod = Physics::AABB;
               phys->treeBuilt3d = false;
          }
          phys->updatePhysics();

          #if TRIALS
          frame_count++;
          
          if(frame_count >= frames_per_genome) {
               // Evaluate ALL agents for current genome
               float total_quality = 0.0f;
               int total_agents_evaluated = 0;
               
               for(int i = 0; i < agents.length(); i++) {
                    g_ptr<Single> agent = agents[i];
                    g_ptr<nodenet> memory = crumb_managers[i];
                    
                    int nodes = memory->id_to_node.size();
                    int invalid = 0;
                    int valid = 0;
                    
                    // Walk graph and count connections
                    memory->profile(memory->meta_root, [&](g_ptr<c_node> node) {
                         for(auto child : node->children) {
                              vec3 from = node->getPosition();
                              vec3 to = child->getPosition();
                              
                              if(!grid->can_see(from, to, {agent->ID, node->getID(), child->getID()})) {
                                   invalid++;
                              } else {
                                   valid++;
                              }
                         }
                         return true;
                    });
                    
                    // Quality: valid connections per node, heavily penalize invalid
                    float quality = 0.0f;
                    if(nodes > 0) {
                         quality = ((float)valid / (float)nodes) - (invalid * 5.0f);
                    } else {
                         quality = -invalid * 10.0f;
                    }
                    
                    total_quality += quality;
                    total_agents_evaluated++;
               }
               
               // Store fitness for current genome
               fitness_scores[current_genome] = total_quality / std::max(1, total_agents_evaluated);
               
               print("Gen ", current_generation, " | Genome ", current_genome, " | Fitness: ", fitness_scores[current_genome]," | Time: ",ftime(time.end()));
               
               current_genome++;
               
               // Check if generation is complete
               if(current_genome >= population_size) {
                    print("\n=== GENERATION ", current_generation, " COMPLETE ===");
                    
                    // Find best of this generation
                    int best_idx = 0;
                    for(int i = 1; i < population_size; i++) {
                         if(fitness_scores[i] > fitness_scores[best_idx]) {
                              best_idx = i;
                         }
                    }
                    print("Best fitness: ", fitness_scores[best_idx]);
                    print("Best params: thresh=", population[best_idx][0], " match=", population[best_idx][1],
                          " growth=", population[best_idx][2], " sig_w=", population[best_idx][3],
                          " spat_w=", population[best_idx][4]);
                    
                    if(fitness_scores[best_idx] > best_fitness) {
                         best_fitness = fitness_scores[best_idx];
                         best_params = population[best_idx];
                         print("*** NEW ALL-TIME BEST! ***");
                    }
                    
                    current_generation++;
                    current_genome = 0;
                    
                    if(current_generation < generations) {
                         // Sort population by fitness
                         list<int> indices;
                         for(int i = 0; i < population_size; i++) indices << i;
                         indices.sort([&](int a, int b) { return fitness_scores[a] > fitness_scores[b]; });
                         
                         // Build next generation
                         list<list<float>> next_pop;
                         list<float> next_fitness;
                         
                         // Elitism: keep top 5
                         print("Elite genomes: ");
                         for(int i = 0; i < 5; i++) {
                              next_pop << population[indices[i]];
                              next_fitness << fitness_scores[indices[i]];
                              print("  ", i+1, ": ", fitness_scores[indices[i]]);
                         }
                         
                         // Crossover + mutation for remaining 15
                         while(next_pop.length() < population_size) {
                              int p1 = tournament_select();
                              int p2 = tournament_select();
                              list<float> child = crossover(population[p1], population[p2]);
                              mutate(child);
                              next_pop << child;
                              next_fitness << -999999.0f;
                         }
                         
                         population = next_pop;
                         fitness_scores = next_fitness;
                         
                         print("Starting generation ", current_generation, "...\n");
                    } else {
                         print("\n=== EVOLUTION COMPLETE ===");
                         print("Best fitness across all generations: ", best_fitness);
                         print("Best params: thresh=", best_params[0], " match=", best_params[1],
                               " growth=", best_params[2], " sig_w=", best_params[3],
                               " spat_w=", best_params[4]);
                         run_thread->pause();
                         time.start();
                         frame_count = 0;
                         return;
                    }
               }
               
               // Set next genome params for all agents
               for(int i = 0; i < agents.length(); i++) {
                    agents[i]->opt_floats[4] = population[current_genome][0];
                    agents[i]->opt_floats[5] = population[current_genome][1];
                    agents[i]->opt_floats[6] = population[current_genome][2];
                    agents[i]->opt_floats[7] = population[current_genome][3];
                    agents[i]->opt_floats[8] = population[current_genome][4];
                    
                    // Reset agent position (random spawn)
                    vec3 spawn = vec3(randf(-10, 10), 0, randf(-10, 10));
                    agents[i]->setPosition(spawn);
                    agents[i]->setLinearVelocity(vec3(0, 0, 0));
               }
               
               // Nuke memory
               for(int i = 0; i < agents.length(); i++) {
                    g_ptr<nodenet> memory = crumb_managers[i];
                    
                    // Walk and break all connections
                    list<c_node*> all_nodes;
                    for(auto entry : memory->id_to_node.entrySet()) {
                         all_nodes << entry.value;
                    }
                    
                    for(auto node : all_nodes) {
                         node->parents.clear();
                         node->children.clear();
                         
                         if(node->getID() != -1) {
                            scene->recycle(scene->singles[node->getID()],"crumb");
                         }
                    }
                    
                    memory->id_to_node.clear();
                    memory->physics_attention.clear();
                    memory->physics_attention << memory->meta_root;
               }
               
               total_connections = 0;
               make_maze();
               
               time.start();
               frame_count = 0;
          }
     #endif

          for(auto a : agents) {
               for(int i=0;i<a->timers.length();i++) {
                    total_times.getOrPut(a->timer_labels[i],0) += a->timers[i];
               }
          }
     },0.008f);
     #if GTIMERS
          run_thread->logSPS = true;
     #endif

     S_Tool update_logger;
     #if GTIMERS
     update_logger.log = [agents,&crumb_managers](){
          map<std::string,double> times;
          int accum4 = 0;
          int accum5 = 0;
          for(auto a : agents) {
               for(int i=0;i<a->timers2.length();i++) {
                    times.getOrPut(a->timer_labels2[i],0) += a->timers2[i];
               }
               accum4 += a->opt_ints[4];
               accum5 += a->opt_ints[5];
          }
          print("------------\n AGENT UPDATE TIMES");
          for(auto e : times.entrySet()) {
               print(e.key,": ",ftime(e.value));
          }
          // print("Observations processed: ",accum5);

          print("TOTAL CONNECTIONS: ",add_commas(total_connections));
     };
     #endif
     update_thread->name = "Update";
     update_thread->run([&update_logger,&agents,&from_phys_to_update](ScriptContext& ctx){
          update_logger.tick();
          for(auto a : agents) {
               if(a->threadUpdate&&a->isActive())
                    a->threadUpdate();
          }
          if(!from_phys_to_update.empty()) {
               for(auto p : from_phys_to_update) {
                      p();
               }
               from_phys_to_update.clear();
            }
     },0.008f);
     #if TRIALS
        run_thread->setSpeed(0.00001f);
        update_thread->pause();
     #endif
     #if GTIMERS
          update_thread->logSPS = true;
     #endif


     S_Tool s_tool;
     #if GTIMERS
          s_tool.log_fps = true;
     #endif

     //For tree visualization
     g_ptr<nodenet> mem = crumb_managers[0];
     g_ptr<c_node> current_view = mem->meta_root;
     int child_index = 0;  // Which child we're looking at
     int parent_index = 0; // Which parent we're looking at

     start::run(window,d,[&]{
          s_tool.tick();
          if(pressed(N)) {
               scene->camera.setTarget(agents[0]->getPosition()+vec3(0,20,20));
          }
          vec3 mousepos = scene->getMousePos();

          auto q = scene->nearestElement();

   

          if(pressed(SPACE)) {
               if(run_thread->runningTurn) run_thread->pause();
               else run_thread->start();
          }
          if(pressed(P)) {
               if(held(LSHIFT)) {
                    run_thread->setSpeed(run_thread->getSpeed()/2); 
               } else {
                    if(run_thread->getSpeed()==0) 
                         run_thread->setSpeed(1.0f);
                    run_thread->setSpeed(run_thread->getSpeed()*2); 
               }
               print("Thread speed to: ",run_thread->getSpeed());
          }

          if(amt==1) {
               // Navigate DOWN to children
               if(pressed(E)) {
                    if(held(LSHIFT)) { // Cycle through siblings
                        if(!current_view->children.empty()) {
                            child_index = (child_index + 1) % current_view->children.length();
                            mem->visualize_structure(current_view);
                            print("Viewing child ", child_index, " of ", current_view->children.length());
                        }
                    } else {
                        if(!current_view->children.empty()) {
                            // Clamp to valid range before moving
                            child_index = child_index % current_view->children.length();
                            current_view = current_view->children[child_index];
                            mem->visualize_structure(current_view);
                            print("Moved to child ", child_index, ". Children: ", current_view->children.length(), 
                                  ", Parents: ", current_view->parents.length());
                        }
                    }
                }
               
               // Navigate UP to parents
               if(pressed(Q)) {
                    if(held(LSHIFT)) { // Cycle through parents
                         if(!current_view->parents.empty()) {
                             parent_index = (parent_index + 1) % current_view->parents.length();
                             mem->visualize_structure(current_view);
                             print("Viewing parent ", parent_index, " of ", current_view->parents.length());
                         }
                     } else {
                         if(!current_view->parents.empty()) {
                             g_ptr<c_node> parent(current_view->parents[parent_index]);
                             current_view = parent;
                             // DON'T reset indices - keep them!
                             mem->visualize_structure(current_view);
                             print("Moved to parent ", parent_index, ". Children: ", current_view->children.length(), 
                                   ", Parents: ", current_view->parents.length());
                         }
                     }
               }
               
               // Reset to agent's current attention
               if(pressed(R)) {
                    current_view = mem->physics_attention.last();
                    child_index = 0;
                    parent_index = 0;
                    mem->visualize_structure(current_view);
                    print("Reset to current attention head");
               }

               //Visualize from root
               if(pressed(V)) {
                    mem->visualize_structure(mem->meta_root);
                    current_view = mem->meta_root;
                    print("Viewing full tree from root");
               }
               
               if(pressed(C)) {
                    mem->clear_debug();
               }

               if(pressed(G)) {
                    mem->visualize_registry();
               }

               // if(pressed(MOUSE_LEFT)) {
               //      vec3 from = agents[0]->getPosition();
               //      vec3 point = scene->getMousePos();
               //      float dist = from.distance(point);
               //      print("Dist: ",dist);
               //      vec3 dir = from.direction(point);
               //      auto cast = grid->raycast(from,dir,dist+1.0f,agents[0]->ID);
               //      print("Hit at: ",cast.first," cell: ",cast.second," which has ",grid->getCell(cast.second).length()," objects");
               //      for(auto d : grid->getCell(cast.second)) {
               //           print("Blocked by a : ",scene->singles[d]->dtype);
               //      }
               //      crumb_managers[0]->debug << draw_line(from,from + dir * cast.first,"white",0.5f);
     
               //      print("Can he see your mouse? ",grid->can_see(agents[0]->getPosition(),scene->getMousePos())?"Yes!":"No.");
               //      print("Can he see your mouse if he's exclued? ",grid->can_see(agents[0]->getPosition(),point,{agents[0]->ID})?"Yes!":"No.");
     
               // }
          }





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
