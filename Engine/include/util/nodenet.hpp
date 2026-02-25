#pragma once
#include <core/helper.hpp>
#include <core/physics.hpp>
#include <util/meshBuilder.hpp>
#include <util/logger.hpp>
#include <gui/gui_core.hpp>
#include <set>
using namespace Golden;
using namespace helper;

#ifndef CRUMB_ROWS
#define CRUMB_ROWS 21
#endif

#ifndef CRUMB_COLS
#define CRUMB_COLS 10
#endif

#define LOG_NODENET 1
#define LOG_REACTIONS 0

const int ALL = (0 << 16) | CRUMB_ROWS;

struct Crumb : public Object
{
    Crumb()
    {
        setmat(0);
    }
    Crumb(float fill)
    {
        setmat(fill);
    }

    void release() override
    {
        if (refCount.fetch_sub(1) == 1)
        {
            delete this;
        }
        else if (getRefCount() == 1)
        {
            if (type_ != nullptr && !recycled.load())
            {
                type_->recycle(this);
            }
        }
    }

    Crumb &operator=(const Crumb &other)
    {
        if (this != &other)
        {
            std::memcpy(mat, other.mat, sizeof(mat)); // Copy raw array
            id = other.id;
        }
        return *this;
    }

    Crumb(const Crumb &other)
    {
        std::memcpy(mat, other.mat, sizeof(mat));
        id = other.id;
        decay = other.decay;
        sub.clear(); // just in case
        for (auto c : other.sub)
        {
            g_ptr<Crumb> new_crumb = g_dynamic_pointer_cast<Crumb>(type_->create());
            *new_crumb = *c;
            sub << new_crumb;
        }
    }

    int id = -1;
    float decay = 1.0f;
    list<g_ptr<Crumb>> sub;
    float mat[CRUMB_ROWS][CRUMB_COLS];

    inline void setmat(float v)
    {
        for (int r = 0; r < CRUMB_ROWS; r++)
        {
            for (int c = 0; c < CRUMB_COLS; c++)
            {
                mat[r][c] = v;
            }
        }
    }

    inline float *data() const
    {
        return (float *)&mat[0][0];
    }

    inline int rows() const
    {
        return CRUMB_ROWS;
    }

    inline float sum()
    {
        float s = 0.0f;
        for (int r = 0; r < CRUMB_ROWS; r++)
        {
            for (int c = 0; c < CRUMB_COLS; c++)
            {
                s += mat[r][c];
            }
        }
        for (auto child : sub)
        {
            s += child->sum();
        }
        return s;
    }

    inline float absSum()
    {
        float s = 0.0f;
        for (int r = 0; r < CRUMB_ROWS; r++)
        {
            for (int c = 0; c < CRUMB_COLS; c++)
            {
                s += std::abs(mat[r][c]);
            }
        }
        for (auto child : sub)
        {
            s += child->absSum();
        }
        return s;
    }

    std::string to_string()
    {
        std::string to_return;
        for (int r = 0; r < CRUMB_ROWS; r++)
        {
            if (r != 0)
                to_return.append("\n");
            for (int c = 0; c < CRUMB_COLS; c++)
            {
                to_return.append(std::to_string(mat[r][c]).substr(0, 4) + (c == 9 ? "" : ", "));
            }
        }
        return to_return;
    }
};

g_ptr<Crumb> clone(g_ptr<Crumb> crumb)
{
    g_ptr<Crumb> new_crumb = g_dynamic_pointer_cast<Crumb>(crumb->type_->create());
    *new_crumb = *crumb;
    return new_crumb;
}

g_ptr<Crumb> IDENTITY = nullptr;
g_ptr<Crumb> ZERO = nullptr;

static bool is_mutable(g_ptr<Crumb> c)
{
    if (c == ZERO || c == IDENTITY)
        return false;
    return true;
}

static g_ptr<Crumb> ensure_mutable(g_ptr<Crumb> c)
{
    if (is_mutable(c))
        return c;
    return clone(c);
}
static g_ptr<Crumb> ensure_imutable(g_ptr<Crumb> c)
{
    if (is_mutable(c))
        return clone(c);
    return c;
}

static float evaluate_elementwise(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb)
{
    int eval_start = (eval_verb >> 16) & 0xFFFF;
    int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF;
    int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count);

    float score = 0.0f;
    const float *eval_ptr = &eval->mat[eval_start][0];
    const float *against_ptr = &against->mat[against_start][0];
    int total = eval_count * CRUMB_COLS;
    for (int i = 0; i < total; i++)
    {
        score += eval_ptr[i] * against_ptr[i];
    }

    int eval_subs = eval->sub.length();
    int against_subs = against->sub.length();
    int max_subs = std::max(eval_subs, against_subs);

    for (int i = 0; i < max_subs; i++)
    {
        g_ptr<Crumb> eval_sub = nullptr;
        g_ptr<Crumb> against_sub = nullptr;

        if (i < eval_subs)
        {
            eval_sub = eval->sub[i];
        }
        else
        {
            eval_sub = ZERO;
        }

        if (i < against_subs)
        {
            against_sub = against->sub[i];
        }
        else
        {
            against_sub = ZERO;
        }

        score += evaluate_elementwise(eval_sub, eval_verb, against_sub, against_verb);
    }

    return score;
}

static float evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb)
{
    return evaluate_elementwise(eval, eval_verb, against, against_verb);
}

static float sub_evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb)
{
    int eval_start = (eval_verb >> 16) & 0xFFFF;
    int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF;
    int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count);

    float score = 0.0f;
    const float *eval_ptr = &eval->mat[eval_start][0];
    const float *against_ptr = &against->mat[against_start][0];
    int total = eval_count * CRUMB_COLS;
    for (int i = 0; i < total; i++)
    {
        score += eval_ptr[i] - against_ptr[i];
    }

    int eval_subs = eval->sub.length();
    int against_subs = against->sub.length();
    int max_subs = std::max(eval_subs, against_subs);

    for (int i = 0; i < max_subs; i++)
    {
        g_ptr<Crumb> eval_sub = nullptr;
        g_ptr<Crumb> against_sub = nullptr;

        if (i < eval_subs)
        {
            eval_sub = eval->sub[i];
        }
        else
        {
            eval_sub = ZERO;
        }

        if (i < against_subs)
        {
            against_sub = against->sub[i];
        }
        else
        {
            against_sub = ZERO;
        }

        score += sub_evaluate(eval_sub, eval_verb, against_sub, against_verb);
    }

    return score;
}

static float sim_evaluate(g_ptr<Crumb> eval, int eval_verb, g_ptr<Crumb> against, int against_verb)
{
    int eval_start = (eval_verb >> 16) & 0xFFFF;
    int eval_count = eval_verb & 0xFFFF;
    int against_start = (against_verb >> 16) & 0xFFFF;
    int against_count = against_verb & 0xFFFF;

    assert(eval_count == against_count);

    float dot = 0.0f, eval_mag = 0.0f, against_mag = 0.0f;
    const float *eval_ptr = &eval->mat[eval_start][0];
    const float *against_ptr = &against->mat[against_start][0];
    int total = eval_count * CRUMB_COLS;

    for (int i = 0; i < total; i++)
    {
        dot += eval_ptr[i] * against_ptr[i];
        eval_mag += eval_ptr[i] * eval_ptr[i];
        against_mag += against_ptr[i] * against_ptr[i];
    }

    float denom = std::sqrt(eval_mag) * std::sqrt(against_mag);
    return denom > 0.0f ? dot / denom : 0.0f;
}

static float evaluate_binary(g_ptr<Crumb> a, int a_verb, g_ptr<Crumb> b, int b_verb)
{
    int a_start = (a_verb >> 16) & 0xFFFF;
    int a_count = a_verb & 0xFFFF;
    int b_start = (b_verb >> 16) & 0xFFFF;
    int b_count = b_verb & 0xFFFF;

    assert(a_count == b_count);

    float *a_ptr = (float *)&a->mat[a_start][0];
    float *b_ptr = (float *)&b->mat[b_start][0];

    int total_elements = a_count * CRUMB_COLS;
    int matching_bits = 0;
    int total_bits = total_elements * 32;

    for (int i = 0; i < total_elements; i++)
    {
        uint32_t a_bits = *(uint32_t *)&a_ptr[i];
        uint32_t b_bits = *(uint32_t *)&b_ptr[i];
        uint32_t xor_result = a_bits ^ b_bits;
        uint32_t matching = ~xor_result;
        matching_bits += __builtin_popcount(matching);
    }
    return (float)matching_bits / (float)total_bits;
}

template <typename Op>
static void apply_mask(g_ptr<Crumb> target, int target_verb, g_ptr<Crumb> mask, int mask_verb, Op op)
{
    int target_start = (target_verb >> 16) & 0xFFFF;
    int target_count = target_verb & 0xFFFF;
    int mask_start = (mask_verb >> 16) & 0xFFFF;
    int mask_count = mask_verb & 0xFFFF;

    assert(target_count == mask_count);

    float *target_ptr = &target->mat[target_start][0];
    const float *mask_ptr = &mask->mat[mask_start][0];
    int total = target_count * CRUMB_COLS;

    for (int i = 0; i < total; i++)
    {
        target_ptr[i] = op(target_ptr[i], mask_ptr[i]);
    }

    int target_subs = target->sub.length();
    int mask_subs = mask->sub.length();
    int max_subs = std::max(target_subs, mask_subs);

    for (int i = 0; i < max_subs; i++)
    {
        g_ptr<Crumb> target_sub = nullptr;
        g_ptr<Crumb> mask_sub = nullptr;

        // Get or create target sub_crumb
        if (i < target_subs)
        {
            target_sub = ensure_mutable(target->sub[i]);
        }
        else
        {
            target_sub = clone(IDENTITY);
            target->sub << target_sub;
        }

        // Get mask sub_crumb or use zeros
        if (i < mask_subs)
        {
            mask_sub = mask->sub[i];
        }
        else
        {
            mask_sub = ZERO;
        }

        // Recursive application
        apply_mask(target_sub, target_verb, mask_sub, mask_verb, op);
    }
}

static void mult_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv)
{
    apply_mask(t, tv, m, mv, [](float a, float b)
               { return a * b; });
}

static void div_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv)
{
    apply_mask(t, tv, m, mv, [](float a, float b)
               { return a / b; });
}

static void add_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv)
{
    apply_mask(t, tv, m, mv, [](float a, float b)
               { return a + b; });
}

static void sub_mask(g_ptr<Crumb> t, int tv, g_ptr<Crumb> m, int mv)
{
    apply_mask(t, tv, m, mv, [](float a, float b)
               { return a - b; });
}

static void scale_mask(g_ptr<Crumb> target, int verb, float scalar)
{
    int start = (verb >> 16) & 0xFFFF;
    int count = verb & 0xFFFF;
    float *ptr = &target->mat[start][0];
    int total = count * CRUMB_COLS;

    for (int i = 0; i < total; i++)
    {
        ptr[i] *= scalar;
    }
}

struct c_node;

static map<int, g_ptr<Crumb>> crumbs;
g_ptr<Crumb> get_crumb(int id)
{
    g_ptr<Crumb> to_return = crumbs.getOrDefault(id, ZERO);
    if (to_return == ZERO)
    {
        print("No crumb in crumb map for: ", "[ACCESED VIA ID]", id);
    }
    return to_return;
}
g_ptr<Crumb> get_crumb(g_ptr<Single> item)
{
    g_ptr<Crumb> to_return = crumbs.getOrDefault(item->ID, ZERO);
    if (to_return == ZERO)
    {
        print("No crumb in crumb map for: ", item->dtype, item->ID);
    }
    return to_return;
}

class Episode : public Object
{
public:
    list<g_ptr<Crumb>> states;
    list<g_ptr<Crumb>> deltas;
    int action_id = 0;
    int timestamp = 0;
    int hits = 0;
    float score = 0.0f;

    std::string to_string() {
        std::string s;
        s.append("EPISODE ID: " + std::to_string(action_id) + " AT: " + std::to_string(timestamp)+": ");
        
        int slots = std::max(deltas.length(), states.length());
        for(int i = 0; i < slots; i++) {
            // STATE inline
            s.append("\nSTATE: ");
            if(i < states.length()) {
                std::string state_str = states[i]->to_string();
                indent_multiline(state_str,std::string(7, ' '));
                s.append(state_str);
            } else {
                s.append("ZERO");
            }
            s.append("\nDELTA: ");
            if(i < deltas.length()) {
                if(deltas[i] == ZERO) s.append("ZERO");
                else if(deltas[i] == IDENTITY) s.append("IDENTITY");
                else {
                    std::string delta_str = deltas[i]->to_string();
                    indent_multiline(delta_str,std::string(7, ' '));
                    s.append(delta_str);
                }
            } else {
                s.append("NONE");
            }
        }
        return s;
    }
};

struct SeqLine : public q_object
{
    SeqLine() {};
    SeqLine(const std::string _label) {
        label = _label;
        Log::Line new_timer; new_timer.start();
        timer = new_timer;
    }

    Log::Line timer;
    std::string label = "";
    SeqLine* parent = nullptr;
    list<g_ptr<SeqLine>> children;
    list<std::string> seqs;

    std::string get_indent() {
        if(!parent) return "";
        int depth = 0;
        SeqLine* cursor = this;
        while(cursor->parent) { depth++; cursor = cursor->parent; }
        std::string indent(depth * 3, ' ');
        return indent;
    }

    std::string to_string() {
        std::string indent = get_indent();
        std::string to_return = "";
        to_return.append(indent+label+" [time: " + ftime(timer.total_time_)+"]\n");
        for(auto& msg : seqs) {
            to_return.append(msg+"\n");
        }
        for(auto& child : children) {
            to_return.append(child->to_string());
        }
        return to_return;
    }
};

class Span : public Object
{
public:
    Span() {
        line_root = make<SeqLine>("Root");
    };

    map<std::string, Log::Line> timers;
    map<std::string, int> counters;

    void start_timer(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            Log::Line &timer = timers.get(label);
            timer.start();
        }
        else
        {
            Log::Line timer;
            timer.start();
            timers.put(label, timer);
        }
    }

    double end_timer(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            Log::Line &timer = timers.get(label);
            return timer.end();
        }
        return 0.0;
    }

    double get_time(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            return timers.get(label).total_time_;
        }
        else
        {
            return -1.0;
        }
    }

    std::string timer_string(const std::string &label)
    {
        return label + ": " + ftime(get_time(label));
    }

    void print_timers()
    {
        for (auto label : timers.keySet())
        {
            print(timer_string(label));
        }
    }

    void increment(const std::string &label, int by = 1)
    {
        counters.getOrPut(label, 0) += by;
    }

    int get_count(const std::string &label)
    {
        return counters.getOrDefault(label, 0);
    }

    void print_counters()
    {
        for (auto label : counters.keySet())
        {
            print(label, ": ", get_count(label));
        }
    }

    map<g_ptr<Crumb>, std::string> crumb_labels;
    map<g_ptr<Episode>, std::string> episode_labels;

    void label_crumb(g_ptr<Crumb> c, const std::string &label)
    {
        crumb_labels.put(c, label);
    }

    void label_episode(g_ptr<Episode> ep, const std::string &label)
    {
        episode_labels.put(ep, label);
    }

    std::string get_crumb_label(g_ptr<Crumb> c)
    {
        std::string fallback_label = "[unlabeled]";
        return crumb_labels.getOrDefault(c, fallback_label);
    }

    std::string get_episode_label(g_ptr<Episode> ep)
    {
        std::string fallback_label = "[unlabeled]";
        return episode_labels.getOrDefault(ep,fallback_label);
    }

    g_ptr<SeqLine> line_root = nullptr;
    g_ptr<SeqLine> on_line = nullptr;

    g_ptr<SeqLine> get_last_line() {
        if(on_line) return on_line;
        else return line_root;
    }

    void add_line(const std::string& label) {
        g_ptr<SeqLine> parent = get_last_line();
        parent->children << make<SeqLine>(label);
        parent->children.last()->parent = parent.getPtr();
        on_line = parent->children.last();
    }

    void end_line() 
    {
        if(!on_line) return;
        on_line->timer.end();
        if(on_line->parent) {
            on_line = on_line->parent;
        }
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        std::string indent = get_last_line()->get_indent();
        indent += "  > "; //Extra space to distinquish from header
        std::string msg = indent+oss.str();
        indent_multiline(msg,indent);
        get_last_line()->seqs << msg;
    }

    void print_all() {
        line_root->timer.end();
        print(line_root->to_string());
    }
};

class nodenet : public Object
{
public:
    nodenet(g_ptr<Scene> _scene) : scene(_scene) {};
    nodenet() {};
    ~nodenet() {};

    g_ptr<Scene> scene = nullptr;
    g_ptr<Span> span;

    void newline(const std::string& label) {
        span->add_line(label);
    }

    void endline() {
        span->end_line();
    }

    template<typename... Args>
    void log(Args&&... args) {
        span->log(std::forward<Args>(args)...);
    }

    void test_logging() {
        log("data ",1);
        log("data ",2);
        newline("Sub-line");
        log("sub_data ",3,"\nLR: 8");
        log("sub_data ",4,"\nLR: 10");
            newline("Sub-Sub-line");
            log("sub_sub_data ",12,"\nLR: 14");
            endline();
        endline();
        newline("Sub-line 2");
        log("sub_data ",5);
        log("sub_data ",6);
        endline();
        span->print_all();
    }
    

    list<g_ptr<Episode>> consolidated_episodes;
    float max_recent_episode_window = 10000;
    list<g_ptr<Episode>> recent_episodes;

    int cognitive_focus = 8;
    list<g_ptr<Crumb>> cognitive_attention;

    virtual list<g_ptr<Episode>> potential_reactions(list<g_ptr<Crumb>> crumbs, bool flip = false)
    {
        // Nothing for now, implment genericly later
        return list<g_ptr<Episode>>{};
    }
    virtual list<g_ptr<Episode>> potential_reactions(g_ptr<Episode> ep, bool flip = false)
    {
        return potential_reactions(ep->states);
    }

    // Visit should return false to stop, true to expand.
    void walk_graph(g_ptr<Episode> start,
                    std::function<bool(g_ptr<Episode>)> visit,
                    std::function<list<g_ptr<Episode>>(g_ptr<Episode>)> expand,
                    bool breadth_first = false)
    {

        std::set<void *> visited;
        list<g_ptr<Episode>> queue;
        queue << start;

        while (!queue.empty())
        {
            g_ptr<Episode> current = breadth_first ? queue.first() : queue.last();
            if (breadth_first)
                queue.removeAt(0);
            else
                queue.pop();

            void *ptr = (void *)current.getPtr();
            if (visited.count(ptr) > 0)
                continue;
            visited.insert(ptr);

            if (!visit(current))
                continue;
            queue << expand(current);
        }
    }

    void profile(g_ptr<Episode> start, std::function<bool(g_ptr<Episode>)> visit)
    {
        walk_graph(start, visit, [this](g_ptr<Episode> ep)
                   { return potential_reactions(ep); }, false);
    }

    virtual float propagate(g_ptr<Episode> start, int& energy, std::function<bool(g_ptr<Episode>, int&, int)> visit, int depth = 0, float last_score = 0.0f)
    {
        if (energy <= 0)
            return 0.0f;
        if (!visit(start, energy, depth))
            return 0.0f;

        auto reactions = potential_reactions(start);
        if(!reactions.empty()) {
            energy -= 1;
            for(auto reaction : reactions) {
                propagate(reaction, energy, visit, depth + 1);
            }
        }
        return 0.0f;
    }

    float match_episode(g_ptr<Episode> ep, int eval_verb, list<g_ptr<Crumb>> crumbs, int against_verb)
    {
        if (ep->states.length() == 0 || crumbs.length() == 0)
            return 0.0f;

        float score = 0.0f;
        float total_weight = 0.0f;
        int slots = std::min(ep->states.length(), crumbs.length());

        for (int i = 0; i < slots; i++)
        {
            float weight = 1.0f / (float)(i + 1);
            score += sim_evaluate(ep->states[i], eval_verb, crumbs[i], against_verb) * weight;
            total_weight += weight;
        }

        return score / total_weight;
    }
    float match_episode(g_ptr<Episode> ep, int eval_verb, g_ptr<Episode> against, int against_verb)
    {
        return match_episode(ep, eval_verb, against->states, against_verb);
    }

    bool has_instinctive_object_permanence = true;
    void apply_delta(g_ptr<Episode> ep, list<g_ptr<Crumb>> &crumbs)
    {
        for (int i = 0; i < ep->deltas.length() && i < crumbs.length(); i++)
        {
            g_ptr<Crumb> delta = ep->deltas[i];
            if (delta == IDENTITY)
            {
                // Carry forward
            }
            else if (delta == ZERO)
            {
                crumbs[i] = ZERO;
            }
            else
            {
                add_mask(crumbs[i], ALL, delta, ALL);
            }
        }
    }

    virtual float salience(g_ptr<Crumb> observation)
    {
        return 0.0f; // Default, should be overriden
    }

    // Standard action unit made to be used across any platform or purpouse
    struct Action
    {
        Action(int _id) : part_id(_id) {}
        Action(int _id, list<float> _params) : part_id(_id), params(_params) {}
        ~Action() {}

        int part_id;
        list<float> params; // Could be positions, velocities, etc.
        float duration;     // Timing
        float max_effort;   // Force/torque limits (optional)

        vec3 asvec3(int offset = 0) const
        {
            vec3 to_return;
            for (int i = 0; i < 3; i++)
            {
                if (i == 0)
                    to_return.x(params[i + offset]);
                if (i == 1)
                    to_return.y(params[i + offset]);
                if (i == 2)
                    to_return.z(params[i + offset]);
            }
            return to_return;
        }
    };

    // Visual abstraction (what each see pass populates, carries metadata about its production for reflection)
    struct Vis : public q_object
    {
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

        // For 3d
        list<vec3> colors;
        list<vec3> normals;
        int elevation_samples = 0;
        int azimuth_samples = 0;
        float vertical_fov = 0.0f;
    };

    virtual list<g_ptr<Crumb>> gather_states()
    {
        list<g_ptr<Crumb>> states;
        for (int i = 0; i < cognitive_attention.length(); i++)
        {
            states << ensure_imutable(cognitive_attention[i]);
        }
        while (states.length() < cognitive_focus)
        {
            states << ZERO;
        }
        return states;
    }

    virtual g_ptr<Episode> form_episode(list<g_ptr<Crumb>> before, list<g_ptr<Crumb>> after, int action_id, int timestamp = 0)
    {
        list<g_ptr<Crumb>> deltas;
        int max_len = std::max(before.length(), after.length());
        for (int i = 0; i < max_len; i++)
        {
            g_ptr<Crumb> delta;
            if (i >= before.length())
            { // Created
                delta = clone(after[i]);
            }
            else if (i >= after.length())
            { // Destroyed
                delta = ZERO;
            }
            else
            { // Observe change
                delta = clone(after[i]);
                sub_mask(delta, ALL, before[i], ALL);
                if (delta->absSum() < 0.001f)
                {
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
        return episode;
    }

    // Returns the number of hits of the episode which was consolidated into.
    // match_threshold is the required similarity between epiosdes to consolidate, as evaluated across the provided verb
    // learn_rate is how much the consolidating episode influences the episode,
    // i.e, the ratio of effect (0.1 would mean 10% of the consolidated epiosde is the effect of the epiosde fuzzed into it)
    virtual int consolidate_episode(g_ptr<Episode> &episode, list<g_ptr<Episode>> &into, int verb = ALL, float match_threshold = 0.7f, float learn_rate = -1.0f)
    {
        int best_idx = -1;
        float best_match = 0.0f;
        for (int i = 0; i < into.length(); i++)
        {
            float match = match_episode(into[i], verb, episode, verb);
            if (match > best_match)
            {
                best_match = match;
                best_idx = i;
            }
        }
        int consolidated_hits = 0;
        if (best_match > match_threshold)
        {
            g_ptr<Episode> ep = into[best_idx];
            ep->hits++;
            float rate = learn_rate < 0 ? 1.0f / (float)ep->hits : learn_rate;
            int slots = std::min(ep->states.length(), episode->states.length());
            for (int i = 0; i < slots; i++)
            {
                scale_mask(ep->states[i], verb, 1.0f - rate);
                g_ptr<Crumb> recent_scaled = clone(episode->states[i]);
                scale_mask(recent_scaled, verb, rate);
                add_mask(ep->states[i], verb, recent_scaled, verb);
            }
            int deltas = std::min(episode->deltas.length(), ep->deltas.length());
            for (int i = 0; i < deltas; i++)
            {
                if (is_mutable(episode->deltas[i]) && is_mutable(ep->deltas[i]))
                {
                    scale_mask(ep->deltas[i], verb, 1.0f - rate);
                    g_ptr<Crumb> delta_scaled = clone(episode->deltas[i]);
                    scale_mask(delta_scaled, verb, rate);
                    add_mask(ep->deltas[i], verb, delta_scaled, verb);
                }
            }
            episode = ep;
            consolidated_hits = ep->hits;
        }
        else
        {
            into << episode;
        }
        return consolidated_hits;
    }

    // virtual void consolidate_episodes() {
    //     // Sleep = propagate each recent episode with high energy
    //     // no action selection, just let consolidation accumulate
    //     for(auto& recent : recent_episodes) {
    //         float sleep_energy = cognitive_focus * 8; // more energy than live reasoning
    //         propagate(recent, sleep_energy, [](g_ptr<Episode> ep, float energy) {
    //             return true; // no selection pressure, just run
    //         });
    //     }
    //     // Full consolidation pass after rehearsal
    //     consolidate_episodes(0.7f, -1.0f, true);
    // }

    // virtual void consolidate_episodes() {
    //     for(auto& recent : recent_episodes) {
    //         // Find best matching consolidated episode
    //         int best_idx = -1;
    //         float best_match = 0.0f;
    //         for(int i = 0; i < consolidated_episodes.length(); i++) {
    //             float match = match_episode(consolidated_episodes[i], recent->states);
    //             if(match > best_match) {
    //                 best_match = match;
    //                 best_idx = i;
    //             }
    //         }

    //         if(best_match > 0.7f) {
    //             auto& ep = consolidated_episodes[best_idx];
    //             ep->hits++;
    //             float rate = 1.0f / (float)ep->hits;

    //             int slots = std::min(ep->states.length(), recent->states.length());
    //             for(int i = 0; i < slots; i++) {
    //                 scale_mask(ep->states[i], ALL, 1.0f - rate);
    //                 g_ptr<Crumb> recent_scaled = clone(recent->states[i]);
    //                 scale_mask(recent_scaled, ALL, rate);
    //                 add_mask(ep->states[i], ALL, recent_scaled, ALL);

    //                 if(recent->deltas[i] != IDENTITY && recent->deltas[i] != ZERO &&
    //                     ep->deltas[i] != IDENTITY && ep->deltas[i] != ZERO) {
    //                     scale_mask(ep->deltas[i], ALL, 1.0f - rate);
    //                     g_ptr<Crumb> delta_scaled = clone(recent->deltas[i]);
    //                     scale_mask(delta_scaled, ALL, rate);
    //                     add_mask(ep->deltas[i], ALL, delta_scaled, ALL);
    //                 }
    //             }
    //         } else {
    //             consolidated_episodes << recent;
    //         }
    //     }
    // }
};

void init_nodenet(g_ptr<Scene> scene)
{
    scene->define("Crumb", []()
                  {
        g_ptr<Crumb> crumb = make<Crumb>();
        return crumb; });
    scene->add_initilizer("Crumb", [](g_ptr<Object> obj)
                          {
        if(auto crumb = g_dynamic_pointer_cast<Crumb>(obj)) {
            crumb->setmat(0.0f);
        } });
    IDENTITY = scene->create<Crumb>("Crumb");
    IDENTITY->setmat(1.0f);
    ZERO = scene->create<Crumb>("Crumb");
    ZERO->setmat(0.0f);
}