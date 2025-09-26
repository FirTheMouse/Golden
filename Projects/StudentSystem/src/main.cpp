#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<util/color.hpp>
#include<util/string_generator.hpp>

using namespace Golden;

g_ptr<Scene> scene;
g_ptr<Font> font;

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

class person : public Object {
public:
    std::string name = "noname";

    g_ptr<person> spouse;
    list<g_ptr<person>> children;


    list<g_ptr<Quad>> lines;
    g_ptr<Quad> txt;


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

    void makeName(g_ptr<person> parent = nullptr) {
        if(!parent) { //If we're creating a root
            setName(name::randsgen({name::AVAL_CENTRAL_FIRST},1));
            txt->set<std::string>("surname",name::randsgen({name::AVAL_CENTRAL_LAST},1));
        } else {
            if(parent->spouse) { //If there is a spouse
                if(parent->spouse == this) { //If this is the spouse
                    setName(name::randsgen({name::AVAL_CENTRAL_FIRST},1));
                    note("surname",name::randsgen({name::AVAL_CENTRAL_LAST},1));
                    if(randi(0,3)==1) {
                        parent->note("Maiden name",parent->retrive("surname"));
                        parent->note("surname",retrive("surname"));
                    } else {
                        note("Maiden name",retrive("surname"));
                        note("surname",parent->retrive("surname"));
                    }
                } else { //If there's a spouse
                    auto o_parent = parent->spouse;
                    setName(name::randsgen({name::AVAL_CENTRAL_FIRST},1));
                    note("surname",parent->retrive("surname"));
                }
            } else { //If there's no spouse
                setName(name::randsgen({name::AVAL_CENTRAL_FIRST},1));
                note("surname",parent->retrive("surname"));
            }
        }
    }

    g_ptr<person> addChild() {
        if(!spouse) {
            print("UNMARRIED CAN'T HAVE CHILDREN - add single parents later");
            return nullptr;
        }
        if(!spouse->txt) {
            spouse->makeName();
        }
        auto child = make<person>();
        child->makeName(this);
        children << child;
        return child;
    }
};

static int max_depth = 4;
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
    if(randi(0,1)==1||depth==0) {
        root->spouse = make<person>();
        root->spouse->makeName(root);
        for(int i = 0;i<randi(0,4);i++) {
        auto c = root->addChild();
        populate(c,depth++);
        }
    }
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
       auto r = text::makeText(label[i]+": "+value[i],font,scene,at.addY((i+1)*40),1);
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

    auto root = make<person>();
    root->makeName();
    populate(root);
    arrange(root);
    g_ptr<Quad> sel = nullptr;
    list<g_ptr<Quad>> infos;
    start::run(window,d,[&]{
        if(pressed(MOUSE_LEFT)) {
            if(sel) {
                sel = nullptr;
                for(auto i : infos) {
                    scene->deactivate(i);
                }
            }
            sel = scene->nearestElement();
            if(sel) {
                if(sel->has("char")) sel = text::parent_of(sel);
                infos = create_info_card(sel);
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
            if(q->check("valid"))
                q->move(move2d);
        }
       
    });

    return 0;
}