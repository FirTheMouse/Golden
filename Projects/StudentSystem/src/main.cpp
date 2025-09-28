#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<util/color.hpp>
#include<util/string_generator.hpp>

using namespace Golden;

g_ptr<Scene> scene;
g_ptr<Font> font;

g_ptr<Quad> info_card;
g_ptr<Quad> thumb;

std::string tstr(int i) {
    return std::to_string(i);
}

g_ptr<Quad> makeLine() {
    auto g = make<Quad>();
    scene->add(g);
    g->scale(vec2(5,5));
    g->setColor(Color::WHITE);
    g->flagOff("valid");
    return g;
}

void updateLine(g_ptr<Quad> item,vec2 from) {
    vec2 dir = from-item->getPosition();
    item->scale(vec2(std::abs(dir.length()),3));
    float angle = atan2(dir.y(), dir.x());
    item->rotate(angle);
}

int weighted_dist(list<ivec2> dist) {
    int r = randi(1,100);
    for(int i=0;i<dist.length();i++) {
        if(r<=dist[i].x()) {
            return dist[i].y();
        } 
    }
    return dist[0].y();
}

class person;

struct life {
    life() {}

    bool dead = false;
    int died = -1;

    std::string name;
    std::string surname;
    std::string maiden_name;

    std::string general_info;

    int age = 0;
    int gender = -1; //Add this later
    int species = -1; //Add this later

    int intel = -1;
    int knowledge = -1;

    int origin = -1;//Size of where they grew up: 1=Wilderness,2=Village,3=Town,4=City
        std::string origin_name;
        std::string origin_info;
    int lives = -1;
        std::string lives_name;
        std::string lives_info;
    int wealth = -1; //Wealth: 1=Impovrished,2=Poor,3=Lower-middle,4=Middle,5=Upper-middle,6=Wealthy,7=Incredibly Wealthy,8=Ultra rich
        std::string wealth_info;
    int magic = -1;
        std::string magic_info;
    int education = -1;
        std::string education_info;
        int enrolled = -1;
        int ended = -1;
        int reenrolled = -1;

    void simulate_life_step(g_ptr<person> node,g_ptr<person> parent = nullptr);
};

class person : public Object {
public:
    std::string name = "noname";

    g_ptr<person> spouse;
    bool isSpouse = false;
    list<g_ptr<person>> children;


    list<g_ptr<Quad>> lines;
    g_ptr<Quad> txt;

    life l;

    void cleanup() {
        scene->recycle(txt);
        if(spouse) {
            spouse->cleanup();
        }
        for(auto c : children) {
            c->cleanup();
        }
    }

    float width() {
        int len = text::string_of(txt).length();
        float wid = 25;
        return len*wid;
    }

    vec2 center() {return text::center_of(txt);}
    vec2 right() {return center().addX(width()/2);}
    vec2 left() {return center().addX(-width()/2);}

    //Add a note to the info card
    void note(const std::string& label, const std::string& content) {
        txt->set<std::string>(label,content);
    }

    //Get a note from the info card
    std::string retrive(const std::string& label) {
        return txt->get<std::string>(label);
    }

    void setName(const std::string& n) {
        name = n;
    }

    void step_life(g_ptr<person> parent = nullptr) {
        l.simulate_life_step(this,parent);
    }

    void realize() {
        if(txt) {
            text::setText(name,txt);
        } else
            txt = text::makeText(name,font,scene,vec2(1250,40),1);
        txt->set<std::string>("name",name);
        txt->set<g_ptr<person>>("person",this);

        if(spouse) {
            spouse->realize();
        }
        for(auto c : children) {
            c->realize();
        }
    }

    vec2 t_vec = vec2(0,0);

    void track(const vec2& delta) {
        if(!txt->isActive()) {
            t_vec+=delta;
        }

        if(spouse) {
            spouse->track(delta);
        }
        for(auto c : children) {
            c->track(delta);
        }
    }

    void hide() {
        if(!txt->culled()) {
            txt->hide();
            t_vec = vec2(0,0);
        }
        for(auto l :lines) {
            l->hide();
        }
        if(spouse) {
            spouse->hide();
        }
        for(auto c : children) {
            c->hide();
        }
    }

    void show() {
        if(txt->culled()) {
            txt->show();
            txt->move(t_vec);
        }
        for(auto l : lines) {
            l->show();
            l->move(t_vec);
        }
        if(spouse) {
            spouse->show();
        }
        for(auto c : children) {
            c->show();
        }
    }
};


std::string note_to_string(const std::string& category, int value) {
    if(category == "Origin") {
        switch(value) {
            case 1: return "Wilderness";
            case 2: return "Village";
            case 3: return "Town";
            case 4: return "City";
            default: return "Unknown";
        }
    }
    else if(category == "Wealth") {
        switch(value) {
            case 1: return "Impoverished";
            case 2: return "Poor";
            case 3: return "Lower-middle";
            case 4: return "Middle";
            case 5: return "Upper-middle";
            case 6: return "Wealthy";
            case 7: return "Incredibly wealthy";
            case 8: return "Ultra Rich";
            default: return "Unknown";
        }
    }
    else if(category == "Magical") {
        switch(value) {
            case 0: return "None";
            case 1: return "Weak";
            case 2: return "Moderate";
            case 3: return "Strong";
            default: return "Unknown";
        }
    }
    else if(category == "Education") {
        switch(value) {
            case 0: return "None";
            case 1: return "Literacy";
            case 2: return "Standard";
            case 3: return "Advanced";
            case 4: return "Academy";
            default: return "Unknown";
        }
    }
    
    // Fallback for unknown categories
    return std::to_string(value);
}

size_t NthNL(const std::string& str, int n) {
    size_t pos = 0;
    for(int i = 0; i < n; ++i) {
        pos = str.find('\n', pos);
        if(pos == std::string::npos) return std::string::npos;
        if(i < n - 1) pos++; 
    }
    // if(pos>0) pos--;
    return pos;
}

void life_info(const life& l) { 

    list<std::string> line_ret_text;

    std::string history = 
    l.name+" "+l.surname+
    "\n------------------------------";
    line_ret_text << "";
    line_ret_text << l.general_info;
    history.append(
        "\nOrigin: "+note_to_string("Origin",l.origin)+
        "\nWealth: "+note_to_string("Wealth",l.wealth)+
        "\nMagic: "+note_to_string("Magical",l.magic)
    );
    line_ret_text << l.origin_info;
    line_ret_text << l.wealth_info;
    line_ret_text << l.magic_info;

    if(l.education>0) {
        history.append(
            "\nEducation: "+note_to_string("Education",l.education)
        );
        line_ret_text << l.education_info;
    }

    history.append(
        "\nLives: "+note_to_string("Origin",l.origin)+
        "\nDied: "+tstr(l.died)
    );
    line_ret_text << l.lives_info;
    line_ret_text << "";

    text::setText(history,info_card);

    for(int i = 0;i<line_ret_text.length();i++) {
        if(line_ret_text[i]!="")
            text::char_at(NthNL(history,i)+1,info_card)->set<std::string>("info",line_ret_text[i]);
    }

    info_card->setPosition(vec2(50,40));
}


list<list<ivec2>> origin_wealth_dist = {
    { //Wilderness - subsistence living, very few wealthy
        {40, 1}, {70, 2}, {85, 3}, {95, 4}, 
        {99, 5}, {100, 6}, {100, 7}, {100, 8}
    },
    { //Village - mostly lower classes, some middle class
        {25, 1}, {50, 2}, {75, 3}, {90, 4}, 
        {96, 5}, {99, 6}, {100, 7}, {100, 8}
    },
    { //Town - more middle class opportunities, some wealthy
        {15, 1}, {35, 2}, {60, 3}, {80, 4}, 
        {92, 5}, {97, 6}, {99, 7}, {100, 8}
    },
    { //City - full wealth spectrum, but still poverty exists
        {8, 1}, {25, 2}, {45, 3}, {70, 4}, 
        {85, 5}, {94, 6}, {98, 7}, {100, 8}
    },
};

std::string get_place_name(int type,const std::string exclude = "") {
    std::string result = "";
    do {
        if(type==1) {
            list<std::string> wldr_n = {"a clearing in the woods","a cave in a hill","a hut in the woods","a burrow in the hills"};
            result = wldr_n.rand();
        } else if(type<4) {
            std::string town_n =  "|, ,Little |Greater ||||||";
            if(randi(0,1)==1) {
                town_n.append(
                    "|South |Lower |Upper |East |North| West ,"
                    "Car|Ba|Ha|No|De,"
                    "ton|rister|decel|lis");
            } else {
                town_n.append(
                    ","
                    "Red|Green|Blue|White|Black|North|South|East|West|Big|Small|Old|New|Long|Tall|High|Low|Short|Thorn|Stone|Wood|Dirt,"
                    "river|hill|glade|stream|rest|tree|wall|field|burrow");
            }
            result = name::randsgen({town_n});
        } else if(type==4) {
            //Can add indivdual districts and areas, like Applehill, or East Hearthstead for Neuin
            list<std::string> cty_n = {"Neuin","Redclaw","Greenlake"};
            result = cty_n.rand();
        }
    }
    while(result==exclude);
    return result;
}

void life::simulate_life_step(g_ptr<person> node,g_ptr<person> parent) {
g_ptr<person> spouse = node->spouse;
g_ptr<person> o_parent = nullptr;
list<g_ptr<person>> had_children;
if(parent) o_parent = parent->spouse;
if(!dead) {
    age++; //Step age, stops if dead but maybe shouldn't?
    if(age<=1) { //Birth years

        name = name::randsgen({name::AVAL_CENTRAL_FIRST},1);
        node->setName(name);

        intel = weighted_dist({{3,1},{8,2},{20,3},{45,4},{75,5},{92,6},{98,7},{100,8}});

        if(!parent) {

            surname = name::randsgen({name::AVAL_CENTRAL_LAST},1);

            origin = weighted_dist({{10,1},{50,2},{75,3},{100,4}});
            lives = origin;
            wealth = weighted_dist(origin_wealth_dist[origin-1]);
            magic = weighted_dist({{93,0},{97,1},{99,2},{100,3}});

            origin_name = get_place_name(origin);
        } else {

            surname = parent->l.surname;

            origin = parent->l.lives;
            lives = parent->l.lives;
            wealth = parent->l.wealth;
            magic = parent->l.magic;
            if(parent->l.magic>0) { //This is complex and will have to be expanded later, it includes species, and manifestation
                int parent_magic = parent->l.magic;
                int o_parent_magic = 0;
                if(o_parent&&o_parent->l.magic>0) {
                    o_parent_magic = o_parent->l.magic;
                }
                magic = std::max(parent_magic,o_parent_magic);
                if(magic>1&&magic<3) magic+=randi(-1,1); //Unrealistic fluctuatio
            }
            origin_name = parent->l.lives_name;
        }
        origin_info = "\n"+name+" was born in "+origin_name;
        lives_name = origin_name;
    }
    if(age<=5) { //Baby and toddler years
        float mortality_rate = (10.0f - lives * 2.0f) * (9.0f - wealth) / 4.0f;
        mortality_rate = std::max(0.1f, std::min(20.0f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;
    } 
    else if (age<=9) {//Childhood years
        float mortality_rate = (6.0f - lives * 1.0f) * (9.0f - wealth) / 8.0f;
        mortality_rate = std::max(0.05f, std::min(5.0f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;
    } 
    else if (age<=21) { //Teenage/Academy years

        float mortality_rate = (3.0f - lives * 0.5f) * (9.0f - wealth) / 20.0f;
        mortality_rate = std::max(0.02f, std::min(1.0f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;

        // if(education==-1) {
        //     int base_detection_chance = lives * 20; // 20%, 40%, 60%, 80% base
        //     if(age+magic>=13&&age<15) { //To track with manifestation ages, it only becomes detectable at some points
        //         base_detection_chance += 30 + (magic * 10); // +40%, +50%, +60% if talented
        //         base_detection_chance = std::min(95, base_detection_chance);
        //         if(parent&&parent->l.magic > 0) { //Ensure the parent also recived a magical education for this too once we add that value!!!!
        //             base_detection_chance = 100;
        //         }
        //     } else {
        //         base_detection_chance = 0;
        //     }
        //     bool detected_magic = randi(1,100)<=base_detection_chance;


        //     int education_quality = 0;
        //     education_quality += lives * 20; // 20, 40, 60, 80 (higher base)
        //     if(wealth >= 4) { // Middle class and above
        //         education_quality += (wealth - 3) * 5; // +5, +10, +15, +20, +25
        //     } else { // Below middle class
        //         education_quality -= (4 - wealth) * 3; // -9, -6, -3 (less harsh penalty)
        //     }

        //     if(parent && parent->l.education > 0) {
        //         education_quality += parent->l.education * 10; // +10 to +40 boost
        //     }

        //     education_quality = std::max(10, std::min(95, education_quality));
            
        //     int base_access_chance = 40 + (education_quality * 0.8); // 48% to 116% range
        //     base_access_chance = std::min(95, base_access_chance);
        //     bool received_education = randi(1,100) <= base_access_chance;

        //     education = 0;
        //     if(detected_magic) {
        //         education = 4;
        //     } else if(received_education) {
        //         education = weighted_dist({
        //             {education_quality/4, 1},      // Basic literacy
        //             {education_quality/2, 2},      // Standard education  
        //             {education_quality*3/4, 3},    // Advanced education
        //             {education_quality, 4}        // Elite education (academy-level)
        //         });
        //     }
        //     if(education>0) {
        //         enrolled = age;
        //         education_info = "\nAt the age of "+std::to_string(age)+" "+name;
        //         if(education==1) education_info.append(" learned to read");
        //         else if(education < 4) education_info.append(" was enrolled in a "+note_to_string("Education",education)+" level school");
        //         else if(education == 4) {
        //             std::string academy_name = "The GRA";
        //             if(detected_magic)
        //                 education_info.append(" was discovered and enrolled at "+academy_name);
        //             else
        //                 education_info.append(" was accepted into "+academy_name);
        //         }
        //     }
        //     education_info.append("\nQuality: "+tstr(education_quality));
        //     education_info.append("\nChance: "+tstr(base_access_chance));
        // } else if(magic!=0&&education!=4) {
        //     int education_level = education;
        //     int base_detection_chance = education_level * 20;
        //     base_detection_chance+=magic*20;
        //     bool was_detected = randi(1,100) <= base_detection_chance;
        //     if(was_detected) {
        //         education_level = 4;
        //         reenrolled = age;
        //         std::string academy_name = "The GRA";
        //         education_info = "\nAt the age of "+std::to_string(age)+" "+name+
        //         "was discovered by "+academy_name+" and enrolled";
        //     }
        // } 
        // if(education>=1&&ended==-1) {
        //     //Progress in school and potential to drop out or graduate earlier
        //     if(randi(1,100)<=5 || age>=20) { //Random dropout chance
        //         education_info.append("\nAt the age of "+std::to_string(age)+" "+name+" finished their education ");
        //         ended = age;
        //     }
        // }
    } 
    else if (age<=50) { //Adult years
        float mortality_rate = (2.0f - lives * 0.3f) * (9.0f - wealth) / 30.0f;
        mortality_rate = std::max(0.01f, std::min(0.5f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;

        if(education>lives) {
            lives=lives+1;
            lives_name = get_place_name(lives);
            lives_info = "\nSeeking better opportunities, "+name+" moved to the "+note_to_string("Origin",lives)+" of "+lives_name;
        } 

        if(!spouse&&!node->isSpouse) {
            if(randi(1,100)<=30) { //Make a marriage chance formula
                node->spouse = make<person>();
                node->spouse->isSpouse = true;
                for(int i = 0;i<(age+randi(-5,5));i++) {
                    node->spouse->step_life(); //Add specific spouse requirments
                }
                spouse = node->spouse;
                if(!spouse->l.dead) {
                    general_info = "\nAt the age of "+tstr(age)+" "+name+" married "+node->spouse->l.name;
                    spouse->l.general_info = "\nAt the age of "+tstr(spouse->l.age)+" "+spouse->l.name+" married "+name;
                    if(randi(0,6)==1) {
                        maiden_name = surname;
                        general_info.append("\nand changed their surname from "+surname+" to "+spouse->l.surname);
                        surname = spouse->l.surname;
                    } else {
                        spouse->l.general_info.append("\nand changed their surname from "+spouse->l.surname+" to "+surname);
                        spouse->l.maiden_name = spouse->l.surname;
                        spouse->l.surname = surname;
                    }
                } else {
                    node->spouse = nullptr;
                }
            }
        } else if(spouse) {
            float age_factor = 0.0f;
            if(age >= 22 && age <= 28) {
                age_factor = 1.0f; // Peak years
            } else if(age >= 29 && age <= 35) {
                age_factor = 0.8f; // Still good
            } else if(age >= 36 && age <= 42) {
                age_factor = 0.4f; // Declining
            } else if(age >= 43 && age <= 50) {
                age_factor = 0.1f; // Very low
            }
            float wealth_factor = 0.3f + (wealth * 0.1f); // 0.4 to 1.1
            float children_factor = 1.0f / (1.0f + node->children.length() * 0.4f);
            float birth_chance = 15.0f * age_factor * wealth_factor * children_factor;
            birth_chance = std::max(0.0f, std::min(25.0f, birth_chance));
            if(randi(1,100)<=birth_chance) { //Take into account species later
                auto child = make<person>();
                node->children << child;
                had_children << child;
            }
        }
    } 
    else if (age<=130) { //Elderly years
        float age_factor = std::pow((age - 50) / 80.0f, 2.5f); // Accelerating after 50
        float base_rate = 2.0f + age_factor * 15.0f; // 2% base rising to ~17%
        float mortality_rate = base_rate * (8.0f - lives) * (9.0f - wealth) / 32.0f;
        mortality_rate = std::max(0.5f, std::min(50.0f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;
    }

    if(education < 1) {
        int base_chance = 0;
        int taught_by = 0; // 0=School/Self, 1=Parent, 2=O_parent, 3=Both, 4=Tutor
        bool is_formal_education = (lives > 2 && age > 9 && wealth > 1);

        if(age >= 5) {
            if(parent && parent->l.education > 0 && parent->l.intel>5) {
                base_chance += parent->l.education * 5;
                taught_by = 1;
            }
            if(o_parent && o_parent->l.education > 0 && o_parent->l.intel>5) {
                base_chance += o_parent->l.education * 5;
                taught_by = (taught_by == 1) ? 3 : 2;
            }
        }
        
        if(is_formal_education) {
            // Formal education path
            base_chance = 80 + (lives - 2) * 5;
            if(wealth > 3) base_chance = 100;
            if(wealth > 5) taught_by = 4;
        } else {
            // Self-taught/family path  
            base_chance = (lives - 1) * 2;
            base_chance += (intel - 6) * 5;
            base_chance += (age-6) * 5;
        }
         
        // Apply education if successful
        if(randi(1,100) <= base_chance) {
            education = 1;
            education_info = "\nAt the age of "+std::to_string(age)+" "+name+" was taught to read ";
            if(taught_by==0) {
                if(!is_formal_education) {
                    if(intel>=7) education_info.append("by analyzing anything they could find");
                    else if(lives==1) education_info.append("by a traveling mage");
                    else if(lives==2) education_info.append("by a former teacher");
                    else if(lives==3) education_info.append("by the librarian");
                    else if(lives==4) education_info.append("by a community program");
                }
                else
                    education_info.append("at a grammer school");
            }
            else if(taught_by==1) education_info.append("by "+parent->l.name);
            else if(taught_by==2) education_info.append("by "+o_parent->l.name);
            else if(taught_by==3) education_info.append("by both parents");
            else if (taught_by==4) education_info.append("by a tutor");
        }
    }
    else if(education==1) {

    }
}
else if(died==-1) {
    died = age;
}
    if(node->spouse) {
        node->spouse->step_life();
    }

    if(!node->children.empty()) {
        for(auto c : node->children) {
            c->step_life(node);
        }
        if(!had_children.empty()) {
            general_info.append("\nAt the age of "+tstr(age)+" "+name+" had ");
            for(auto c : had_children) {
                general_info.append(c->l.name+(c==had_children.last()?"":", "));
            }
        }
    }
}


int max_depth = 130; //years to simulate
static int spouse_gap = 50;
static int child_gap = 100;

float calculateChildrenWidth(g_ptr<person> root); 

float calculateSubtreeWidth(g_ptr<person> root) {
    //If a leaf, just return the width
    if(root->children.empty()) {
        float width = root->width();
        if(root->spouse) width += root->spouse->width() + spouse_gap;
        return width;
    }
    float totalWidth = calculateChildrenWidth(root);
    //Find the width of this node
    float nodeWidth = root->width();
    if(root->spouse) nodeWidth += root->spouse->width() + spouse_gap;
    
    //Then return whichever is biggest
    return std::max(nodeWidth, totalWidth);
}

float calculateChildrenWidth(g_ptr<person> root) {
    //Find the width of all children
    float totalWidth = 0;
    for(auto c : root->children) {
        totalWidth += calculateSubtreeWidth(c);
    } //Add the gaps
    totalWidth += (root->children.length() - 1) * child_gap;
    return totalWidth;
}

void arrange(g_ptr<person> root, vec2 rootPos = vec2(640, 100)) {
    root->txt->setCenter(rootPos);
    if(root->spouse) { //Gap between spouse and root
        root->spouse->txt->setCenter(rootPos + vec2(root->width() + spouse_gap, 0));
    }
    
    //Drawing the line between root and spouse
    if(root->spouse) {
        auto l2 = makeLine();
        l2->setPosition(root->right());
        updateLine(l2,root->spouse->left());
        root->lines << l2;
    }
  
    vec2 parentCenter = rootPos;
    if(root->spouse) { //Setting the 'center' to be between the spouse
        float leftEdge = rootPos.x() - root->width()/2;
        float rightEdge = rootPos.x() + root->width() + spouse_gap + root->spouse->width()/2;
        parentCenter.setX((leftEdge + rightEdge) / 2);
    }
    
    float startX = parentCenter.x() - (calculateChildrenWidth(root) / 2);
    float currentX = startX;

    for(auto c : root->children) {
        float childSubtreeWidth = calculateSubtreeWidth(c);
        vec2 childPos = vec2(currentX + childSubtreeWidth/2, rootPos.y() + 120);
        
        c->txt->setCenter(childPos);
        if(c->spouse) { //Set child and spouse position
            c->spouse->txt->setCenter(childPos + vec2(c->width() + spouse_gap, 0));
        }
        
        auto l = makeLine(); //Drawing the line to the child
        if(root->spouse) {
            l->setPosition(vec2((root->right().x() + root->spouse->left().x()) / 2,rootPos.y()+25));
        } else
            l->setPosition(rootPos+vec2(0,25));
        updateLine(l, childPos + vec2(0, -25));
        root->lines << l;
    
        arrange(c, childPos);
        
        currentX += childSubtreeWidth + child_gap;
    }
}

void populate(g_ptr<person> root,int depth = 0) {
    if(depth>max_depth) return;
    root->step_life();
    populate(root,++depth);

    // if(randi(0,1)==1||depth==0) {
    //     root->spouse = make<person>();
    //     root->spouse->makeName(root);
    //     for(int i = 0;i<randi(0,4);i++) {
    //     auto c = root->addChild();
    //     populate(c,depth++);
    //     }
    // }
}

int main()  {
    using namespace helper;

    std::string MROOT = "../Projects/StudentSystem/assets/models/";
    std::string IROOT = "../Projects/StudentSystem/assets/images/";

    vec2 win(2560,1536);
    Window window = Window(win.x()/2, win.y()/2, "FamilyTree 0.1");
    scene = make<Scene>(window,2);
    Data d = make_config(scene,K);
    scene->tickEnvironment(0);
    font = make<Font>("../Engine/assets/fonts/source_code.ttf",50);
    info_card = text::makeText("",font,scene,vec2(50,40),1);
    info_card->flagOn("fix_to_screen");

    //To scroll the info card
    thumb = make<Quad>();
    scene->add(thumb);
    thumb->setColor(Color::WHITE);
    thumb->scale(vec2(40,40));
    thumb->flagOn("fix_to_screen");
    thumb->flagOn("draggable");

    auto root = make<person>();
    populate(root);
    while(root->children.length()<3) {
        root = make<person>();
        populate(root);
    }
    max_depth = 1200;
    populate(root);
    root->realize();
    arrange(root);
    info_card->set<g_ptr<person>>("person",root);
    g_ptr<Quad> sel = nullptr;
    int clicked_at = -1;
    list<g_ptr<Quad>> infos;
    S_Tool s_tool;
    //s_tool.log_fps = true;
    start::run(window,d,[&]{
        if(pressed(ESCAPE)) {
            sel = nullptr;
            clicked_at = -1;
            text::setText("",info_card);
            info_card->setPosition(vec2(50,40));
        }

        if(pressed(MOUSE_LEFT)) {
            if(sel) {
                sel = nullptr;
                clicked_at = -1;
            }
            sel = scene->nearestWithin(20.0f);
            if(sel) {
                if(sel->has("char")) {
                    clicked_at = sel->get<size_t>("pID");
                    sel = text::parent_of(sel);
                    if(sel==info_card) {
                        g_ptr<person> p = sel->get<g_ptr<person>>("person");
                        std::string nstr = text::string_of(sel);
                        int l_line = nstr.find('\n',clicked_at);
                        std::string entry = "";
                        if(l_line!=std::string::npos) {
                            if(nstr.at(l_line-1)=='>') {
                                g_ptr<Quad> sent = text::char_at(l_line,sel);
                                if(sent->has("_added")) {
                                    text::removeText(l_line-1,sent->get<int>("_added")+1,sel);
                                }
                            } else {
                                g_ptr<Quad> sent = text::char_at(nstr.rfind('\n',clicked_at)+1,sel);
                                if(!sent->has("info")) sent = text::char_at(nstr.rfind('\n',clicked_at)+1,sel);
                                if(sent->has("info")) { 
                                    entry = " >"+sent->get<std::string>("info");
                                    if(text::char_at(l_line,sel)->get<char>("char")==' ') { text::removeChar(l_line,sel); }
                                    else { text::insertChar(l_line+1,' ',sel); }
                                    text::insertText(l_line+1,entry,sel);
                                }                          
                            }
                            if(entry!="") {
                                text::char_at(nstr.find('\n',clicked_at)+2,sel)->set<int>("_added",entry.length());
                            }
                        }
                    }
                }
                if(sel->has("person")&&sel!=info_card) {
                    g_ptr<person> p = sel->get<g_ptr<person>>("person");
                    life_info(p->l);
                    info_card->set<g_ptr<person>>("person",p);
                }
            }
        }

        if(sel&&sel->has("person")&&sel!=info_card) {
            g_ptr<person> p = sel->get<g_ptr<person>>("person");
            if(pressed(NUM_0)) {
                p->show();
            } 
            int num = -1;
            if(pressed(NUM_1)) num = 0;
            if(pressed(NUM_2)) num = 1;
            if(pressed(NUM_3)) num = 2;
            if(pressed(NUM_4)) num = 3;
            if(pressed(NUM_5)) num = 4;
            if(pressed(NUM_6)) num = 5;
            if(!p->children.empty()&&num!=-1) {
                if(num<p->children.length()) {
                    if(!p->children[num]->txt->culled())
                        p->children[num]->hide();
                    else
                        p->children[num]->show();
                }
            }
        }

        if(held(MOUSE_LEFT)) {
            if(sel&&sel->has("draggable")) {
                sel->setCenter(scene->mousePos2d().setX(20));
            }
        }

        if(pressed(G)) {
            if(sel->has("person")) {
                auto p = sel->get<g_ptr<person>>("person");
                populate(p);
                arrange(p,sel->getCenter());
            }
        }

        vec2 move2d = input_move_2d_keys(8.0f)*-1;
        if(move2d.length()!=0) {
            root->track(move2d);
            for(int i=0;i<scene->quads.length();i++) {
                auto q = scene->quads[i];
                if(!q->parent&&!q->culled()) {
                    if(!q->check("fix_to_screen")) {
                        // vec2 p = q->getPosition();
                        // if(p.x()>win.x()) 
                        q->move(move2d);
                    }
                }
            }
        }
       s_tool.tick();
    });

    return 0;
}