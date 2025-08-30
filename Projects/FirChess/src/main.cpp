#include<core/helper.hpp>
#include<core/numGrid.hpp>
#include<util/meshBuilder.hpp>
#include<core/type.hpp>
#include<core/thread.hpp>
#include<util/logger.hpp>

using namespace Golden;

#define EVALUATE 0

g_ptr<Scene> scene = nullptr;
g_ptr<NumGrid> num_grid = nullptr;
bool debug_move = false;
bool free_camera = true;
bool bot_turn = false;

struct Move {
    int id, score;
    ivec2 from, to;
    int takes=-1;
    bool first_move = false;
    int rule = 0;
    
    int c_id = -1;
    ivec2 c_to,c_from;
    //0 == No rule
    //1 == Promotions
    //2 == Castling
};

list<g_ptr<Single>> white_losses;
list<g_ptr<Single>> black_losses;

list<std::string> dtypes;
list<g_ptr<Single>> ref;
list<bool> captured;
list<int> colors;
list<int> values;
list<int> specialRules;
list<ivec2> cells;
list<list<ivec2>> moves;
list<bool> hasMoved;
Move last_move;
int white_king_id = -1;
int black_king_id = -1;
int white_queen_id = -1;
int black_queen_id = -1;
int white_rook_ids[2] = {-1,-1};
int black_rook_ids[2] = {-1,-1};

list<ivec2> white_pawn_moves;
list<ivec2> black_pawn_moves;
int pawn_value;
int pawn_specialRule;

list<list<int>> grid;

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
                // part->setPosition(level->snapToGrid(scene->getMousePos()));
                // list<int> myCells = level->cellsAround(part->getPosition(),((part->model->localBounds.getSize().x-1)/2));
                // for(auto cell : myCells)
                // {
                //     grid->cells[cell].push(part->ID);
                // }
                // part->set<list<int>>("_cells",myCells);
            }
            ctx.set<g_ptr<Object>>("toReturn",part);
    });
    scene->define(type,make_part);
    }
}

bool isMultiple(ivec2 move, ivec2 pattern) {
    if(pattern.x() == 0 && pattern.y() == 0) return false;
    if(pattern.x() == 0) return move.x() == 0 && ((int)move.y()%(int)pattern.y()) == 0 && (move.y() / pattern.y()) > 0;
    if(pattern.y() == 0) return move.y() == 0 &&(int)move.x() % (int)pattern.x() == 0 && (move.x() / pattern.x()) > 0;
    return (int)move.x() % (int)pattern.x() == 0 && (int)move.y() % (int)pattern.y() == 0 && 
           (move.x() / pattern.x()) == (move.y() / pattern.y()) && (move.x() / pattern.x()) > 0;
}

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

vec3 board_to_world(const ivec2& pos) {
    return board_to_world(int(pos.x()),int(pos.y()));
}

ivec2 world_to_board(const vec3& pos) {
    return ivec2((int)((pos.x()/2)+4),(int)((pos.z()/2)+4));
}

//Board to string
std::string bts(const ivec2& pos) {
    return file_to_char(pos.x())+std::to_string(pos.y());
}

void update_num_grid(g_ptr<Single> selected,const vec3& pos) {
    num_grid->getCell(selected->getPosition()).erase(selected->ID);
    num_grid->getCell(pos) << selected->ID;
}

inline int& square(const ivec2& c) {
    return grid[c.x()][c.y()];
}

inline void update_grid(int id,const ivec2& to) {
    // print(dtypes[id],"-",id," removed from: ");
    // print(bts(cells[id]));
    // print(" to ");
    // print(bts(to));

    square(cells[id]) = -1;
    square(to) = id;
    cells[id] = to;
}


//poached code from the Physics class meant to be use multi-threaded but just raw here
void update_physics() {
    //print(scene->active.length());
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


void print_move(Move move) {
    print(dtypes[move.id],"-",bts(move.from)," to ",bts(move.to),move.takes!=-1?" takes "+dtypes[move.takes]:"");
}


inline ivec2 pos_of(int piece) {
    return cells[piece];
}

vec3 world_pos_of(int piece) {
    return board_to_world(pos_of(piece));
}

bool in_bounds(const vec3& pos) {
    return(pos.x()<=8.0f&&pos.x()>=-6.0f&&pos.z()<=8.0f&&pos.z()>=-6.0f);
}

bool in_bounds(const ivec2& pos) {
    return(pos.x()<=8&&pos.x()>=1&&pos.y()<=8&&pos.y()>=1);
}

void setup_piece(const std::string& type,int file,int rank) {
    auto piece = scene->create<Single>(type);
    piece->setPosition(board_to_world(file,rank));
    ivec2 mySquare{file,rank};
    square(mySquare) = piece->ID;
    cells << mySquare;

    //update_cells(piece);
    std::string dtype = piece->get<std::string>("dtype");
    if(dtype=="king_white") white_king_id = dtypes.length();
    if(dtype=="king_black") black_king_id = dtypes.length();
    if(dtype=="queen_white") white_queen_id = dtypes.length();
    if(dtype=="queen_black") black_queen_id = dtypes.length();
    if(dtype=="rook_white") {
        if(white_rook_ids[0]==-1) white_rook_ids[0] = dtypes.length();
        else white_rook_ids[1] = dtypes.length();
    }
    if(dtype=="rook_black") {
        if(black_rook_ids[0]==-1) black_rook_ids[0] = dtypes.length();
        else black_rook_ids[1] = dtypes.length();
    }
    dtypes << dtype;
    colors << piece->get<int>("color");
    int value = piece->get<int>("value");
    values << value;
    int specialRule = piece->get<int>("specialRule");
    specialRules << specialRule;
    list<ivec2> n_moves = piece->get<list<ivec2>>("moves");
    moves << n_moves;
    captured << false;
    ref << piece;
    hasMoved << false;
    if(dtype=="pawn_black") {
        black_pawn_moves = n_moves;
        pawn_value = value;
        pawn_specialRule = specialRule;
    }
    if(dtype=="pawn_white") {
        white_pawn_moves = n_moves;
        pawn_value = value;
        pawn_specialRule = specialRule;
    }
}

int turn_color = 1; //0 = White, 1 = Black
g_ptr<Single> selected = nullptr;
int s_id = 0;
ivec2 start_pos(0,0);
list<g_ptr<Single>> drop;
long sleep_time = 0;

void select_piece(int id) {
    selected = ref[id];
    s_id = id;
    start_pos = pos_of(id);
}

void takePiece(int id,bool real = true) {
    //print(dtypes[id],"-",id," taken ");
    int color = colors[id];
    square(cells[id]) = -1;
    captured[id] = true;
    if(color==0) {
        if(real) {
            ref[id]->setPosition(vec3(((int)white_losses.length()-6),1,-9));
            white_losses << ref[id];
        }
    } else if(color==1) {
        if(real) {
            ref[id]->setPosition(vec3(((int)black_losses.length()-6),1,11));
            black_losses << ref[id];
        }
    }
}

void promote(int id,bool real = true) {
    int q_id = colors[id]==0?white_queen_id:black_queen_id;
    moves[id] = moves[q_id];
    values[id] = values[q_id];
    specialRules[id] = specialRules[q_id];
    if(real) {
        ref[id]->setModel(ref[q_id]->model);
    }
}


void unpromote(int id,bool real = true) {
    moves[id] = colors[id]==0?white_pawn_moves:black_pawn_moves;
    values[id] = pawn_value;
    specialRules[id] = pawn_specialRule;
    if(real) {
        std::string type = colors[id]==0?"pawn_white":"pawn_black";
        ref[id]->setModel(scene->get<g_ptr<Model>>("_"+type+"_model"));
    }
}

void castle(Move& move,bool real = true) {
    int king_id = move.id;
    int rook_id = square(move.to);
    if(rook_id==-1) {
        // if(move.to==ivec2(8,8))
        //     rook_id = black_rook_ids[0];
        print("No rook for castle at square: ");
        move.to.print();
        // print(bts({8,8}));
        // print(square({8,8}));
        // print(bts({8,1}));
        // print(square({8,1}));
        // print(bts({1,8}));
        // print(square({1,8}));
        // print(bts({1,1}));
        // print(square({1,1}));
    }

    move.c_from = move.to;
    move.c_id = rook_id;

    ivec2 rook_pos = pos_of(rook_id);
    ivec2 king_pos = move.from;
    ivec2 dir = (rook_pos.x() > king_pos.x()) ? ivec2(1,0) : ivec2(-1,0);
    ivec2 new_king_pos = king_pos+(dir*2);
    move.to = new_king_pos;
    move.c_to = (new_king_pos-dir);
}

void makeMove(Move& move,bool real = true) {
    if(!hasMoved[move.id]) {
        move.first_move = true;
        hasMoved[move.id] = true;
    }
    if(move.rule != 2) {
        int t = square(move.to);
        if(t!=-1) {
            takePiece(t,real);
            move.takes = t;
        }
    }
    
    //promotion
    if(move.rule == 1) {
        promote(move.id,real);
    } //castle
    else if(move.rule == 2) {
        // print("-------");
        // print("Pre-castle: ");
        // print_move(move);
        if(move.c_id==-1)
            castle(move,real);

        // print("Original: ");
        // print_move(move);
        Move linked;
        linked.from = move.c_from;
        linked.to = move.c_to;
        linked.id = move.c_id;
        // print("Linked: ");
        // print_move(linked);
        makeMove(linked,real);
    }

    // if(move.from==ivec2{8,1}) {
    //     print(dtypes[move.id],"-",move.id," left 8,1 ");
    // }
    update_grid(move.id,move.to);
    if(real) {
        update_num_grid(ref[move.id],board_to_world(move.to));
        ref[move.id]->setPosition(board_to_world(move.to));
        #if !EVALUATE
        print_move(move);
        #endif
    }
    
    // if(bot_turn)
    //     std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
}

void unmakeMove(Move move,bool real = true) {
    if(move.first_move) {
        hasMoved[move.id] = false;
    }
    //promotion
    if(move.rule == 1) {
        unpromote(move.id,real);
    }
    else if(move.rule == 2) {
        // print("Unmake Original: ");
        // print_move(move);
        Move linked;
        linked.from = move.c_from;
        linked.to = move.c_to;
        linked.id = move.c_id;
        linked.first_move = true;
        // print("Unmake Linked: ");
        // print_move(linked);
        unmakeMove(linked,real);
    }

    update_grid(move.id,move.from);
    if(real) {
        update_num_grid(ref[move.id],board_to_world(move.from));
        ref[move.id]->setPosition(board_to_world(move.from));
        #if !EVALUATE
        print("Unmade move: "); print_move(move);
        #endif
    } 

    if(move.takes!=-1) {
        captured[move.takes] = false;
        //print(dtypes[move.takes],"-",move.takes," restored ");
        update_grid(move.takes,move.to);
        if(real) {
            update_num_grid(ref[move.takes],board_to_world(move.to));
            ref[move.takes]->setPosition(board_to_world(move.to));
        }
    }
    // if(bot_turn)
   //std::this_thread::sleep_for(std::chrono::seconds(1));
}

bool can_attack(const ivec2& pos,int by_color) {
    //Looking at all the attack moves that can be made
    for(int i=(by_color==0?0:16);i<(by_color==0?16:32);i++) {
        if(captured[i]) continue;
        ivec2 place = pos_of(i);
        int rule = specialRules[i];
        if(rule==1) {
            ivec2 d_r = ivec2(1,moves[i][0].y());
            ivec2 d_l = ivec2(-1,moves[i][0].y());
            if((d_r+place)==pos) return true;
            if((d_l+place)==pos) return true;
        } 
        else if(rule==2) {
            for(int m=0;m<moves[i].length();m++) {
                for(int d=1;d<=7;d++) {
                    ivec2 dir = moves[i][m]*d;
                    if((place+dir)==pos) return true;
                    vec3 nPos = board_to_world(place+dir);
                    if(!in_bounds(place+dir)) break;
                    if(square(place+dir)!=-1) { //Raycasting
                        break;
                    }
                }
            }
        }
        else { //Normal moves only, because all special rules cover attacks they can do
            for(auto v : moves[i]) {
                if((place+v)==pos) return true;
            }
        }
    }
    return false;
}

bool isKingInCheck(int color) {
    if(can_attack(pos_of(color==0?white_king_id:black_king_id),1-color)) 
        return true;
    return false;
}



int is_attacking(const ivec2& pos,int by_color) {
    //Looking at all the attack moves that can be made
    for(int i=(by_color==0?0:16);i<(by_color==0?16:32);i++) {
        if(captured[i]) continue;
        ivec2 place = pos_of(i);
        int rule = specialRules[i];
        if(rule==1) {
            ivec2 d_r = ivec2(1,moves[i][0].y());
            ivec2 d_l = ivec2(-1,moves[i][0].y());
            if((d_r+place)==pos) return i;
            if((d_l+place)==pos) return i;
        } 
        else if(rule==2) {
            for(int m=0;m<moves[i].length();m++) {
                for(int d=1;d<=7;d++) {
                    ivec2 dir = moves[i][m]*d;
                    if((place+dir)==pos) return i;
                    vec3 nPos = board_to_world(place+dir);
                    if(!in_bounds(place+dir)) break;
                    if(square(place+dir)!=-1) { //Raycasting
                        break;
                    }
                }
            }
        }
        else { //Normal moves only, because all special rules cover attacks they can do
            for(auto v : moves[i]) {
                if((place+v)==pos) return i;
            }
        }
    }
    return -1;
}

bool validate_castle(int id,const ivec2& to) {
    int color = colors[id];
    if(hasMoved[id]) return false;
    if(id==(color==0?white_king_id:black_king_id)) {
        //This is just for fun, there's way more efficent ways to do this... 
        //but none cooler!
        if(square(to)==color==0?white_rook_ids[0]:black_rook_ids[0]||
           square(to)==color==0?white_rook_ids[1]:black_rook_ids[1])
            {
            int rook_id = square(to);
            if(hasMoved[rook_id]) return false;
            ivec2 king_pos = cells[id];
            ivec2 rook_pos = cells[rook_id];
            ivec2 dir = (rook_pos.x() > king_pos.x()) ? ivec2(1,0) : ivec2(-1,0);
            int start = std::min(king_pos.x(), rook_pos.x()) + 1;
            int end = std::max(king_pos.x(), rook_pos.x());
            
            for(int file = start; file < end; file++) {
                ivec2 check_pos(file, king_pos.y());
                if(square(check_pos)!=-1) 
                    return false;
                // Also checking check
                if(can_attack(check_pos, 1-color)) {
                    return false; 
                }
            }
            //Can't castle in check!
            if(can_attack(king_pos, 1-color)) {
                return false; 
            }
            
            return true;
        }
    }
    return false;
}

bool validate_move(const ivec2& to) {
    if(!in_bounds(to)) return false;
    ivec2 move = ivec2(to-start_pos);
    int color = colors[s_id];
    bool can_take = true;
      int rule = specialRules[s_id];
      bool validMove = false;
      for(auto m : moves[s_id]) {
          bool meets_special = false;
          switch(rule) {
              case 2: //Sliding
                  if(isMultiple(move, m)) meets_special = true;
              break;
              case 1: //Pawn
              {
                  can_take = false;
                  ivec2 d_l = ivec2(-1,m.y());
                  if(d_l == move) {
                      if(square(start_pos+d_l)!=-1) {
                          if(colors[square(start_pos+d_l)]!=color) {
                              meets_special=true;
                              can_take = true;
                          }
                      }
                  }
                 ivec2 d_r = ivec2(1,m.y());
                  if(d_r == move) {
                      if(square(start_pos+d_r)!=-1) {
                          if(colors[square(start_pos+d_r)]!=color) {
                              meets_special=true;
                              can_take = true;
                          }
                      }
                  }

                  if(!hasMoved[s_id]) {
                      if(ivec2(0,color==1?-2:2)==move) {
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
            if(square(to)!=-1) {
                if(can_take&&colors[square(to)]!=colors[s_id]) {
                    // print(dtypes[s_id]," takes ",dtypes[square(to)]);
                        validMove = true;
                    }
            }
            else {
               validMove = true;
            }
        }
      }
    if(!validMove) return false;    
    return true;
}

bool check_promotion(Move& move) {
    if(specialRules[move.id]==1) {
        int color = colors[move.id];
        float promotion_rank = color==0?8:1;
        if(move.to.y()==promotion_rank) {
            return true;
        }
    }
    return false;
}

list<ivec2> can_castle(int color) {
    list<ivec2> result;
    if(hasMoved[color==0?white_king_id:black_king_id]) return result;
    int side = -1;
    ivec2 king_pos = pos_of(color==0?white_king_id:black_king_id);
    for(int i=0;i<2;i++) {
        if(!hasMoved[color==0?white_rook_ids[i]:black_rook_ids[i]]
            &&!captured[color==0?white_rook_ids[i]:black_rook_ids[i]]) {
            ivec2 rook_pos = pos_of(color==0?white_rook_ids[i]:black_rook_ids[i]);
            ivec2 dir = (rook_pos.x() > king_pos.x()) ? ivec2(1,0) : ivec2(-1,0);
            //bool is_long = king_pos.x()<rook_pos.x();
            int start = std::min(king_pos.x(), rook_pos.x()) + 1;
            int end = std::max(king_pos.x(), rook_pos.x());
            bool valid = true;
            for(int file = start; file < end; file++) {
                ivec2 check_pos(file, king_pos.y());
                if(square(check_pos)!=-1) {
                    valid = false;
                    break;
                }
                // Also checking check
                if(can_attack(check_pos, 1-color)) {
                    valid = false; 
                    break;
                }
            }

            if(valid) {
                result << (rook_pos-king_pos);
            }
        }
    }
    return result;
}


list<Move> generateMoves(int color) {
    // Line total;
    // total.start();
    // Line s;
    // s.start();
    list<Move> result;
    //if(captured[color==0?white_king_id:black_king_id]) return result;
    for(int i=(color==0?0:16);i<(color==0?16:32);i++) {
        // Line inner;
        // inner.start();
        bool castling = false;
        if(captured[i]) continue;
        select_piece(i);
        list<ivec2> special_moves;
        if(specialRules[i]==1) {
            ivec2 d_r = ivec2(1,moves[i][0].y());
            ivec2 d_l = ivec2(-1,moves[i][0].y());
            if(square(start_pos+moves[i][0])!=-1) { 
                ivec2 dd_m = ivec2(0,colors[i]==0?2:-2);
                special_moves << dd_m;
            }
            special_moves << d_r;
            special_moves << d_l;
        } 
        else if(specialRules[i]==2) {
            for(int m=0;m<moves[i].length();m++) {
                for(int d=1;d<=7;d++) {
                    ivec2 dir = moves[i][m]*d;
                    if(!in_bounds(start_pos+dir)) break;
                    if(square(start_pos+dir)!=-1) {
                        if(colors[square(start_pos+dir)]!=color) {
                            special_moves << dir;
                        }
                        break;
                    }
                    special_moves << dir;
                }
            }
        }
        if(i==(colors[i]==0?white_king_id:black_king_id)) {
            list<ivec2> side = can_castle(color);
            if(!side.empty()) {
                castling = true;
                for(auto s : side) {
                    special_moves << s;
                }
            }
        }

        if(specialRules[i]!=2) //Don't evaluate normal moves for sliding peices
        for(int m=0;m<moves[i].length();m++) {
            if(validate_move(start_pos+moves[i][m])) {
                Move move;
                move.id = s_id;
                move.from = start_pos;
                move.to = start_pos+moves[i][m];
                result << move;
            }
        }
        for(int m=0;m<special_moves.length();m++) {
            if(validate_move(start_pos+special_moves[m])||castling) {
                Move move;
                move.id = s_id;
                move.from = start_pos;
                move.to = start_pos+special_moves[m];
                if(castling) move.rule = 2;
                result << move;
            }
        }
        //print(dtypes[i]," time ",inner.end());
        selected = nullptr;
    }
    // print("Loop time ",s.end());
    // s.start();
    list<Move> to_back;
    list<Move> final_result;
    for(auto& m : result) {
        // print_move(m);
        makeMove(m, false);
        bool kingInCheck = isKingInCheck(color);
        unmakeMove(m, false);
        if(!kingInCheck) {
            if(check_promotion(m)) m.rule = 1;
            if(m.takes==-1) 
                to_back << m;
            else 
                final_result << m;
        }
        else {
            // int k_id = color==0?white_king_id:black_king_id;
            // int checker = is_attacking(pos_of(k_id),1-color);
            // if(checker!=-1) {
            //     print_move(m);
            //     print("Puts ",dtypes[k_id],"-",k_id," on ",bts(pos_of(k_id))," in check from ",dtypes[checker],"-",checker," on ",bts(pos_of(checker)));
            // } else print("NO checker");
        }
    }
    final_result << to_back;
   
    // print("Final time ",s.end());
    // print("Total time ",total.end());
    return final_result;
}


int getPositionalValue(int pieceId) {
    if(captured[pieceId]) return 0;
    
    ivec2 pos = pos_of(pieceId);
    int file = pos.x() - 1; // Convert to 0-7
    int rank = pos.y() - 1; // Convert to 0-7
    int color = colors[pieceId];
    
    // Flip for black pieces (they see board upside-down)
    if(color == 1) rank = 7 - rank;
    
    std::string piece = dtypes[pieceId];
    int bonus = 0;
    
    if(piece.find("pawn") != std::string::npos) {
        // Pawn table: advance = good, center files = good
        int pawn_table[8][8] = {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 5, 10, 10,-20,-20, 10, 10,  5},
            { 5, -5,-10,  0,  0,-10, -5,  5},
            { 0,  0,  0, 20, 20,  0,  0,  0},
            { 5,  5, 10, 25, 25, 10,  5,  5},
            {10, 10, 20, 30, 30, 20, 10, 10},
            {50, 50, 50, 50, 50, 50, 50, 50},
            { 0,  0,  0,  0,  0,  0,  0,  0}
        };
        bonus = pawn_table[rank][file];
    }
    else if(piece.find("knight") != std::string::npos) {
        // Knights love the center, hate the rim
        int knight_table[8][8] = {
            {-50,-40,-30,-30,-30,-30,-40,-50},
            {-40,-20,  0,  0,  0,  0,-20,-40},
            {-30,  0, 10, 15, 15, 10,  0,-30},
            {-30,  5, 15, 20, 20, 15,  5,-30},
            {-30,  0, 15, 20, 20, 15,  0,-30},
            {-30,  5, 10, 15, 15, 10,  5,-30},
            {-40,-20,  0,  5,  5,  0,-20,-40},
            {-50,-40,-30,-30,-30,-30,-40,-50}
        };
        bonus = knight_table[rank][file];
    }
    
    return bonus;
}

int evaluateKingSafety(int color) {    
    ivec2 kingPos = pos_of(color==0?white_king_id:black_king_id);
    int safety = 100; // Start with base safety score
    
    // Check 3x3 area around king
    for(int df = -1; df <= 1; df++) {
        for(int dr = -1; dr <= 1; dr++) {
            if(df == 0 && dr == 0) continue; // Skip king's own square
            
            int checkFile = kingPos.x() + df;
            int checkRank = kingPos.y() + dr;
            
            if(checkFile < 1 || checkFile > 8 || checkRank < 1 || checkRank > 8) continue;
            
            ivec2 checkPos(checkFile, checkRank);
            
            if(square(checkPos)!=-1) {
                if(colors[square(checkPos)] == color) {
                    safety += 15; // Friendly defender nearby
                    
                    // Extra bonus for pawn shield
                    if(dtypes[square(checkPos)].find("pawn") != std::string::npos) {
                        safety += 25; // Pawn shields are extra valuable
                    }
                } else {
                    safety -= 20; // Enemy piece nearby is dangerous
                }
            } else {
                safety -= 10; // Empty squares around king = exposed
            }
        }
    }
    
    // Penalty for being in check
    if(isKingInCheck(color)) {
        safety -= 50;
    }
    
    return safety;
}


int evaluate() {  // 0 = white, 1 = black
    int score = 0;
    for(int i = 0; i < 32; i++) {
        if(!captured[i]) {
            int piece_value = values[i] + getPositionalValue(i);
            
            if(colors[i] == 0) {
                score += piece_value;
            } else {
                score -= piece_value;
            }
        }
    }

    score += evaluateKingSafety(0);
    score -= evaluateKingSafety(1);
    return score;
}

static int calcs = 0;
int minimax(int depth, int current_turn, int alpha, int beta) {
    if(depth == 0) return evaluate();
    auto moves = generateMoves(current_turn);
    if(moves.empty()) return evaluate();
    calcs+=2;
    if(current_turn == 0) {  // White maximizes
        int maxEval = -9999;
        for(auto move : moves) {
            #if EVALUATE
            makeMove(move, true);
            if(current_turn==0&&depth>1)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, true);
            #else
            makeMove(move, false);
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, false);
            #endif
            
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if(beta <= alpha) break;
        }
        return maxEval;
    } else {  // Black minimizes
        int minEval = 9999;
        for(auto move : moves) {
            #if EVALUATE
            makeMove(move, true);
            if(current_turn==1&&depth>1)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, true);
            #else
            makeMove(move, false);
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, false);
            #endif
            
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if(beta <= alpha) break;
        }
        return minEval;
    }
}


Move findBestMove(int depth,int color) {
    Line s; s.start();
    auto moves = generateMoves(color);
    if(moves.empty()) print("Out of moves");
    Move bestMove;
    bestMove.id = -1;
    int bestScore = color == 0 ? -9999 : 9999;
    print("-----Finding move for ",color==0?"white":"black","-----");
    for(auto& move : moves) {
        makeMove(move,false);
        int score = minimax(depth-1, 1-color, -9999, 9999);
        move.score = score;
        unmakeMove(move,false);
        calcs+=2;
        
        bool isBetter = color == 0 ? (score > bestScore) : (score < bestScore);
        if(isBetter) {
            bestScore = score;
            print("New best: ",bestScore);
            print_move(move);
            bestMove = move;
        }
    }
    if(bestMove.id==-1) {
        print("Checkmate!");
        return bestMove;
    }

    print("Cacls performed: ",calcs," time: ",s.end()/1000000000,"s Chosen score: ",bestMove.score);
    calcs = 0;
    return bestMove;
}


int main() {
    using namespace helper;

    std::string MROOT = "../Projects/FirChess/assets/models/";

    Window window = Window(1280, 768, "FirChess 0.6.0");
    scene = make<Scene>(window,2);
    scene->camera.toOrbit();
    //scene->camera.lock = true;
    Data d = make_config(scene,K);
    // load_gui(scene, "FirChess", "firchessgui.fab");

    num_grid = make<NumGrid>(2.0f,21.0f);
    
    //Define the objects, this pulls in the models and uses the CSV to code them
    type_define_objects(num_grid);

    for(int a=0;a<9;a++) {
        grid << list<int>();
        for(int b=0;b<9;b++) {
            grid[a] << -1;
        }
    }

    for(int k = 0;k<2;k++) {
    std::string col = k==0?"white":"black";
    int rank = k==0?1:8;
        setup_piece("queen_"+col,ctf('d'),rank);
        for(int i=0;i<2;i++) setup_piece("rook_"+col,ctf(i==0?'a':'h'),rank);
        for(int i=0;i<2;i++) setup_piece("bishop_"+col,ctf(i==0?'c':'f'),rank);
        for(int i=0;i<2;i++) setup_piece("knight_"+col,ctf(i==0?'b':'g'),rank);
        setup_piece("king_"+col,ctf('e'),rank);
        for(int i=1;i<9;i++) setup_piece("pawn_"+col,i,k==0?2:7);
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
    auto thread = make<Thread>();
    thread->run([&](ScriptContext& ctx){
            Move m = findBestMove(4,turn_color);
            if(m.id!=-1) {
                makeMove(m);
                turn_color = turn_color==0?1:0;
            }
            else
                thread->pause();
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    },0.02f);

    auto phys_thread = make<Thread>();
    phys_thread->run([&](ScriptContext& ctx){
        update_physics();
    },0.016);
    phys_thread->start();

    // auto box = make<Single>(makeTestBox(1.0f));
    // scene->add(box);
    // box->setPosition(grid->indexToLoc(grid->toIndex(board_to_world('e',7))));

    list<Move> madeMoves;

    start::run(window,d,[&]{
        vec3 mousePos = scene->getMousePos(0);
        if(mousePos.x()>8.0f) mousePos.setX(8.0f);
        if(mousePos.x()<-6.0f) mousePos.setX(-6.0f);
        if(mousePos.z()>8.0f) mousePos.setZ(8.0f);
        if(mousePos.z()<-6.0f) mousePos.setZ(-6.0f);

        if(!free_camera) {
            scene->camera.setTarget(vec3(1,-1,1));
            if(turn_color==1) {
                scene->camera.setPosition(vec3(1,20,15));
            }
            else if(turn_color==0) {
                scene->camera.setPosition(vec3(1,20,-14));
            }
        }
        if(pressed(SPACE)) turn_color = turn_color==0?1:0;
       
        if(pressed(I)) {
            if(debug_move) {print("Debug move off"); debug_move=false;}
            else {print("Debug move on"); debug_move=true;}
        } 
        if(pressed(C)) {
            if(free_camera){print("Free camera"); scene->camera.lock = true; free_camera=false;}
            else{print("Locked camera"); scene->camera.lock = false; free_camera=true;}
        }
        if(pressed(T)) {
            // makeMove(findBestMove(1,turn_color));
            // turn_color = turn_color==0?1:0;
            if(held(LSHIFT)) {
                generateMoves(turn_color);
            }
            else {
                // list<Move> result = generateMoves(turn_color);
                // Move m = result.get(randi(0,result.length()-1),"get_move");
                Move m = findBestMove(4,turn_color);
                if(m.id!=-1) {
                    makeMove(m);
                    madeMoves << m;
                }
            }
        }
        if(pressed(Y)) {
            vec3 clickPos = num_grid->snapToGrid(scene->getMousePos());
            takePiece(square(world_to_board(clickPos)));
        }
        if(pressed(R)) unmakeMove(madeMoves.pop());
        if(pressed(NUM_1)) scene->camera.toOrbit();
        if(pressed(NUM_2)) scene->camera.toIso();
        if(pressed(NUM_3)) scene->camera.toFirstPerson();

        if(pressed(E)) {
            auto clickPos = num_grid->snapToGrid(scene->getMousePos());
            ivec2 v = world_to_board(clickPos);
            //grid->toIndex(clickPos)
            print("----",bts(v),"----");
           if(square(v)!=-1) {
                print("E:",dtypes[square(v)]," I:",square(v));
           }
           else print("EMPTY");
        }
        if(pressed(G)) {
            print(isKingInCheck(turn_color)==0?"No check":"In check");
        }
        if(pressed(MOUSE_LEFT)) {
            if(!selected) {
                 auto clickPos = num_grid->snapToGrid(scene->getMousePos());
                 ivec2 v = world_to_board(clickPos);
                if(square(v)!=-1) {
                    int t_s_id = square(v);
                     if(auto g = ref[t_s_id]) {
                        if(colors[t_s_id]==turn_color||debug_move) {
                         select_piece(t_s_id);
                         start_pos = world_to_board(clickPos);
                        }
                     }
                 }
             }
             else
             {
              //End the move here
              vec3 newPos = num_grid->snapToGrid(mousePos).addY(selected->getPosition().y());
              ivec2 v = world_to_board(newPos);
              update_num_grid(selected,newPos);
              bool castling = false;
              if(validate_castle(s_id,v)) {
                castling = true;
              } 
              if(validate_move(v)||debug_move||castling) {
                Move move;
                move.id = s_id;
                move.from = start_pos;
                move.to = v;
                if(castling) move.rule = 2;
                makeMove(move,false);
                if(isKingInCheck(turn_color)) {
                    unmakeMove(move,false);
                    selected->setPosition(board_to_world(start_pos));
                }
                else {
                    unmakeMove(move,false);
                    if(check_promotion(move)) {
                        move.rule = 1;
                    }
                    makeMove(move);
                    madeMoves << move;
                }
              }
              else 
                selected->setPosition(board_to_world(start_pos));

              selected->setLinearVelocity(vec3(0,-2.5f,0));
              drop << selected;
              //print("Moving ",selected->ID," to "); v.print();
              //update_grid(selected->ID,v);
              selected = nullptr;
             }
         }

         if(selected&&!bot_turn)
         {
            vec3 targetPos = num_grid->snapToGrid(mousePos).addY(1);
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

         if(pressed(Q)) {
            if(held(LSHIFT)) unmakeMove(last_move);
            else {
                if(thread->runningTurn) {thread->pause(); bot_turn=false;}
                else {thread->start(); bot_turn = true;}
            }
        }

        if(pressed(P)) {
            if(phys_thread->runningTurn) {phys_thread->pause();}
            else {phys_thread->start();}
        }

    });

return 0;
}