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
    list<int> idxs = grid->addToGrid(q->ID,q->getWorldBounds());
    q->opt_idx_cache = idxs;
    return idxs;
}
list<int> removeFromGrid(g_ptr<Single> q) {
    for(auto i : q->opt_idx_cache) {
        grid->cells[i].erase(q->ID);
    }
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
    g_ptr<Single> draw_line_3d(vec3 start, vec3 end, vec4 color) {
        g_ptr<Single> line = scene->create<Single>("white");
        line->setColor(color);
        
        float dist = start.distance(end);
        line->setScale({0.1f, 0.1f, dist});
        
        vec3 midpoint = (start + end) * 0.5f;
        line->setPosition(midpoint);
        
        // Assuming line geometry points along +Z axis by default
        vec3 default_dir = vec3(0, 0, 1);
        vec3 target_dir = (end - start).normalized();
        
        // Calculate rotation axis and angle
        vec3 axis = default_dir.cross(target_dir);
        float axis_len = axis.length();
        
        if(axis_len < 0.0001f) {
            // Parallel or anti-parallel
            if(default_dir.dot(target_dir) < 0) {
                // Anti-parallel: 180° rotation
                line->rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(1, 0, 0));
            }
            // Else: already aligned, identity rotation (default)
        } else {
            // Normal case
            axis = axis.normalized();
            float angle = acos(std::clamp(default_dir.dot(target_dir), -1.0f, 1.0f));
            line->rotation = glm::angleAxis(angle, axis.toGlm());
        }
        
        line->updateTransform();
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

         if(randf(0,1)<0.5f)
            wall->setColor(vec4(1,0,0,1));
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
#define VISGRID 0
#define TRIALS 0
// #define CRUMB_ROWS 40 
// // Packed ints with start / count so we can be more dynamic!
// const int META  = (0 << 16) | 1; 
// const int IS    = (1 << 16) | 10;
// const int DOES  = (11 << 16) | 10;
// const int WANTS = (21 << 16) | 10;
// const int HAS   = (31 << 16) | 9;
// const int ALL = (0 << 16) | CRUMB_ROWS;

#define CRUMB_ROWS 20
//Verbs
const int META  = (0 << 16) | 1; 
const int IS    = (1 << 16) | 10;
const int DOES  = (6 << 16) | 9;
const int ALL = (0 << 16) | CRUMB_ROWS;

//Columns
const int MET = 0; //Meta
const int OBS = 1; //Observed
const int CUR = 11; //Curiositiy
const int LER = 12; //Learned

//Rows
const int CUR_OBSERVE = 0; const int CUR_INTERACT = 1; const int CUR_USE = 2; const int CUR_DROP = 3; const int CUR_HIT = 4;
const int BOREDOM = 0; const int HUNGER = 1; const int FATIGUE = 2;

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
    list<Crumb> sub;

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
    int eval_start = (eval_verb >> 16) & 0xFFFF; int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF; int against_count = against_verb & 0xFFFF;

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

static float sub_evaluate(const Crumb& eval, int eval_verb, const Crumb& against, int against_verb) {
    int eval_start = (eval_verb >> 16) & 0xFFFF; int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF; int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count); 

    float score = 0.0f;
    const float* eval_ptr = &eval.mat[eval_start][0];
    const float* against_ptr = &against.mat[against_start][0];
    int total = eval_count * 10;
    for(int i = 0; i < total; i++) {
         score += eval_ptr[i] - against_ptr[i];
    }

    return score;
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

static void sub_mask(Crumb& t, int tv, const Crumb& m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a - b; });
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

 struct c_node;

static list<g_ptr<Single>> agents;
static list<vec3> goals;
static map<int,Crumb> crumbs;

 struct Instance : public q_object {
    g_ptr<Single> form = nullptr;
    vec3 position = vec3(0,0,0);
    int last_seen = 0;
    c_node* vantage = nullptr;
    c_node* category = nullptr; 

    bool is_stale(int current_frame, int max_age = 1000) const {
        return (current_frame - last_seen) > max_age;
    }
    
    bool verify() const {
        if(!scene->active[form->ID]) return false;
        float dist = (form->getPosition() - position).length();
        return dist < 2.0f;
    }
};

struct c_node : public q_object {
    c_node() {
        cmask.setmat(1.0f);
    }
    c_node(std::string _label) {
        label = _label;
        cmask.setmat(1.0f);
    }

    std::string label;
    Crumb archetype;
    Crumb cmask;
    
    list<c_node*> parents;
    list<g_ptr<c_node>> children;
    list<g_ptr<Instance>> instances; 

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
    
    void update_instance(int instid,int onframe,const vec3& pos, c_node* new_node) {
        instances[instid]->last_seen = onframe;
        instances[instid]->vantage = this;
        instances[instid]->category = new_node;
        instances[instid]->position = pos;
    }
    void add_instance(g_ptr<Single> inst,int onframe,c_node* node) {
        int instid = -1;
        for(int i=instances.length()-1;i>=0;i--) {
            if(instances[i]->verify()) {
                if(instances[i]->form == inst) {
                    instid = i;
                    break;
                } 
            } else { //Just cleaning up instances here because we're iterating anyways
                instances[i]->category->instances.erase(instances[i]);
                instances.removeAt(i);
            }
        }
        if(instid!=-1) {
            update_instance(instid,onframe,inst->getPosition(),node);
        } else {
            g_ptr<Instance> ninst = make<Instance>();
            ninst->form = inst; ninst->category = node; ninst->vantage = this;
            ninst->last_seen = onframe; ninst->position = inst->getPosition();
            instances << ninst;
            ninst->vantage = this;
            if(node != this) {
                node->instances << ninst;
            }
        }
    }


    list<g_ptr<c_node>> neighbors() {
        list<g_ptr<c_node>> n;
        n << children;
        for(auto p : parents) n << p;
        return n;
    }

    float matches_category(const Crumb& item) {
        return std::abs(sub_evaluate(archetype,IS,item,IS));
    }   
    float matches_category(g_ptr<Single> item) {
        if(crumbs.hasKey(item->ID)) {
            return matches_category(crumbs.get(item->ID));
        } else {
            print("c_node::matches_cateogry no crumb found in global registry for ",item->dtype,"-",item->ID);
            return 1000.0f;
        }
    }   

    // void add_to_category(Crumb& n_crumb) {
    //     get_crumbs() << n_crumb;
    // }
    
    g_ptr<c_node> subnode(const Crumb& seed) {
        g_ptr<c_node> new_cat = make<c_node>();
        new_cat->archetype = std::move(seed);
        new_cat->parents << this;
        children << new_cat;
        
        return new_cat;
    }
    // void split(const list<list<int>>& to_split, int verb = ALL) {
    //     for(int s = 0; s<to_split.length();s++) {
    //         list<Crumb> crumbs;
    //         for(int i=0;i<to_split[s].length();i++) {
    //                 crumbs.push(get_crumbs()[to_split[s][i]]);
    //         }
            
    //         Crumb new_arc;
    //         for(Crumb& c : crumbs) {
    //                 add_mask(new_arc,verb,c,verb);
    //         }
    //         scale_mask(new_arc,verb,1.0f / crumbs.length());

    //         auto new_cat = subnode(new_arc);
    //         new_cat->set_crumbs(crumbs);
    //     }
    // }
};




static int total_connections = 0;
static float min_sal = 1.0f;

struct Event {
    Event(float _time, std::function<void()> _func) : time(_time), func(_func) {}
    Event() {}
    std::function<void()> func;
    float time;
    bool tick(float tpf) {
        if((time-=tpf)<=0) {
            func();
            return true;
        }
        return false;
    }
};
list<Event> events;

g_ptr<Single> objInFront(g_ptr<Single> agent) {
    std::pair<float,int> info = grid->raycast(agent->getPosition(),agent->facing(),grid->cellSize*2.0f,agent->ID);
    #if VISPATHS
        g_ptr<Single> vismark = draw_line_3d(agent->getPosition(),agent->getPosition()+(agent->facing()*info.first),vec4(1,0,0,1));
        events << Event(1.0f,[vismark](){scene->recycle(vismark);});
    #endif
    if(info.second!=-1) {
        if(!grid->empty(info.second)) {
            return scene->singles[grid->cells[info.second][0]];
        } 
    }
    return nullptr;
}

list<g_ptr<Single>> objInSweep(g_ptr<Single> agent) {
    list<g_ptr<Single>> to_return;
    vec3 pos = agent->getPosition()+agent->facing()*2.0f;
    float r = 1.0f;
    list<int> cells = grid->cellsAround(pos,r);
    #if VISPATHS
        g_ptr<Single> vismark = scene->create<Single>("white");
        vismark->setColor(vec4(1,0,0,1));
        vismark->setPosition(pos);
        vismark->setScale({r*2,r,r*2});
        events << Event(1.0f,[vismark](){scene->recycle(vismark);});
    #endif
    for(auto idx : cells) {
        for(auto sidx : grid->cells[idx]) {       
            to_return << scene->singles[sidx];
        }
    }
    return to_return;
}


class nodenet : public Object {
public:
    nodenet(Crumb& agent_state) : state(agent_state) {
        meta_root = make<c_node>("meta");
        obs_root = make<c_node>("observed");

        physics_attention << meta_root;
        cognitive_attention << obs_root;

        for(int i = 0; i < CRUMB_ROWS; i++)
        for(int j = 0; j < 10; j++)
            cmask.mat[i][j] = 1.0f;
    };
    ~nodenet() {};

    Crumb cmask;

    g_ptr<c_node> meta_root = nullptr;
    g_ptr<c_node> obs_root = nullptr;

    Crumb& state;
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
        return state.mat[0][1];
    }

    float spatial_salience(const Crumb& observation) {
        return evaluate(state, META, observation, META);
    }

    float desire_salience(const Crumb& observation) {
        return evaluate(state, DOES, observation, DOES);
    }

    float salience(const Crumb& observation) {
        // float sp_sal = spatial_salience(observation);
        float ds_sal = desire_salience(observation);
        return ds_sal;
        // print("SP_SAL: ",sp_sal);
        // print("DS_SAL: ",ds_sal);
        // print("NAV FOCUS: ",navigation_focus());
        // print("SAL: ",std::lerp(sp_sal,ds_sal,navigation_focus()));
        // return std::lerp(ds_sal,sp_sal,navigation_focus());
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

        g_ptr<c_node> best_node = closest_attended_node(single);
        node->parents << best_node.getPtr();
        best_node->children << node;
        total_connections+=2;

        // int l = physics_attention.length();
        // //Do the N last objects of our attention list
        // int n = 1;
        // for(int i= l<n ? 0: l-n ;i<l;i++) {
        //      auto recent = physics_attention[i];
        //      node->parents << recent.getPtr();
        //      recent->children << node;
        //      total_connections += 2;
        // }

        if(!node) {
            print("OBSERVE WARNING: formless archetpye for category!");
        } else if(single) {
            id_to_node.put(node->getID(),node.getPtr());
        } else {
            //Push to some grid form?
        }
        return node;
    }

    //Standard action unit made to be used across any platform or purpouse
    struct Action {
        Action(int _id) : part_id(_id) {}
        Action(int _id, list<float> _params) : part_id(_id), params(_params) {}
        ~Action() {}

        int part_id;
        list<float> params;  // Could be positions, velocities, etc.
        float duration;      // Timing
        float max_effort;    // Force/torque limits (optional)

        vec3 asvec3(int offset=0) const {
            vec3 to_return;
            for(int i=0;i<3;i++) {
                if(i==0) to_return.x(params[i+offset]);
                if(i==1) to_return.y(params[i+offset]);
                if(i==2) to_return.z(params[i+offset]);
            }
            return to_return;
        }
    };

    //Visual abstraction (what each see pass populates, carries metadata about its production for reflection)
    struct Vis : public q_object {
        Vis() {}
        Vis(int nrays, float rdist, float cwidth) : num_rays(nrays), ray_distance(rdist), cone_width(cwidth) {}

        list<float> dists;
        list<vec3> dirs;
        list<int> cells;
        int num_rays = 0;
        float ray_distance = 0.0f;
        float cone_width = 0.0f;
        
        vec3 forward;
        vec3 pos;

        list<vec3> features;

        //For 3d
        list<vec3> colors;
        list<vec3> normals;
        int elevation_samples = 0; 
        int azimuth_samples = 0; 
        float vertical_fov = 0.0f; 
    };

    g_ptr<Vis> vis = nullptr; //The cached struct for the data of the visual pass
    g_ptr<tensor> visual_signature = nullptr; //Produced features by the visual system

    // g_ptr<c_node> find_goal() {

    // }

    g_ptr<c_node> closest_attended_node(g_ptr<Single> single) {
        float best_dist = 1000.0f;
        g_ptr<c_node> best_node = physics_attention.last();
        vec3 pos = single->getPosition();
        for(auto n : physics_attention) {
            float dist = n->getPosition().distance(pos);
            if(dist<best_dist) {
                if(grid->can_see(n->getPosition(),pos,{agent_id,single->ID,n->getID()})) {
                    best_node = n;
                    best_dist = dist;
                }
            }
        }
        return best_node;
    }

    //Visual cortex method, when called it populates the vis for other passes to use
    void see2d(g_ptr<Single> agent,int num_rays,float ray_distance,float cone_width) {
        vis = make<Vis>(num_rays,ray_distance,cone_width);

        vec3 forward =  agent->facing().nY();
        vec3 right = vec3(forward.z(), 0, -forward.x());
        float start_angle = -cone_width / 2.0f;
        float angle_step = cone_width / (num_rays - 1);

        vis->forward = forward;
        vis->pos = agent->getPosition();

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
            vis->dists << hit_info.first;
            vis->cells << hit_info.second;   
            vis->dirs << ray_dir;      
            #if VISPATHS
                vec3 ray_end = agent->getPosition() + ray_dir * hit_info.first;
                debug << draw_line(agent->getPosition(), ray_end, "white", 0.5f);
            #endif
        }
    }

    void observe_instances(g_ptr<Single> agent,int on_frame) {
        vec3 forward =  vis->forward;
        vec3 right = vec3(forward.z(), 0, -forward.x()); 
        float start_angle = -vis->cone_width / 2.0f;
        float angle_step = vis->cone_width / (vis->num_rays - 1);
        list<g_ptr<Single>> observed_objects;
        for(int i = 0; i < vis->num_rays; i++) {
            float angle_deg = start_angle + i * angle_step;
            float angle_rad = angle_deg * 3.14159f / 180.0f;
            
            vec3 ray_dir = vis->dirs[i];
            float hit_dist = vis->dists[i];
            float hit_cell = vis->cells[i];
            bool curr_hit = (hit_cell >= 0 && !grid->cells[hit_cell].empty());
            if(curr_hit) {
                for(int obj_id : grid->cells[hit_cell]) {
                    g_ptr<Single> single = scene->singles[obj_id];
                    if(obj_id != agent->ID && !observed_objects.has(single)) {
                        observed_objects << single;
                    }
                }
            }
        }

        list<std::pair<g_ptr<Single>,c_node*>> add_inst;

        int energy = 1000;
        propagate(obs_root,energy,[&observed_objects,&add_inst](g_ptr<c_node> node){
            for(int i=observed_objects.length()-1;i>=0;i--) {
                g_ptr<Single> s = observed_objects[i];
                if(node->matches_category(s)==0.0f) { //setting it at 0.1 for now because we don't have big object diversity in the crumb
                    add_inst <<  std::make_pair(s,node.getPtr());
                    observed_objects.removeAt(i);
                }
            }   
            return !observed_objects.empty();
        });

        list<g_ptr<c_node>> new_cats;

        //All the ones with no category match found
        for(auto o : observed_objects) {
            bool catagorized = false;
            for(auto c : new_cats) { //Same as above
                if(c->matches_category(o)==0.0f) { 
                    add_inst << std::make_pair(o,c.getPtr());
                    catagorized = true;
                    break;
                }
            }
            if(!catagorized) {
                Crumb arct = crumbs.get(o->ID);
                arct.mat[CUR][CUR_OBSERVE] = 10.0f;
                arct.mat[CUR][CUR_INTERACT] = 10.0f;
                arct.mat[CUR][CUR_USE] = 10.0f;
                arct.mat[CUR][CUR_DROP] = 10.0f;
                arct.mat[CUR][CUR_HIT] = 10.0f;
                g_ptr<c_node> new_node = obs_root->subnode(arct);
                new_node->label = o->dtype; //debug label
                new_cats << new_node;
                add_inst << std::make_pair(o,new_node.getPtr());
            }
        }

        // print("ID OF PHYS_HEAD: ",physics_attention.last()->getID()," LABEL: ",physics_attention.last()->label);
        for(auto instpair : add_inst) {
            closest_attended_node(instpair.first)->add_instance(instpair.first,on_frame,instpair.second);
        }
    }

    void process_visual_for_gaps(g_ptr<Single> agent) {
        int input_size = vis->num_rays;
        int search_energy = 150;
        int rayspace = grid->cellSize * 8.0f;

        list<g_ptr<c_node>> matching_nodes;
        int gap_start = -1;
        vec3 forward = vis->forward;
        vec3 right = vec3(forward.z(), 0, -forward.x()); 
        float start_angle = -vis->cone_width / 2.0f;
        float angle_step = vis->cone_width / (vis->num_rays - 1);   
        for(int i = 1; i < input_size; i++) {
            float depth_jump = vis->dists[i] - vis->dists[i-1];
            if(gap_start < 0 && depth_jump > rayspace) {
                gap_start = i;
            } else if(gap_start >= 0 && depth_jump < -grid->cellSize * 2.0f) {
                int left_edge_idx = gap_start - 1;
                float left_angle_deg = start_angle + left_edge_idx * angle_step;
                float left_angle_rad = left_angle_deg * 3.14159f / 180.0f;
                vec3 left_ray_dir = (forward * cos(left_angle_rad) + right * sin(left_angle_rad)).normalized();
                vec3 point_A = agent->getPosition() + left_ray_dir * vis->dists[left_edge_idx];
                int right_edge_idx = i;
                float right_angle_deg = start_angle + right_edge_idx * angle_step;
                float right_angle_rad = right_angle_deg * 3.14159f / 180.0f;
                vec3 right_ray_dir = (forward * cos(right_angle_rad) + right * sin(right_angle_rad)).normalized();
                vec3 point_B = agent->getPosition() + right_ray_dir * vis->dists[right_edge_idx];
                
                vec3 gap_pos = (point_A + point_B) * 0.5f;
                if(!grid->empty(gap_pos)) continue;
                vis->features << gap_pos;

                propagate(physics_attention.last(), search_energy, 
                [&](g_ptr<c_node> node) {
                    if(node->getID()==-1) return true;
                    if(scene->singles[node->getID()]->dtype != "gap") return true;
                    float spatial_dist = gap_pos.distance(node->getPosition());
                    if(spatial_dist<rayspace) {
                        matching_nodes << node;
                    }
                    
                    return true;
                });
        
                if(!matching_nodes.empty()) {
                    for(auto match : matching_nodes) {
                        for(int f = 0; f < 6; f++) {
                            //Update existing nodes
                                // float delta = (query.mat[0][f] - match->archetype.mat[0][f]) * 0.1f;
                                // match->archetype.mat[0][f] += delta;
                        }
                    }
                    push_attention_head(matching_nodes[0]);  // Attend to first match
                } else {

                    #if VISPATHS
                        auto vismark = scene->create<Single>("white");
                        vismark->setPosition(gap_pos.addY(1.0f));
                        vismark->setScale({0.5f,1,0.5f});
                        vismark->setColor(vec4(0.8f,1,1,1));
                        debug << vismark;
                    #endif
                    // Novel gap - create new memory
                    auto marker = scene->create<Single>("crumb");
                    marker->setPosition(gap_pos);
                    marker->hide();
                    marker->dtype = "gap";
                    
                    Crumb gap_crumb;
                    gap_crumb.id = marker->ID;
                    
                    crumbs.put(marker->ID, gap_crumb);
                    push_attention_head(observe(gap_crumb, marker));
                }


                gap_start = -1;
            }
        }
    }

    vec3 steering_force(g_ptr<Single> agent,const vec3& goal,float goal_focus) {
        vec3 to_goal = agent->getPosition().direction(goal).nY();
        float goal_distance = agent->getPosition().distance(goal);      

        vec3 forward =  vis->forward;
        vec3 right = vec3(forward.z(), 0, -forward.x()); 
        float start_angle = -vis->cone_width / 2.0f;
        float angle_step = vis->cone_width / (vis->num_rays - 1);

        float best_openness = 0.0f;
        vec3 best_direction = forward;

        float best_goal_alignment = -1.0f;
        float best_aligned_clearance = 0.0f;
            
        vec3 left_clearance_vec(0, 0, 0);
        vec3 right_clearance_vec(0, 0, 0);
        float left_count = 0.0f;
        float right_count = 0.0f;

        for(int i = 0; i < vis->num_rays; i++) {
            float angle_deg = start_angle + i * angle_step;
            float angle_rad = angle_deg * 3.14159f / 180.0f;
            
            vec3 ray_dir = vis->dirs[i];
            float hit_dist = vis->dists[i];
            float hit_cell = vis->cells[i];
    
            float side = forward.cross(ray_dir).y();  // Positive = right, negative = left
            
            if(side < 0) {
                left_clearance_vec = left_clearance_vec + ray_dir * hit_dist;
                left_count++;
            } else {
                right_clearance_vec = right_clearance_vec + ray_dir * hit_dist;
                right_count++;
            }
            
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


        }

        //Going to be not just gaps eventually
        list<vec3> gaps = vis->features;

        float gap_bias_strength = randf(0.0f,0.6f);
        float goal_bias_strength = 0.4f;
        float wall_bias_strength = 0.8f;

        vec3 gap_bias(0, 0, 0);        
        if(!gaps.empty())
            gap_bias = agent->direction(gaps[0]) * gap_bias_strength; // Weight for gap bias
        
        vec3 wall_bias(0, 0, 0);
        if(left_count > 0 && right_count > 0) {
            vec3 left_avg = left_clearance_vec / left_count;
            vec3 right_avg = right_clearance_vec / right_count;
            wall_bias = (right_avg - left_avg).normalized() * 0.8f;
        }
    
        // Goal bias
        float wall_proximity = 1.0f / (best_openness + 1.0f);
        float goal_weight = std::lerp(0.5f, 0.0f, wall_proximity);
        vec3 goal_bias = to_goal * goal_weight * 0.4f;
        
        // Beeline check
        float beeline_threshold = 0.95f * goal_focus;
        bool can_beeline = (best_goal_alignment > beeline_threshold && 
                            best_aligned_clearance >= goal_distance);
        
        //print("BEST DIR: ",best_direction.to_string(),"\n  WALL_BIAS: ",wall_bias.to_string(),"\n  GOAL_BIAS: ",goal_bias.to_string(),"\n  GAP_BIAS: ",gap_bias.to_string());
        vec3 environmental_force = can_beeline ? 
             to_goal.normalized() : 
             (best_direction + wall_bias + goal_bias + gap_bias).normalized();

                                                                     
        if(environmental_force.length() > 0.01f) {
            environmental_force = environmental_force.normalized();
            
            // Blend with momentum (if we're already moving)
            // vec3 final_direction;
            // if(current_velocity.length() > 0.1f) {
            //         final_direction = (current_velocity.normalized() * 0.2f + environmental_force * 0.8f).normalized();
            // } else {
            //         final_direction = environmental_force;
            // }
            
            vec3 velocity = environmental_force * agent->opt_floats[1]; //<- desired speed
            vec3 f = agent->getPosition()+velocity;
            execute_action_on_agent({0,{velocity.x(),velocity.y(),velocity.z()}},agent);
            execute_action_on_agent({1,{f.x(),f.y(),f.z()}},agent);

            // agent->faceTo(agent->getPosition() + velocity);
            // agent->setLinearVelocity(velocity);
        }
        return environmental_force;
    }

    struct RayHit {
        float distance;
        int cell_id;
        vec3 color;
        vec3 normal;
    };

    RayHit sample_ray_with_color(vec3 origin, vec3 direction, float max_dist, int exclude_id) {
        RayHit hit;
        
        // Stage 1: Grid raycast (what you already have)
        auto grid_hit = grid->raycast(origin, direction, max_dist, exclude_id);
        hit.distance = grid_hit.first;
        hit.cell_id = grid_hit.second;
        
        // Stage 2: Color sampling
        if(grid_hit.second >= 0 && !grid->cells[grid_hit.second].empty()) {
            // Get first object in cell
            int obj_id = grid->cells[grid_hit.second].first();
            
            // Sample color from object
            hit.color = vec4_to_vec3(scene->colors[obj_id]);
            
            // Approximate normal (just reverse ray for now)
            hit.normal = direction*-1;
            
        } else {
            // Sky/empty space
            hit.color = vec3(0.5f, 0.7f, 1.0f);  // Sky blue
            hit.normal = vec3(0, 1, 0);
        }
        
        return hit;
    }

    void see_3d(g_ptr<Single> agent, int elevation_samples, int azimuth_samples, float horizontal_fov, float vertical_fov, float ray_distance) {

        vec3 forward = agent->facing().nY();
        vec3 up = vec3(0, 1, 0);
        vec3 right = forward.cross(up);
        if(right.length() < 0.001f) {
            right = vec3(1, 0, 0);
        } else {
            right = right.normalized();
        }
        up = right.cross(forward).normalized(); 

        vis = make<Vis>();
        vis->elevation_samples = elevation_samples;
        vis->azimuth_samples = azimuth_samples;
        vis->num_rays = elevation_samples * azimuth_samples;
        vis->vertical_fov = vertical_fov;
        vis->cone_width = horizontal_fov;
        vis->ray_distance = ray_distance;
        vis->forward = forward;
        vis->pos = agent->getPosition();

        float elev_step = vertical_fov / (elevation_samples - 1);
        float azim_step = horizontal_fov / (azimuth_samples - 1);

        for(int e = 0; e < elevation_samples; e++) {
            // Elevation: -vertical_fov/2 to +vertical_fov/2
            float elev_angle = -vertical_fov/2.0f + e * elev_step;
            float elev_rad = elev_angle * 3.14159f / 180.0f;
            
            for(int a = 0; a < azimuth_samples; a++) {
                // Azimuth: -horizontal_fov/2 to +horizontal_fov/2 (centered on forward)
                float azim_angle = -horizontal_fov/2.0f + a * azim_step;  // Changed
                float azim_rad = azim_angle * 3.14159f / 180.0f;
                
                // Spherical to Cartesian (same math, different range)
                vec3 horizontal = (forward * cos(azim_rad) + right * sin(azim_rad)).normalized();
                vec3 ray_dir = (horizontal * cos(elev_rad) + up * sin(elev_rad)).normalized();
                
                RayHit hit = sample_ray_with_color(agent->getPosition(), ray_dir, 
                                                ray_distance, agent->ID);
                
                vis->dists << hit.distance;
                vis->cells << hit.cell_id;
                vis->colors << hit.color;
                vis->normals << hit.normal;
                vis->dirs << ray_dir;
                
                #if VISPATHS
                    vec3 ray_end = agent->getPosition() + ray_dir * hit.distance;
                    vec3 c = hit.color;
                    debug << draw_line_3d(agent->getPosition(), ray_end, vec4(c.x(),c.y(),c.z(),1));
                #endif
            }
        }
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

    //Returns a list of c_nodes in order of how strongly they activated in response to the crumb query, propagates with energy.
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


    //Example interpreter unit (like the cerbellum creates) turns the standard action struct into specific actions
    void execute_action_on_agent(const Action& action,g_ptr<Single> agent) {
        switch(action.part_id) {
            case 0: agent->setLinearVelocity(action.asvec3()); break;
            case 1: agent->faceTo(action.asvec3()); break;
            case 2: { //INTERACT
                g_ptr<Single> obj = objInFront(agent);
                if(obj) {
                    ScriptContext ctx; ctx.set<g_ptr<Single>>("from",agent);
                    obj->run("onInteract",ctx);
                }
            } break;
            case 3: { //USE
                if(!agent->children.empty()) {
                    agent->children[0]->run("onUse");
                }
            } break;
            case 4: { //DROP
                if(!agent->children.empty()) {
                    agent->children[0]->run("onDrop");
                }
            } break;
            case 5: {
                list<g_ptr<Single>> objs = objInSweep(agent);
                for(auto obj : objs) {
                    if(obj==agent) continue;
                    if(obj) {
                        ScriptContext ctx; ctx.set<g_ptr<Single>>("from",agent);
                        obj->run("onHit",ctx);
                    }
                }
            } break;
        }
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

static list<g_ptr<nodenet>> crumb_managers;

g_ptr<Quad> make_button(std::string slot) {
    g_ptr<Quad> q = scene->create<Quad>("gui_element");
    q->scale(vec2(slot.length()*100.0f,40));
    q->setColor(vec4(1,0,0,1));
    q->addSlot("test");
    q->addSlot("otherTest");
    q->setDepth(1.0f);
    q->setPosition({0,0});
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


struct Genome : public q_object {
    list<float> genes;
    list<vec2> ranges;
    list<list<float>> genepool;
    float fitness = 0.0f;

    void copy_from(g_ptr<Single> thing, int offset, int start = 0, int count = -1) {
        if(count==-1) count = genes.length();
        for(int i=start;i<count;i++) {
            genes[i] = thing->opt_floats[i+offset];
        }
    }
    void apply_to(g_ptr<Single> thing, int offset, int start = 0, int count = -1) {
        if(count==-1) count = genes.length();
        for(int i=start;i<count;i++) {
            thing->opt_floats[i+offset] = genes[i];
        }
    }

    void add_gene(float init_value, vec2 range, list<float> mutations = {}) {
        genes << init_value; ranges << range; genepool << mutations;
    }

    void mutate(float rate = 0.3f, vec2 degree = vec2(0.5f, 1.5f)) {
        for(int i = 0; i < genes.length(); i++) {
            if(randf(0, 1) < rate) {
                if(genepool[i].empty()) {
                    genes[i] *= randf(degree.x(), degree.y());
                    genes[i] = std::max(ranges[i].x(), std::min(ranges[i].y(), genes[i]));
                } else {
                    genes[i] = genepool[i][randi(0, genepool[i].length() - 1)];
                }
            }
        }
    }

    void random_init() {
        for(int i = 0; i < genes.length(); i++) {
            if(genepool[i].empty()) {
                genes[i] = randf(ranges[i].x(), ranges[i].y());
            } else {
                genes[i] = genepool[i][randi(0, genepool[i].length() - 1)];
            }
        }
    }
    
    g_ptr<Genome> crossover(g_ptr<Genome> other) {
        g_ptr<Genome> child = make<Genome>();
        child->ranges = ranges;
        child->genepool = genepool;
        
        for(int i = 0; i < genes.length(); i++) {
            child->genes << (randf(0, 1) < 0.5f ? genes[i] : other->genes[i]);
        }
        
        return child;
    }

    g_ptr<tensor> to_tensor() const {
        return make<tensor>(list<int>{1, (int)genes.length()}, genes);
    }

    static g_ptr<Genome> from_tensor(g_ptr<tensor> t, const list<vec2>& ranges, const list<list<float>>& genepool) {
        g_ptr<Genome> genome = make<Genome>();
        genome->ranges = ranges;
        genome->genepool = genepool;
        
        for(int i = 0; i < t->shape_[1]; i++) {
            float val = t->flat(i);
            // Clamp to valid range
            val = std::max(ranges[i].x(), std::min(ranges[i].y(), val));
            genome->genes << val;
        }
        
        return genome;
    }
};


struct Trial : public q_object {
    list<g_ptr<Genome>> population;
    std::function<float(list<g_ptr<Single>>&)> evaluate;
    std::function<void(list<g_ptr<Single>>&)> reset;
    std::function<void()> regenerate;
    Log::Line time;
    
    int population_size = 20;
    int generations = 50;
    int frames_per_trial = 5000;
    float mutation_rate = 0.2f;
    int elite_count = 5;
    
    int current_generation = 0;
    int current_genome = 0;
    int frame_count = 0;

    bool use_surrogate = false;
    int hidden_size = 10;
    g_ptr<tensor> W1 = nullptr;
    g_ptr<tensor> W2 = nullptr;
    list<list<float>> param_history;
    list<float> fitness_history;
    
    void initialize(g_ptr<Genome> template_genome) {
        population.clear();
        
        // Create initial population with random genes
        for(int i = 0; i < population_size; i++) {
            g_ptr<Genome> genome = make<Genome>();
            genome->ranges = template_genome->ranges;
            genome->genepool = template_genome->genepool;
            genome->genes = list<float>(template_genome->genes.length(), 0.0f);
            genome->random_init();
            population << genome;
        }

        if(use_surrogate) {
            int input_size = template_genome->genes.length();
            W1 = weight(input_size, hidden_size, 0.1f);
            W2 = weight(hidden_size, 1, 0.1f);
            param_history.clear();
            fitness_history.clear();
        }
        
        current_generation = 0;
        current_genome = 0;
        frame_count = 0;
    }
    bool step(list<g_ptr<Single>>& subjects, float offset) {
        if(frame_count == 0) {
            for(auto subject : subjects) {
                population[current_genome]->apply_to(subject, offset); 
            }
            time.start();
        }
        
        frame_count++;
        
        if(frame_count >= frames_per_trial) {
            // Evaluate
            float fitness = evaluate(subjects);
            population[current_genome]->fitness = fitness;

            if(use_surrogate) {
                param_history << population[current_genome]->genes;
                fitness_history << fitness;
            }
            
            print("Gen ", current_generation, " | Genome ", current_genome, " | Fitness: ", fitness," | Time: ",ftime(time.end()));
            
            // Reset for next genome
            reset(subjects);
            regenerate();
            
            current_genome++;
            frame_count = 0;
            time.start();
            // Generation complete?
            if(current_genome >= population_size) {
                return evolve_generation();
            }
        }
        
        return false;
    }

    void train_surrogate(int epochs = 10) {
        if(!use_surrogate || param_history.length() < 5) return;
        
        float lr = 0.01f;
        
        for(int epoch = 0; epoch < epochs; epoch++) {
            float total_loss = 0.0f;
            
            for(int i = 0; i < param_history.length(); i++) {
                // Forward pass
                auto input = make<tensor>(list<int>{1, (int)param_history[i].length()}, 
                                         param_history[i]);
                
                Pass matmul1 = {MATMUL, W1};
                auto hidden = input->forward(matmul1);
                Pass relu = {RELU};
                hidden = hidden->forward(relu);
                
                Pass matmul2 = {MATMUL, W2};
                auto predicted = hidden->forward(matmul2);
                
                // Loss: MSE
                float target = fitness_history[i];
                float pred = predicted->at({0, 0});
                float error = pred - target;
                total_loss += error * error;
                
                // Backward pass
                predicted->grad_[0] = 2.0f * error;  // d(MSE)/d(pred)
                predicted->backward();
                
                // SGD update - use flat() not flat_mut()
                for(int j = 0; j < W1->numel(); j++) {
                    W1->flat(j) -= lr * W1->grad_[j];
                    W1->grad_[j] = 0.0f;
                }
                
                for(int j = 0; j < W2->numel(); j++) {
                    W2->flat(j) -= lr * W2->grad_[j];
                    W2->grad_[j] = 0.0f;
                }
                
                // Clear graph to free memory
                tensor::clear_graph(predicted);
            }
            
            if(epoch % 5 == 0) {
                print("Surrogate epoch ", epoch, " | Loss: ", total_loss / param_history.length());
            }
        }
    }
    
    g_ptr<Genome> gradient_ascend(g_ptr<Genome> start, int steps = 50) {
        if(!use_surrogate) return start;
        
        auto params = start->to_tensor();
        float lr = 0.1f;
        
        for(int step = 0; step < steps; step++) {
            // Forward: predict fitness
            Pass matmul1 = {MATMUL, W1};
            auto hidden = params->forward(matmul1);
            Pass relu = {RELU};
            hidden = hidden->forward(relu);
            
            Pass matmul2 = {MATMUL, W2};
            auto fitness_pred = hidden->forward(matmul2);
            
            // Gradient ascent: maximize fitness
            fitness_pred->grad_[0] = 1.0f;
            fitness_pred->backward();
            
            // Update parameters
            list<float> new_params;
            for(int i = 0; i < start->genes.length(); i++) {
                float grad_val = params->grad_[i];
                float new_val = params->flat(i) + lr * grad_val;
                
                // Clamp to valid range
                new_val = std::max(start->ranges[i].x(), 
                                  std::min(start->ranges[i].y(), new_val));
                new_params << new_val;
            }
            tensor::clear_graph(fitness_pred);
            
            // Create new tensor for next iteration
            params = make<tensor>(list<int>{1, (int)start->genes.length()}, new_params);
        }
        
        return Genome::from_tensor(params, start->ranges, start->genepool);
    }
    
    bool evolve_generation() {
        print("\n=== GENERATION ", current_generation, " COMPLETE ===");
        auto best = get_best();
        print("Best fitness: ", best->fitness);
        print("Best genes: ", genes_to_string(best));
        
        // Train surrogate model on accumulated data
        if(use_surrogate) {
            train_surrogate(10);
        }
        
        current_generation++;
        
        if(current_generation >= generations) {
            print("\n=== EVOLUTION COMPLETE ===");
            return true;
        }
        
        // Sort by fitness
        population.sort([](auto a, auto b) { return a->fitness > b->fitness; });
        
        list<g_ptr<Genome>> next_pop;
        
        // Keep elites
        for(int i = 0; i < elite_count; i++) {
            next_pop << population[i];
        }
        
        // Generate rest of population
        if(use_surrogate && param_history.length() > 20) {
            // Use surrogate-guided mutation
            while(next_pop.length() < population_size) {
                int parent_idx = randi(0, elite_count - 1);
                g_ptr<Genome> child = gradient_ascend(population[parent_idx], 50);
                child->mutate(mutation_rate * 0.5f);  // Smaller mutations after gradient step
                next_pop << child;
            }
        } else {
            // Standard tournament selection
            while(next_pop.length() < population_size) {
                int p1 = randi(0, elite_count - 1);
                int p2 = randi(0, elite_count - 1);
                g_ptr<Genome> child = population[p1]->crossover(population[p2]);
                child->mutate(mutation_rate);
                next_pop << child;
            }
        }
        
        population = next_pop;
        current_genome = 0;
        
        print("Starting generation ", current_generation, "...\n");
        return false;
    }
    
    g_ptr<Genome> get_best() {
        g_ptr<Genome> best = population[0];
        for(auto genome : population) {
            if(genome->fitness > best->fitness) {
                best = genome;
            }
        }
        return best;
    }
    
    std::string genes_to_string(g_ptr<Genome> g) {
        std::string result = "[";
        for(int i = 0; i < g->genes.length(); i++) {
            result += std::to_string(g->genes[i]);
            if(i < g->genes.length() - 1) result += ", ";
        }
        result += "]";
        return result;
    }
};


bool pickUp(g_ptr<Single> item, g_ptr<Single> agent) {
    if(agent->markers().has("grip_pos")&&!agent->children.has(item)) {
        removeFromGrid(item);
        //for(auto c : item->children) removeFromGrid(c); //Needs to recurse to all children of children
        agent->addChild(item); 
        item->setPosition(agent->markerPos("grip_pos"));
        return true;
    } else {
        return false;
    }
}

bool drop(g_ptr<Single> item) {
    if(item->parent) {
        item->parent->removeChild(item);
        item->move(vec3(0,-item->getPosition().y(),0));
        addToGrid(item);
        // for(auto c : item->children) addToGrid(c); //Needs to recurse to all children of children
        return true;
    } else {
        return false;
    }
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

     int amt = 1;
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
     terrain->setPosition({0,-0.1f,0});

     g_ptr<Model> a_model = make<Model>(MROOT+"agents/WhiskersLOD3.glb");
     g_ptr<Model> b_model = make<Model>(makeTestBox(1.0f));
     g_ptr<Model> c_model = make<Model>(makeTestBox(grid->cellSize));

    #if VISGRID
     list<g_ptr<Single>> gridsqs;
     scene->define("grid_square",[c_model](){
        auto q = make<Single>(c_model);
        scene->add(q);
        q->setScale({grid->cellSize,0.1f,grid->cellSize});
        q->setColor(vec4(1,0,0,1));
        q->setPhysicsState(P_State::NONE);
        return q;
     });
     float half = grid_size/2.0f;
     for(float c=-half;c<half;c+=grid->cellSize) {
        for(float r=-half;r<half;r+=grid->cellSize) {
            auto sq = scene->create<Single>("grid_square");
            sq->setPosition({r,0,c});
            gridsqs << sq;
        }
     }
    #endif



     g_ptr<Model> grass_model = make<Model>(MROOT+"products/grass.glb");
     g_ptr<Model> berry_model = make<Model>(MROOT+"products/berry.glb");
     g_ptr<Model> clover_model = make<Model>(MROOT+"products/clover.glb");
     g_ptr<Model> stone_model = make<Model>(MROOT+"products/stone.glb");
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



     scene->define("agent",[a_model](){
        g_ptr<Single> q = make<Single>(a_model);
        scene->add(q);
        addToGrid(q);
        q->opt_vec_3_3 = q->getPosition();
        q->joint = [q](){
            q->unlockJoint = true; q->updateTransform(); q->unlockJoint = false;
            for(auto i : q->opt_idx_cache) {grid->cells[i].erase(q->ID);}
            q->opt_idx_cache = grid->addToGrid(q->ID, q->getWorldBounds());
            return false;
        };
        q->isAnchor = true;
        return q;
     });
     scene->define("crumb",Script<>("make_crumb",[c_model,phys](ScriptContext& ctx){
          g_ptr<Single> q = make<Single>(c_model);
          scene->add(q);
          q->scale(0.5f);
          q->setPhysicsState(P_State::NONE);
          q->setColor(vec4(0,0,1,1));
          q->opt_ints = list<int>(4,0);
          q->opt_floats = list<float>(2,0);
          q->opt_vec_3 = vec3(0,0,0);
          ctx.set<g_ptr<Object>>("toReturn",q);
     }));

    scene->define("berry",[berry_model](){
        g_ptr<Single> berry= make<Single>(berry_model);
        scene->add(berry);    
        return berry;
    });
    scene->add_initilizer("berry",[](g_ptr<Object> obj){ 
        if(g_ptr<Single> berry = g_dynamic_pointer_cast<Single>(obj)) {
            //Cleanup
            berry->scripts.clear(); //Clear all scipts
            removeFromGrid(berry); //Remove it from grid

            //Intilize
            berry->setPhysicsState(P_State::PASSIVE);
            addToGrid(berry);
            Crumb berry_crumb; //Intilize it with evrything
            berry_crumb.id = berry->ID;
            berry_crumb.mat[OBS][0] = 0.01f; //Is small
            
            berry_crumb.mat[LER][BOREDOM] = 1.1f; 
            berry_crumb.mat[LER][HUNGER] = 2.3f; 
            crumbs.put(berry->ID,berry_crumb);

            berry->addScript("onInteract",[berry](ScriptContext& ctx){
                g_ptr<Single> from = ctx.get<g_ptr<Single>>("from");
                pickUp(berry,from);
            });
            berry->addScript("onDrop",[berry](ScriptContext& ctx){drop(berry);});

            berry->addScript("onUse",[berry](ScriptContext& ctx){
                g_ptr<Single> from = nullptr;
                if(ctx.has("from")) from = ctx.get<g_ptr<Single>>("from");
                else from = berry->parent;
                sub_mask(crumb_managers[from->UUID]->state, DOES, crumbs.get(berry->ID), DOES);

                if(berry->parent) {
                    berry->parent->removeChild(berry);
                } else {
                    removeFromGrid(berry);
                }
                scene->recycle(berry);
            });

            berry->joint = [berry](){
                g_ptr<Single> parent = berry->parent; 
                if(parent) {
                    if(parent->isAnchor) {
                        berry->position = parent->markerPos("grip_pos");
                    } else {
                        vec3 offset = berry->getPosition() - parent->getPosition();
                        berry->position = parent->position + offset;
                    }
                } else {    
                    berry->unlockJoint = true; berry->updateTransform(); berry->unlockJoint = false;
                    for(auto i : berry->opt_idx_cache) {grid->cells[i].erase(berry->ID);}
                    berry->opt_idx_cache = grid->addToGrid(berry->ID, berry->getWorldBounds());
                    return false;
                }
                return true;
            };
        }
    });

    //WARNING:
    //The crumbs map isn't being updated when new instanecs are created via pooling, so once crumb defs are hammered out
    //ensure they're defined in the actual intilization code. Honestly define here is being a bit more than it actually should
    //there should be a create berry opperation so that these things don't accidently get brought through by pooling!
    //But that's a later problem

    scene->define("berry_bush",[c_model](){
        g_ptr<Single> bush= make<Single>(c_model);
        scene->add(bush);
        return bush;
    });
    scene->add_initilizer("berry_bush",[](g_ptr<Object> obj){ 
        if(g_ptr<Single> bush = g_dynamic_pointer_cast<Single>(obj)) {
            bush->scripts.clear();
            removeFromGrid(bush);

            bush->setScale(vec3(1,8.0f,1));
            bush->setColor(vec4(0.6f,0.3f,0.1f,1));
            bush->setPhysicsState(P_State::PASSIVE);
            addToGrid(bush);
            Crumb bush_crumb; //Intilize it with evrything
            bush_crumb.id = bush->ID;
            bush_crumb.mat[OBS][0] = 0.06f; //Is slightly bigger than a mouse
            bush_crumb.mat[CUR][CUR_OBSERVE] = 10.0f;
            bush_crumb.mat[CUR][CUR_INTERACT] = 10.0f;
    
            crumbs.put(bush->ID,bush_crumb);
    
            for(int i=0;i<randi(2,6);i++) {
                auto b = scene->create<Single>("berry");
                b->setPosition(vec3(randf(-0.5f,0.5f),randf(1,4),randf(-0.5f,0.5f)));
                removeFromGrid(b);
                bush->addChild(b);
            }
    
            bush->addScript("onInteract",[bush](ScriptContext& ctx){
                g_ptr<Single> from = ctx.get<g_ptr<Single>>("from");
                if(!bush->children.empty()) {
                    auto c = bush->children.last();
                    bush->removeChild(c);
                    pickUp(c,from);
                }
            });
            bush->addScript("onDrop",[bush](ScriptContext& ctx){drop(bush);});
    
            bush->addScript("onUse",[bush](ScriptContext& ctx){
                g_ptr<Single> from = nullptr;
                if(ctx.has("from")) from = ctx.get<g_ptr<Single>>("from");
                else from = bush->parent;
                sub_mask(crumb_managers[from->UUID]->state, DOES, crumbs.get(bush->ID), DOES);
    
                if(bush->parent) {
                    bush->parent->removeChild(bush);
                } else {
                    removeFromGrid(bush);
                }
                scene->recycle(bush);
            });
    
            bush->addScript("onHit",[bush](ScriptContext& ctx){
                bush->setColor(vec4(1,0,0,1));
                events << Event(1.0f,[bush](){bush->setColor(vec4(0.6f,0.3f,0.1f,1));});
            });
    
            bush->joint = [bush](){
                g_ptr<Single> parent = bush->parent; 
                if(parent) {
                    if(parent->isAnchor) {
                        bush->position = parent->markerPos("grip_pos");
                    } else {
                        vec3 offset = bush->getPosition() - parent->getPosition();
                        bush->position = parent->position + offset;
                    }
                } else {    
                    bush->unlockJoint = true; bush->updateTransform(); bush->unlockJoint = false;
                    for(auto i : bush->opt_idx_cache) {grid->cells[i].erase(bush->ID);}
                    bush->opt_idx_cache = grid->addToGrid(bush->ID, bush->getWorldBounds());
                    return false;
                }
                return true;
            };
        }
    });

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


    // scene->define("wall",[b_model](){
    //     g_ptr<Single> q = make<Single>(b_model);
    //     scene->add(q);
    //     return q;
    //  });
    //  make_maze();
     int grass_count = (int)(grid_size * grid_size * 0.04f);
     for(int i = 0; i < grass_count; i++) {
         g_ptr<Single> box = make<Single>(grass_model);
         scene->add(box);
         box->dtype = "grass";
         box->getLayer().setLayer(1);
         box->getLayer().setCollision(0);
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
         mem.mat[OBS][0] = 0.2f; //Grass sized
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
          agent->getLayer().addCollision(1);
        //   #if TRIALS
            //making it so agents don't interfere with each other
            // agent->setPosition(vec3(0,0,0));
            agent->getLayer().setLayer(0);
            agent->getLayer().setCollision(1);
            grid->make_seethru(agent->ID);
        //   #endif

          agent->UUID = i; //ITR id

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

          Crumb mem;
          mem.mat[MET][0] = 1.0f; //Distance saliance factor
          mem.mat[MET][1] = 0.1f; //Navigation focus
          mem.mat[MET][2] = 1.0f; //Salience impactor

          mem.mat[CUR][CUR_OBSERVE] = 0;
          mem.mat[CUR][CUR_INTERACT] = 0;
          mem.mat[CUR][CUR_USE] = 0; 
          mem.mat[CUR][CUR_DROP] = 0; 
          mem.mat[CUR][CUR_HIT] = 0; 

          mem.mat[LER][BOREDOM] = 0;
          mem.mat[LER][HUNGER] = 0; 
          mem.mat[LER][FATIGUE] = 0;
          mem.id = agent->ID;
          crumbs.put(agent->ID,mem);

          g_ptr<nodenet> memory = make<nodenet>(mem);
          memory->agent_id = agent->ID;



          Crumb mem_root;
          g_ptr<Single> root_form = scene->create<Single>("crumb");
          root_form->setPosition(base_pos);
          mem_root.id = root_form->ID;
          memory->meta_root->archetype = mem_root;
          memory->obs_root->archetype = mem_root;
          memory->id_to_node.put(root_form->ID,memory->meta_root.getPtr());
                         
          crumb_managers << memory;
    
          agent->physicsJoint = [agent, memory, marker, update_thread, phys, side_length]() {
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
               g_ptr<Single> goal_crumb = marker;
               int& spin_accum = agent->opt_ints[2];
               int& on_frame = agent->opt_ints[3];
               float& goal_focus = agent->opt_floats[2];
               int& accum5 = agent->opt_ints[5];
               int& frames_since_progress = agent->opt_ints[7];
               int& nav_strat = agent->opt_ints[8];
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
                    
                    #if VISPATHS
                        memory->visualize_registry(memory->physics_attention.last());
                    #endif

                    // Tunable parameters
                    int num_rays = 24; 
                    float ray_distance = 30.0f;
                    float cone_width = 235.0f; 
                    // memory->see_3d(agent, 
                    //     8,      // elevation_samples
                    //     24,     // azimuth_samples  
                    //     120.0f, // horizontal_fov (120° left-right, human is ~114°)
                    //     90.0f,  // vertical_fov (90° up-down, human is ~135° but asymmetric)
                    //     30.0f   // ray_distance
                    // );
                    //memory->see2d(agent,num_rays,ray_distance,cone_width);
                    #if GTIMERS
                        timers << l.end(); timer_labels << "raycast_sample"; l.start();
                    #endif
                    //memory->process_visual_for_gaps(agent);
                    #if GTIMERS
                        timers << l.end(); timer_labels << "feature_detection"; l.start();
                    #endif
                    //memory->observe_instances(agent,on_frame);
                    #if GTIMERS
                        timers << l.end(); timer_labels << "observe_instances"; l.start();
                    #endif
                    //vec3 environmental_force = memory->steering_force(agent,goal,goal_focus).nY(); //nY so that it's 2d
                    #if GTIMERS
                    timers << l.end(); timer_labels << "derive_force";
                    #endif
                }

               #if GTIMERS
               timers << overall.end(); timer_labels << "overall";
               #endif
               
               return true;
          };


          agent->threadUpdate = [agent, i, memory, update_thread](){
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
     phys_logger.log = [phys,&total_times,amt](){
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
        // g_ptr<Genome> cognitive_template = make<Genome>();
        // cognitive_template->add_gene(0.03f, vec2(0.001f, 0.1f));     // threshold
        // cognitive_template->add_gene(3.0f, vec2(0.5f, 15.0f));       // match_threshold
        // cognitive_template->add_gene(1.02f, vec2(1.0f, 1.1f));       // growth_rate
        // cognitive_template->add_gene(0.4f, vec2(0.1f, 1.0f));        // sig_weight
        // cognitive_template->add_gene(20.0f, vec2(20.0f, 30.0f));     // spatial_weight
                
        // // Evaluator: measure graph quality
        // search->evaluate = [&](list<g_ptr<Single>>& subjects) -> float {
        //     float total_quality = 0.0f;
            
        //     for(int i = 0; i < subjects.length(); i++) {
        //         g_ptr<nodenet> memory = crumb_managers[i];
                
        //         int nodes = memory->id_to_node.size();
        //         int invalid = 0;
        //         int valid = 0;
                
        //         memory->profile(memory->meta_root, [&](g_ptr<c_node> node) {
        //             for(auto child : node->children) {
        //                 vec3 from = node->getPosition();
        //                 vec3 to = child->getPosition();
                        
        //                 if(!grid->can_see(from, to, {subjects[i]->ID, node->getID(), child->getID()})) {
        //                     invalid++;
        //                 } else {
        //                     valid++;
        //                 }
        //             }
        //             return true;
        //         });
                
        //         float quality = 0.0f;
        //         if(nodes > 0) {
        //             float valid_ratio = (float)valid / (float)(valid + invalid);
        //             quality = valid_ratio * 100.0f - (nodes * 0.1f);  // Reward valid %, small penalty for node count
        //         } else {
        //             quality = -10.0f;  // Fixed penalty for no memory
        //         }
                
        //         total_quality += quality;
        //     }
            
        //     return total_quality / subjects.length();
        // };
        
        // // Reset: clear memories and respawn agents
        // search->reset = [&](list<g_ptr<Single>>& subjects) {
        //     for(int i = 0; i < subjects.length(); i++) {
        //         g_ptr<nodenet> memory = crumb_managers[i];
                
        //         // Break all connections
        //         list<c_node*> all_nodes;
        //         for(auto entry : memory->id_to_node.entrySet()) {
        //             all_nodes << entry.value;
        //         }
                
        //         for(auto node : all_nodes) {
        //             node->parents.clear();
        //             node->children.clear();
                    
        //             if(node->getID() != -1) {
        //                 scene->recycle(scene->singles[node->getID()],"crumb");
        //             }
        //         }
                
        //         memory->id_to_node.clear();
        //         memory->physics_attention.clear();
        //         memory->physics_attention << memory->meta_root;
                
        //         // Respawn agent
        //         subjects[i]->setPosition(vec3(0,0,0));
        //         subjects[i]->setLinearVelocity(vec3(0, 0, 0));
        //     }
            
        //     total_connections = 0;
        // };
        
        // // Regenerate: rebuild maze
        // search->regenerate = [&]() {
        //     make_maze();
        // };
        
        // search->initialize(cognitive_template);    
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

        #if VISGRID
          for(int i=0;i<gridsqs.length();i++) {
            if(grid->empty(i)) gridsqs[i]->setColor(vec4(1,0,0,1));
            else gridsqs[i]->setColor(vec4(0,0,1,1));
         }
        #endif
            #if TRIALS
                // if(search->step(agents,4)) {
                //     auto best = search->get_best();
                //     print("\n=== BEST GENOME FOUND ===");
                //     print("Fitness: ", best->fitness);
                //     print("Genes: thresh=", best->genes[0], " match=", best->genes[1],
                //         " growth=", best->genes[2], " sig_w=", best->genes[3],
                //         " spat_w=", best->genes[4]);
                //     run_thread->pause();
                // }
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
     update_logger.log = [](){
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
     update_thread->run([&update_logger,&from_phys_to_update](ScriptContext& ctx){
          update_logger.tick();
        //   for(auto a : agents) {
        //        if(a->threadUpdate&&a->isActive())
        //             a->threadUpdate();
        //   }
        //   if(!from_phys_to_update.empty()) {
        //        for(auto p : from_phys_to_update) {
        //               p();
        //        }
        //        from_phys_to_update.clear();
        //     }
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

    auto berry = scene->create<Single>("berry_bush");
    berry->setPosition({-3,berry->getWorldBounds().max.y()/2,6});

    for(int i=0;i<8;i++) {
        auto b = scene->create<Single>("berry");
        b->setPosition(vec3(randf(-5,5),0,randf(-5,5)));
    }

    //For tree visualization
    g_ptr<nodenet> mem = crumb_managers[0];
    g_ptr<c_node> current_view = mem->meta_root;
    int child_index = 0;  // Which child we're looking at
    int parent_index = 0; // Which parent we're looking at

    //My little avatar for interacting with the world
    g_ptr<Single> fir = scene->create<Single>("agent");
    fir->setPhysicsState(P_State::FREE);
    fir->getLayer().setLayer(0);
    fir->getLayer().setCollision(1);
    Crumb fircrumb;
    fircrumb.id = fir->ID;
    g_ptr<nodenet> net = make<nodenet>(fircrumb);
    net->agent_id = fir->ID;
    fir->UUID = crumb_managers.length();
    crumbs.put(fir->ID,fircrumb);

    Crumb mem_root;
    g_ptr<Single> root_form = scene->create<Single>("crumb");
    root_form->setPosition(fir->getPosition());
    mem_root.id = root_form->ID;
    net->meta_root->archetype = mem_root;
    net->obs_root->archetype = mem_root;
    net->id_to_node.put(root_form->ID,net->meta_root.getPtr());
    crumb_managers << net;
    agents << fir;
    
    bool control_fir = true;
    int cam_mode = 2;

     start::run(window,d,[&]{
          s_tool.tick();
          if(pressed(N)) {
               scene->camera.setTarget(agents[0]->getPosition()+vec3(0,20,20));
          }
          vec3 mousepos = scene->getMousePos();
          bool shift = held(LSHIFT);

          for(int i=events.length()-1;i>=0;i--) {
            if(events[i].tick(s_tool.tpf)) {
                events.removeAt(i);
            }
          }

          if(pressed(B)) {
            auto nberry = scene->create<Single>("berry");
            nberry->setPosition(mousepos);
          }

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

          if(pressed(NUM_1)) {
            scene->camera.toIso();
            window.unlock_mouse();
            cam_mode = 1;
            print("Switched to isometric camera");
          }
          else if(pressed(NUM_2)) {
            scene->camera.toOrbit();
            window.unlock_mouse();
            cam_mode = 2;
            print("Switched to orbit camera");
          }
          else if(pressed(NUM_3)) {
            scene->camera.toFirstPerson();
            window.lock_mouse();
            cam_mode = 3;
            print("Switched to first person camera");
          }

        if(pressed(F)) control_fir = !control_fir;
        if(fir&&control_fir) {
            if(cam_mode==3) {
                fir->setLinearVelocity(dir_fps(shift?6.0f:3.0f,fir).setY(fir->getVelocity().position.y()));
                vec3 f = scene->camera.front;
                fir->faceTowards(vec3(f.x(),0,f.z()),vec3(0,1,0));
                vec3 camPos = fir->getPosition().addY(0.7f)+fir->facing().mult(0.8f);
                scene->camera.setPosition(fir->markerPos("camera_pos"));  
            }
            else if(cam_mode==2||cam_mode==1) {
                vec3 dir = fir->direction(mousepos);
                float dist = fir->distance(mousepos);
                vec3 go_to = dir*(shift?8.0f:4.0f);
                if(dist>grid->cellSize) {
                    fir->setLinearVelocity(go_to);
                } else {
                    fir->setLinearVelocity(vec3(0,0,0));
                }
                fir->faceTo(fir->getPosition()+go_to);
            }

            if(pressed(E)) { //INTERACT
                net->execute_action_on_agent({2},fir);
            }
            if(pressed(R)) { //USE
                //print("BEFORE: ",net->state.mat[LER][0]," | ",fircrumb.mat[LER][0]," | ",crumbs[fir->ID].mat[LER][0]);
                net->execute_action_on_agent({3},fir);
                //print("AFTER: ",net->state.mat[LER][0]," | ",fircrumb.mat[LER][0]," | ",crumbs[fir->ID].mat[LER][0]);
            }
            if(pressed(Q)) { //DROP
                net->execute_action_on_agent({4},fir);
            }
            if(pressed(MOUSE_LEFT)) {
                net->execute_action_on_agent({5},fir);
            }

            if(pressed(H)) {
                net->see2d(fir,24,10.0f,225.0f);
                net->process_visual_for_gaps(fir);
                net->observe_instances(fir,s_tool.frame);

                print("------------\nCHECKING FOR CATEGORIES: ",net->obs_root->children.length());
                for(auto c : net->obs_root->children) {
                    print("CATEGORY: ",c->getID()," CLEN: ",c->children.length());
                    for(auto i : c->instances) {
                        print(" INST: ",i->form->dtype,"-",i->form->ID," VANTAGE AT: ",i->vantage->getPosition().to_string());
                        net->debug << draw_line(i->form->getPosition(),i->vantage->getPosition(),"red",1.0f);
                    }
                }
            }

            if(pressed(U)) {
                for(auto c : net->obs_root->children) {
                    if(c->label=="berry") {
                        for(auto i : c->instances) {
                            net->debug << draw_line(i->form->getPosition(),i->vantage->getPosition(),i->last_seen==s_tool.frame?"blue":"red",1.0f);
                        }
                    }
                }
            }

            if(pressed(C)) {
                net->clear_debug();
            }

            if(pressed(G)) {
                net->visualize_registry();
            }
         }



          if(amt==1) {
               // Navigate DOWN to children
            //    if(pressed(E)) {
            //         if(held(LSHIFT)) { // Cycle through siblings
            //             if(!current_view->children.empty()) {
            //                 child_index = (child_index + 1) % current_view->children.length();
            //                 mem->visualize_structure(current_view);
            //                 print("Viewing child ", child_index, " of ", current_view->children.length());
            //             }
            //         } else {
            //             if(!current_view->children.empty()) {
            //                 // Clamp to valid range before moving
            //                 child_index = child_index % current_view->children.length();
            //                 current_view = current_view->children[child_index];
            //                 mem->visualize_structure(current_view);
            //                 print("Moved to child ", child_index, ". Children: ", current_view->children.length(), 
            //                       ", Parents: ", current_view->parents.length());
            //             }
            //         }
            //     }
               
            //    // Navigate UP to parents
            //    if(pressed(Q)) {
            //         if(held(LSHIFT)) { // Cycle through parents
            //              if(!current_view->parents.empty()) {
            //                  parent_index = (parent_index + 1) % current_view->parents.length();
            //                  mem->visualize_structure(current_view);
            //                  print("Viewing parent ", parent_index, " of ", current_view->parents.length());
            //              }
            //          } else {
            //              if(!current_view->parents.empty()) {
            //                  g_ptr<c_node> parent(current_view->parents[parent_index]);
            //                  current_view = parent;
            //                  // DON'T reset indices - keep them!
            //                  mem->visualize_structure(current_view);
            //                  print("Moved to parent ", parent_index, ". Children: ", current_view->children.length(), 
            //                        ", Parents: ", current_view->parents.length());
            //              }
            //          }
            //    }
               
            //    // Reset to agent's current attention
            //    if(pressed(R)) {
            //         current_view = mem->physics_attention.last();
            //         child_index = 0;
            //         parent_index = 0;
            //         mem->visualize_structure(current_view);
            //         print("Reset to current attention head");
            //    }

            //    //Visualize from root
            //    if(pressed(V)) {
            //         mem->visualize_structure(mem->meta_root);
            //         current_view = mem->meta_root;
            //         print("Viewing full tree from root");
            //    }
               
               if(pressed(C)) {
                    mem->clear_debug();
               }

               if(pressed(G)) {
                    mem->visualize_registry();
               }
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
     window.unlock_mouse();
     return 0;
}
