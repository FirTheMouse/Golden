#include<util/util.hpp>
#include<util/logger.hpp>


g_ptr<Log::Span> span = nullptr;
static inline void newline(const std::string& label) {
    span->add_line(label);
}
static inline double endline() {
    return span->end_line();
}
template<typename... Args>
static inline void log(Args&&... args) {
    span->log(std::forward<Args>(args)...);
}

#define RAW_ITEMS 10
#define PRODUCTS 5
#define TOTAL_ITEMS RAW_ITEMS+PRODUCTS

struct Agent : public Object {
    Agent() {
        for(int i=0;i<TOTAL_ITEMS;i++) {
            carrying << 0;
        }   
    }
    list<int> carrying;
    list<std::function<bool()>> skills;
    float money;
};

struct Appliance : public Object {
    bool broken;
    std::function<bool(list<int>&)> process;
    void use(list<int>& inventory) {
        if(!broken) {
            if(process(inventory)) {
                broken = true;
            }
        }
    }
};

struct Location : public Object {
    Location() {
        for(int i=0;i<TOTAL_ITEMS;i++) {
            inventory << 0;
        }   
    }
    list<int> inventory;
    list<std::function<bool()>> menu;

    list<g_ptr<Appliance>> appliances;
    list<g_ptr<Agent>> workers;
    list<g_ptr<Agent>> managers;

    list<g_ptr<Agent>> customers;


    void train_worker(int at) {
        g_ptr<Agent> agent = workers.get(at);

    }
};

g_ptr<Agent> make_new_worker() {
    g_ptr<Agent> agent = make<Agent>();
    return agent;
}

g_ptr<Agent> make_new_customer() {
    g_ptr<Agent> agent = make<Agent>();
    return agent;
}

g_ptr<Appliance> make_new_appliance() {
    g_ptr<Appliance> app = make<Appliance>();
    int break_chance = randi(0,100);
    int product = randi(RAW_ITEMS,TOTAL_ITEMS);
    list<int> raw_items(RAW_ITEMS);
    for(int i=0;i<randi(1,12);i++) {
        raw_items[randi(0,RAW_ITEMS)]++;
    }
    app->process = [=](list<int>& inventory){
        bool can_cook = true;
        for(int i = 0; i<raw_items.length();i++) {
            if(inventory[i]>=raw_items[i]) {
                inventory[i] -= raw_items[i];
            } else {
                can_cook = false;
                break;
            }
        }

        if(can_cook) {
            inventory[product]++;
        }

        return randi(0,100) < break_chance;
    };

    return app;
}

std::function<bool()> make_new_menu_item() {
    return [](){
        return true;
    };
}

g_ptr<Location> make_new_location() {
    g_ptr<Location> loc = make<Location>();
    for(int i = 0;i<10;i++) {
        loc->menu << make_new_menu_item();
    }
    return loc;
}

int main() {
    span = make<Log::Span>();
    list<g_ptr<Location>> locs;
    locs << make_new_location();



    return 0;
}