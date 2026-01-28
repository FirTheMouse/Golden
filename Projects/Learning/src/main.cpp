#include<core/helper.hpp>

using namespace Golden;
using namespace helper;


template<typename T>
static T fuzzy_match(const T& query, list<T> database, list<std::function<int(T,T)>> score_funcs,bool debug = false) {
    map<T,int> scores;
    for(auto query_against : database) {
        int score = 0;
        for(auto function : score_funcs) {
            score+=function(query,query_against);
        }
        scores.put(query_against,score);
        if(debug) {
            print(query_against, ": ", score);
        }
    }
    list<entry<T,int>> score_list = scores.entrySet();
    score_list.sort([](entry<T,int> a, entry<T,int> b){
        return a.value > b.value;
    });
    return score_list[0].key;
}


struct Query {
    list<g_ptr<Object>> objs;

    template<typename T>
    Query& where(const std::string& label, std::function<bool(T)> func) {
        list<g_ptr<Object>> survivours;
        for(auto o : objs) {
            if(func(o->get<T>(label))) survivours << o;
        }
        objs = survivours;
        return *this;
    }

    list<g_ptr<Object>> get() {
        return objs;
    }
};



int main() {

        // Setup some test objects
        auto e1 = make<Object>();
        e1->set<int>("health", 75);
        e1->set<std::string>("type", "enemy");
        
        auto e2 = make<Object>();
        e2->set<int>("health", 30);
        e2->set<std::string>("type", "enemy");
        
        auto e3 = make<Object>();
        e3->set<int>("health", 90);
        e3->set<std::string>("type", "friendly");
        
        Query q;
        q.objs = {e1, e2, e3};
        
        q.where<int>("health", [](int h) { return h > 50; });
        q.where<std::string>("type", [](std::string t) { return t == "enemy"; });
        
        auto results = q.get();
        print("Found ", results.length(), " objects");
        for(auto obj : results) {
            print("  Health: ", obj->get<int>("health"));
        }

    return 0;
}