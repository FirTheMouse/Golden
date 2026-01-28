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
        bool pressed(int code) {return Input::get().keyJustPressed(code);}
        bool held(int code) {return Input::get().keyPressed(code);}
        const char n_pressed = '\t';
        int currently_pressed_keycode() {
            list<int> pressedKeys = Input::get().pressed();
            return pressedKeys.empty() ? -1 : pressedKeys[0];
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

            list<g_ptr<Quad>> debug;

            int last_keycode = 0;
            int true_last_keycode = 0;
            
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

                // int at = 0;
                // if(sel) {
                //     at = twig->chars.find(sel);
                // }
                
                cursor = twig->insertChar('|',sel);
            }

            void click_move(g_ptr<Quad> sel = nullptr) {
                //clearSelection();
                move_cursour(sel);
            }

            void open(g_ptr<Text> _twig, g_ptr<Quad> sel = nullptr) {
                twig = _twig;
                move_cursour(sel);
            }

            struct KeyState {
                KeyState() {}
                float timer = 0.0f;
                bool wasPressed = false;
            }; 
            map<int,KeyState> keyStates;
            bool shouldTrigger(int keycode,float repeat_delay = 0.6f,float repeat_rate = 0.05f) {
                if(!held(keycode)) {
                    keyStates.remove(keycode);  // Clean up released keys
                    return false;
                }

                auto& state = keyStates.getOrPut(keycode, KeyState{});

                if(pressed(keycode)) {
                    state.timer = repeat_delay;
                    state.wasPressed = true;
                    return true;
                }
                
                if(state.timer <= 0) {
                    state.timer = repeat_rate;
                    return true;
                }

                return false;
            } 



            map<int,float> keyPause;

            void tick(float tpf) {
                if(!twig) return;

                //Debug boxes to show the cursour's parent and child
                    // if(cursor) {
                    //     for(int i=0;i<3;i++) {
                    //         g_ptr<Quad> ind;
                    //         if(i>=debug.length()) {
                    //             ind = make<Quad>();
                    //             twig->scene->add(ind);
                    //             debug << ind;
                    //         }
                    //         else 
                    //             ind = debug[i];
                    //         ind->scale({10,10});
                    //         g_ptr<Quad> next = nullptr;
                    //         if(i==0) {
                    //             next = cursor->parent;
                    //             ind->setColor(vec4(0,0,1,1));
                    //         }
                    //         else if(i==1) {
                    //             if(cursor->parent) {
                    //                 next = cursor->parent->parent;
                    //                 ind->setColor(vec4(0,0,1,1));
                    //             }
                    //         }
                    //         else if(i==2) {
                    //             next = cursor->children[0];
                    //             ind->setColor(vec4(1,0,0,1));
                    //         }

                    //         if(next) {
                    //             ind->setPosition(next->getPosition().addY(next->opt_offset.y()));
                    //         }
                    //     }
                    // } else {
                    //     for(auto d : debug) {
                    //         twig->scene->recycle(d);
                    //     }
                    // }

                if(!cursor->children.empty()&&shouldTrigger(RIGHT,0.4f,0.02f)) {
                    if(held(LSHIFT)) {  
                        if(!selectionStart) 
                            selectionStart = cursor->parent;
                        selectionEnd = cursor->children[0];
                    } 
                    move_cursour(cursor->children[0]);
                }

                if(cursor->parent&&shouldTrigger(LEFT,0.4f,0.02f)) {
                    if(held(LSHIFT)) {  
                        if(!selectionEnd) 
                            selectionEnd = cursor->parent;
                        selectionStart = cursor->parent->parent;
                    }
                    move_cursour(cursor->parent->parent);
                }


                g_ptr<Quad> at = cursor->parent;
                // if(!at&&!twig->chars.empty()) 
                //     at = twig->chars[0];

                if(shouldTrigger(BACKSPACE,0.5f,0.03f)) {
                    twig->removeChar(at);
                }

                int main_current_keycode = currently_pressed_keycode();
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
                        twig->insertText(clipboard,at);
                        clearSelection();
                    }
                }
                else if(main_current_keycode!=-1) {
                    list<int> pressedKeys = Input::get().pressed();
                    for(auto current_keycode : pressedKeys) {
                        char current_char = n_pressed;
                        keycodeToChar(current_keycode,held(LSHIFT),current_char);
                        bool proceed = false;
                        proceed = shouldTrigger(current_keycode);
                        if(proceed) {
                            if(current_char!=n_pressed) {
                                if(cursor->parent) {
                                    twig->insertChar(current_char,at);
                                } else { //Fallback for empty boxes or when the cursor needs to make the anchor
                                    g_ptr<Quad> n_anchor = twig->insertChar(current_char,0);
                                    move_cursour(n_anchor);
                                }

                            }
                        }
                    }

                    if(main_current_keycode!=-1&&!held(LSHIFT)) {
                        clearSelection();
                    }
                    true_last_keycode = main_current_keycode;
                    if(main_current_keycode!=-1)
                        last_keycode = main_current_keycode;

                }

                for(auto key : keyStates.keySet()) {
                    KeyState& state = keyStates.get(key);
                    if(state.timer>=0)
                        state.timer -= tpf;
                }
                    
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