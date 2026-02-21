#pragma once
#include<core/helper.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>
#include<util/logger.hpp>
#include<gui/gui_core.hpp>
#include<set>
using namespace Golden;
using namespace helper;

#ifndef CRUMB_ROWS
    #define CRUMB_ROWS 21
#endif

#ifndef CRUMB_COLS
    #define CRUMB_COLS 10
#endif

const int ALL = (0 << 16) | CRUMB_ROWS;

struct Crumb : public Object {
    Crumb() {
        setmat(0);
    }
    Crumb(float fill) {
        setmat(fill);
    }

    void release() override {
        if(refCount.fetch_sub(1) == 1) {
            delete this;
        }
        else if(getRefCount() == 1) {
            if(type_!=nullptr&&!recycled.load()) {
                type_->recycle(this);
            }
        }
    }

    Crumb& operator=(const Crumb& other) {
        if(this != &other) {
            std::memcpy(mat, other.mat, sizeof(mat));  // Copy raw array
            id = other.id;
        }
        return *this;
    }

    Crumb(const Crumb& other) {
        std::memcpy(mat, other.mat, sizeof(mat));
        id = other.id;
        decay = other.decay;
        sub.clear(); //just in case
        for(auto c : other.sub) {
            g_ptr<Crumb> new_crumb = g_dynamic_pointer_cast<Crumb>(type_->create());
            *new_crumb = *c;
            sub << new_crumb;
        }
    }

    int id = -1;
    float decay = 1.0f;
    list<g_ptr<Crumb>> sub;
    float mat[CRUMB_ROWS][CRUMB_COLS];

    inline void setmat(float v) {
        for(int r = 0; r < CRUMB_ROWS; r++) {
            for(int c = 0; c < CRUMB_COLS; c++) {
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

    inline float sum() {
        float s = 0.0f;
        for(int r = 0; r < CRUMB_ROWS; r++) {
            for(int c = 0; c < CRUMB_COLS; c++) {
                s+=mat[r][c];
            }
        }
        for(auto child : sub) {
            s+=child->sum();
        }
        return s;
    }

    inline float absSum() {
        float s = 0.0f;
        for(int r = 0; r < CRUMB_ROWS; r++) {
            for(int c = 0; c < CRUMB_COLS; c++) {
                s+=std::abs(mat[r][c]);
            }
        }
        for(auto child : sub) {
            s+=child->absSum();
        }
        return s;
    }

    std::string to_string() {
        std::string to_return;
        for(int r = 0; r < CRUMB_ROWS; r++) {
            if(r!=0)
                to_return.append("\n");
            for(int c = 0; c < CRUMB_COLS; c++) {
                to_return.append(std::to_string(mat[r][c]).substr(0,4)+(c==9?"":", "));
            }
        }
        return to_return;
    }
};

g_ptr<Crumb> clone(g_ptr<Crumb> crumb) {
    g_ptr<Crumb> new_crumb = g_dynamic_pointer_cast<Crumb>(crumb->type_->create());
    *new_crumb = *crumb;
    return new_crumb;
}

g_ptr<Crumb> IDENTITY = nullptr;
g_ptr<Crumb> ZERO = nullptr;

static bool is_mutable(g_ptr<Crumb> c) {
    if(c == ZERO || c == IDENTITY) return false;
    return true;
}

static g_ptr<Crumb> ensure_mutable(g_ptr<Crumb> c) {
    if(is_mutable(c)) return c;
    return clone(c);
}
static g_ptr<Crumb> ensure_imutable(g_ptr<Crumb> c) {
    if(is_mutable(c)) return clone(c);
    return c;
}
 
static float evaluate_elementwise(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb) {
    int eval_start = (eval_verb >> 16) & 0xFFFF; int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF; int against_count = against_verb & 0xFFFF;

     assert(eval_count == against_count); 

     float score = 0.0f;
     const float* eval_ptr = &eval->mat[eval_start][0];
     const float* against_ptr = &against->mat[against_start][0];
     int total = eval_count * CRUMB_COLS;
     for(int i = 0; i < total; i++) {
          score += eval_ptr[i] * against_ptr[i];
     }

     int eval_subs = eval->sub.length();
     int against_subs = against->sub.length();
     int max_subs = std::max(eval_subs, against_subs);
     
     for(int i = 0; i < max_subs; i++) {
         g_ptr<Crumb> eval_sub = nullptr;
         g_ptr<Crumb> against_sub = nullptr;
         
         if(i < eval_subs) {
            eval_sub = eval->sub[i];
         } else {
            eval_sub = ZERO;
         }
         
         if(i < against_subs) {
            against_sub = against->sub[i];
         } else {
            against_sub = ZERO;
         }
 
         score += evaluate_elementwise(eval_sub, eval_verb, against_sub, against_verb);
     }

     return score;
}

static float evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb) {
    return evaluate_elementwise(eval, eval_verb, against, against_verb);
}

static float sub_evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb) {
    int eval_start = (eval_verb >> 16) & 0xFFFF; int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF; int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count); 

    float score = 0.0f;
    const float* eval_ptr = &eval->mat[eval_start][0];
    const float* against_ptr = &against->mat[against_start][0];
    int total = eval_count * CRUMB_COLS;
    for(int i = 0; i < total; i++) {
         score += eval_ptr[i] - against_ptr[i];
    }

    int eval_subs = eval->sub.length();
    int against_subs = against->sub.length();
    int max_subs = std::max(eval_subs, against_subs);
    
    for(int i = 0; i < max_subs; i++) {
        g_ptr<Crumb> eval_sub = nullptr;
        g_ptr<Crumb> against_sub = nullptr;
        
        if(i < eval_subs) {
            eval_sub = eval->sub[i];
        } else {
            eval_sub = ZERO;
        }
        
        if(i < against_subs) {
            against_sub = against->sub[i];
        } else {
            against_sub = ZERO;
        }

        score += sub_evaluate(eval_sub, eval_verb, against_sub, against_verb);
    }

    return score;
}

static float sim_evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb) {
    int eval_start = (eval_verb >> 16) & 0xFFFF; int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF; int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count);

    float dot = 0.0f, eval_mag = 0.0f, against_mag = 0.0f;
    const float* eval_ptr = &eval->mat[eval_start][0];
    const float* against_ptr = &against->mat[against_start][0];
    int total = eval_count * CRUMB_COLS;

    for(int i = 0; i < total; i++) {
        dot        += eval_ptr[i] * against_ptr[i];
        eval_mag   += eval_ptr[i] * eval_ptr[i];
        against_mag += against_ptr[i] * against_ptr[i];
    }

    float denom = std::sqrt(eval_mag) * std::sqrt(against_mag);
    return denom > 0.0f ? dot / denom : 0.0f;
}

static float evaluate_binary(g_ptr<Crumb> a, int a_verb, g_ptr<Crumb> b, int b_verb) {
     int a_start = (a_verb >> 16) & 0xFFFF;
     int a_count = a_verb & 0xFFFF;
     int b_start = (b_verb >> 16) & 0xFFFF;
     int b_count = b_verb & 0xFFFF;
     
     assert(a_count == b_count);
     
     float* a_ptr = (float*)&a->mat[a_start][0];
     float* b_ptr = (float*)&b->mat[b_start][0];
     
     int total_elements = a_count * CRUMB_COLS;
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
static void apply_mask(g_ptr<Crumb> target, int target_verb, g_ptr<Crumb> mask, int mask_verb, Op op) {
    int target_start = (target_verb >> 16) & 0xFFFF;
    int target_count = target_verb & 0xFFFF;
    int mask_start = (mask_verb >> 16) & 0xFFFF;
    int mask_count = mask_verb & 0xFFFF;
    
    assert(target_count == mask_count);
    
    float* target_ptr = &target->mat[target_start][0];
    const float* mask_ptr = &mask->mat[mask_start][0];
    int total = target_count * CRUMB_COLS;
    
    for(int i = 0; i < total; i++) {
        target_ptr[i] = op(target_ptr[i], mask_ptr[i]);
    }

    int target_subs = target->sub.length();
    int mask_subs = mask->sub.length();
    int max_subs = std::max(target_subs, mask_subs);
    
    for(int i = 0; i < max_subs; i++) {
        g_ptr<Crumb> target_sub = nullptr;
        g_ptr<Crumb> mask_sub = nullptr;
        
        // Get or create target sub_crumb
        if(i < target_subs) {
            target_sub = ensure_mutable(target->sub[i]);
        } else {
            target_sub = clone(IDENTITY);
            target->sub << target_sub;
        }
        
        // Get mask sub_crumb or use zeros
        if(i < mask_subs) {
            mask_sub = mask->sub[i];
        } else {
            mask_sub = ZERO;
        }
        
        // Recursive application
        apply_mask(target_sub, target_verb, mask_sub, mask_verb, op);
    }
}

static void mult_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a * b; });
}

static void div_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a / b; });
}

static void add_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a + b; });
}

static void sub_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv) {
    apply_mask(t, tv, m, mv, [](float a, float b) { return a - b; });
}

static void scale_mask(g_ptr<Crumb> target, int verb, float scalar) {
     int start = (verb >> 16) & 0xFFFF;
     int count = verb & 0xFFFF;
     float* ptr = &target->mat[start][0];
     int total = count * CRUMB_COLS;
     
     for(int i = 0; i < total; i++) {
         ptr[i] *= scalar;
     }
 }

struct c_node;

static map<int,g_ptr<Crumb>> crumbs;
g_ptr<Crumb> get_crumb(int id) {
    g_ptr<Crumb> to_return = crumbs.getOrDefault(id,ZERO);
    if(to_return==ZERO) {
        print("No crumb in crumb map for: ","[ACCESED VIA ID]",id);
    }
    return to_return;
}
g_ptr<Crumb> get_crumb(g_ptr<Single> item) {
    g_ptr<Crumb> to_return = crumbs.getOrDefault(item->ID,ZERO);
    if(to_return==ZERO) {
        print("No crumb in crumb map for: ",item->dtype,item->ID);
    }
    return to_return;
}


class Episode : public Object {
public:
    list<g_ptr<Crumb>> states;
    list<g_ptr<Crumb>> deltas;
    int action_id = 0;
    int timestamp = 0;
    int hits = 0;

    std::string to_string() {
        std::string s;
        s.append("ID: "+std::to_string(action_id)+" AT: "+std::to_string(timestamp));
        for(int i=0;i<deltas.length();i++) {
            s.append("\n STATE: ");
            if(i<states.length()) {
                s.append("\n"+states[i]->to_string());
            } else {
                s.append(" ZERO");
            }

            s.append("\n  DELTA: ");
            if(deltas[i]==ZERO) s.append(" ZERO");
            else if(deltas[i]==IDENTITY) s.append(" IDENTITY");
            else  s.append("\n"+deltas[i]->to_string());
        }
        return s;
    }
};


class nodenet : public Object {
public:
    nodenet(g_ptr<Scene> _scene) : scene(_scene) {};
    nodenet() {};
    ~nodenet() {};
    
    g_ptr<Scene> scene = nullptr;

    list<g_ptr<Episode>> consolidated_episodes;
    float max_recent_episode_window = 1000;
    list<g_ptr<Episode>> recent_episodes;

    int cognitive_focus = 8;
    list<g_ptr<Crumb>> cognitive_attention;

    float match_episode(g_ptr<Episode> ep, list<g_ptr<Crumb>> crumbs) {
        if(ep->states.length() == 0 || crumbs.length() == 0) return 0.0f;
        
        float score = 0.0f;
        float total_weight = 0.0f;
        int slots = std::min(ep->states.length(), crumbs.length());
        
        for(int i = 0; i < slots; i++) {
            float weight = 1.0f / (float)(i + 1);
            score += sim_evaluate(crumbs[i], ALL, ep->states[i], ALL) * weight;
            total_weight += weight;
        }
        
        return score / total_weight;
    }

    bool has_instinctive_object_permanence = true;
    void apply_delta(g_ptr<Episode> ep, list<g_ptr<Crumb>>& crumbs) {
        for(int i = 0; i < ep->deltas.length() && i < crumbs.length(); i++) {
            g_ptr<Crumb> delta = ep->deltas[i];
            if(delta == IDENTITY) {
                //Carry forward
            } else if(delta == ZERO) {
                crumbs[i] = ZERO;
            } else {
                add_mask(crumbs[i], ALL, delta, ALL);
            }
        }
    }

    virtual float salience(g_ptr<Crumb> observation) {
        return 0.0f; //Default, should be overriden
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

    virtual list<g_ptr<Crumb>> gather_states() {
        list<g_ptr<Crumb>> states; 
        for(int i=0;i<cognitive_attention.length();i++) {
            states << ensure_imutable(cognitive_attention[i]);
        }
        while(states.length() < cognitive_focus) {
            states << ZERO;
        }
        return states;
    }

    virtual void form_episodes(list<g_ptr<Crumb>> before, list<g_ptr<Crumb>> after,int action_id,int timestamp = 0) {
        list<g_ptr<Crumb>> deltas;
        int max_len = std::max(before.length(), after.length());
        for(int i = 0; i < max_len; i++) {
            g_ptr<Crumb> delta;
            if(i >= before.length()) { //Created
                delta = clone(after[i]);
            }
            else if(i >= after.length()) { //Destroyed
                delta = ZERO; 
            }
            else { //Observe change
                delta = clone(after[i]);
                sub_mask(delta, ALL, before[i], ALL);
                if(delta->absSum() < 0.001f) {
                    delta = IDENTITY;
                }
            }
            deltas << delta;
        }
        g_ptr<Episode> episode = make<Episode>();
        episode->action_id = action_id;
        episode->timestamp = timestamp;
        episode->states = before;
        episode->deltas = deltas;
        recent_episodes << episode;
    }

    virtual void consolidate_episodes() {
        for(auto& recent : recent_episodes) {
            // Find best matching consolidated episode
            int best_idx = -1;
            float best_match = 0.0f;
            for(int i = 0; i < consolidated_episodes.length(); i++) {
                float match = match_episode(consolidated_episodes[i], recent->states);
                if(match > best_match) {
                    best_match = match;
                    best_idx = i;
                }
            }
            
            if(best_match > 0.7f) {
                auto& ep = consolidated_episodes[best_idx];
                ep->hits++;
                float rate = 1.0f / (float)ep->hits;
                
                int slots = std::min(ep->states.length(), recent->states.length());
                for(int i = 0; i < slots; i++) {
                    scale_mask(ep->states[i], ALL, 1.0f - rate);
                    g_ptr<Crumb> recent_scaled = clone(recent->states[i]);
                    scale_mask(recent_scaled, ALL, rate);
                    add_mask(ep->states[i], ALL, recent_scaled, ALL);
                    
                    if(recent->deltas[i] != IDENTITY && recent->deltas[i] != ZERO &&
                        ep->deltas[i] != IDENTITY && ep->deltas[i] != ZERO) {
                        scale_mask(ep->deltas[i], ALL, 1.0f - rate);
                        g_ptr<Crumb> delta_scaled = clone(recent->deltas[i]);
                        scale_mask(delta_scaled, ALL, rate);
                        add_mask(ep->deltas[i], ALL, delta_scaled, ALL);
                    }
                }
            } else {
                consolidated_episodes << recent;
            }
        }
    }
};

void init_nodenet(g_ptr<Scene> scene) {
     scene->define("Crumb",[](){
        g_ptr<Crumb> crumb = make<Crumb>();
        return crumb;
     });
     scene->add_initilizer("Crumb",[](g_ptr<Object> obj){
        if(auto crumb = g_dynamic_pointer_cast<Crumb>(obj)) {
            crumb->setmat(0.0f);
        }
     });
     IDENTITY = scene->create<Crumb>("Crumb"); IDENTITY->setmat(1.0f);
     ZERO = scene->create<Crumb>("Crumb"); ZERO->setmat(0.0f);
}