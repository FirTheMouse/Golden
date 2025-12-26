#pragma once
#include <vector>
#include <queue>
#include <rendering/single.hpp>
#include <rendering/camera.hpp>
#include <rendering/renderer.hpp>
#include <util/util.hpp>
#include <rendering/scene_object.hpp>
#include <gui/quad.hpp>
#include <gui/text.hpp>
#include <util/q_list.hpp>


namespace Golden
{


uint loadTexture2D(const std::string& path, bool flipY = true);
uint loadTexture2D(const std::string& path, bool flipY,int& outW, int& outH);

struct Q_Fab {
    vec2 pos;
    vec2 scale;
    glm::vec4 color;
    std::string type = "none";
    std::string filePath = "[No File Path]";
    std::string text = "[Not Text]";
    list<std::string> slots;


    Q_Fab() {}
    Q_Fab(const vec2& _pos, const vec2& _scale, const glm::vec4& _color,
    const std::string& _type, const list<std::string>& _slots) :
    pos(_pos), scale(_scale), color(_color), type(_type), slots(_slots) {}

    Q_Fab(const vec2& _pos, const vec2& _scale,const std::string& _filePath,
        const std::string& _type, const list<std::string>& _slots) :
        pos(_pos), scale(_scale), filePath(_filePath), type(_type), slots(_slots) {}
    
    Q_Fab(const vec2& _pos, const vec2& _scale,const std::string& _filePath, const list<std::string>& _slots) :
        pos(_pos), scale(_scale), filePath(_filePath), slots(_slots) {
            type = "image";
        }

    Q_Fab(const std::string& _text,const std::string& _fontPath, const vec2& _pos, float _text_scale,
        float _font_scale, const list<std::string>& _slots) :
        text(_text), filePath(_fontPath), pos(_pos), slots(_slots) {
            scale = vec2(_font_scale,_text_scale);
            type = "text";
        }

    Q_Fab(g_ptr<Quad> g) {
        if(g->has("char")) {
            encodeText(g);
        }
        else {
            encodeQuad(g);
        }
    } 
    Q_Fab(const std::string& path) {
        if(path.substr(path.size() - 4) == ".fab")
        {
            loadBinary(path);
        } else {
            std::cerr << "Failed to read path in Q_Fab constructor: \n" << path << 
            "\n Invalid file type, expected .fab" << std::endl;
        }
    }

    //Encodes images as well
    void encodeQuad(const g_ptr<Quad> g) {
        pos = g->getPosition();
        scale = g->getScale();
        color = g->color;
        slots = g->getSlots();
        type = "none";
        if(g->has("image")){ 
            type = "image";
            filePath = g->get<std::string>("image");
        }
    }

    void encodeText(const g_ptr<Quad> g) {
        if(!g->has("char")) {
            print("q_fab::encodeText::61 Tried to encode a non-text quad");
            return;
        }
        auto chars = g->get<txt>("chars");
        // g_ptr<S_Object> sobj = chars->Object::get<g_ptr<Scene>>("scene")->getObject((*chars)[0]);
        // if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
        // {
        auto quad = chars->Object::get<g_ptr<Quad>>("parent");
        text = chars->Object::get<std::string>("string");
        g_ptr<Font> font = chars->Object::get<g_ptr<Font>>("font");
        filePath = font->get<std::string>("path");
        pos = quad->getPosition();
        float text_scale = chars->Object::get<float>("scale");
        float font_scale = font->get<float>("scale");
        scale = vec2(font_scale,text_scale);
        type = "text";
        slots = g->getSlots();
    }

    void saveBinary(const std::string& path) {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: " + path);
        saveBinary(out);
        out.close();
    }

    void loadBinary(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read from file: " + path);
        loadBinary(in);
        in.close();
    }

    void saveBinary(std::ostream& out) {
        // Position & scale (vec2)
        out.write(reinterpret_cast<const char*>(&pos), sizeof(vec2));
        out.write(reinterpret_cast<const char*>(&scale), sizeof(vec2));
    
        // Color (vec4)
        out.write(reinterpret_cast<const char*>(&color), sizeof(glm::vec4));
    
        // Type (std::string)
        uint32_t typeLen = type.length();
        out.write(reinterpret_cast<const char*>(&typeLen), sizeof(typeLen));
        out.write(type.data(), typeLen);
    
        // Slots (list<string>)
        uint32_t slotCount = slots.length();
        out.write(reinterpret_cast<const char*>(&slotCount), sizeof(slotCount));
        for (const auto& s : slots) {
            uint32_t len = s.length();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(s.data(), len);
        }

        //Text
        uint32_t txtlen = text.length();
        out.write(reinterpret_cast<const char*>(&txtlen), sizeof(txtlen));
        out.write(text.data(), txtlen);

        //FontPath
        uint32_t fpathlen = filePath.length();
        out.write(reinterpret_cast<const char*>(&fpathlen), sizeof(fpathlen));
        out.write(filePath.data(), fpathlen);
    }

    void loadBinary(std::istream& in) {
        // vec2s
        in.read(reinterpret_cast<char*>(&pos), sizeof(vec2));
        in.read(reinterpret_cast<char*>(&scale), sizeof(vec2));
    
        // vec4
        in.read(reinterpret_cast<char*>(&color), sizeof(glm::vec4));
    
        // Type
        uint32_t typeLen;
        in.read(reinterpret_cast<char*>(&typeLen), sizeof(typeLen));
        type.resize(typeLen);
        in.read(&type[0], typeLen);
    
        // Slots
        uint32_t slotCount;
        in.read(reinterpret_cast<char*>(&slotCount), sizeof(slotCount));
        slots.clear();
        for (uint32_t i = 0; i < slotCount; ++i) {
            uint32_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string s(len, '\0');
            in.read(&s[0], len);
            slots << s;
        }

        uint32_t txtlen;
        in.read(reinterpret_cast<char*>(&txtlen), sizeof(txtlen));
        text.resize(txtlen);
        in.read(&text[0], txtlen);

        uint32_t fpathlen;
        in.read(reinterpret_cast<char*>(&fpathlen), sizeof(fpathlen));
        filePath.resize(fpathlen);
        in.read(&filePath[0], fpathlen);
    }
};


class Light : virtual public Object {
public:
    glm::vec3 position;
    glm::vec3 color;
    Light(glm::vec3 pos,glm::vec3 col) : position(pos), color(col) {}
};



class Scene : virtual public Object {
public:

    //When reintroducing remove, fix all the newly added or changed arrays
    //Single Arrays:
    q_list<bool> active;
    q_list<bool> culled;
    q_list<g_ptr<Single>> singles;
    list<g_ptr<Model>> models;
    q_list<glm::mat4> transforms;
    q_list<glm::mat4> endTransforms;
    q_list<AnimState> animStates;
    q_list<Velocity> velocities;
    q_list<P_State> physicsStates;
    q_list<CollisionLayer> collisonLayers;
    q_list<CollisionShape> collisionShapes;
    q_list<P_Prop> physicsProp;

    //Quad Arrays:
    q_list<bool> quadActive;
    q_list<bool> quadCulled;
    q_list<g_ptr<Quad>> quads;
    q_list<glm::mat4> guiTransforms;
    q_list<glm::mat4> guiEndTransforms;
    q_list<P_State> quadPhysicsStates;
    q_list<AnimState> quadAnimStates;
    q_list<Velocity> quadVelocities;
    q_list<CollisionLayer> quadCollisonLayers;
    q_list<P_Prop> quadPhysicsProp;
    q_list<list<std::string>> slots;

    //Misc Arrays:
    map<std::string,g_ptr<Model>> instanceModels;
    std::vector<std::string> instanceTypes;
    std::vector<g_ptr<Light>> lights;
    glm::mat4 lightSpaceMatrix;

    glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, -0.5f,-0.5f));
    glm::vec3 moonDir = glm::normalize(glm::vec3(-0.5f, -0.5f,-0.5f));
    glm::vec3 sunColor = glm::vec3(7.9f, 7.87f, 7.82f);
    glm::vec3 moonColor = glm::vec3(6.85f, 6.82f, 6.92f);

    Window& window;
    Camera camera;
    Shader singleShader;
    Shader depthShader;
    Shader instanceShader;
    Shader instanceDepthShader;
    Shader guiShader;

    float sceneTime = 0.0f;

    unsigned int depthMapFBO;
    unsigned int depthMap;

    Scene(Window& win,const Camera& cam,const Shader& singleShad,const Shader& instanceShad,
    const Shader& depthShad,const Shader& depthShadI,const Shader& guiShad) 
    : window(win), camera(cam), singleShader(singleShad), instanceShader(instanceShad), 
    depthShader(depthShad), instanceDepthShader(depthShadI), guiShader(guiShad) {};

    Scene(Window& _window,int _type)
    : window(_window),
      singleShader(
          Shader(readFile("../Engine/shaders/vertex.glsl").c_str(),
                 readFile("../Engine/shaders/fragment.glsl").c_str())
      ),
      instanceShader(
            Shader(readFile("../Engine/shaders/vertexI.glsl").c_str(),
                 readFile("../Engine/shaders/fragment.glsl").c_str())
      ),
      depthShader(
          Shader(readFile("../Engine/shaders/depth_vert.glsl").c_str(),
                 readFile("../Engine/shaders/depth_frag.glsl").c_str())
      ),
      instanceDepthShader(
          Shader(readFile("../Engine/shaders/depth_verti.glsl").c_str(),
                 readFile("../Engine/shaders/depth_frag.glsl").c_str())
      ),
      guiShader(
        Shader(readFile("../Engine/shaders/gui_vert.glsl").c_str(),
               readFile("../Engine/shaders/gui_frag.glsl").c_str())
      ),
      camera(45.0f, 1280.0f/768.0f, 0.1f, 1000.0f, 1)
    {
        if(_type==1)
        {
        camera.setPosition(glm::vec3(0.0f, 15.0f, 15.0f));
        setupShadows();
        }
        else if(_type==2)
        {
        camera = Camera(45.0f, 1280.0f/768.0f, 0.1f, 1000.0f, 4);
        camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
        }
        tickEnvironment(1100);
        setupShadows();
    }

    // Scene& operator=(const Scene& other)
    // {
    //     window = other.window;
        
    // }

    size_t nextId = 0;
    std::queue<size_t> freeIds;
    size_t getUid() {
    if (!freeIds.empty()) {
        size_t id = freeIds.front();
        freeIds.pop();
        return id;
    } else {
        return nextId++;
    }
    }



    virtual void removeObject(g_ptr<S_Object> obj) {}

    virtual void add(const g_ptr<S_Object>& sobj);

    // template<typename T, typename = std::enable_if_t<std::is_base_of_v<Golden::Object, T>>>
    // g_ptr<T> make(const std::string& type) {

    // }
    

    void deactivate(const g_ptr<S_Object>& sobj)
    {
        sobj->stop();
        if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
        {
            quadActive.get(sobj->ID,"scene::deactivate::214") = false;
            if(quad->lockToParent)
            {
             quad->callingChild=true;
             deactivate(quad->parent);
             quad->callingChild=false;
             return;
            }
            else if(!quad->children.empty())
            {
             for(auto c : quad->children)
             {
                 if(c->lockToParent&&!c->callingChild)
                 {
                    c->stop();
                    quadActive.get(c->ID,"scene::deactivate::214") = false;
                 }
             }
             }
        }
        else if(auto obj = g_dynamic_pointer_cast<Single>(sobj))
        {
            active.get(sobj->ID,"scene::deactivate::219") = false;
        }
    }

    void reactivate(const g_ptr<S_Object>& sobj)
    {
        sobj->resurrect();
        if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
        {
            quadActive.get(sobj->ID,"scene::reactivate::228") = true;
            if(quad->lockToParent)
            {
             quad->callingChild=true;
             reactivate(quad->parent);
             quad->callingChild=false;
             return;
            }
            else if(!quad->children.empty())
            {
             for(auto c : quad->children)
             {
                 if(c->lockToParent&&!c->callingChild)
                 {
                    c->resurrect();
                    quadActive.get(c->ID,"scene::deactivate::214") = true;
                 }
             }
             }
        }
        else if(auto obj = g_dynamic_pointer_cast<Single>(sobj))
        {
            active.get(sobj->ID,"scene::reactivate::233") = true;
        }
    }

    void relight(glm::vec3 direction,glm::vec4 color) {
        sunDir = direction;
        moonDir = direction;
        sunColor = color;
        moonColor = color;
    }

    vec3 getMousePos(float y_level = 0) {
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
    
        if (fabs(rayDir.y) < 1e-6) return glm::vec3(0); // Parallel, fail

        float t = (y_level - rayOrigin.y) / rayDir.y;
        return vec3(rayOrigin + rayDir * t);
    }

    vec2 mousePos2d()
    {
    int windowWidth = window.width;
    int windowHeight = window.height;
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);
    return vec2(xpos*2,ypos*2);
    }

    g_ptr<Quad> nearestElement(vec2 from = vec2(-10000,0))
    {
    //Inefficent and dangerous at this point, but it functions
    //A better option could be a dedicated selectable list, like in GUIDE, but that adds
    //overhead elsewhere
    vec2 pos = mousePos2d();
    if(from.x()!=-10000) pos = from;
    float closestDist = 10000;
    g_ptr<Quad> closest = nullptr;
    for(int i=0;i<quadActive.length();i++)
    {
        if(i>=quads.length()) break;
        auto qobj = quads.get(i,"gguim::nearestElement::514");
        if (!qobj->check("valid")) continue;
        if(!quadActive[i]) continue;

        if (qobj->pointInQuad(pos)) {
            return qobj;
        }
        else if(from.x()!=-10000){
            float dist = qobj->getPosition().distance(pos);
            if(dist<=closestDist) {
                closestDist = dist;
                closest = qobj;
            }
        }
    }
    return closest;
    }

    g_ptr<Quad> nearestWithin(float min = 5.0f)
    {
    vec2 pos = mousePos2d();
    float closestDist = min;
    g_ptr<Quad> closest = nullptr;
    for(int i=0;i<quadActive.length();i++)
    {
        if(i>=quads.length()) break;
        auto qobj = quads[i];
        if (!qobj->check("valid")) continue;
        if(!quadActive[i]) continue;

        if (qobj->pointInQuad(pos)) {
            return qobj;
        }
        else {
            float dist = qobj->getCenter().distance(pos);
            if(dist<=closestDist) {
                closestDist = dist;
                closest = qobj;
            }
        }
    }
    return closest;
    }

    void cullSinglesSimplePoint()
    {
        // Build VP once
        glm::mat4 VP = camera.getProjectionMatrix() * camera.getViewMatrix();

        for (size_t i = 0; i < singles.length(); ++i)
        {
            if (!active[i]) { culled[i] = true; continue; }

            // World position of the object origin (transform translation)
            // GLM matrices are column-major; translation is column 3.
            glm::vec3 worldPos = glm::vec3(transforms[i][3]);

            glm::vec4 clip = VP * glm::vec4(worldPos, 1.0f);

            // Behind camera or invalid
            if (clip.w <= 0.0f) { culled[i] = true; continue; }

            // OpenGL clip space frustum: -w..w for x,y,z
            // (equivalently NDC after divide: -1..1)
            bool inside =
                (clip.x >= -clip.w && clip.x <= clip.w) &&
                (clip.y >= -clip.w && clip.y <= clip.w) &&
                (clip.z >= -clip.w && clip.z <= clip.w);

            culled[i] = !inside;
        }
    }

    //Move an object between scenes, currently only works for singles
    void repo(const g_ptr<S_Object>& obj);


    class pool : public Object {
    public:
        pool() {
        }

        pool(g_ptr<Scene> _scene,const std::string& name,list<Script<>> _make_funcs) 
        : scene(_scene), pool_name(name), make_funcs(_make_funcs) {
            free_stack_top.store(0);
        }
        ~pool() {}

        std::string pool_name = "bullets";
        g_ptr<Scene> scene;
        list<Script<>> make_funcs;
        list<g_ptr<Object>> members;
        list<int> free_ids;
        std::atomic<size_t> free_stack_top{0};  // Points to next free slot
    
        int get_next() {
            size_t current_top, new_top;
            do {
                current_top = free_stack_top.load();
                if (current_top == 0) return -1;
                new_top = current_top - 1;
            } while (!free_stack_top.compare_exchange_weak(current_top, new_top));
            
            return free_ids.get(new_top);
        }
        
        void return_id(int id) {
            size_t current_top = free_stack_top.load();
            if (current_top < free_ids.size()) {
                // There's space, just write and increment pointer
                size_t slot = free_stack_top.fetch_add(1);
                free_ids.get(slot) = id;
            } else {
                // Need to grow the list
                free_ids.push(id);
                free_stack_top.fetch_add(1);
            }
        }
        

        g_ptr<Object> get() {
            int next_id = get_next();
            if(next_id!=-1)
            {
                auto obj = members.get(next_id,"pool::535");
                obj->recycled.store(false);
                return obj;
            }
            else
            {
                ScriptContext makeNew;
                make_funcs[0].run(makeNew);
                for(int i=1;i<make_funcs.length();i++) {
                    make_funcs[i].run(makeNew);
                }
                g_ptr<Object> new_item = makeNew.get<g_ptr<Object>>("toReturn");
                new_item->UUID = members.size();
                members.push(new_item);
                return new_item;
            }
        }

        void recycle(g_ptr<Object> item) {
            if(item->recycled.load()) {
                return;
            }
            if(auto s_obj = g_dynamic_pointer_cast<S_Object>(item)) {
                scene->deactivate(s_obj);
            }

            item->recycled.store(true);
            return_id(item->UUID);
        }
    };

    map<std::string,g_ptr<pool>> pools;

    void define(const std::string& type, Script<> make_func) {
        if(!pools.hasKey(type))
        {
            pools.put(type,make<pool>(this,type,list<Script<>>{make_func}));
        }
        else
        {
           pools.get(type)->make_funcs.push(make_func);
        }
    }

    void define(const std::string& type, list<Script<>> make_funcs) {
        for(auto f : make_funcs) define(type,f);
    }


    //I liked "make" better but that conflicts with Object
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Golden::Object, T>>>
    g_ptr<T> create(const std::string& type) {
        if(pools.hasKey(type))
        {
            g_ptr<Object> obj = pools.get(type)->pool::get();
            if(auto cobj = g_dynamic_pointer_cast<T>(obj))
            {
                cobj->dtype = type;
                reactivate(cobj);
                return cobj;
            }
            return nullptr;
        }
        else
        {
            print("scene::create::641 attempted to create an undefined type!");
            return nullptr;
        }
    }

    /// @brief Deactivates an object and puts it back in the Object pool
    /// @param item You can call recycle on this directly
    /// @param type Not nessecary if a dtype was already specified
    void recycle(g_ptr<Object> item, const std::string& type = "undefined") {
        std::string useType = type;
        if(type=="undefined")
        {
            useType = item->dtype;
        }
        if(pools.hasKey(useType))
        {
            pools.get(useType)->recycle(item);
        }
        else
        {
            print("scene::recycle::601 attempted to recycle an undefined type: ",type,"!");
        }
    }
    


    virtual void updateScene(float tpf);

    virtual void tickEnvironment(int time);

    void setupShadows();

    void saveBinary(const std::string& path) const;
    void loadBinary(const std::string& path);

    void saveBinary(std::ostream& out) const;
    void loadBinary(std::istream& in);

private:
    struct w_part {
        w_part(const std::string& _id, const std::string& _slot, g_ptr<Quad> _g) : 
        id(_id), slot(_slot), g(_g) {
        auto l = split_str(id,'_');
        type = l[0];
        isText = l[l.length()-1] == "text";
        if(isText) itemID = l[l.length()-2];
        else itemID = l[l.length()-1];
        //print(id,": ",l[l.length()-1],isText);
        }
        w_part() {}
        std::string id;
        std::string slot;
        g_ptr<Quad> g;
        std::string type;
        std::string itemID = "none";
        bool isText = false;
    };

public:
    map<std::string,list<g_ptr<Quad>>> loaded_slots;
    list<uint32_t> fired;
    list<uint32_t> prevFired;

    list<g_ptr<Quad>> getSlot(const std::string& slot)
    {
        if(loaded_slots.hasKey(slot)) return loaded_slots.get(slot);
        else return list<g_ptr<Quad>>{};
    }

    bool hasSlot(const std::string& slot)
    {
        return loaded_slots.hasKey(slot);
    }

    void advanceSlots()
    {
        prevFired.clear();
        prevFired <= fired;
    }

    bool slotFired(const std::string& slot)
    {
        uint32_t hash = loaded_slots.hashString(slot);
        return fired.has(hash);
    }

    bool slotJustFired(const std::string& slot)
    {
        uint32_t hash = loaded_slots.hashString(slot);
        return fired.has(hash)&&!prevFired.has(hash);
    }

    void fireSlots(g_ptr<Quad> g)
    {
        for(auto s : g->getSlots())
        {
            uint32_t hash = loaded_slots.hashString(s);
            if(s!="all") 
            {
                if(!fired.has(hash))
                    fired.push(hash);  
            }
        }
    }

    g_ptr<Quad> loadFab(Q_Fab& fab)
    {
        if(fab.type=="none"||fab.type=="image")
        {
            auto q = make<Quad>();
            q->color = fab.color;
            add(q);

            if(fab.type=="image")
            {
            int w,h;
            q->setTexture(loadTexture2D(fab.filePath,true,w,h),0);
            }

            q->setPosition(fab.pos);
            q->scale(fab.scale);
            for(std::string s : fab.slots)
            {
                if(loaded_slots.hasKey(s))
                loaded_slots.get(s).push(q);
                else loaded_slots.put(s,list<g_ptr<Quad>>{q});

                if(s.at(0)=='_')
                {
                    auto l = split_str(s,'_');
                    if(l[1]=="wdgt")
                    {
                       std::string compID = "";
                       for(int i=2;i<l.length()-1;i++)
                       {
                        if(i>2) compID.append("_");
                         compID.append(l[i]);
                       }
                       w_part w(compID,s,q);
                       if(partBin.hasKey(compID))
                       {
                       // print("Has comp: ",compID);
                        //This actually means there's a duplicate, handle collison logic here
                        partBin.get(compID) << w;
                       } 
                       else {partBin.put(compID,list<w_part>{w});}

                       if(l.length()>=6)
                       {
                            std::string parentID = "";
                            for(int i=2;i<l.length()-2;i++)
                            {
                            if(i>2) parentID.append("_");
                            parentID.append(l[i]);
                            }
                            // print(compID,", Parent: ",parentID);
                            if(partBin.hasKey(parentID))
                            {
                                partBin.get(parentID) << w;
                            } 
                            else {partBin.put(parentID,list<w_part>{w});}
                       }
                    }
                }
            }
            q->getSlots() <= fab.slots;
            return q;
        }
        else if (fab.type=="text")
        {
            auto font = make<Font>();
            std::string fontID = fab.filePath+std::to_string(fab.scale.x());
            if(has(fontID)) font = get<g_ptr<Font>>(fontID);
            else {
                font->loadFromTTF(fab.filePath,fab.scale.x());
                set<g_ptr<Font>>(fontID,font);
            }
            g_ptr<Quad> first = text::makeText(fab.text,font,this,fab.pos,fab.scale.y());

            for(std::string s : fab.slots)
            {
                if(loaded_slots.hasKey(s))
                loaded_slots.get(s).push(first);
                else loaded_slots.put(s,list<g_ptr<Quad>>{first});

                if(s.at(0)=='_')
                {
                    auto l = split_str(s,'_');
                    if(l[1]=="wdgt")
                    {
                       std::string compID = "";
                       for(int i=2;i<l.length()-1;i++)
                       {
                        if(i>2) compID.append("_");
                         compID.append(l[i]);
                       }
                       w_part w(compID,s,first);
                    //    if(partBin.hasKey(compID))
                    //    {
                    //    // print("Has comp: ",compID);
                    //     //This actually means there's a duplicate, handle collison logic here
                    //     partBin.get(compID) << w;
                    //    } 
                    //    else {partBin.put(compID,list<w_part>{w});}

                       if(l.length()>=6)
                       {
                            std::string parentID = "";
                            for(int i=2;i<l.length()-2;i++)
                            {
                            if(i>2) parentID.append("_");
                            parentID.append(l[i]);
                            }
                            // print(compID,", Parent: ",parentID);
                            if(partBin.hasKey(parentID))
                            {
                                partBin.get(parentID) << w;
                            } 
                            else {partBin.put(parentID,list<w_part>{w});}
                       }
                    }
                }
            }

            first->getSlots() <= fab.slots;
            //Could also store slots in chars, I'll see what I like most later
            return first;
        }
        else 
        {print("Unrecognized type in GGUIM::loadFab");}
        return make<Quad>();
    }

    g_ptr<Quad> loadFab(const std::string& path)
    {
        Q_Fab fab(path);
        return loadFab(fab);
    }

    void saveQFabList(const std::string& path,list<Q_Fab>& fabs) {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write Q_Fab list");
        uint32_t count = fabs.length();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (auto fab : fabs) fab.saveBinary(out);
        out.close();
    }
    
    list<Q_Fab> loadQFabList(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read Q_Fab list");
        uint32_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        list<Q_Fab> fabs;
        for(int i=0;i<count;i++)
        {
            Q_Fab fab;
            fab.loadBinary(in);
            loadFab(fab);
            fabs << fab;
        }
        in.close();
        loadWidgets();
        return fabs;
    }

    g_ptr<Quad> makeImageQuad(const std::string& path,float scalar=1.0f)
    {
        //Once we fiqure out how to sort GAM
        g_ptr<Quad> q = make<Quad>();
        q->set<std::string>("image",path);
        add(q);
        int w,h;
        q->setTexture(loadTexture2D(path,true,w,h),0);
        q->scale(vec2(w*scalar,h*scalar));
        return q;
    }

private:
    using itms = g_ptr<d_list<g_ptr<Quad>>>;
    using itmsM = d_list<g_ptr<Quad>>;
    map<std::string,list<std::string>> wdgtRec;
    map<std::string,list<w_part>> partBin;

    void loadWidgets()
    {
        for(auto part : partBin.keySet())
        {
            auto l = split_str(part,'_');
           // print("Checking: ",part," bin size: ",partBin.get(part).length());
            if(l[0]=="base") base(part,partBin.get(part));
            else print("scene::loadWidget::789 unkown widget tpye ",l[0]);
        }
    }

    void set_valid(g_ptr<Quad> g,bool valid)
    {
        if(!g->children.empty())
        {
            g->set<bool>("valid",valid);
            for(auto c : g->children)
            {
                c->set<bool>("valid",valid);
            }
        }
        else if(g->lockToParent)
        {
            if(g->parent)
            {
            for(auto c : g->parent->children)
            {
                c->set<bool>("valid",valid);
            }
            }
        }
        else {
            g->set<bool>("valid",valid);
        } 
    }

    void houseKeepSlots(g_ptr<Quad> item,const std::string& start)
    {
        if(item->check("exporting")) return;
        std::string exportSlot = start;
        if(!item->has("export_slot_loc")) 
        {
            bool found = false;
            for(int i=0;i<item->getSlots().length();i++)
            {
                if(item->getSlots()[i].at(0)=='_')
                {
                    auto l = split_str(item->getSlots()[i],'_');
                    if(l[1]=="wdgt")
                    {
                    found = true;
                    item->set<size_t>("export_slot_loc",i);
                    item->getSlots().get(i,"scene::houseKeepSlots::857") 
                    = exportSlot;
                    break;
                }
                }
            }

            if(!found)
            {
            item->set<size_t>("export_slot_loc",item->getSlots().length());
            item->getSlots() << exportSlot;
            }
        }
        else
        {
            item->getSlots().get(item->get<size_t>("export_slot_loc"),"scene::houseKeepSlots::869") 
            = exportSlot;
        }
    }

    void base(const std::string& parentID, list<w_part> parts)
    {
        g_ptr<Quad> g = nullptr;
        g_ptr<Quad> g_txt = nullptr;
        w_part parent;
        w_part text_parent;
        auto items = make<itmsM>();
        for(auto w : parts)
        {
            //print(parentID,"   : ",w.id);
            if(w.id==parentID) {
                g = w.g;
                parent = w;
            }
            else if(w.isText)
            {
                g_txt = w.g;
                text_parent = w;
            }
            else {
                items->push(w.g);
            }
        }
        if(!g) {
            print("scene::base::833 No parent found for part set ",parentID);
            return;
        }
        // if(!g_txt) {
        //     print("scene::base::869 No text found for part set ",parentID);
        //     return;
        // }
        g->set<std::string>("label",parent.id);
        g->flagOff("click_switch");
        g->add<itms>("items",items);
        auto l = split_str(parent.itemID,'|');
        // print(parent.itemID)
        if(l.length()>1)
        {
            auto pos = split_str(l[1],',');
            if(pos.length()>1)
            g->set<vec2>("pos_dif",vec2(std::stof(pos[0]),std::stof(pos[1])));

            auto scale = split_str(l[2],',');
            if(scale.length()>1)
            g->set<vec2>("scale_dif",vec2(std::stof(scale[0]),std::stof(scale[1])));
            else
            g->set<vec2>("scale_dif",vec2(0,0));
        }

        std::string ext_label = "_wdgt_base_";
        g->set<std::string>("ext_label",ext_label);
    
        std::string int_label = parent.id;
        g->set<std::string>("int_label",int_label);

        std::string base_label = parent.id;
        g->set<std::string>("base_label",base_label);

        houseKeepSlots(g,ext_label+int_label);

        if(g_txt)
        {
        g->set<g_ptr<Quad>>("text",g_txt);
        set_valid(g_txt,false);
        g_txt->addChild(g,true);
        std::string ext_label_text = "_wdgt_base_";
        g_txt->set<std::string>("ext_label",ext_label_text);

        std::string int_label_text = text_parent.id+"_text";
        g_txt->set<std::string>("int_label",int_label_text);

        std::string base_label_text = text_parent.id+"_text";
        g_txt->set<std::string>("base_label",base_label_text);

        houseKeepSlots(g_txt,ext_label_text+int_label_text);
        }

        g->addScript("onClick",[g,this](ScriptContext& ctx){
            if(g->toggle("click_switch"))
            {
                g->run("onClose");
            }
            else
            {
                g->run("onOpen");
            }
        });

        g->addScript("onOpen",[g,this](ScriptContext& ctx){
            g->flagOn("open");
            auto items = g->get<itms>("items");
            if(!items->empty())
            for(int i=0;i<items->length();i++)
            {
                    auto item = (*items)[i];
                    std::string current_base_label = item->get<std::string>("base_label");
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
                        houseKeepSlots(i_txt,item->get<std::string>("ext_label")+item->get<std::string>("int_label")+"_text");
                    }
                    houseKeepSlots(item,item->get<std::string>("ext_label")+item->get<std::string>("int_label"));

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
            if(!g->isActive()) {
                reactivate(g);
            }
        });

        g->addScript("onDeactivate",[g,this](ScriptContext& ctx){
            print("Deactivating a quad, is active? ",g->isActive());
            if(g->isActive()) deactivate(g);
            g->run("onClose");
            auto items = g->get<itms>("items");
            for(int i=0;i<items->length();i++)
            {
                    auto item = (*items)[i];
                    item->run("onDeactivate");
            }
            g->flagOn("click_switch");
        });

        //addExportScripts(g);

        //Deactivate it so we don't start with it on
        if(g->isActive()) g->run("onDeactivate");
        //g->flagOn("locked");
        //And lock as well to keep positioning right

        g->addScript("onExport",[g,this](ScriptContext& ctx){
            if(!g->check("open")) g->run("onOpen");
        });
    }

public:

    g_ptr<Quad> openWidget(std::string type,bool toggle = false,const vec2& pos = vec2(-10000,0))
    {
        if(hasSlot(type))
        {
            auto g = getSlot(type)[0];
            if(!g->isActive()) {
                if(g->scripts.hasKey("onActivate")) {
                    g->run("onActivate");
                } else {
                    reactivate(g);
                }
            }
            else if(toggle) {
                closeWidget(type);
            }
            if(pos.x()!=-10000) g->setPosition(pos);
            return g;
        }
        print("gguim::openWidget::479 Widget ",type," does not exist");
        return nullptr;
    }

    void closeWidget(std::string type)
    {
        if(hasSlot(type))
        {
            auto g = getSlot(type)[0];
            if(g->scripts.hasKey("onDeactivate")) {
                g->run("onDeactivate");
            } else {
                deactivate(g);
            }
        }
    }

};

}
