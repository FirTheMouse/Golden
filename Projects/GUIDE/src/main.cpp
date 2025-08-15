#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <rendering/model.hpp>
#include <rendering/single.hpp>
#include <guide/gui_grid.hpp>
#include <guide/gui_cell_object.hpp>
#include <core/scriptable.hpp>
#include <guide/editor_scene.hpp>
#include <extension/simulation.hpp>
#include <fstream>
#include <string>
#include<util/color.hpp>
#include<util/files.hpp>
#include <typeinfo>

using namespace Golden;
using itms = g_ptr<d_list<g_ptr<Quad>>>;
using itmsM = d_list<g_ptr<Quad>>;

glm::vec3 getWorldMousePosition(g_ptr<Scene> scene) {
    Window& window = scene->window;
    Camera& camera = scene->camera;
    int windowWidth = window.width;
    int windowHeight = window.height;
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);

    // Flip y for OpenGL
    float glY = windowHeight - float(ypos);

    // Get view/proj
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix();
    glm::ivec4 viewport(0, 0, windowWidth, windowHeight);

    // Unproject near/far points
    glm::vec3 winNear(float(xpos), glY, 0.0f);   // depth = 0
    glm::vec3 winFar(float(xpos), glY, 1.0f);    // depth = 1
    

    glm::vec3 worldNear = glm::unProject(winNear, view, projection, viewport);
    glm::vec3 worldFar  = glm::unProject(winFar,  view, projection, viewport);

    // Ray from near to far
    glm::vec3 rayOrigin = worldNear;
    glm::vec3 rayDir = glm::normalize(worldFar - worldNear);

     // Intersect with Z = -5 plane (Z-up)
    float targetZ = 0.0f;
    if (fabs(rayDir.z) < 1e-6f) return glm::vec3(0); // Ray is parallel to plane

    float t = (targetZ - rayOrigin.z) / rayDir.z;
    return rayOrigin + rayDir * t;
}

vec2 mousePos2d(g_ptr<Scene> scene)
{
    Window& window = scene->window;
    int windowWidth = window.width;
    int windowHeight = window.height;
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);
    return vec2(xpos*2,ypos*2);
}

inline std::string clean_underscores(const std::string& str) {
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '_'), result.end());
    return result;
}

typedef enum {
    BASE = 0,
} wType;

class GGUI : virtual public Object
{
public:
g_ptr<gui_scene> gui;
Window& window;
Input& input;
Grid& grid;
g_ptr<Quad> indicator;
float idpause = 0.0f;
bool idchange = false;
std::string ROOT = "../Projects/GUIDE/storage/";

    GGUI(g_ptr<gui_scene> _gui,Window& _window,Input& _input,Grid& _grid) : window(_window),
    input(_input), grid(_grid),gui(_gui) {}

    void addClickEffect(g_ptr<Button> button)
    {
        button->addScript("onClick",[button,this](ScriptContext& ctx)
        {
        button->setPosition(button->getPosition().addZ(-button->weight));
        gui->queueEvent(button->deadTime+0.1f,[button]
            {
            button->setPosition(button->getPosition().addZ(button->weight));
            return true;
            });
        });
    }

    void click(g_ptr<G_Object> g) {

        if(g->isActive())
        {
            ScriptContext ctx;
            g->run("onClick",ctx);
            g->valid = false;
            gui->queueEvent(g->deadTime,[g]
            {
            g->valid = true;
            return true;
            });
        }
    }

    void duplicate(g_ptr<G_Object> g) {                
        // g_ptr<G_Object> copy = make<G_Object>();
        // Model* newModel;
        // newModel->copy(*g->model.get());
        // copy->setModel(newModel);
        // gui->add(copy);
        // gui->selectable.push_back(copy);
        //copy->transform(g->getTransform());
        //gui.addClickEffect(copy);}

        g->model->saveBinary(ROOT+"copy.gmdl");
        auto copy = make<G_Object>(make<Model>(ROOT+"copy.gmdl"));
        gui->add(copy);
    }
    void remove(g_ptr<S_Object> g){
        gui->selectable.erase(
        std::remove(gui->selectable.begin(), gui->selectable.end(), g),
        gui->selectable.end()
        );
        g->remove();
        g=nullptr;
    }

    void indicatorSetup(g_ptr<Quad> _indicator) {indicator = _indicator;}
    void tickIndicator(float tpf) {
        if(idpause>=0.0f) idpause-=tpf;
        if(idpause<=0.0f&&idchange)
        {
            text::setText("[      ]",indicator);
            //May need position reseting if drift continues
            idchange = false;
        }
    }
    void indicate(const std::string& text,float time) { 
        text::setText(text,indicator);
        idpause=time;
        idchange = true;
    }

    void set_valid(g_ptr<Quad> g,bool valid)
    {
        if(!g->children.empty())
        {
            g->set<bool>("valid",valid);
            //g->set<bool>("locked",!valid);
            for(auto c : g->children)
            {
                c->set<bool>("valid",valid);
                //c->set<bool>("locked",!valid);
            }
        }
        else if(g->lockToParent)
        {
            if(g->parent)
            {
            for(auto c : g->parent->children)
            {
                c->set<bool>("valid",valid);
                //c->set<bool>("locked",!valid);
            }
            }
        }
        else {
            g->set<bool>("valid",valid);
            //g->set<bool>("locked",!valid);
        } 
    }

    void houseKeepSlots(g_ptr<Quad> item,int i,const std::string& start)
    {
        if(item->check("exporting")) return;
        //Could break if slots are added before and it gets offset...
        //Though that's unlikely considering any slot added before isn't likely to be removed
        //And any slot added after won't impact it
        //Still a possible bug to be aware of though
        std::string exportSlot = start;
        //exportSlot.append(std::to_string(i));
        if(!item->has("export_slot_loc")) 
        {
            item->set<size_t>("export_slot_loc",item->getSlots().length());
            item->getSlots() << exportSlot;
        }
        else
        {
            item->getSlots().get(item->get<size_t>("export_slot_loc"),"ggui::houseKeepSlots::170") 
            = exportSlot;
        }
    }
private:

    void addExportScripts(g_ptr<Quad> g)
    {
        g->addScript("onExport",[g,this](ScriptContext& ctx){

            //Need to clarify the use of these atatchments in the new system
            //Meant to be for duplicate reductions

            //print("exporting ",g->get<std::string>("int_label"));
            auto items = g->get<itms>("items");
            if(!ctx.has("wdgt_num")) print("ggui::onExport::254 no wdgt_num passed from export data");
            int num = ctx.get<int>("wdgt_num");

            g->getSlots().get(g->get<size_t>("export_slot_loc"),"ggui::onExport::257").append("_"+std::to_string(num));
            if(g->has("text"))
            {
            auto text = g->get<g_ptr<Quad>>("text");
            text->getSlots().get(text->get<size_t>("export_slot_loc"),"ggui::onExport::259").append("_"+std::to_string(num));
            }

            // for(auto& item : *items)
            // {
            //     item->flagOn("exporting");
            //     item->getSlots().get(item->get<size_t>("export_slot_loc"),"ggui::onExport::263").append("_"+std::to_string(num));
            //     // print( item->getSlots().get(item->get<size_t>("export_slot_loc"),"ggui::onExport::263"));
            //     if(item->has("text"))
            //     {
            //     auto text = item->get<g_ptr<Quad>>("text");
            //     text->flagOn("exporting");
            //     text->getSlots().get(text->get<size_t>("export_slot_loc"),"ggui::onExport::265").append("_"+std::to_string(num));
            //     //print( text->getSlots().get(text->get<size_t>("export_slot_loc"),"ggui::onExport::265"));
            //     text->flagOff("exporting");
            //     }
            //     item->flagOff("exporting");
            // }
        });

        g->addScript("onExportEnd",[g,this](ScriptContext& ctx){
            auto items = g->get<itms>("items");
            
            auto stripLastUnderscore = [](std::string& slotName) {
                size_t lastUnderscore = slotName.find_last_of('_');
                if (lastUnderscore != std::string::npos) {
                    slotName = slotName.substr(0, lastUnderscore);
                }
            };
            
            std::string& mainSlot = g->getSlots().get(g->get<size_t>("export_slot_loc"), "ggui::onExportEnd::279");
            stripLastUnderscore(mainSlot);
            
            if(g->has("text"))
            {
            auto text = g->get<g_ptr<Quad>>("text");
            std::string& textSlot = text->getSlots().get(text->get<size_t>("export_slot_loc"), "ggui::onExportEnd::283");
            stripLastUnderscore(textSlot);
            }
            
            for(auto item : *items) {
                std::string& itemSlot = item->getSlots().get(item->get<size_t>("export_slot_loc"), "ggui::onExportEnd::287");
                stripLastUnderscore(itemSlot);
                
                if(item->has("text"))
                {
                auto itemText = item->get<g_ptr<Quad>>("text");
                std::string& itemTextSlot = itemText->getSlots().get(itemText->get<size_t>("export_slot_loc"), "ggui::onExportEnd::291");
                stripLastUnderscore(itemTextSlot);
                }
            }
        });
    }


    g_ptr<Quad> base(Data& d)
    {
        auto vl = d.validate(list<std::string>{"body_item","items"});
        if(!vl.empty()) {
            print("ggui::base::243 Missing: ");
            for(auto s : vl) print(s);
        }

        auto g = d.get<g_ptr<Quad>>("body_item");
        g_ptr<Quad> g_txt = nullptr;
        if(d.has("text_item")) g_txt = d.get<g_ptr<Quad>>("text_item");
        std::string label = "none";
        label = d.get<std::string>("label");
        // std::string opens_to = "right";
        // if(g->has("opens_to"))
        //     opens_to = g->get<std::string>("opens_to");
        //label.append("-"+opens_to);
        g->set<std::string>("label",label);
        g->flagOff("click_switch");
        g->flagOff("open");
        g->add<itms>("items",d.get<itms>("items"));

        std::string ext_label = "_wdgt_base_"; //+d.get<std::string>("label");
        g->set<std::string>("ext_label",ext_label);
    
        std::string int_label = label;
        g->set<std::string>("int_label",int_label);

        std::string base_label = label;
        g->set<std::string>("base_label",base_label);

        houseKeepSlots(g,0,ext_label+int_label);

        if(g_txt)
        {
        g->set<g_ptr<Quad>>("text",g_txt);
        set_valid(g_txt,false);
        g_txt->addChild(g,true);
        std::string ext_label_text = "_wdgt_base_"; //+d.get<std::string>("label");
        g_txt->set<std::string>("ext_label",ext_label_text);

        std::string int_label_text = label+"_text";
        g_txt->set<std::string>("int_label",int_label_text);

        std::string base_label_text = label+"_text";
        g_txt->set<std::string>("base_label",base_label_text);

        houseKeepSlots(g_txt,0,ext_label_text+int_label_text);
        }

        g->addScript("onClick",[g,this](ScriptContext& ctx){
            if(!g->check("toggle_off"))
            {
            if(g->toggle("click_switch"))
            {
                g->run("onOpen");
            }
            else
            {
                g->run("onClose");
            }
            }
        });

        g->addScript("onOpen",[g,this](ScriptContext& ctx){
            g->flagOn("open");
            auto items = g->get<itms>("items");
            for(int i=0;i<items->length();i++)
            {
                    auto item = (*items)[i];
                    std::string current_base_label = item->get<std::string>("base_label");
                    if(!item->has("pos_dif")&&item->has("opens_to"))
                    {
                        vec2 pos = g->getPosition();
                        std::string opens_to = "right";
                        if(item->has("opens_to"))
                        { opens_to = item->get<std::string>("opens_to");}
                        if(opens_to=="left") {
                            pos = g->getPosition().addX(-((i+1)*(g->getScale().x()*1.1f)));
                        }
                        else if (opens_to=="right") {
                            pos = g->getPosition().addX(((i+1)*(g->getScale().x()*1.1f)));
                        }
                        else if (opens_to=="top") {
                            pos = g->getPosition().addY(-((i+1)*(g->getScale().y()*1.1f)));
                        }
                        else if (opens_to=="bottom") {
                            pos = g->getPosition().addY(((i+1)*(g->getScale().y()*1.1f)));
                        }
                        item->setPosition(pos); 
                        vec2 posDif = item->getPosition()-g->getPosition();
                        std::string posDifs = posDif.to_string();
                        current_base_label.append("|").append(posDifs);
                        item->set<vec2>("pos_dif",posDif);
                    }

                    if(item->check("locked"))
                    {
                        vec2 posDif = item->getPosition()-g->getPosition();
                        vec2 scaleDif = item->getScale()-g->getScale();
                        std::string posDifs = posDif.to_string();
                        std::string scaleDifs = scaleDif.to_string();
                        current_base_label.append("|").append(posDifs).append("|").append(scaleDifs);
                        item->set<vec2>("pos_dif",posDif);
                        item->set<vec2>("scale_dif",scaleDif);
                    }
                    else
                    {
                        vec2 pos = g->getPosition();
                        vec2 scale = item->getScale();
                        if(item->has("pos_dif")) pos = g->getPosition()+item->get<vec2>("pos_dif");
                        if(item->has("scale_dif")) scale = item->get<vec2>("scale_dif")+g->getScale();
                        item->setPosition(pos);
                        //item->scale(scale);

                        vec2 posDif = item->getPosition()-g->getPosition();
                        vec2 scaleDif = item->getScale()-g->getScale();
                        std::string posDifs = posDif.to_string();
                        std::string scaleDifs = scaleDif.to_string();
                        current_base_label.append("|").append(posDifs).append("|").append(scaleDifs);
                        item->set<vec2>("pos_dif",posDif);
                        item->set<vec2>("scale_dif",scaleDif);
                    }

                    std::string parent_int_label = g->get<std::string>("int_label");
                    item->set<std::string>("int_label", parent_int_label + "_" + current_base_label);
                    
                    if(item->has("text"))
                    {
                        auto i_txt = item->get<g_ptr<Quad>>("text");
                        //Text is just the 3rd to last field, all else is inhereted from the item
                        houseKeepSlots(i_txt,i+1,item->get<std::string>("ext_label")+item->get<std::string>("int_label")+"_text");
                        //print(item->get<std::string>("ext_label")+item->get<std::string>("int_label")+"_text");
                    }
                    houseKeepSlots(item,i+1,item->get<std::string>("ext_label")+item->get<std::string>("int_label"));

                    item->run("onActivate");
            }
        });

        g->addScript("onClose",[g,this](ScriptContext& ctx){
            g->flagOff("open");
            auto items = g->get<itms>("items");
            for(int i=0;i<items->length();i++)
            {
                auto item = (*items)[i];
                item->run("onDeactivate");
            }
        });

        g->addScript("onActivate",[g,this](ScriptContext& ctx){
            if(!g->isActive()&&!g->check("dead")) {
                gui->reactivate(g);
            }
        });

        g->addScript("onDeactivate",[g,this](ScriptContext& ctx){
            if(g->isActive()) gui->deactivate(g);
            g->run("onClose");
            auto items = g->get<itms>("items");
            for(int i=0;i<items->length();i++)
            {
                    auto item = (*items)[i];
                    item->run("onDeactivate");
            }
            //g->flagOn("click_switch");
        });

        g->addScript("onRemove",[g,this](ScriptContext& ctx){
            //Temporary system for now, improve later
            g->flagOn("dead");
            g->flagOn("no_export");
            g->flagOn("no_save");
        });

        addExportScripts(g);

        g->addScript("onExport",[g,this](ScriptContext& ctx){
            if(!g->isActive()) { g->run("onActivate"); g->flagOn("export_activated");}
            if(!g->check("open")) { g->run("onOpen"); g->flagOn("export_opened"); print("export opened");}
        });

        g->addScript("onExportEnd",[g,this](ScriptContext& ctx){
        //    if(g->check("export_activated")) {g->run("onDeactivate"); g->flagOff("export_activated");}
        //    if(g->check("export_opened")) {g->run("onClose"); g->flagOff("export_opened");}
        });
        return g;
    }
public:

    map<std::string,g_ptr<Quad>> widgets;
    g_ptr<Quad> openWidget(const std::string& use_label,bool toggle = false,const vec2& pos = vec2(-10000,0))
    {
        std::string label = clean_underscores(use_label);
            if(widgets.hasKey(label))
            {
                auto g = widgets.get(label);
                if(pos.x()!=-10000) g->setPosition(pos);
                if(!g->isActive()) {
                    g->run("onActivate");
                }
                else if(toggle) {
                    closeWidget(label);
                }
                return g;
            }
        print("ggui::openWidget::705 Widget ",label," does not exist");
        return nullptr;
    }

    g_ptr<Quad> getWidget(const std::string& use_label,const vec2& pos,bool toggle = false)
    {
        std::string label = clean_underscores(use_label);
            if(widgets.hasKey(label))
            {
                auto g = widgets.get(label);
                return g;
            }
        print("ggui::getWidget::717 Widget ",label," does not exist");
        return nullptr;
    }

    void closeWidget(const std::string& use_label)
    {
        std::string label = clean_underscores(use_label);
        if(widgets.hasKey(label))
        {
            auto g = widgets.get(label);
            g->run("onDeactivate");
        }
    }

    g_ptr<Quad> makeWidget(wType wtype,const std::string& use_label,Data& d)
    {
        std::string label = clean_underscores(use_label);
        g_ptr<Quad> g = nullptr;
        if(widgets.hasKey(label)) {
           // print("collison label ",label);
            auto l = split_str(label,'.');
            if(l.length()>1) {
                label = l[0].append(std::to_string(
                    (std::stoi(l[1])+1)
                ));
            }
            else
            {
                label.append(".0");
            }
            //print("Final label: ",label);
        }

        d.set<std::string>("label",label);
        switch(wtype) {
            case wType::BASE: g = base(d); break;
            default: 
            print("ggui::openWidget::163 Invalid widget type");
            break;
        }
        widgets.put(label,g);
        
        if(g->isActive()) gui->deactivate(g);
        return g;
    }

    
};


/// @brief 2d tool for tracking mouse position and relative transforms
struct G2_Tool {
    G2_Tool(KeyCode _key,g_ptr<Scene> _scene) : key(_key),scene(_scene) {}
    g_ptr<Scene> scene;
    vec2 start;
    vec2 startDist;
    vec2 startDistCenter;
    vec2 basePos;
    vec2 baseCenter;
    vec2 baseScale;
    float baseRotation;
    glm::vec4 baseColor;
    g_ptr<Quad> selected;
    KeyCode key;

    void scan(g_ptr<Quad> g) { 
        if(Input::get().keyPressed(key)) 
        {
        if(!selected&&g&&!g->check("locked")) {
            vec2 pos = mousePos2d(scene);
            selected = g;
            start = pos;
            vec2 center = g->getCenter();
            baseCenter = center;
            basePos = g->getPosition();
            baseScale = g->getScale();
            startDistCenter = vec2(pos.x() - center.x(),
                                   pos.y() - center.y());
            startDist = vec2(pos.x() - basePos.x(),
                             pos.y() - basePos.y());
            baseRotation = g->getRotation();
        }
        }
        else {selected=nullptr;} 
    }

    vec2 distance() {  
        vec2 pos = mousePos2d(scene);
        return vec2(std::max(1.0f, std::abs(pos.x() - start.x())),
                    std::max(1.0f, std::abs(pos.y() - start.y())));
    } 

    vec2 distFactor() {
        vec2 pos = mousePos2d(scene);
        return (pos-basePos) / startDist;
    }
};  

struct G3_Tool {

};

struct TextEditor {
    TextEditor(g_ptr<Scene> _scene) : scene(_scene) {}
    bool editing = false;
    g_ptr<Quad> crsr = nullptr;
    size_t crsPos = 0;
    float pause = 0.0f;
    float pause2 = 0.0f;
    float pause3 = 0.0f;
    float blink = 0.0f;
    bool blinkOn = true;
    int lastChar = 0;
    vec2 fromTo = vec2(1.0f,1.0f);
    std::string clipboard = "";
    g_ptr<Scene> scene;
    void tick(float tpf)
    {
        bool shiftHeld = Input::get().keyPressed(LSHIFT) || Input::get().keyPressed(RSHIFT);
        pause -= tpf;
        pause2 -= tpf;
        pause3 -= tpf;
        if(blink>0.0f) {
            blink-=tpf; 
            if(crsr) 
            {
            if(blinkOn&&!crsr->isActive()) {crsr->lockToParent=false; scene->reactivate(crsr); crsr->lockToParent=true;}
            else if(!blinkOn&&crsr->isActive()) {crsr->lockToParent=false; scene->deactivate(crsr); crsr->lockToParent=true;}
            }
        }
        else {blink = 0.4f; blinkOn = !blinkOn;}
        if(Input::get().keyPressed(KeyCode::LEFT)&&pause<=0.0f)
        {
            if(crsr&&crsPos>2)
            {
                pause = 0.1f;
                text::removeChar(crsPos,crsr);
                crsPos--;
                crsr=text::insertChar(crsPos,'|',crsr);
            }
        }

        if(Input::get().keyPressed(KeyCode::RIGHT)&&pause2<=0.0f)
        {
            if(crsr&&crsPos<crsr->get<txt>("chars")->length()-2)
            {
                pause2 = 0.1f;
                text::removeChar(crsPos,crsr);
                crsPos++;
                crsr=text::insertChar(crsPos,'|',crsr);
            }
        }

        if(Input::get().keyJustPressed(KeyCode::ESCAPE))
        {
            if(editing)
            {
                editing = false;
                text::removeChar(crsPos,crsr);
                crsr = nullptr;
            }
        }

        if(crsr)
        {
            std::string string = crsr->get<txt>("chars")->Object::get<std::string>("string");
            if(Input::get().keyPressed(KeyCode::BACKSPACE)&&pause3<=0.0f)
            {
                pause3 = 0.1f;
                if(crsPos>1)
                text::removeChar(--crsPos,crsr);
            }

            if(Input::get().keyPressed(KeyCode::CMD))
            {
                if(Input::get().keyJustPressed(KeyCode::C))
                {
                    int x = fromTo.x();
                    int y = fromTo.y();
                    int from = std::min(x,y);
                    int to =  std::max(x,y);
                    clipboard = string.substr(from,to-from);
                }

                if(Input::get().keyJustPressed(KeyCode::V))
                {
                    text::insertText(crsPos,clipboard,crsr);
                }
            }
            else
            {
            list<int> pressed = Input::get().justPressed();
            for(int i=0;i<pressed.length();i++)
            {
                int t = pressed[i];
                char c;
                if (t == KeyCode::BACKSPACE) {
                    continue;
                }
                else if(keycodeToChar(t,shiftHeld,c))
                {
                    text::insertChar(crsPos,c,crsr);
                    crsPos++;
                    lastChar = t;
                }
            }
            }

        }
    }

    void scan(g_ptr<Quad> g) {
        bool shiftHeld = Input::get().keyPressed(LSHIFT) || Input::get().keyPressed(RSHIFT);
        if(Input::get().keyPressed(KeyCode::MOUSE_LEFT)&&pause<=0.0f)
        {
            pause=0.1f;
            if(g->has("char"))
            {
            size_t at = g->get<size_t>("pID");
            size_t length = g->get<txt>("chars")->length();
            if(at<length-1&&at>1)
            {
            editing = true;
            if(!crsr) {
                if(at==length-2) at = length-1;
                crsr=text::insertChar(at,'|',g);
            }
            else {
                text::removeChar(crsPos,crsr);
                crsr=text::insertChar(at,'|',g);
                if(shiftHeld) fromTo.setX(at);
                else fromTo.setY(at+1);
            }
            crsPos = at;
            }
            }
        }
    }
};

int main() {
    std::string ROOT = "../Projects/GUIDE/storage/";
    std::string EROOT = "../Engine/assets/";
    std::string IROOT = "../Projects/GUIDE/assets/images/";
    std::string GROOT = "../Projects/GUIDE/assets/gui/";


    Window window = Window(1280, 768, "GUIDE 0.2.2");
    glfwSwapInterval(0); //Vsync 1=on 0=off
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 0;
    }

    glm::vec4 BLUE = glm::vec4(0,0,1,1);
    glm::vec4 RED = glm::vec4(1,0,0,1);
    glm::vec4 BLACK = glm::vec4(0,0,0,1);

    Input& input = Input::get();

    g_ptr<gui_scene> gui = make<gui_scene>(window,2);
    gui->camera = Camera(60.0f, 1280.0f/768.0f, 0.1f, 1000.0f, 4);
    gui->camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    gui->relight(glm::vec3(0.0f,0.1f,0.9f),glm::vec4(1,1,1,1));
    gui->setupShadows();
    list<g_ptr<Quad>> noSave;
    list<g_ptr<Quad>> noExport;
    Grid grid(
        0.25f,
        30);

    auto ggui = make<GGUI>(gui,window,input,grid);

   
    auto top_bar = make<G_Quad>();
    gui->add(top_bar);
    top_bar->scale(vec2(2600,100));
    top_bar->setPosition(vec2(0,0));
    top_bar->color = glm::vec4(0.1f,0.1f,0.1f,1);
    noSave << top_bar;
    gui->removeSlectable(top_bar);

    // auto t = make<G_Quad>();
    // gui->add(t);
    // t->scale(vec2(200,200));
    // t->setPosition(vec2(500,500));
    // t->color = glm::vec4(0.8f,0.1f,0.1f,1);
    // noSave << t;

    auto source_code = make<Font>(EROOT+"fonts/source_code.ttf",50);
    auto source_code_bold = make<Font>(EROOT+"fonts/source_code_bold.ttf",60);
    auto loaded = gui->loadQFabList(GROOT+"autosave.fab");
    g_ptr<Quad> set_title_indicator = text::makeText("Export: ",source_code_bold,gui,vec2(20.0f,60.0f),1.0f);
    noSave << set_title_indicator;
    gui->removeSlectable(set_title_indicator);
    g_ptr<Quad> add_slot_indicator = text::makeText("Slot: ",source_code_bold,gui,vec2(1350.0f,60.0f),1.0f);
    noSave << add_slot_indicator;
    gui->removeSlectable(add_slot_indicator);
    g_ptr<Quad> indicator = text::makeText("[      ]",source_code,gui,vec2(2100.0f,60.0f),1.0f);
    noSave << indicator;
    gui->removeSlectable(indicator);
    ggui->indicatorSetup(indicator);

    g_ptr<Quad> set_title = nullptr;
    if(gui->loaded_slots.hasKey("set_title")) {
        set_title = gui->loaded_slots.get("set_title")[0];
        set_title->flagOn("no_export");
    }
    else {
        set_title = text::makeText("../Projects/GUIDE/assets/gui/guide.fab",source_code,gui,vec2(195.0f,60.0f),1.0f);
        set_title->getSlots().push("set_title");
        //set_title->flagOn("no_save");
        set_title->flagOn("no_export");
    }
    noExport << set_title;

    auto save_scene = [ggui,gui,GROOT,noExport,noSave]() -> void{
        print("Saving scene");
        ggui->indicate("[saving]",1.3f);
        list<g_ptr<Quad>> exported;
        ScriptContext exp_ctx;
        for(auto wgt : ggui->widgets.getAll())
        {
            if(noExport.has(wgt)) continue;

            exp_ctx.inc<int>("wdgt_num",1);
            wgt->run("onExport",exp_ctx);
            exported << wgt;
        }

        list<Q_Fab> toSave;
        for(int i=0;i<gui->quadActive.length();i++)
        {
            if(gui->quadActive[i])
            {
                auto q = gui->quads[i];
                if(noSave.has(q)) continue;
                if(q->check("no_save")) continue;
                Q_Fab fab;
                if(q->has("char")) {
                    if(!q->parent) 
                    { 
                      fab.encodeText(q);
                      toSave << fab;
                    }
                }
                else {
                    fab.encodeQuad(q);
                    toSave << fab;
                }
            }
        }
        gui->saveQFabList(GROOT+"autosave.fab",toSave);
        for(auto p : exported)
        {
            p->run("onExportEnd",exp_ctx);
        }
    };

    auto export_scene = [ggui,gui,ROOT,noExport,noSave,set_title]() -> void{
        print("Exporting scene");
        ggui->indicate("[epxort]",1.3f);
        list<g_ptr<Quad>> exported;
        ScriptContext exp_ctx;
        for(auto wgt : ggui->widgets.getAll())
        {
            if(noExport.has(wgt)) continue;

            exp_ctx.inc<int>("wdgt_num",1);
            wgt->run("onExport",exp_ctx);
            exported << wgt;
        }

        list<Q_Fab> toSave;
        for(int i=0;i<gui->quadActive.length();i++)
        {
            if(gui->quadActive[i])
            {
                auto q = gui->quads[i];
                if(noSave.has(q)) continue;
                if(noExport.has(q)) continue;
                if(q->check("no_save")) continue;
                if(q->check("no_export")) continue;
                Q_Fab fab;
                // print(q->UUID);
                // q->getSlots()([](const std::string s){if(s!="all") print(s);});
                if(q->has("char")) {
                    if(!q->parent) 
                    { 
                    //print("encoding text ",q->UUID);
                    fab.encodeText(q);
                    toSave << fab;
                    }
                }
                else {
                // print("encoding quad ",q->UUID);
                    fab.encodeQuad(q);
                    toSave << fab;
                }
            }
        }
        print("exported to: ",text::string_of(set_title));
        gui->saveQFabList(text::string_of(set_title),toSave);

        for(auto p : exported)
        {
            // if(poked.has(p)) p->run("onClick",poke);
            p->run("onExportEnd",exp_ctx);
        }
    };

    auto clear_scene = [ggui,gui,ROOT,noExport,noSave,set_title]() -> void{ 
        print("Clearing scene");
        ggui->indicate("[Cleared]",1.3f);

        list<g_ptr<Quad>> toDelete;
        for(int i=0;i<gui->quads.length();i++)
        {
            if(i>=gui->quads.length()) break;

            auto q = gui->quads[i];
            if(noSave.has(q)) continue;
            if(noExport.has(q)) continue;
            if(q->check("no_save")) continue;
            if(q->check("no_export")) continue;
            if(q->parent) continue;
            toDelete << q;
        }

        for(int i=toDelete.length()-1;i>=0;i--)
        {
            if(i>=toDelete.length()) continue;
            if(toDelete[i])
                toDelete[i]->remove();
        }
    };

    auto load_scene = [ggui,gui,ROOT,noExport,noSave,set_title,clear_scene]() -> void{ 
        print("Loading scene");
        ggui->indicate("[Loaded]",1.3f);

        clear_scene();

       auto loaded = gui->loadQFabList(text::string_of(set_title));

    };

    g_ptr<Quad> add_slot = nullptr;
    if(gui->loaded_slots.hasKey("add_slot")) {
        add_slot = gui->loaded_slots.get("add_slot")[0];
        add_slot->flagOn("no_export");
    }
    else {
        add_slot = text::makeText("all",source_code,gui,vec2(1500.0f,60.0f),1.0f);
        add_slot->getSlots().push("add_slot");
        add_slot->flagOn("no_export");
    }
    noExport << add_slot;

    G2_Tool move2d(G,gui);
    G2_Tool scale2d(S,gui);
    G2_Tool rotate2d(R,gui);
    TextEditor text(gui);

    
    //Lock widget:
    auto lock_items = make<itmsM>();
    for(int i=0;i<2;i++)
    {
    Data dd_l;
    dd_l.set<itms>("items",lock_items);
    g_ptr<Quad> lock_body = nullptr;
    std::string label = "lockwdgt open";
    if(i==1) label = "lockwdgt closed";
    if(i==0)
    lock_body = gui->makeImageQuad(IROOT+"lock_open.png",2.5f);
    else if(i==1) 
    lock_body = gui->makeImageQuad(IROOT+"lock_closed.png",2.5f);

    dd_l.add<g_ptr<Quad>>("body_item",lock_body);
    auto base = ggui->makeWidget(wType::BASE,label,dd_l);
    noSave << base;
    noExport << base;
    base->flagOn("moving_wdgt");
    base->flagOn("no_save");
    base->flagOn("no_export");

    if(i==0)
    {
    base->addScript("onClick",[ggui,base,gui](ScriptContext& ctx){
        if(base->has("on_item"))
        {
            auto g = base->get<g_ptr<Quad>>("on_item");
        g->set<bool>("locked",true);
            if(Input::get().keyPressed(CMD)) g->lockWithParent();
            ggui->closeWidget("lockwdgt open");
            auto closed = ggui->openWidget("lockwdgt closed",false,base->getPosition());
            closed->set<g_ptr<Quad>>("on_item",g);
        }
    });
    }
    if(i==1)
    {
    base->addScript("onClick",[ggui,base,gui](ScriptContext& ctx){
        if(base->has("on_item"))
        {
            auto g = base->get<g_ptr<Quad>>("on_item");
            g->set<bool>("locked",false);
            if(Input::get().keyPressed(CMD))  g->lockToParent = false;
            ggui->closeWidget("lockwdgt closed");
            auto open = ggui->openWidget("lockwdgt open",false,base->getPosition());
            open->set<g_ptr<Quad>>("on_item",g);
        }
    });
    }

    base->addScript("onUpdate",[ggui,base,gui](ScriptContext& ctx){
        if(base->isActive()&&base->has("on_item"))
        {
        auto g = base->get<g_ptr<Quad>>("on_item");
        if(!g->isActive())
        {
            base->run("onDeactivate");
        }

        //This is **SUPER** non performant, but, it works because GUIDE is just an editor
        //Can cut this later if it becomes a bottleneck, just nice to have for now
        //Or, rework once I re-integrate a proper screen grid into GUIDE for broadphase
        vec2 pos(0,0);
        vec2 top_left = g->getPosition().addX(-80.0f).addY(-100);
        vec2 bottom_left = g->getPosition().addX(-80.0f).addY(g->getScale().y()*0.7f);
        vec2 top_right = g->getPosition().addX(g->getScale().x()).addY(-100);
        vec2 bottom_right = g->getPosition().addX(g->getScale().x()).addY(g->getScale().y()*0.7f);

        bool top_left_valid = true;
        bool bottom_left_valid = true;
        bool top_right_valid = true;
        bool bottom_right_valid = true;

        for(int j=0;j<gui->quadActive.length();j++)
        {
            if(j>=gui->quads.length()) break;
            if(gui->quads[j]->has("char")) continue;
            if(gui->quadActive[j]) 
            {
                auto quad = gui->quads[j];
                if(quad==g) continue;
                if(quad==base) continue;
                if(quad->check("moving_wdgt")) continue;
                for(int i=0;i<4;i++)
                {
                vec2 pos_type(0,0);
                if(i==0) pos_type = top_left;
                else if(i==1) pos_type = bottom_left;
                else if(i==2) pos_type = top_right;
                else if(i==3) pos_type = bottom_right;

                if(quad->getCenter().distance(pos_type)<base->getScale().length())
                {
                    if(i==0) top_left_valid = false;
                    else if(i==1) bottom_left_valid = false;
                    else if(i==2) top_right_valid = false;
                    else if(i==3) bottom_right_valid = false;
                }
                }
            }
        }

        if(top_right_valid) pos = top_right;
        else if(top_left_valid) pos = top_left;
        else if(bottom_left_valid) pos = bottom_left;
        else if (bottom_right_valid) pos = bottom_right;
        else pos = top_right;
        base->setPosition(pos);
        }
    });
    }

    //Connection widget:
    auto connect_items = make<itmsM>();
    for(int i=0;i<1;i++)
    {
    Data dd_i;
    dd_i.set<itms>("items",connect_items);
    g_ptr<Quad> connect_body = nullptr;
    std::string label = "connectwdgt";
    connect_body = gui->makeImageQuad(IROOT+"widget_connect.png",2.5f);
    dd_i.add<g_ptr<Quad>>("body_item",connect_body);
    auto base = ggui->makeWidget(wType::BASE,label,dd_i);
    noSave << base;
    noExport << base;
    base->flagOn("moving_wdgt");
    base->flagOn("toggle_off");
    base->flagOn("no_save");
    base->flagOn("no_export");

    base->addScript("onClick",[ggui,base,gui,connect_items](ScriptContext& ctx){
        if(Input::get().keyPressed(CMD))
        {
            if(base->toggle("lines_off"))
                base->run("onClose");
            else
                base->run("onOpen");
        }
        else if(base->has("on_item"))
        {
            auto g = base->get<g_ptr<Quad>>("on_item");
            Data dd_r;
            dd_r.set<itms>("items",make<itmsM>());
            auto body = make<Quad>();
            gui->add(body);
            body->scale(vec2(5,5));
            dd_r.add<g_ptr<Quad>>("body_item",body);
            //Odd glitch here, causing bad_cast exceptions whenever we try to modify the transofrm of this widget
            //Tried using openWidget instead, tried checking the retrived type of get transform.
            auto line = ggui->makeWidget(wType::BASE,"connectwdgtline",dd_r);
            line->flagOn("no_export");
            line->flagOn("moving_wdgt");
            line->flagOn("no_save");
            line->flagOn("no_export");
            line->set<g_ptr<Quad>>("from",g);
            line->run("onActivate");

            connect_items->push(line);
        }
    });

    base->addScript("onUpdate",[ggui,base,gui,connect_items](ScriptContext& ctx){
        if(base->isActive()&&base->has("on_item"))
        {
        auto g = base->get<g_ptr<Quad>>("on_item");
        if(!g->isActive())
        {
            base->run("onDeactivate");
        }
        list<int> cleanUp;
        for(int i=0;i<connect_items->length();i++)
        {
            auto item = (*connect_items)[i];
            bool see = false;
            if(item->has("from")) {
                if(item->get<g_ptr<Quad>>("from")==g) {
                    see = true;
                }
            }

            if(item->has("connection")) {
                if(item->get<g_ptr<Quad>>("connection")==g){
                    see = true;
                }
            }

            if(base->check("lines_off")) see = false;


            if(!see) {
                if(item->isActive()) {
                    item->run("onDeactivate");
                }
                continue;
            }

            if(!item->has("connection"))
            {
                item->setPosition(g->getCenter());
                vec2 from = gui->mousePos2d();
                g_ptr<Quad> connection = nullptr;
                if(auto quad = gui->Scene::nearestElement())
                {
                    if(quad==item) continue;
                    if(quad->check("moving_wdgt")) continue;
                    if(quad->has("connected_to")) {
                        if(quad->get<itms>("connected_to")->list::has(g)) 
                            continue;
                    }
                    if(quad==g) continue;

                    from=quad->getCenter();
                    connection=quad;
                }

                vec2 dir = from-item->getPosition();
                item->scale(vec2(std::abs(dir.length()),5));
                float angle = atan2(dir.y(), dir.x());
                item->rotate(angle);

                if(Input::get().keyJustPressed(KeyCode::MOUSE_LEFT)&&connection)
                {
                    // print("Connected ",base->ID," to ",connection->ID);
                    item->set<g_ptr<Quad>>("connection",connection);
                    item->set<g_ptr<Quad>>("from",g);
                    connection->flagOn("has_connection");
                    item->setColor(connection->color);

                    if(!connection->has("connected_to")) {
                       connection->set<itms>("connected_to",make<itmsM>());
                    }
                    connection->get<itms>("connected_to")->push(base);

                    if(g->has("items"))
                    {
                    if(connection->has("items"))
                    {
                        g->get<itms>("items")->push(connection);
                        //ggui->indicate("~W["+std::to_string(connection->UUID)+"]",0.8f);
                    }
                    else
                    {
                        Data dd_c;
                        dd_c.set<itms>("items",make<itmsM>());
                        dd_c.add("body_item",connection);
                        auto newWidget = ggui->makeWidget(wType::BASE,g->get<std::string>("label"),dd_c);
                        newWidget->run("onActivate");
                        g->get<itms>("items")->push(newWidget);
                        ggui->indicate("+W["+std::to_string(connection->UUID)+"]",0.8f);
                    }
                    }
                    else //Improve this later by adding more options and control
                    {
                        item->addScript("onRemove",[connection](ScriptContext& ctx){
                            connection->removeFromParent();
                        });
                        g->addChild(connection,true);
                    }
                }
            }
            else
            {
                item->setPosition(item->get<g_ptr<Quad>>("from")->getCenter());
                auto con = item->get<g_ptr<Quad>>("connection");
                if(con&&con->isActive())
                {
                    item->setColor(con->color);
                    // print("Valid connection ",item->inc("i",1));
                    vec2 dir = con->getCenter()-item->getPosition();
                    item->scale(vec2(std::abs(dir.length()),5));
                    float angle = atan2(dir.y(), dir.x());
                    item->rotate(angle);
                }
                else
                {
                    item->run("onDeactivate");
                    if(con->check("dead"))
                    {
                    item->remove();
                    cleanUp << i;
                    }
                }
            }
        }
        for(auto i : cleanUp) connect_items->removeAt(i);

        vec2 pos(0,0);
        vec2 top_left = g->getPosition().addX(-80.0f).addY(0);
        vec2 bottom_left = g->getPosition().addX(-80.0f).addY(g->getScale().y()*0.3f);
        vec2 top_right = g->getPosition().addX(g->getScale().x()*1.1f).addY(0);
        vec2 bottom_right = g->getPosition().addX(g->getScale().x()*1.1f).addY(g->getScale().y()*0.3f);

        bool top_left_valid = true;
        bool bottom_left_valid = true;
        bool top_right_valid = true;
        bool bottom_right_valid = true;

        for(int j=0;j<gui->quadActive.length();j++)
        {
            if(j>=gui->quads.length()) break;
            if(gui->quads[j]->has("char")) continue;
            if(gui->quadActive[j]) 
            {
                auto quad = gui->quads[j];
                if(quad==g) continue;
                if(quad==base) continue;
                if(quad->check("moving_wdgt")) continue;
                for(int i=0;i<4;i++)
                {
                vec2 pos_type(0,0);
                if(i==0) pos_type = top_left;
                else if(i==1) pos_type = bottom_left;
                else if(i==2) pos_type = top_right;
                else if(i==3) pos_type = bottom_right;

                if(quad->getCenter().distance(pos_type)<base->getScale().length())
                {
                    if(i==0) top_left_valid = false;
                    else if(i==1) bottom_left_valid = false;
                    else if(i==2) top_right_valid = false;
                    else if(i==3) bottom_right_valid = false;
                }
                }
            }
        }

        if(top_right_valid) pos = top_right;
        else if(top_left_valid) pos = top_left;
        else if(bottom_left_valid) pos = bottom_left;
        else if (bottom_right_valid) pos = bottom_right;
        else pos = top_right;
        base->setPosition(pos);
        }
    });
    }


    gui->set<std::string>("selected_project","GUIDE");
    gui->set<std::string>("selected_file","autosave.fab");

    //File panel:
    std::string projects = "";
    auto subdir = list_subdirectories("../Projects",true);
    for(auto s : subdir)
    {
        projects.append("> ").append(s).append("\n");
    }
    auto files = text::makeText(projects,source_code_bold,gui,vec2(20,180),1.0f);
    files->flagOn("no_edit");
    files->flagOn("file_panel");
    files->flagOn("locked");
    noSave << files;
    noExport << files;
    for(auto g : files->children)
    {
        g->addScript("onClick",[g,gui,ggui](ScriptContext& ctx){
            std::string ref_txt = text::string_of(g);
            size_t pos = g->get<size_t>("pID");
            size_t check_down = ref_txt.rfind('v', pos);
            size_t check_up = ref_txt.rfind('>', pos);
            size_t found = check_up;
            bool down = true;
            if(check_up < check_down&&check_down!=ref_txt.npos) 
            {
                down = false;
                found = check_down;
            }
            //if(found==ref_txt.npos) return;
            text::removeChar(found+1,g);
            text::insertChar(found+1,down ? 'v' : '>',g);
            if(down)
            {
                size_t end_line = ref_txt.find('\n', pos-1);
                std::string project = ref_txt.substr(check_up+2,(end_line)-check_up-2);
                if(text::char_at(end_line,g)->get<char>("char")==' ')
                {
                    text::removeChar(end_line,g);
                }
                else
                {
                text::insertChar(end_line+1,' ',g);
                }
                std::string files = "";
                auto subdir = list_files("../Projects/"+project+"/assets/gui",true);
                gui->set<std::string>("selected_project",project);
                for(int i=0;i<subdir.length();i++)
                {
                    files.append("  ").append(subdir[i]);
                    if(i<subdir.length()-1)
                        files.append("\n");
                    
                }
                text::insertText(end_line+1,files,g);
                text::insertChar(end_line+1,'\n',g);
                text::char_at(check_up+2,g)->set<std::string>("prev_files",files);
            }
            else
            {

                std::string prev_files =  text::char_at(check_down+2,g)->get<std::string>("prev_files");
                size_t end_line = ref_txt.find('\n', pos-1);
                text::removeText(end_line+1,prev_files.length()+2,g);
            }
        });
    }

    // auto testing = text::makeText("What a wonder",source_code_bold,gui,vec2(800,180),1.0f);
    // testing->addScript("onHover",[testing](ScriptContext& ctx){
    //     if(!testing->check("hovered"))
    //     {
    //         testing->setPosition(testing->getPosition()-vec2(0,5));
    //         testing->flagOn("hovered");
    //     }
    // });

    // testing->addScript("onUnHover",[testing](ScriptContext& ctx){
    //     if(testing->check("hovered"))
    //     {
    //         testing->setPosition(testing->getPosition()+vec2(0,5));
    //         testing->flagOff("hovered");
    //     }
    // });
   

    //File opening buttons
    list<g_ptr<Quad>> file_items;
    for(int i=0;i<1;i++)
    {
    auto f_btn = make<Quad>();
    gui->add(f_btn);
    f_btn->scale(vec2(80,40));
    f_btn->setColor(Color::GREY);
    f_btn->setPosition(vec2(0,100));
    noSave << f_btn;
    noExport << f_btn;
    file_items << f_btn;

    Data dd_s;
    auto s_btn = make<Quad>();
    gui->add(s_btn);
    s_btn->scale(vec2(140,40));
    s_btn->setColor(glm::vec4(0.3,0.8,0.3,1));
    s_btn->setPosition(vec2(80,100));
    auto s_txt = text::makeText("save",source_code,gui,vec2(95,130),1.0f);
    noSave << s_txt;
    noExport << s_txt;
    file_items << s_txt;
    dd_s.set<g_ptr<Quad>>("body_item",s_btn);
    dd_s.set<g_ptr<Quad>>("text_item",s_txt);
    dd_s.set<itms>("items",make<itmsM>());
    auto save_button = ggui->makeWidget(wType::BASE,"save button",dd_s);
    save_button->flagOn("locked");
    noSave << save_button;
    noExport << save_button;
    save_button->addScript("onClick",[save_scene](ScriptContext& ctx){
        save_scene();
    });
    file_items << save_button;

    Data dd_l;
    auto l_btn = make<Quad>();
    gui->add(l_btn);
    l_btn->scale(vec2(140,40));
    l_btn->setColor(Color::BLUE);
    l_btn->setPosition(vec2(200,100));
    auto l_txt = text::makeText("load",source_code,gui,vec2(220,130),0.9f);
    noSave << l_txt;
    noExport << l_txt;
    file_items << l_txt;
    dd_l.set<g_ptr<Quad>>("body_item",l_btn);
    dd_l.set<g_ptr<Quad>>("text_item",l_txt);
    dd_l.set<itms>("items",make<itmsM>());
    auto load_button = ggui->makeWidget(wType::BASE,"load button",dd_l);
    noSave << load_button;
    noExport << load_button;
    load_button->flagOn("locked");
    load_button->addScript("onClick",[load_scene](ScriptContext& ctx){
        load_scene();
    });
    file_items << load_button;

    Data dd_e;
    auto e_btn = make<Quad>();
    gui->add(e_btn);
    e_btn->scale(vec2(140,40));
    e_btn->setColor(Color::RED);
    e_btn->setPosition(vec2(340,100));
    auto e_txt = text::makeText("export",source_code,gui,vec2(345,130),0.9f);
    noSave << e_txt;
    noExport << e_txt;
    file_items << e_txt;
    dd_e.set<g_ptr<Quad>>("body_item",e_btn);
    dd_e.set<g_ptr<Quad>>("text_item",e_txt);
    dd_e.set<itms>("items",make<itmsM>());
    auto export_button = ggui->makeWidget(wType::BASE,"export button",dd_e);
    noSave << export_button;
    noExport << export_button;
    export_button->flagOn("locked");
    export_button->addScript("onClick",[export_scene](ScriptContext& ctx){
        export_scene();
    });
    file_items << export_button;


    Data dd_fi;
    dd_fi.set<g_ptr<Quad>>("body_item",files);
    dd_fi.set<itms>("items",make<itmsM>());
    auto file_widget = ggui->makeWidget(wType::BASE,"files",dd_fi);
    noSave << file_widget;
    noExport << file_widget;
    file_items << file_widget;

    Data dd_f;
    dd_f.set<g_ptr<Quad>>("body_item",f_btn);
    auto fitems = make<itmsM>();
    fitems->push(file_widget);
    fitems->push(save_button);
    fitems->push(load_button);
    fitems->push(export_button);
    dd_f.set<itms>("items",fitems);
    auto file_button = ggui->makeWidget(wType::BASE,"file button",dd_f);
    ggui->openWidget("file button");
    noSave << file_button;
    noExport << file_button;
    file_items << file_button;
    }
    for(auto f : file_items)
    {
        f->flagOn("no_save");
        f->flagOn("no_export");
    }


    float tpf = 0.1; float frametime = 0; int frame = 0;
    auto last = std::chrono::high_resolution_clock::now();
    bool flag = true;
    float pause = 0.0f;
    float snap_grace = 0.0f;
    g_ptr<G_Object> selected = nullptr;
    g_ptr<G_Object> mSelected = nullptr;
    g_ptr<Quad> mgSelected = nullptr;
    g_ptr<G_Object>sSelected = nullptr;
    g_ptr<Quad>sgSelected = nullptr;
    Cell* startPos = nullptr;
    bool flagDist = false;
    int startDistX = 0;
    int startDistY = 0;
    float baseScaleX = 0;
    float baseScaleY = 0;
    int inc = 0;

    list<g_ptr<Quad>> selection;
    list<g_ptr<Quad>> backings;
    list<std::string> openWdgts;

    g_ptr<Quad> hovered = nullptr;
    std::string project_string = "";
    int fNum = 0;
    while (!window.shouldClose()) {
        if(Input::get().keyPressed(KeyCode::K)&&!text.editing) break;
        if(pause>0) pause -=tpf;
        if(snap_grace>0) snap_grace -=tpf;
        bool shift = input.keyPressed(KeyCode::LSHIFT);
        
        std::string new_project_string = "../Projects/"
            +gui->get<std::string>("selected_project")+"/assets/gui/"
            +gui->get<std::string>("selected_file");

        if(new_project_string!=project_string)
        {
            project_string = new_project_string;
            text::setText(new_project_string,set_title);
        }

        if(input.keyJustPressed(H))
        {
            ggui->openWidget("testbtn",true,vec2(500,500));
        }
        // if(input.keyJustPressed(MOUSE_RIGHT))
        // {
        //     ggui->openWidget("ctx_test",mousePos2d(gui),true);
        // }
        if(input.keyJustPressed(MOUSE_LEFT))
        {
            if(!shift)
            {
            selection.clear();
            for(auto b : backings) 
            {
                b->removeFromParent();
                gui->deactivate(b);
            }
            backings.clear();
            }
        }

        if(input.keyJustPressed(ESCAPE))
        {
            selection.clear();
            for(auto b : backings) 
            {
                b->removeFromParent();
                gui->deactivate(b);
            }
            backings.clear();

            for(auto w : openWdgts)
            {
                ggui->closeWidget(w);
            }
        }

        if(hovered&&hovered->has("char"))
        {
            if(!hovered->pointInQuad(gui->mousePos2d()))
            {
                hovered->run("onUnHover");
                hovered = nullptr;
            }  
        }

        ggui->tickIndicator(tpf);

        if(input.keyJustPressed(KeyCode::A)&&!text.editing)
        {
            auto a = make<G_Quad>();
            gui->add(a);
            a->scale(vec2(100,100));
            a->setPosition(mousePos2d(gui));
            a->color = glm::vec4(0.8f,0.1f,0.1f,1);
        }

        if(input.keyJustPressed(KeyCode::T)&&!text.editing)
        {
            text::makeText("add text",source_code_bold,gui,mousePos2d(gui),1.0f);
        }

        if(input.keyPressed(KeyCode::CMD))
        {
            if(input.keyJustPressed(KeyCode::S))
            {
                save_scene();
            }

            if(input.keyJustPressed(KeyCode::E))
            {
               export_scene();
            }

            if(input.keyJustPressed(KeyCode::BACKSPACE))
            {
                clear_scene();
            }

            if(input.keyJustPressed(KeyCode::I))
            {
                print(gui->quads.length());
            }
        }

        if(input.keyJustPressed(KeyCode::Q)&&!text.editing&&shift)
        {
            g_ptr<Quad> closest = nullptr;
            float closestDist = 10000;
            vec2 pos = mousePos2d(gui);
            for(auto gobj : gui->selectable)
            {
                if(auto g = g_dynamic_pointer_cast<Quad>(gobj))
                {
                    if(g==move2d.selected) continue;
                    if(!g->get<bool>("valid")&&g->isActive())
                    {
                        float dist = (g->getPosition()-pos).length();
                        if(dist<=closestDist) {
                            closestDist = dist;
                            closest = g;
                        }
                    }  
                }
            }

            if(closest)
            {
            g_ptr<Quad> g = closest;
            print("Validated ",g->UUID);
            ggui->indicate("+V["+std::to_string(g->UUID)+"]",0.8f);
            ggui->set_valid(g,true);
            }
            
        }

        text.tick(tpf);

        g_ptr<S_Object> nearest = gui->nearestElement();
        if(auto g = g_dynamic_pointer_cast<G_Object>(nearest))
        {
            if(input.keyPressed(KeyCode::MOUSE_RIGHT)) {
                selected = g;
            }

            if(input.keyPressed(KeyCode::MOUSE_LEFT)) {
                ggui->click(g);
            }

            if(Input::get().keyPressed(D)&&pause<=0&&!text.editing) {
                pause=0.3f;
                ggui->duplicate(g);
            }

            if(Input::get().keyPressed(C)&&pause<=0&&!text.editing){
                pause=0.3f;
                ggui->remove(g);
            }

            if(input.keyPressed(NUM_1)&&pause<=0&&!text.editing) {
                pause=0.3f;
                g->setColor(BLUE);
            }

            if(input.keyPressed(NUM_2)&&pause<=0&&!text.editing) {
                pause=0.3f;
                g->setColor(RED);
            }

            if(input.keyPressed(KeyCode::G)&&!text.editing)
            {
                if(!mSelected&&g){mSelected = g;}
            }

            if (input.keyPressed(KeyCode::S)&&!text.editing) {
                if (!sSelected && g) {
                    sSelected = g;
        
                    // Record starting cell delta between object and mouse
                    Cell& objCell = grid.getCell(g->getPosition());
                    Cell& startCell = grid.getCell(getWorldMousePosition(gui));
                    startDistX = std::max(1, std::abs(startCell.x - objCell.x));
                    startDistY = std::max(1, std::abs(startCell.z - objCell.z));
        
                    // Store initial object scale
                    baseScaleX = g->sizeX;
                    baseScaleY = g->sizeY;
                }
            }
        }
        else if(auto g = g_dynamic_pointer_cast<Quad>(nearest))
        {
            bool block_text_scan = false;
            if(input.keyJustPressed(KeyCode::MOUSE_LEFT)) {
                if(input.keyPressed(KeyCode::LSHIFT))
                {
                    //The world's jankiest selection method:
                    auto a = make<G_Quad>();
                    gui->add(a);
                    a->scale(vec2(g->getScale().x()*0.5f,25.0f));
                    a->setCenter(g->getCenter().addY(g->getScale().y()*-0.55f));
                    a->color = glm::vec4(0.9f,0.9f,0.9f,1.0f);
                    backings << a;
                    noSave << a;
                    ggui->set_valid(a,false);

                    if(g->has("char"))
                    {
                        g_ptr<Quad> quad = g->get<txt>("chars")->Object::get<g_ptr<Quad>>("parent");
                        a->scale(vec2(g->getScale().x()*quad->get<txt>("chars")->length()-1,g->getScale().y()*0.5f));
                        a->setPosition(quad->getPosition().addY(g->getScale().y()*-1.5f));
                        if(!selection.has(quad))
                            selection << quad;
                        quad->addChild(a,true);
                    }
                    else
                    {
                        if(!selection.has(g))
                            selection << g;
                        g->addChild(a,true);

                        //In the future, add a swap feature to q_list so we can control ordering without deleting things

                        // Q_Fab fab(g);
                        // //This feels like bad practice, but it's convinent for now.
                        // //Remeber to add GC later
                        // gui->deactivate(g);
                        // auto n = gui->loadFab(fab);
                        // selection << n;
                        // n->addChild(a,true);
                    }
                }
                else {
                    g->run("onClick");
                    gui->fireSlots(g);
                    // g->set<bool>("valid",false);
                    // gui->queueEvent(0.3f,[g]
                    // {
                    // g->set<bool>("valid",true);
                    // return true;
                    // });
                }
                
            }

            if(input.keyPressed(KeyCode::MOUSE_LEFT)&&shift&&g->has("char"))
                block_text_scan = true;

            if(input.keyPressed(KeyCode::MOUSE_LEFT)&&g->has("char"))
            {
                auto p = text::parent_of(g);
                if(p->check("no_edit")) {
                    block_text_scan = true;
                }
            }

            if(input.keyJustPressed(KeyCode::MOUSE_LEFT)&&g->has("char"))
            {
                auto p = text::parent_of(g);
                if(p->has("file_panel"))
                {
                    std::string ref_txt = text::string_of(p);
                    size_t pos = g->get<size_t>("pID");

                    size_t end_line_front = ref_txt.find('\n', pos);
                    size_t end_line_back = ref_txt.rfind('\n', pos);
                    std::string file_name = ref_txt.substr(end_line_back+1,(end_line_front-1)-end_line_back);
                    file_name.erase(std::remove(file_name.begin(), file_name.end(), ' '), file_name.end());
                    // int line_returns = 0;
                    // for(char f : file_name) if(f=='\n') line_returns++;
                    // print(line_returns);
                    gui->set<std::string>("selected_file",file_name);
                    print(file_name);

                }
            }

            if(g->pointInQuad(gui->mousePos2d())&&g->has("char"))
            {
                auto q = text::parent_of(g);
                if(hovered!=q)
                {
                q->run("onHover");
                hovered=q;
                }
                
            }

            move2d.scan(g);
            scale2d.scan(g);
            rotate2d.scan(g);
            if(!block_text_scan) text.scan(g);

            if(input.keyJustPressed(KeyCode::MOUSE_RIGHT)&&!text.editing)
            {
                g_ptr<Quad> q = g;
                if(g->has("char"))
                {
                    q = text::parent_of(g);
                }
                if(!q->has("moving_wdgt"))
                {
                ggui->closeWidget("lockwdgt closed");
                ggui->closeWidget("lockwdgt open");
 
                ggui->closeWidget("connectwdgt");
                if(!openWdgts.has("connectwdgt")) openWdgts << "connectwdgt";
                if(!openWdgts.has("lockwdgt closed")) openWdgts << "lockwdgt closed";
                if(!openWdgts.has("lockwdgt open")) openWdgts << "lockwdgt open";

                auto connect_wdgt = ggui->openWidget("connectwdgt");
                connect_wdgt->run("onOpen");
                // if(connect_wdgt->has("on_item")&&connect_wdgt->check("toggle_pass"))
                // {
                //     if(connect_wdgt->get<g_ptr<Quad>>("on_item")==g)
                //     {
                //         gui->closeWidget("connectwdgt");
                //         connect_wdgt->run("onDeactivate");
                //         connect_wdgt->flagOn("toggle_pass");
                //         goto out_of_loop;
                //     }
                // }
                // connect_wdgt->flagOff("toggle_pass");

                connect_wdgt->set<g_ptr<Quad>>("on_item",q);
                if(shift) connect_wdgt->run("onClick");

                if(g->check("locked"))
                {
                    auto lock_wdgt = ggui->openWidget("lockwdgt closed");
                    lock_wdgt->set<g_ptr<Quad>>("on_item",q);
                    if(shift) lock_wdgt->run("onClick");
                }
                else
                {
                    auto lock_wdgt = ggui->openWidget("lockwdgt open");
                    lock_wdgt->set<g_ptr<Quad>>("on_item",q);
                    if(shift) lock_wdgt->run("onClick");
                }
                }
            }

            out_of_loop:;

            if(input.keyJustPressed(KeyCode::W)&&!text.editing)
            {
                g_ptr<Quad> q = g;
                if(g->has("char"))
                {
                    q = text::parent_of(g);
                }

                if(!q->has("items"))
                {
                    ggui->indicate("+W["+std::to_string(q->UUID)+"]",0.8f);
                    std::string label = text::string_of(add_slot);
                    Data dd_w;
                    dd_w.set<itms>("items",make<itmsM>());
                    dd_w.add("body_item",q);
                    auto newWidget = ggui->makeWidget(wType::BASE,label,dd_w);
                    newWidget->run("onActivate");
                }
                else
                {
                    ggui->indicate("~W["+std::to_string(q->UUID)+"]",0.8f);
                }
            }

            if(input.keyJustPressed(KeyCode::F)&&!text.editing)
            {
                std::string slot = text::string_of(add_slot);
                if(g->has("char")) g = text::parent_of(g);

                if(input.keyPressed(KeyCode::CMD))
                {
                    std::string report;
                    //Add repeat counting later
                    for(auto s : g->getSlots())
                    {
                        report.append(s+"\n");
                        print(s);
                    }
                    ggui->indicate(report,2.0f);

                }
                else if(input.keyPressed(KeyCode::LSHIFT))
                {
                    if(g->getSlots().has(slot))
                    {
                        print("Removed slot ",slot);
                        ggui->indicate("-["+slot+"]",1.3f);

                        g->getSlots().erase(slot);
                    }
                    else {
                        print("No such slot ",slot);
                        ggui->indicate("~["+slot+"]",1.3f);
                    }
                }
                else
                {
                    if(!g->getSlots().has(slot))
                    {
                        print("Added slot ",slot);
                        ggui->indicate("+["+slot+"]",1.3f);
                        g->getSlots().push(slot);
                    }
                    else
                    {
                        print("Already has slot ",slot);
                        ggui->indicate("~["+slot+"]",1.3f);
                    }
                }
            }

            if(input.keyJustPressed(KeyCode::O)&&!text.editing)
            {
                print("saved quad to ",text::string_of(set_title));
                Q_Fab fab;
                if(g->has("char")) fab.encodeText(g);
                else               fab.encodeQuad(g);
                fab.saveBinary(text::string_of(set_title));
                //++fNum;
                //fab.saveBinary("../Projects/StudentSystem/assets/images/fab"+std::to_string(fNum)+".fab");
            }
            
            if(Input::get().keyJustPressed(KeyCode::D)&&!text.editing) {
                if(g->has("char"))
                {
                    // auto chars = g->get<txt>("chars");
                    // g_ptr<S_Object> sobj = gui->getObject((*chars)[0]);
                    // if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
                    // {
                    //     text::makeText(
                    //         chars->Object::get<std::string>("string")
                    //         ,font
                    //         ,gui,quad->getPosition()
                    //         ,chars->Object::get<float>("scale")
                    //     );
                    // }

                    Q_Fab temp;
                    temp.encodeText(g);
                    auto nQ = gui->loadFab(temp);
                }
                else
                {
                Q_Fab temp;
                temp.encodeQuad(g);
                //temp.saveBinary(ROOT+"clipboard.fab");
                auto nQ = gui->loadFab(temp);
                }
            }

            if(Input::get().keyJustPressed(KeyCode::BACKSPACE)&&!text.editing){
                //ggui->remove(g);
                gui->removeObject(g);
            }
        
            if(input.keyJustPressed(KeyCode::Q)&&!text.editing&&!shift)
            {
                print("Invalidated ",g->UUID);
                ggui->indicate("-V["+std::to_string(g->UUID)+"]",0.8f);
                ggui->set_valid(g,false);
            }
        
            if(input.keyJustPressed(KeyCode::NUM_1)&&!text.editing)
            {
                g->color = Color::RED;
            }
            if(input.keyJustPressed(KeyCode::NUM_2)&&!text.editing)
            {
                g->color = Color::BLUE;
            }
            if(input.keyJustPressed(KeyCode::NUM_3)&&!text.editing)
            {
                g->color = Color::GREEN;
            }
            if(input.keyJustPressed(KeyCode::NUM_4)&&!text.editing)
            {
                g->color = Color::GREY;
            }
            if(input.keyJustPressed(KeyCode::NUM_5)&&!text.editing)
            {
                g->color = Color::WHITE;
            }
        }

        if(input.keyPressed(KeyCode::G)&&!text.editing)
        {
            if(mSelected)
            {
            Cell& cell = grid.getCell(getWorldMousePosition(gui));
            //mSelected->moveTo(cell.getPosition(),mSelected->size/2);
            mSelected->setPosition(cell.getPosition());
            mSelected->grid = &grid;
            mSelected->updateCells();
            }
        }
        else if(mSelected) mSelected=nullptr;

        if(input.keyPressed(move2d.key)&&!text.editing)
        {
            if(move2d.selected)
            {
                vec2 to =  mousePos2d(gui) - move2d.startDistCenter;
                if(shift)
                {
                    snap_grace = 0.2f;
                    vec2 center = move2d.selected->getCenter();
                    float closestDist = 100;
                    g_ptr<Quad> closest = nullptr;
                    for(auto gobj : gui->selectable)
                    {
                        if(auto g = g_dynamic_pointer_cast<Quad>(gobj))
                        {
                            if(g==move2d.selected) continue;
                            if(g->isActive()&&!g->has("char"))
                            {
                                float dist = (g->getCenter()-center).length();
                                if(dist<=closestDist) {
                                    closestDist = dist;
                                    closest = g;
                                }
                            }  
                        }
                    }
                    if(closest)
                    {
                        to = closest->getCenter();
                    }
                    move2d.selected->setCenter(to);
                }
                else if(snap_grace<=0.0f)
                {
                    move2d.selected->setCenter(to);
                }
                // if(move2d.selected->has("char")) move2d.selected->setPosition(
                //     gui->mousePos2d()-move2d.startDist
                // );
            }
        }

        if(input.keyPressed(scale2d.key)&&!text.editing)
        {
            if(scale2d.selected)
            {
                vec2 factor = scale2d.distFactor();
                // factor.setX(std::min(factor.x(),2.0f));
                // factor.setY(std::min(factor.y(),2.0f));
                vec2 newScale(
                    std::min(8000.0f,std::max(5.0f, scale2d.baseScale.x() * factor.x())),
                    std::min(8000.0f,std::max(5.0f, scale2d.baseScale.y() * factor.y()))
                );
                scale2d.selected->scale(
                   newScale
                );
            }
        }

        if(input.keyPressed(rotate2d.key)&&!text.editing)
        {
            if(rotate2d.selected)
            {
               rotate2d.selected->rotate(
                rotate2d.selected->getRotation()+0.08f
               );
            }
        }

        if (input.keyPressed(KeyCode::S)&&!text.editing) {
        if (sSelected) {
            Cell& objCell = grid.getCell(sSelected->getPosition());
            Cell& curCell = grid.getCell(getWorldMousePosition(gui));

            int curDistX = std::max(1, std::abs(curCell.x - objCell.x));
            int curDistY = std::max(1, std::abs(curCell.z - objCell.z));

            // Compute relative scale factors
            float factorX = float(curDistX) / float(startDistX);
            float factorY = float(curDistY) / float(startDistY);

            int newScaleX = std::max(1, int(baseScaleX * factorX));
            int newScaleY = std::max(1, int(baseScaleY * factorY));

            sSelected->scale(vec3(newScaleX, newScaleY,1));
            sSelected->size  = newScaleX + newScaleY;
            sSelected->sizeX = newScaleX;
            sSelected->sizeY = newScaleY;
        }
        } else if (sSelected) {
            sSelected = nullptr;
        }
        

        gui->updateScene(tpf);
        auto end = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(end - last);
        tpf = delta.count(); last = end; frame++; 

        window.swapBuffers();
        window.pollEvents();
    }

    
    glfwTerminate();
    return 0;
}

//Q_Fab fab(vec2(400,400),vec2(100,100),glm::vec4(0.2f,0.8f,0.2f,1),"none",list<std::string>{});

 // //     auto t2 = gui->loadFab(ROOT+"fab.fab");

//     auto q = make<Quad>();
//     gui->add(q);
//     q->scale(vec2(200,200));
//     q->setPosition(vec2(1000,400));
// q->color = glm::vec4(0.8f,0.2f,0.2f,1);
//     q->addScript("onClick",[q](ScriptContext& ctx) {
//         print(q->inc("w",1));
//     });

//     q->addChild(t,true);

    // auto chars = make<d_list<size_t>>();
    // Data a;  a.add<txt>("list",chars);
    // Data b;  b.add<txt>("list",chars);
    // chars->push(1); chars->push(2); chars->push(3);
    // a.get<txt>("list")->insert(6,1);
    // print("A:");
    // a.get<txt>("list")->operator()([](size_t i){print(i);});
    // print("B:");
    // b.get<txt>("list")->operator()([](size_t i){print(i);});

    // auto button = make<Button>("test",0.5f);
    // gui->add(button);
    // ggui->addClickEffect(button);
    // button->addScript("onClick",[ggui,BLUE](ScriptContext& ctx){
    //     auto button = make<Button>("spawned",0.5f);
    //     ggui->gui->add(button);
    //     ggui->addClickEffect(button);
    // });


   // auto imgHandle = loadTexture2D(EROOT+"images/test.png");   // anywhere

    // auto quadTex = make<Quad>();
    // gui->add(quadTex);
    // quadTex->scale(vec2(192,108));
    // quadTex->setCenter(vec2(640,384));
    // quadTex->setTexture(imgHandle, 0);          // unit 0
    // quadTex->color = glm::vec4(1,1,1,1); 

    // auto quadTex = gui->makeImageQuad(EROOT+"images/test.png",0.4f);
    // quadTex->setCenter(vec2(640,384));

    // t2->addChild(quadTex,true);

    // auto hey = make<Quad>();
    // gui->add(hey);
    // hey->scale(vec2(200,400));
    // hey->setPosition(vec2(0,400));
    // hey->color = glm::vec4(0.8f,0.2f,0.2f,1);

    // t2->addChild(hey,true);

    // auto text = make<Text>();
    // text->scene = gui;
    // text->font = gam->registerFont(EROOT+"fonts/roboto.ttf");
    // text->setText("Visions of metal Horizons dance in my eyes");
    // text->emit();
    // list<size_t>& text_uuids = text->C_UUIDs;

    //g_ptr<Quad> txth = text::makeText("Visions of metal Horizons dance in my eyes",font,gui,vec2(3.0f,200.0f),1.0f);
    // text::insertText(1,"so ",txth);
    // text::removeChar(9,txth);
    // txth->get<txt>("chars")->forEach([gui](size_t u){auto g = gui->getObject(u); 
    //     print(g->get<char>("char")," : ",g->get<size_t>("pID"));});


// if(input.keyPressed(R)&&pause<=0)
// {
//     pause=0.3f;
//     if(selected)
//     {
//         selected->model->saveBinary(ROOT+"guide.gmdl");
//     }
// }
//    if(input.keyPressed(T)&&pause<=0)
//     {
//         pause=0.3f;
//         Model m;
//         m.loadBinary(ROOT+"guide.gmdl");
//         auto newObj = make<Button>("loaded",0.5f);
//         newObj->setModel(m); 
//         gui.addClickEffect(newObj);
//         gui.selectable.push_back(newObj);
//         scene->add(newObj);
//         Cell& cell = grid.getCell(getWorldMousePosition(scene));
//         newObj->setPosition(cell.getPosition());
//     }

//     if(input.keyPressed(U)&&pause<=0)
//     {
//         pause=0.5f;
//         scene->saveBinary(ROOT+"guide.gscn");
//     }

//     if(input.keyPressed(I)&&pause<=0)
//     {
//         pause=0.5f;
//         scene = std::make_shared<Scene>(window,2);
//         gui.scene=scene;
//         scene->camera = Camera(60.0f, 1280.0f/768.0f, 0.1f, 1000.0f, 4);
//         scene->camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
//         scene->loadBinary(ROOT+"guide.gscn");
//         scene->relight(glm::vec3(0.0f,0.1f,0.9f),glm::vec4(1,1,1,1));
//         scene->setupShadows();
//     }

//     if(input.keyPressed(B)&&pause<=0)
//     {
//             pause=0.3f;
//         auto button = make<Button>("spawned",0.5f);
//         scene->add(button);
//         guiPtr->addClickEffect(button);
//         guiPtr->selectable.push_back(button);
//         Cell& cell = grid.getCell(getWorldMousePosition(scene));
//         button->setPosition(cell.getPosition());
//     }

        // if (input.keyPressed(KeyCode::S)) {
        //     if (sgSelected) {
                
        //         vec2 mpos = mousePos2d(gui);
        //         vec2 pos = sgSelected->getPosition();
        //         int curDistX = std::max(1.0f, std::abs(mpos.x() - pos.x()));
        //         int curDistY = std::max(1.0f, std::abs(mpos.y() - pos.y()));
    
        //         // Compute relative scale factors
        //         float factorX = float(curDistX) / float(startDistX);
        //         float factorY = float(curDistY) / float(startDistY);
    
        //         int newScaleX = std::max(1, int(baseScaleX * factorX));
        //         int newScaleY = std::max(1, int(baseScaleY * factorY));
    
        //         sgSelected->scale(vec2(newScaleX, newScaleY));
        //     }
        //     } else if (sgSelected) {
        //         sgSelected = nullptr;
        //     }

