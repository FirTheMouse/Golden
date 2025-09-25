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

    vec2 center() {
        return text::center_of(txt);
    }

    float width() {
        int len = text::string_of(txt).length();
        float wid = 25;
        return len*wid;
    }


    vec2 right() {
        return center().addX(width()/2);
    }

    vec2 left() {
        return center().addX(-width()/2);
    }

    void setName(const std::string& n) {
        name = n;
        if(txt) {
            text::setText(name,txt);
        } else
            txt = text::makeText(name,font,scene,vec2(1250,40),1);
    }

    void makeName() {
        setName(name::randsgen({name::STANDARD},1));
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
        child->makeName();
        children << child;
        return child;
        // for(int i = 0;i<children.length();i++) {
        //     auto l = makeLine();
        //     l->setPosition(getPos().addY(-25));
        //     lines << l;
        // }
    }
};

static int max_depth = 4;
float calculateSubtreeWidth(g_ptr<person> node) {
    if(node->children.empty()) {
        float width = node->width();
        if(node->spouse) width += node->spouse->width() + 50;
        return width;
    }
    
    float childrenWidth = 0;
    for(auto child : node->children) {
        childrenWidth += calculateSubtreeWidth(child);
    }
    childrenWidth += (node->children.length() - 1) * 100;
    
    float nodeWidth = node->width();
    if(node->spouse) nodeWidth += node->spouse->width() + 50;
    
    return std::max(nodeWidth, childrenWidth);
}

void arrange(g_ptr<person> root, vec2 rootPos = vec2(640, 100)) {
    root->txt->setCenter(rootPos);
    if(root->spouse) {
        root->spouse->txt->setCenter(rootPos + vec2(root->width() + 50, 0));
    }
    
    if(root->children.empty()) return;
    float totalChildrenWidth = 0;
    for(auto child : root->children) {
        totalChildrenWidth += calculateSubtreeWidth(child);
    }
    totalChildrenWidth += (root->children.length() - 1) * 100; // gaps
    
    vec2 parentCenter = rootPos;
    if(root->spouse) {
        float leftEdge = rootPos.x() - root->width()/2;
        float rightEdge = rootPos.x() + root->width() + 50 + root->spouse->width()/2;
        parentCenter.setX((leftEdge + rightEdge) / 2);
    }
    
    float startX = parentCenter.x() - (totalChildrenWidth / 2);
    float currentX = startX;
    for(auto c : root->children) {
        float childSubtreeWidth = calculateSubtreeWidth(c);
        vec2 childPos = vec2(currentX + childSubtreeWidth/2, rootPos.y() + 120);
        
        c->txt->setCenter(childPos);
        if(c->spouse) {
            c->spouse->txt->setCenter(childPos + vec2(c->width() + 50, 0));
        }
        
        auto l = makeLine();
        if(root->spouse) {
            l->setPosition(vec2((root->right().x() + root->spouse->left().x()) / 2,rootPos.y()));
        } else
            l->setPosition(rootPos+vec2(0,25));
        updateLine(l, childPos + vec2(0, -25));

        if(c->spouse) {
            auto l2 = makeLine();
            l2->setPosition(c->right());
            updateLine(l2,c->spouse->left());
        }
    
        arrange(c, childPos);
        
        currentX += childSubtreeWidth + 100;
    }
}

void populate(g_ptr<person> root,int depth = 0) {
    if(depth>max_depth) return;
    if(randi(0,1)==1||depth==0) {
        root->spouse = make<person>();
        root->spouse->makeName();
        for(int i = 0;i<randi(0,3);i++) {
        auto c = root->addChild();
        populate(c,depth++);
        }
    }
}

void create_info_card(g_ptr<Quad> root) {
    list<std::string> label;
    list<std::string> value;
    for(auto e : root->data.notes.entrySet()) {
        try {
            std::string nstr = *std::any_cast<std::string>(&e.value);
            label << e.key;
            value << nstr;
        } catch (std::exception e) {}
    }
    for(int i =0;i<label.length();i++) {
       print(label[i],": ",value[i]);
    }
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
    create_info_card(text::parent_of(root->txt));
    g_ptr<Quad> sel = nullptr;
    start::run(window,d,[&]{
        if(pressed(MOUSE_LEFT)) {
            if(sel) sel = nullptr;
            else sel = scene->nearestElement();
        }
        if(sel)
            sel->setCenter(scene->mousePos2d());

        vec2 move2d = input_move_2d_keys(8.0f)*-1;
        for(int i=0;i<scene->quads.length();i++) {
            auto q = scene->quads[i];
                q->move(move2d);
        }
       
    });

    return 0;
}