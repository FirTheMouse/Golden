#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<util/color.hpp>
#include<util/string_generator.hpp>

using namespace Golden;

g_ptr<Scene> scene;
g_ptr<Font> font;
g_ptr<Quad> info_card;

std::string tstr(int i) {
    return std::to_string(i);
}

g_ptr<Quad> makeLine() {
    auto g = make<Quad>();
    scene->add(g);
    g->scale(vec2(5,5));
    g->setColor(Color::WHITE);
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
    int died = 0;

    std::string name;
    std::string surname;
    std::string maiden_name;

    int age = 0;
    int gender = 0; //Add this later
    int species = 0; //Add this later

    int origin = -1;//Size of where they grew up: 1=Wilderness,2=Village,3=Town,4=City
    int lives = -1;
    int wealth = -1; //Wealth: 1=Impovrished,2=Poor,3=Lower-middle,4=Middle,5=Upper-middle,6=Wealthy,7=Incredibly Wealthy,8=Ultra rich
    int magic = -1;
    int education = -1;
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
        if(txt) {
            text::setText(name,txt);
        } else
            txt = text::makeText(name,font,scene,vec2(1250,40),1);
        txt->set<std::string>("name",name);
        txt->set<g_ptr<person>>("person",this);
    }

    void step_life(g_ptr<person> parent = nullptr) {
        l.simulate_life_step(this,parent);
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

void life_info(const life& l) { 
    std::string history = 
    l.name+" "+l.surname+"\n"
    "------------------------------";

    if(l.maiden_name!="") {
        history.append(
            "\nMaiden name: "+l.maiden_name
        );
    }
    history.append(
        "\nOrigin: "+note_to_string("Origin",l.origin)+
        "\nWealth: "+note_to_string("Wealth",l.wealth)+
        "\nMagic: "+note_to_string("Magical",l.magic)
    );

    if(l.education>0) {
        history.append(
            "\nEducation: "+note_to_string("Education",l.education)+
            "\nEnrolled: "+tstr(l.enrolled)+" to "+tstr(l.ended)
        );
        if(l.reenrolled>0) {
            history.append(
                "\nRe-enrolled: "+tstr(l.reenrolled)
            );
        }
    }

    history.append(
        "\nLives: "+note_to_string("Origin",l.origin)+
        "\nDied: "+tstr(l.died)
    );

    text::setText(history,info_card);
    info_card->setPosition(vec2(1900,40));
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

void life::simulate_life_step(g_ptr<person> node,g_ptr<person> parent) {
g_ptr<person> spouse = node->spouse;
g_ptr<person> o_parent = nullptr;
if(parent) o_parent = parent->spouse;
if(!dead) {
    age++; //Step age, stops if dead but maybe shouldn't?
    if(age<=1) { //Birth years

        name = name::randsgen({name::AVAL_CENTRAL_FIRST},1);
        node->setName(name);

        if(!parent) {

            surname = name::randsgen({name::AVAL_CENTRAL_LAST},1);

            origin = weighted_dist({{10,1},{50,2},{75,3},{100,4}});
            lives = origin;
            wealth = weighted_dist(origin_wealth_dist[origin-1]);
            magic = weighted_dist({{93,0},{97,1},{99,2},{100,3}});
        } else {

            surname = parent->retrive("surname");

            origin = parent->l.lives;
            lives = parent->l.lives;
            wealth = parent->l.wealth;
            magic = parent->l.magic;
            if(parent->l.magic>0) { //This is complex and will have to be expanded later, it includes species, and manifestation
                if(o_parent&&o_parent->l.magic>0) {
                    int parent_magic = parent->l.magic;
                    int o_parent_magic = o_parent->l.magic;
                    magic = std::max(parent_magic,o_parent_magic);
                    if(magic>1&&magic<3) magic+=randi(-1,1); //Unrealistic fluctuation
                }
            }
        }

        node->note("Origin", note_to_string("Origin", lives));
        node->note("Lives", note_to_string("Origin", lives));
        node->note("Wealth", note_to_string("Wealth", wealth));
        node->note("Magical", note_to_string("Magical", magic));
        node->note("surname",surname);
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

        if(education==-1) {
            int base_detection_chance = lives * 20; // 20%, 40%, 60%, 80% base
            if(age+magic>=13&&age<15) { //To track with manifestation ages, it only becomes detectable at some points
                base_detection_chance += 30 + (magic * 10); // +40%, +50%, +60% if talented
                base_detection_chance = std::min(95, base_detection_chance);
                if(parent&&parent->l.magic > 0) { //Ensure the parent also recived a magical education for this too once we add that value!!!!
                    base_detection_chance = 100;
                }
            } else {
                base_detection_chance = 0;
            }
            bool detected_magic = randi(1,100)<=base_detection_chance;


            int education_quality = 0;
            education_quality += lives * 20; // 20, 40, 60, 80 (higher base)
            if(wealth >= 4) { // Middle class and above
                education_quality += (wealth - 3) * 5; // +5, +10, +15, +20, +25
            } else { // Below middle class
                education_quality -= (4 - wealth) * 3; // -9, -6, -3 (less harsh penalty)
            }

            if(parent && parent->l.education > 0) {
                education_quality += parent->l.education * 10; // +10 to +40 boost
            }

            education_quality = std::max(10, std::min(95, education_quality));
            
            int base_access_chance = 40 + (education_quality * 0.8); // 48% to 116% range
            base_access_chance = std::min(95, base_access_chance);
            bool received_education = randi(1,100) <= base_access_chance;

            education = 0;
            if(detected_magic) {
                education = 4;
            } else if(received_education) {
                education = weighted_dist({
                    {education_quality/4, 1},      // Basic literacy
                    {education_quality/2, 2},      // Standard education  
                    {education_quality*3/4, 3},    // Advanced education
                    {education_quality, 4}        // Elite education (academy-level)
                });
            }

            node->note("Education", note_to_string("Education", education));
            if(education>0) {
                enrolled = age;
                node->note("Enrolled", std::to_string(age));
            }
        } else if(magic!=0&&education!=4) {
            int education_level = education;
            int base_detection_chance = education_level * 20;
            base_detection_chance+=magic*20;
            bool was_detected = randi(1,100) <= base_detection_chance;
            if(was_detected) {
                education_level = 4;
                reenrolled = age;
                node->note("Re-enrolled", std::to_string(age));
            }
            node->note("Education", note_to_string("Education", education_level));
        } 
        if(education>=1&&ended==-1) {
            //Progress in school and potential to drop out or graduate earlier
            if(randi(1,100)<=5 || age>=20) { //Random dropout chance
                node->note("Enrolled",node->retrive("Enrolled")+" to "+std::to_string(age));
                ended = age;
            }
        }
    } 
    else if (age<=50) { //Adult years
        float mortality_rate = (2.0f - lives * 0.3f) * (9.0f - wealth) / 30.0f;
        mortality_rate = std::max(0.01f, std::min(0.5f, mortality_rate));
        dead = randf(0,100) <= mortality_rate;

        if(education>lives) {
            lives=lives+1;
            node->note("Lives",note_to_string("Origin",lives));
        } 

        if(!spouse&&!node->isSpouse) {
            if(randi(1,100)<=30) { //Make a marriage chance formula
                node->spouse = make<person>();
                node->spouse->isSpouse = true;
                for(int i = 0;i<(age+randi(-5,5));i++) {
                    node->spouse->step_life(); //Add specific spouse requirments
                }
                node->spouse->l.maiden_name = node->spouse->l.surname;
                node->spouse->l.surname = surname;
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
}
else if(!node->txt->has("dead")) {
    died = age;
    node->note("Died",std::to_string(age));
}
    if(node->spouse) {
        node->spouse->step_life();
    }

    if(!node->children.empty()) {
        for(auto c : node->children) {
            c->step_life(node);
        }
    }
}


static int max_depth = 200; //years to simulate
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


list<g_ptr<Quad>> create_info_card(g_ptr<Quad> root) {
    list<g_ptr<Quad>> infos;
    if(!root->has("name")) return infos;
    list<std::string> label;
    list<std::string> value;
    for(auto e : root->data.notes.entrySet()) {
        try {
            if (auto* nstr = std::any_cast<std::string>(&e.value)) {
                if(e.key=="name"||e.key=="surname") continue;
                label << e.key;
                value << *nstr;
            }
        } catch (std::exception e) {}
    }

    vec2 at(1900,40);
    auto g = make<Quad>();
    scene->add(g);
    g->scale(vec2(600,(label.length()+2)*40));
    g->setColor(Color::RED);
    g->setPosition(at+vec2(-10,-30));
    g->flagOff("valid");
    infos << g;

    auto ll = text::makeText(root->get<std::string>("name")+" "+root->get<std::string>("surname"),font,scene,at,1);
    ll->flagOff("valid");
    for(auto c : ll->children) c->flagOff("valid");
    infos << ll;

    auto lll = text::makeText("----------------",font,scene,at.addY(40),1);
    lll->flagOff("valid");
    for(auto c : lll->children) c->flagOff("valid");
    infos << lll;

    for(int i =0;i<label.length();i++) {
       //print(label[i],": ",value[i]);
       auto r = text::makeText(label[i]+": "+value[i],font,scene,at+vec2(0,(i+1)*40),1);
       r->flagOff("valid");
       for(auto c : r->children) c->flagOff("valid");
       infos << r;
    }
    return infos;
}

int main()  {
    using namespace helper;

    std::string MROOT = "../Projects/StudentSystem/assets/models/";
    std::string IROOT = "../Projects/StudentSystem/assets/images/";

    Window window = Window(1280, 768, "FamilyTree 0.1");
    scene = make<Scene>(window,2);
    Data d = make_config(scene,K);
    scene->tickEnvironment(0);
    font = make<Font>("../Engine/assets/fonts/source_code.ttf",50);
    info_card = text::makeText("",font,scene,vec2(1900,40),1);
    info_card->flagOff("valid");

    // auto g = make<Quad>();
    // scene->add(g);
    // g->setColor(Color::RED);
    // g->scale(vec2(100,100));
    // g->flagOff("valid");

    // auto c = make<Quad>();
    // scene->add(c);
    // c->setColor(Color::BLUE);
    // c->setPosition(vec2(0,400));
    // c->scale(vec2(100,100));

    // g->addChild(c,false);



    auto root = make<person>();
    populate(root);
    arrange(root);
    g_ptr<Quad> sel = nullptr;
    list<g_ptr<Quad>> infos;
    start::run(window,d,[&]{
        if(pressed(MOUSE_LEFT)) {
            if(sel) {
                if(sel==info_card) {
                    text::setText("",info_card);
                    info_card->setPosition(vec2(1900,40));
                }
                sel = nullptr;
            }
            sel = scene->nearestElement();
            if(sel) {
                if(sel->has("char")) sel = text::parent_of(sel);
                if(sel->has("person"))
                    life_info(sel->get<g_ptr<person>>("person")->l);
                else print("NO PERSON FOR SELECTED");
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
        for(int i=0;i<scene->quads.length();i++) {
            auto q = scene->quads[i];
            if(q->check("valid")&&!q->parent)
                q->move(move2d);
        }
       
    });

    return 0;
}