#pragma once

#include<functional>
#include<rendering/scene.hpp>
#include<core/input.hpp>
#include<util/files.hpp>
#include<core/grid.hpp>
#include<gui/twig.hpp>

namespace Golden
{
    namespace helper
    {
        bool pressed(KeyCode code) {return Input::get().keyJustPressed(code);}
        bool held(KeyCode code) {return Input::get().keyPressed(code);}
        const char n_pressed = '\t';
        int currently_pressed_keycode() {
            list<int> pressed = Input::get().justPressed();
            return pressed.empty() ? -1 : pressed[0];
        }
        char currently_pressed() {
            char c = n_pressed;
            keycodeToChar(currently_pressed_keycode(),held(LSHIFT),c);
            return c;
        }

        void check_loaded(list<std::string> needed,g_ptr<Scene> scene) {
            for(auto s : needed)
                if(!scene->hasSlot(s)) print("helper::check_loaded::17 Missing GUI element: ",s);
        }
        std::string path_to_root() {
            std::filesystem::path exePath = std::filesystem::current_path();
            while (exePath != exePath.root_path()) {
                if (std::filesystem::exists(exePath / "Projects")) {
                    return exePath.string();
                }
                exePath = exePath.parent_path();
            }
            return "../"; // fallback
        }

        std::string path_to_gui(const std::string& project,const std::string& filename)
        {
            std::string start = root()+"/Projects/";
            std::string file = filename;
            size_t dot = filename.find('.');
            if (dot != std::string::npos) {
                file = filename.substr(0,dot);
            }
            return start+project+"/assets/gui/"+file+".fab";
        }

        void load_gui(g_ptr<Scene> scene,const std::string& project,const std::string& filename)
        {
            scene->loadQFabList(path_to_gui(project,filename));
        }

        Data make_config(g_ptr<Scene> scene,int exit_key)
        {
            Data d;
            d.set<g_ptr<Scene>>("scene",scene);
            d.set<int>("exit_key",exit_key);
            return d;
        }
    
  
        vec2 input_2d_arrows(float s)
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            Input& input = Input::get();

            if(input.keyPressed(UP)) z-=1.0;
            if(input.keyPressed(DOWN)) z+=1.0f;
            if(input.keyPressed(LEFT)) x-=1.0f;
            if(input.keyPressed(RIGHT)) x+=1.0f;
            return vec2(x*s,z*s);
        }

        vec3 input_3d_arrows(float s) {
            vec2 v = input_2d_arrows(s);
            return vec3(v.x(),0,v.y());
        }

        vec2 input_2d_keys(float s)
        {
            float x = 0.0f;
            float z = 0.0f;
            Input& input = Input::get();

            if(input.keyPressed(W)) z-=1.0f;
            if(input.keyPressed(S)) z+=1.0f;
            if(input.keyPressed(A)) x-=1.0f;
            if(input.keyPressed(D)) x+=1.0f;
            return vec2(x*s,z*s);
        }

        vec3 input_3d_keys(float s) {
            vec2 v = input_2d_keys(s);
            return vec3(v.x(),0,v.y());
        }

        //Requires physics
        void move_fps(float s,g_ptr<Single> one)
        {
        vec2 mIn = input_2d_keys(s);
        one->setLinearVelocity(
            (one->facing()*-mIn.y())+
            (one->right()*-mIn.x()));
        }

        vec3 dir_fps(float s,g_ptr<Single> one)
        {
        vec2 mIn = input_2d_keys(s);
        return(
            (one->facing()*-mIn.y())+
            (one->right()*-mIn.x()));
        }

        struct S_Tool
        {
            float tpf = 0.1; float frametime = 0; int frame = 0;
            std::chrono::steady_clock::time_point last = std::chrono::high_resolution_clock::now();
            int frames = 0;
            float pause = 0.0f;
            bool log_fps = false;

            void tick() {
                auto end = std::chrono::high_resolution_clock::now();
                auto delta = std::chrono::duration<float>(end - last);
                tpf = delta.count(); last = end; frame++; frames++;
                frametime+=tpf; frame++;
                if(frametime>=1) {
                    if(log_fps)
                        std::cout << frames << " FPS" << std::endl;
                    frametime=0;
                    frames=0;
                }
            }
        };

        class TextEditor : public Object {
        private:
            std::string clipboard;
        public:
            g_ptr<Quad> cursor = nullptr;
            g_ptr<Text> twig = nullptr;

            g_ptr<Quad> selectionStart = nullptr;
            g_ptr<Quad> selectionEnd = nullptr;

            int last_keycode = 0;
            
            float pause = 0;
        
            void clearSelection() {
                selectionStart = nullptr;
                selectionEnd = nullptr;
                //Add charachter unhilghiting
            }


            void close() {
                if(cursor&&twig) 
                    twig->removeChar(cursor);

                twig = nullptr;
                cursor = nullptr;
            }

            void move_cursour(g_ptr<Quad> sel = nullptr) {
                if(!twig) return;
                if(cursor)
                    twig->removeChar(cursor);

                int at = 0;
                if(sel) {
                    at = twig->chars.find(sel);
                }
                
                cursor = twig->insertChar('|',at);
            }

            void click_move(g_ptr<Quad> sel = nullptr) {
                //clearSelection();
                move_cursour(sel);
            }

            void open(g_ptr<Text> _twig, g_ptr<Quad> sel = nullptr) {
                twig = _twig;
                move_cursour(sel);
            }

            void tick(float tpf) {
                if(!twig) return;

                char current_char = currently_pressed();
                int current_keycode = currently_pressed_keycode();

                auto checkArrow = [&](KeyCode key, g_ptr<Quad> next) {
                    bool proceed = false;
                    if(pressed(key)) {
                        pause = 0.6f;
                        proceed = true;
                    } else if(pause <= 0 && held(key)) {
                        proceed = true;
                    }

                    if(proceed) {
                        if(held(LSHIFT)) {
                            if(key==RIGHT) {       
                                if(!selectionStart||last_keycode==LEFT) 
                                    selectionStart = cursor->parent;
                                selectionEnd = cursor->children[0];
                            }
                            else if(key==LEFT) {
                                if(!selectionEnd||last_keycode==RIGHT) 
                                    selectionEnd = cursor->parent;
                                selectionStart = cursor->parent->parent;
                            }
                        } 
                        move_cursour(next);
                    }
                };

                checkArrow(RIGHT,cursor->children[0]);
                checkArrow(LEFT,cursor->parent->parent);
                pause -= tpf;


                if(held(CMD)) {
                    if(held(CMD) && pressed(C)) {
                        if(selectionStart && selectionEnd) {
                            std::string copied;
                            g_ptr<Quad> walker = selectionStart;
                            while(walker && walker != selectionEnd) {
                                if(walker->opt_char != '|') {
                                    copied += walker->opt_char;
                                }
                                walker = walker->children.empty() ? nullptr : walker->children[0];
                            }
                            clipboard = copied;
                        }
                        clearSelection();
                    }
            
                    if(held(CMD) && pressed(V)) {
                        g_ptr<Quad> last = cursor;
                        twig->insertText(clipboard,cursor->parent->parent);
                        clearSelection();
                    }
                }
                else {
                    if(current_keycode==BACKSPACE) {
                        twig->removeChar(cursor->parent->parent);
                    }
                    else if(current_char!=n_pressed) {
                        twig->insertChar(current_char,cursor->parent->parent);
                    }

                    if(current_keycode!=-1&&!held(LSHIFT)) {
                        clearSelection();
                    }
                }

                if(current_char!=n_pressed)
                    last_keycode = current_keycode;
            }
        };

    }
    namespace start
    {

        void define_objects(g_ptr<Scene> scene,const std::string& project_name,g_ptr<Grid> level = nullptr) {
            map<std::string,std::string> type_modelPath;
            std::string src = root();
        
            auto is_model = [](const std::string& filename) -> std::string{ 
                auto split = split_str(filename,'.');
                if(split.length()>0)
                {
                    if(split[split.length()-1]=="glb") {
                        std::string toReturn = "";
                        for(int i=0;i<split.length()-1;i++)
                        {
                            toReturn.append(split[i]);
                        }
                        return toReturn;
                    }
                }
                return "[NULL]";
            };
        
            auto process_files = [&is_model,&type_modelPath](const std::string& path) -> void{ 
                auto files = list_files(path);
                for(auto f : files)
                {
                list<std::string> split = split_str(f,'/');
                std::string filename = split[split.length()-1];
                std::string name = is_model(filename);
                if(name!="[NULL]") {
                    type_modelPath.put(name,f);
                }
                }
            };
        
            std::function<void(const std::string&)> process_directory_recursive = [&](const std::string& path) -> void {
                process_files(path);
                auto subdirs = list_subdirectories(path, false);
                for(const auto& subdir : subdirs) {
                    process_directory_recursive(subdir);
                }
            };
            process_directory_recursive(src+"/Projects/"+project_name+"/assets/models/");
        
            std::string data_string = "NONE";
            try {
            data_string = readFile(src+"/Projects/"+project_name+"/assets/models/"+project_name+" - data.csv");
            }
            catch(std::exception e)
            {
                print("No - data.csv provided for ",project_name," this is required for object definitions to work");
            }
            std::string cleaned_data = data_string;
            size_t pos = 0;
            while((pos = cleaned_data.find("\r\n", pos)) != std::string::npos) {
                cleaned_data.replace(pos, 2, "\n");
                pos += 1;
            }
            auto lines = split_str(cleaned_data, '\n');
            list<std::string> headers;
            list<std::string> types;
            for(int i=0;i<2;i++)
            {
                for(auto s : split_str(lines[i],',')) {
                    if(i==0) {
                        headers << s;
                    }  
                    else if(i==1) {
                        types << s;
                    }
                }
            }
        
        
            for(auto entry : type_modelPath.entrySet())
            {
                std::string type = entry.key;
                std::string path = entry.value;
                scene->set<g_ptr<Model>>("_"+type+"_model",make<Model>(path));

                Script<> make_part("make_"+type,[scene,level,type,lines,headers,types](ScriptContext& ctx){
                    auto model = scene->get<g_ptr<Model>>("_"+type+"_model");
                    // auto data = scene->get<g_ptr<q_data>>("_"+type+"_data");
                    auto modelCopy = make<Model>();
                    modelCopy->copy(*model);
                    auto part = make<Single>(modelCopy);
                    scene->add(part);
                    
                    for(int i=2;i<lines.length();i++)
                    {
                        list<std::string> values = split_str(lines[i],',');
                        if(values[0]==type) {
                            for(int t = 0;t<values.length();t++)
                            {
                                std::string t_type = types[t];
                                if(values[t]=="") continue;
                                if(t_type=="string") part->add<std::string>(headers[t],values[t]);
                                else if (t_type=="int") part->add<int>(headers[t],std::stoi(values[t]));
                                else if (t_type=="float") part->add<float>(headers[t],std::stof(values[t]));
                                else if(t_type=="bool") part->add<bool>(headers[t],values[t]=="true"?1:0);
                                else if (t_type=="vec4") {
                                    list<std::string> sub_sub = split_str(values[t],':');
                                    vec4 v(std::stof(sub_sub[0]),std::stof(sub_sub[1]),std::stof(sub_sub[2]),std::stof(sub_sub[3]));
                                    part->add<vec4>(headers[t],v);
                                }
                                else if (t_type=="intlist") {
                                    list<int> moves;
                                    list<std::string> sub = split_str(values[t],'|');
                                    for(int e = 0;e<sub.length();e++) {
                                       moves << std::stoi(sub[e]);
                                    }
                                    part->add<list<int>>(headers[t],moves);
                                }
                                else if (t_type=="vec2list") {
                                    list<ivec2> moves;
                                    list<std::string> sub = split_str(values[t],'|');
                                    for(int e = 0;e<sub.length();e++) {
                                        list<std::string> sub_sub = split_str(sub[e],':');
                                        ivec2 v(std::stof(sub_sub[0]),std::stof(sub_sub[1]));
                                        moves << v;
                                    }
                                    part->add<list<ivec2>>(headers[t],moves);
                                }
                            }
                        }
                    }
                    if(level)
                    {
                        part->setPosition(level->snapToGrid(scene->getMousePos()));
                        list<Cell> myCells = level->cellsAround(part->getPosition(),((part->getModel()->localBounds.getSize().x()-1)/2));
                        for(auto cell : myCells)
                        {
                            cell->push(part);
                        }
                        part->set<list<Cell>>("_cells",myCells);
                    }
                    ctx.set<g_ptr<Object>>("toReturn",part);
            });
            scene->define(type,make_part);
            }
        }

        /// @brief A helper function if you don't want to do the boilerplate each time
        /// @param gameLogic The main logic to run
        /// @param window The window in which to run it
        /// @param config Here's where you pass the important things, scene, gguim, etc...
        void run(Window& window,Data& config,std::function<void()> gameLogic) {
            g_ptr<Scene> scene = nullptr;
            int exit_key = -1;
            if(config.has("scene")) scene = config.get<g_ptr<Scene>>("scene");
            if(config.has("exit_key")) exit_key = config.get<int>("exit_key");

            bool disable_click = config.check("disable_click");

            bool has_exit_key = exit_key!=-1;
            while (!window.shouldClose()) {
                if(has_exit_key)
                    if(Input::get().keyJustPressed(exit_key)) return;
                if(scene&&!disable_click) {
                    if(auto g = scene->nearestElement())
                    {
                        if(helper::pressed(MOUSE_LEFT))
                        {
                            g->run("onClick");
                            scene->fireSlots(g);
                        }
                    }
                }
                gameLogic();
                if(scene) scene->updateScene(1.0f);
                if(scene) scene->advanceSlots();
                window.swapBuffers();
                window.pollEvents();
            }
            glfwTerminate();
        }
    }
}