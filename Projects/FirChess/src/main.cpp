#include<core/helper.hpp>
#include<core/numGrid.hpp>
#include<util/meshBuilder.hpp>
#include<core/type.hpp>

using namespace Golden;


g_ptr<Scene> scene = nullptr;
g_ptr<NumGrid> grid = nullptr;


list<g_ptr<Single>> white_losses;
list<g_ptr<Single>> black_losses;

list<std::string> dtypes;
list<g_ptr<Single>> ref;
list<bool> captured;
list<int> colors;
list<int> values;
list<int> specialRules;
list<list<vec2>> moves;

void type_define_objects(g_ptr<NumGrid> level = nullptr,const std::string& project_name = "FirChess") {
    map<std::string,std::string> type_modelPath;

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
    process_directory_recursive("../Projects/"+project_name+"/assets/models/");

    std::string data_string = "NONE";
    try {
    data_string = readFile("../Projects/"+project_name+"/assets/models/"+project_name+" - data.csv");
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
        
        Script<> make_part("make_"+type,[level,type,lines,headers,types](ScriptContext& ctx){
            auto model = scene->get<g_ptr<Model>>("_"+type+"_model");
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
                    else if (t_type=="vec2list") {
                        list<vec2> moves;
                        list<std::string> sub = split_str(values[t],'|');
                        for(int e = 0;e<sub.length();e++) {
                            list<std::string> sub_sub = split_str(sub[e],':');
                            vec2 v(std::stof(sub_sub[0]),std::stof(sub_sub[1]));
                            moves << v;
                        }
                        part->add<list<vec2>>(headers[t],moves);
                    }
                }
            }
            }

            if(level)
            {
                part->setPosition(level->snapToGrid(scene->getMousePos()));
                list<int> myCells = level->cellsAround(part->getPosition(),((part->model->localBounds.getSize().x-1)/2));
                for(auto cell : myCells)
                {
                    grid->cells[cell].push(part->ID);
                }
                part->set<list<int>>("_cells",myCells);
            }
            ctx.set<g_ptr<Object>>("toReturn",part);
    });
    scene->define(type,make_part);
    }
}

bool isMultiple(vec2 move, vec2 pattern) {
    if(pattern.x() == 0 && pattern.y() == 0) return false;
    if(pattern.x() == 0) return move.x() == 0 && ((int)move.y()%(int)pattern.y()) == 0 && (move.y() / pattern.y()) > 0;
    if(pattern.y() == 0) return move.y() == 0 &&(int)move.x() % (int)pattern.x() == 0 && (move.x() / pattern.x()) > 0;
    return (int)move.x() % (int)pattern.x() == 0 && (int)move.y() % (int)pattern.y() == 0 && 
           (move.x() / pattern.x()) == (move.y() / pattern.y()) && (move.x() / pattern.x()) > 0;
}

void update_cells(g_ptr<Single> selected) {
    list<int> myCells = grid->cellsAround(selected->getPosition(),((selected->model->localBounds.getSize().x-1)/2));
    list<int> oldCells = selected->get<list<int>>("_cells");
    for(auto cell : oldCells)
    {
        grid->cells[cell].erase(selected->ID);
    }
    for(auto cell : myCells)
    {
        grid->cells[cell].push(selected->ID);
    }
    selected->set<list<int>>("_cells",myCells);
}

//poached code from the Physics class meant to be use multi-threaded but just raw here
void update_physics() {
    for(int i=0;i<scene->active.length();i++) {
        if(scene->active[i]) {
            glm::mat4& transform = scene->transforms.get(i,"physics::141");
            Velocity& velocity = scene->velocities.get(i,"physics::142");
            float velocityScale = 1.0f;
            glm::vec3 pos = glm::vec3(transform[3]);
            pos += vec3(velocity.position * 0.016f * velocityScale).toGlm();
            transform[3] = glm::vec4(pos, 1.0f);
            glm::vec3 rotVel = vec3(velocity.rotation * 0.016f * velocityScale).toGlm();
            if (glm::length(rotVel) > 0.0001f) {
                glm::mat4 rotationDelta = glm::mat4(1.0f);
                rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                transform = transform * rotationDelta; // Apply rotation in object space
            }
            glm::vec3 scaleChange = vec3(velocity.scale * 0.016f * velocityScale).toGlm();
            if (glm::length(scaleChange) > 0.0001f) {
                glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                transform = transform * scaleDelta; // Apply after rotation
            }
        }
    }
    
}
//character to file
int ctf(char c) {
    switch(c) {
        case 'h': return 1;
        case 'g': return 2;
        case 'f': return 3;
        case 'e': return 4;
        case 'd': return 5;
        case 'c': return 6;
        case 'b': return 7;
        case 'a': return 8;
        default: return 0;
    }
}

char file_to_char(int file) {
    switch(file) {
        case 1: return 'h';
        case 2: return 'g';
        case 3: return 'f';
        case 4: return 'e';
        case 5: return 'd';
        case 6: return 'c';
        case 7: return 'b';
        case 8: return 'a';
        default: return '\0';
    }
}

//File = column, rank = row
vec3 board_to_world(int file, int rank) {
    return vec3((file-4)*2,0,(rank-4)*2);
}

vec3 board_to_world(char file, int rank) {
    return board_to_world(ctf(file),rank);
}

vec3 board_to_world(const vec2& pos) {
    return board_to_world(int(pos.x()),int(pos.y()));
}

vec2 world_to_board(const vec3& pos) {
    return vec2((int)((pos.x()/2)+4),(int)((pos.z()/2)+4));
}


void takePiece(g_ptr<Single> piece) {
    int color = colors[piece->ID];
    for(int i=0;i<grid->cells.length();i++) {
        grid->cells[i].erase(piece->ID);
    }
    captured[piece->ID] = true;
    if(color==0) {
        piece->setPosition(vec3(((int)white_losses.length()-6),1,-9));
        white_losses << piece;
    } else if(color==1) {
        piece->setPosition(vec3(((int)black_losses.length()-6),1,11));
        black_losses << piece;
    }
}

void setup_piece(const std::string& type,int file,int rank) {
    auto piece = scene->create<Single>(type);
    piece->setPosition(board_to_world(file,rank));
    update_cells(piece);
    colors << piece->get<int>("color");
    values << piece->get<int>("value");
    specialRules << piece->get<int>("specialRule");
    moves << piece->get<list<vec2>>("moves");
    captured << false;
    ref << piece;
}


int main() {
    using namespace helper;

    std::string MROOT = "../Projects/FirChess/assets/models/";

    Window window = Window(1280, 768, "FirChess 0.1.0");
    scene = make<Scene>(window,2);
    scene->camera.toOrbit();
    scene->camera.lock = true;
    Data d = make_config(scene,K);
    // load_gui(scene, "FirChess", "firchessgui.fab");

    grid = make<NumGrid>(2.0f,21.0f);
    
    //Define the objects, this pulls in the models and uses the CSV to code them
    type_define_objects(grid);

    for(int k = 0;k<2;k++) {
    std::string col = k==0?"white":"black";
    int rank = k==0?1:8;
        for(int i=1;i<9;i++) setup_piece("pawn_"+col,i,k==0?2:7);
        for(int i=0;i<2;i++) setup_piece("bishop_"+col,ctf(i==0?'c':'f'),rank);
        for(int i=0;i<2;i++) setup_piece("knight_"+col,ctf(i==0?'b':'g'),rank);
        for(int i=0;i<2;i++) setup_piece("rook_"+col,ctf(i==0?'a':'h'),rank);
        setup_piece("king_"+col,ctf('e'),rank);
        setup_piece("queen_"+col,ctf('d'),rank);
    }

    //Make the little mouse to reperesnt the bot (Fir!)
    auto Fir = make<Single>(make<Model>("../models/agents/Snow.glb"));
    scene->add(Fir);
    Fir->setPosition(vec3(1,-1,-9));
        //Make the chess board and offset it so it works with the grid
    auto board = make<Single>(scene->get<g_ptr<Model>>("_board_model"));
    scene->add(board);
    board->move(vec3(1,-1.3,1));


    //Setting up the lighting, ticking environment to 0 just makes it night
    scene->tickEnvironment(0);
    auto l1 = make<Light>(Light(glm::vec3(0,10,0),glm::vec4(300,300,300,1)));
    scene->lights.push_back(l1);
    // auto l2 = make<Light>(Light(glm::vec3(-15,10,0),glm::vec4(500,500,500,1)));
    // scene->lights.push_back(l2);
    int turn_color = 1; //0 = White, 1 = Black
    g_ptr<Single> selected = nullptr;
    int s_id = 0;
    vec2 start(0,0);
    list<g_ptr<Single>> drop;
    start::run(window,d,[&]{
        vec3 mousePos = scene->getMousePos(0);
        if(mousePos.x()>8.0f) mousePos.setX(8.0f);
        if(mousePos.x()<-6.0f) mousePos.setX(-6.0f);
        if(mousePos.z()>8.0f) mousePos.setZ(8.0f);
        if(mousePos.z()<-6.0f) mousePos.setZ(-6.0f);

        scene->camera.setTarget(vec3(1,-1,1));
        if(turn_color==1) {
            scene->camera.setPosition(vec3(1,20,15));
        }
        else if(turn_color==0) {
            scene->camera.setPosition(vec3(1,20,-14));
        }
        if(pressed(SPACE)) turn_color = turn_color==0?1:0;

        if(pressed(MOUSE_LEFT)) {
            if(!selected) {
                 auto clickPos = grid->snapToGrid(scene->getMousePos());
                 auto clickedCell = grid->getCell(clickPos);
                if(!clickedCell.empty()) {
                    if(clickedCell.length()>1) print("More than one piece in cell");
                     if(auto g = ref[clickedCell[0]]) {
                         selected = g;
                         s_id = clickedCell[0];
                         clickedCell.erase(0);
                         //Start the move here
                         start = world_to_board(clickPos);
                     }
                 }
             }
             else
             {
              //End the move here
              vec3 newPos = grid->snapToGrid(mousePos).setY(selected->getPosition().y());
              vec2 end = world_to_board(newPos);
              bool legal = false;
              vec2 move = vec2((int)(end.x()-start.x()),(int)(end.y()-start.y()));
              int color = colors[s_id];
              bool can_take = true;
                int rule = specialRules[s_id];
                for(auto m : moves[s_id]) {
                    bool meets_special = false;
                    switch(rule) {
                        case 4: //King

                        break;
                        case 3: //Knight

                        break;
                        case 2: //Sliding
                            if(isMultiple(move, m)) meets_special = true;
                        break;
                        case 1: //Pawn
                        {
                            can_take = false;
                            //Inefficent logic for checking pawn movment, fix this later, possibly by using a piece class to encode color
                            //and a Type table for quick refrences of positions and type
                            vec2 d_l = vec2(-1,m.y());
                            if(d_l == move) {
                                auto c = grid->getCell(board_to_world(start+d_l));
                                if(!c.empty()) {
                                    if(colors[c[0]]!=color) {
                                        meets_special=true;
                                        can_take = true;
                                    }
                                }
                            }
                            vec2 d_r = vec2(1,m.y());
                            if(d_r == move) {
                                auto c = grid->getCell(board_to_world(start+d_l));
                                if(!c.empty()) {
                                    if(colors[c[0]]!=color) {
                                        meets_special=true;
                                        can_take = true;
                                    }
                                }
                            }

                            if(!selected->check("moved")) {
                                if(vec2(0,color==1?-2:2)==move) {
                                    meets_special=true;
                                }
                            } 
                        }
                        break;
                        case 0:
                        default: 
                        meets_special = false;
                    }

                    if(move == m || meets_special) {
                        legal = true;
                        break;
                    }
                }
              if(legal) {
                auto c = grid->getCell(newPos);
                if(!c.empty()) {
                    if(can_take&&colors[c[0]]!=colors[s_id]) {
                            takePiece(ref[c[0]]);
                            selected->flagOn("moved");
                            selected->setPosition(newPos);
                        }
                        else {
                        selected->setPosition(board_to_world(start));
                    }
                }
                else {
                    selected->flagOn("moved");
                    selected->setPosition(newPos);
                }
              }
              else 
                selected->setPosition(board_to_world(start));

              selected->setLinearVelocity(vec3(0,-2.5f,0));
              drop << selected;
              update_cells(selected);
              selected = nullptr;
             }
         }

         if(selected)
         {
            vec3 targetPos = grid->snapToGrid(mousePos).addY(1);
            vec3 direction = targetPos - selected->getPosition();
            float distance = direction.length();
            float moveSpeed = distance>=4.0f?distance*2:4.0f;
            if (distance > 0.1f) {
                selected->setLinearVelocity((direction / vec3(distance,distance,distance)) * moveSpeed);
            } else {
                selected->setLinearVelocity(vec3(0,0,0));
            }
         }

         for(auto d : drop) {
            if(d->getPosition().y()<=0) {
                d->setLinearVelocity(vec3(0,0,0)); 
                drop.erase(d);
            }
         }

         update_physics();

    });

return 0;
}