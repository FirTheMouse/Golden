#include<core/helper.hpp>
#include<core/numGrid.hpp>
#include<util/meshBuilder.hpp>
#include<core/type.hpp>
#include<core/thread.hpp>
#include<util/logger.hpp>

using namespace Golden;

#define EVALUATE 0
#define LOG 1

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

    ivec2 e_square = {-1,-1};
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

ivec2 enpassant_square(-1,-1);

list<list<int>> grid;
list<list<int>> copyGrid() {
    return grid;
}

uint64_t zobrist_table[32][64];
uint64_t turn_zobrist_key;
uint64_t current_hash = 0;
map<uint64_t,int> history;


#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA 2

struct TTEntry {
    uint64_t hash = 0;
    int depth = -1;
    int score = 0;
    int flag = 0; // 0=EXACT, 1=ALPHA, 2=BETA
    Move best_move;
    bool valid = false;
};

static const int TT_SIZE = 1024 * 1024; 
static TTEntry transposition_table[TT_SIZE];

uint64_t random64bit() {
    return ((uint64_t)rand() << 32) | rand();
}


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

//Board to string
std::string bts(const ivec2& pos) {
    return file_to_char(pos.x())+std::to_string(pos.y());
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

void update_num_grid(g_ptr<Single> selected,const vec3& pos) {
    num_grid->getCell(selected->getPosition()).erase(selected->ID);
    num_grid->getCell(pos) << selected->ID;
}

inline int& square(const ivec2& c) {
    return grid[c.x()][c.y()];
}

inline void update_grid(int id,const ivec2& to) {
    ivec2 from = cells[id];
    int old_square = (from.y() - 1) * 8 + (from.x() - 1);
    int new_square = (to.y() - 1) * 8 + (to.x() - 1);
    current_hash ^= zobrist_table[id][old_square];  // Remove from old square
    current_hash ^= zobrist_table[id][new_square];  // Add to new square


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

// Piece to string
std::string pts(int id) {
    return dtypes[id]+"-"+std::to_string(id);
}

// Move to string
std::string mts(Move move) {
    return pts(move.id)+" from "+bts(move.from)+" to "+bts(move.to)+(move.takes!=-1?" takes "+pts(move.takes):"");
}

void print_move(Move move) {
    print(pts(move.id)," from ",bts(move.from)," to ",bts(move.to),move.takes!=-1?" takes "+pts(move.takes):"");
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

uint64_t hash_board() {
    uint64_t hash = 0;
    for(int piece = 0; piece < 32; piece++) {
        if(!captured[piece]) {
            ivec2 pos = pos_of(piece);
            int square = (pos.y() - 1) * 8 + (pos.x() - 1);  // Convert to 0-63
            hash ^= zobrist_table[piece][square];
        }
    }
    return hash;
}

uint64_t get_search_hash(int color) {
    uint64_t hash = current_hash;
    if(color == 1) hash ^= turn_zobrist_key;
    return hash;
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

int turn_color = 0; //0 = White, 1 = Black
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
    ivec2 pos = cells[id];
    int square_index = (pos.y() - 1) * 8 + (pos.x() - 1);
    current_hash ^= zobrist_table[id][square_index];
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
        if(move.c_id==-1)
            castle(move,real);
        Move linked;
        linked.from = move.c_from;
        linked.to = move.c_to;
        linked.id = move.c_id;
        makeMove(linked,real);
    } //Pawn double move
    else if(move.rule == 3) {
        if(real) {
            enpassant_square = move.to-(colors[move.id]==0?ivec2(0,-1):ivec2(0,1));
        } else {
            move.e_square = move.to-(colors[move.id]==0?ivec2(0,-1):ivec2(0,1));
        }
    }
    move.e_square = enpassant_square;

    update_grid(move.id,move.to);
    if(real) {
        update_num_grid(ref[move.id],board_to_world(move.to));
        ref[move.id]->setPosition(board_to_world(move.to));
        history.getOrPut(current_hash,0)++;
        #if !EVALUATE
        print_move(move);
        #endif
    }

    // uint64_t hash_after = hash_board();
    // uint64_t expected_hash = current_hash;
    
    // if(hash_after != expected_hash) {
    //     print("Move caused hash mismatch:");
    //     print_move(move);
    //     print("Expected:", expected_hash, "Actual:", hash_after);
    // }
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
        Move linked;
        linked.from = move.c_from;
        linked.to = move.c_to;
        linked.id = move.c_id;
        linked.first_move = true;
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
        int new_square = (move.to.y() - 1) * 8 + (move.to.x() - 1);
        current_hash ^= zobrist_table[move.takes][new_square];
        square(cells[move.takes]) = -1;
        square(move.to) = move.takes;
        //update_grid(move.takes,move.to);
        if(real) {
            update_num_grid(ref[move.takes],board_to_world(move.to));
            ref[move.takes]->setPosition(board_to_world(move.to));
        }
    }

    if(real) {
        history.get(current_hash)--;
    }
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

list<int> is_attacking(const ivec2& pos,int by_color) {
    //Looking at all the attack moves that can be made
    list<int> result;
    for(int i=(by_color==0?0:16);i<(by_color==0?16:32);i++) {
        if(captured[i]) continue;
        ivec2 place = pos_of(i);
        int rule = specialRules[i];
        if(rule==1) {
            ivec2 d_r = ivec2(1,moves[i][0].y());
            ivec2 d_l = ivec2(-1,moves[i][0].y());
            if((d_r+place)==pos)  result << i;
            if((d_l+place)==pos)  result << i;
        } 
        else if(rule==2) {
            for(int m=0;m<moves[i].length();m++) {
                for(int d=1;d<=7;d++) {
                    ivec2 dir = moves[i][m]*d;
                    if((place+dir)==pos) {result << i; break;}
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
                if((place+v)==pos) {result << i; break;}
            }
        }
    }
    return result;
}

int basic_capture_value(Move move,int color) {
    int gain = values[move.takes]; 
    makeMove(move, false);
    if(can_attack(move.to, 1-color)) {
        gain -= values[move.id];
    }
    unmakeMove(move, false);
    return gain;
}

int capture_value(Move move,int color) {
    int gain = values[move.takes]; 
    makeMove(move, false);
    auto attackers = is_attacking(move.to, 1-color);
    if(!attackers.empty()) {
        int cheapest_attacker = attackers[0];
        for(int attacker : attackers) {
            if(values[attacker] < values[cheapest_attacker]) {
                cheapest_attacker = attacker;
            }
        }
        gain -= values[move.id];

        auto counter_attackers = is_attacking(move.to, color);
        if(!counter_attackers.empty()) {
            gain += values[cheapest_attacker];
        }
    }
    unmakeMove(move, false);
    return gain;
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

bool validate_move(Move& move) {
    if(!in_bounds(move.to)) return false;
    ivec2 pattern = (move.to-move.from);
    int color = colors[move.id];
    bool can_take = true;
      int rule = specialRules[move.id];
      bool validMove = false;
      for(auto m : moves[move.id]) {
          bool meets_special = false;
          switch(rule) {
              case 2: //Sliding
                  if(isMultiple(pattern, m)) meets_special = true;
              break;
              case 1: //Pawn
              {
                  can_take = false;
                  ivec2 d_l = ivec2(-1,m.y());
                  if(d_l == pattern) {
                      if(square(start_pos+d_l)!=-1) {
                          if(colors[square(start_pos+d_l)]!=color) {
                              meets_special=true;
                              can_take = true;
                          }
                      }
                  }
                 ivec2 d_r = ivec2(1,m.y());
                  if(d_r == pattern) {
                      if(square(start_pos+d_r)!=-1) {
                          if(colors[square(start_pos+d_r)]!=color) {
                              meets_special=true;
                              can_take = true;
                          }
                      }
                  }

                  if(!hasMoved[s_id]) {
                      if(ivec2(0,color==1?-2:2)==pattern) {
                          move.rule = 3;
                          meets_special=true;
                      }
                  } 
              }
              break;
              case 0:
              default: 
              meets_special = false;
          }

          if(pattern == m || meets_special) {
            if(square(move.to)!=-1) {
                if(can_take&&colors[square(move.to)]!=colors[s_id]) {
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
            if(can_attack(king_pos, 1-color)) {
                valid = false;
            }
            else {
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
            if(square(start_pos+moves[i][0])==-1) { 
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
            Move move;
            move.id = s_id;
            move.from = start_pos;
            move.to = start_pos+moves[i][m];
            if(validate_move(move)) {
                result << move;
            }
        }
        for(int m=0;m<special_moves.length();m++) {
            Move move;
            move.id = s_id;
            move.from = start_pos;
            move.to = start_pos+special_moves[m];
            if(specialRules[s_id]==1) {
                ivec2 dd_m = ivec2(0,colors[i]==0?2:-2);
                if(special_moves[m]==dd_m) {
                    move.rule = 3;
                }
            }
            if(castling) move.rule = 2;
            if(validate_move(move)||castling) {
                result << move;
            }
        }
        //print(dtypes[i]," time ",inner.end());
        selected = nullptr;
    }
    // print("Loop time ",s.end());
    // s.start();
    list<Move> good_captures;
    list<Move> neutral_captures; 
    list<Move> bad_captures;
    list<Move> quiet_moves;
    for(auto& m : result) {
        // print_move(m);
        makeMove(m, false);
        bool kingInCheck = isKingInCheck(color);
        unmakeMove(m, false);
        if(!kingInCheck) {
            if(check_promotion(m)) m.rule = 1;
            if(m.takes==-1) 
                quiet_moves << m;
            else {
                int cap_val = capture_value(m, color);
                m.score = cap_val;
                if(cap_val >= 200) good_captures << m;
                else if(cap_val >= 0) neutral_captures << m;
                else bad_captures << m;
            }
        }
    }
    std::sort(good_captures.begin(), good_captures.end(), [](Move& a, Move& b) {
        return a.score > b.score;
    });
    good_captures << neutral_captures << quiet_moves << bad_captures;
   
    // print("Final time ",s.end());
    // print("Total time ",total.end());
    return good_captures;
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

static int tt_hits = 0;
static int tt_misses = 0;

void test_hash_consistency() {
    uint64_t hash1 = current_hash;
    uint64_t hash2 = hash_board();
    if(hash1 != hash2) {
        print("HASH MISMATCH! Incremental:", hash1, "Recalc:", hash2);
    }
}

int tt_lookup(uint64_t hash, int depth, int alpha, int beta) {
    TTEntry& entry = transposition_table[hash % TT_SIZE];
    
    if(!entry.valid || entry.hash != hash) {
        tt_misses++;
        return INT_MIN;
    }
    
    if(entry.depth >= depth) {
        tt_hits++;
        if(entry.flag == TT_EXACT) {
            return entry.score;
        }
        if(entry.flag == TT_ALPHA && entry.score <= alpha) {
            return entry.score;
        }
        if(entry.flag == TT_BETA && entry.score >= beta) {
            return entry.score;
        }
    }
    tt_misses++;
    return INT_MIN;
}

void tt_store(uint64_t hash, int depth, int score, int flag) {
    TTEntry& entry = transposition_table[hash % TT_SIZE];
    
    if(!entry.valid || entry.depth <= depth) {
        entry.hash = hash;
        entry.depth = depth;
        entry.score = score;
        entry.flag = flag;
        entry.valid = true;
    }
}

static int max_depth = 4;
#define ENABLE_AB 1
#define ENABLE_TT 1
#define ENABLE_PENALTY 1
#define ENABLE_TACTEXT 1
static int calcs = 0;
int minimax(int depth, int current_turn, int alpha, int beta) {
    if(depth == 0) return evaluate();
    test_hash_consistency();
    #if ENABLE_TT
    uint64_t hash = get_search_hash(current_turn);
    int tt_score = tt_lookup(hash, depth, alpha, beta);
    if(tt_score != INT_MIN) {
        return tt_score;
    }
    #endif

    auto moves = generateMoves(current_turn);
    if(moves.empty()) return evaluate();
    calcs+=2;
    if(current_turn == 0) {  // White maximizes
        int maxEval = -9999;
        int original_alpha = alpha;
        for(auto move : moves) {
            #if EVALUATE
            makeMove(move, true);
            if(current_turn==0&&depth>1)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, true);
            #else
            int extension = 0;
            #if ENABLE_TACTEXT
            if(move.takes != -1 && depth < max_depth+1) {
                int cap_val = capture_value(move, current_turn);
                if(cap_val > 200) {
                    extension = 1;
                }
            }
            #endif
            makeMove(move, false);
            #if ENABLE_PENALTY
            int repeats = history.getOrDefault(current_hash,1)-1;
            int penalty = 50 * (repeats * repeats);
            #endif
            int eval = minimax(depth+extension-1, 1-current_turn, alpha, beta);
            #if ENABLE_PENALTY
            eval -= penalty;
            #endif
            unmakeMove(move, false);
            #endif
            
            maxEval = std::max(maxEval, eval);
            #if ENABLE_AB
            alpha = std::max(alpha, eval);
            if(alpha >= beta) break;
            #endif
        }
        #if ENABLE_TT
        int flag = TT_EXACT;
        if(maxEval <= original_alpha) flag = TT_ALPHA;
        if(maxEval >= beta) flag = TT_BETA;
        
        tt_store(hash, depth, maxEval, flag);
        #endif
        return maxEval;
    } else {  // Black minimizes
        int minEval = 9999;
        int original_beta = beta;
        for(auto move : moves) {
            #if EVALUATE
            makeMove(move, true);
            if(current_turn==1&&depth>1)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int eval = minimax(depth-1, 1-current_turn, alpha, beta);
            unmakeMove(move, true);
            #else
            int extension = 0;
            #if ENABLE_TACTEXT
            if(move.takes != -1 && depth < max_depth+1) {
                int cap_val = capture_value(move, current_turn);
                if(cap_val > 200) {
                    extension = 1;
                }
            }
            #endif
            makeMove(move, false);
            #if ENABLE_PENALTY
            int repeats = history.getOrDefault(current_hash,1)-1;
            int penalty = 50 * (repeats * repeats);
            #endif
            int eval = minimax(depth+extension-1, 1-current_turn, alpha, beta);
            #if ENABLE_PENALTY
            eval += penalty;
            #endif
            unmakeMove(move, false);
            #endif
            
            minEval = std::min(minEval, eval);
            #if ENABLE_AB
            beta = std::min(beta, eval);
            if(alpha >= beta) break;
            #endif
        }
        #if ENABLE_TT
        int flag = TT_EXACT;
        if(minEval >= original_beta) flag = TT_BETA;
        if(minEval <= alpha) flag = TT_ALPHA;
        
        tt_store(hash, depth, minEval, flag);
        #endif
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
    list<Move> equal_moves;
    bool new_equal = false;
    tt_hits = 0;
    tt_misses = 0;
    #if LOG
    print("-----Finding move for ",color==0?"white":"black","-----");
    #endif
    int alpha = -9999;
    int beta = 9999;
    for(auto& move : moves) {
        makeMove(move,false);
        int repeats = history.getOrDefault(current_hash,1)-1;
        int penalty = 50 * (repeats * repeats);
        int score = minimax(depth-1, 1-color, alpha, beta);
        if(color == 0) {
            score -= penalty;
        } else {
            score += penalty;
        }
        if(penalty>50) print(mts(move)," Score: ",score);
        move.score = score;
        unmakeMove(move,false);
        calcs+=2;
        
        
        bool isBetter = color == 0 ? (score > bestScore) : (score < bestScore);
        if(isBetter) {
            bestScore = score;
            #if LOG
            print("New best: ",bestScore);
            print_move(move);
            #endif
            bestMove = move;
            new_equal = true;
        } else if (score==bestScore) {
            if(new_equal) {
                new_equal = false;
                equal_moves.clear();
                equal_moves << bestMove;
            }
            equal_moves << move;
        }

        if(color == 0) {
            alpha = std::max(alpha, score);
        } else {
            beta = std::min(beta, score);
        }
        
       if(alpha >= beta) break;
    }
    if(bestMove.id==-1) {
        print("Checkmate!");
        return bestMove;
    }

    if(!equal_moves.empty()) {
        //bestMove = equal_moves[randi(0,equal_moves.length()-1)];
    }
    #if LOG
    print("From depth: ",depth," Cacls performed: ",calcs," time: ",s.end()/1000000000,"s Moves: ",equal_moves.length()," Chosen score: ",bestMove.score);
    #if ENABLE_TT
    print("TT hits: ", tt_hits, " misses: ", tt_misses, " hit rate: ", (float)tt_hits/(tt_hits+tt_misses));
    #endif
    #endif
    calcs = 0;
    return bestMove;
}

list<Move> madeMoves;
void debug_hash() {
    print("=== NESTED CAPTURE DEBUG ===");
    
    // Get initial state
    uint64_t initial_hash = current_hash;
    uint64_t initial_board_hash = hash_board();
    print("Initial current_hash:", initial_hash);
    print("Initial board_hash:", initial_board_hash);
    print("Hashes match:", (initial_hash == initial_board_hash) ? "YES" : "NO");
    
    // Get some moves including captures
    auto moves = generateMoves(turn_color);
    Move capture_move;
    Move other_move;
    bool found_capture = false, found_other = false;

    for(auto& move : moves) {
        test_hash_consistency();
        makeMove(move,false);
        makeMove(move,false);
        unmakeMove(move,false);
        unmakeMove(move,false);
    }
    print("===DONE===");
}

int main() {
    using namespace helper;

    std::string MROOT = "../Projects/FirChess/assets/models/";

    Window window = Window(1280, 768, "FirChess 0.9.0");
    scene = make<Scene>(window,2);
    scene->camera.toOrbit();
    //scene->camera.lock = true;
    Data d = make_config(scene,K);
    load_gui(scene, "FirChess", "firchessgui.fab");

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
        for(int i=0;i<2;i++) setup_piece("bishop_"+col,ctf(i==0?'c':'f'),rank);
        for(int i=0;i<2;i++) setup_piece("rook_"+col,ctf(i==0?'a':'h'),rank);
        for(int i=0;i<2;i++) setup_piece("knight_"+col,ctf(i==0?'b':'g'),rank);
        for(int i=1;i<9;i++) setup_piece("pawn_"+col,i,k==0?2:7);
        setup_piece("king_"+col,ctf('e'),rank);
    }

    srand(12345);
    for(int piece = 0; piece < 32; piece++) {
        for(int square = 0; square < 64; square++) {
            zobrist_table[piece][square] = random64bit();
        }
    }
    turn_zobrist_key = random64bit();
    current_hash = hash_board();
    for(int i = 0; i < TT_SIZE; i++) {
        transposition_table[i].valid = false;
    }


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
   // Move bot_move;

    bool bot_color = 0;
    //Make the little mouse to reperesnt the bot (Fir!)
    auto Fir = make<Single>(make<Model>("../models/agents/Snow.glb"));
    scene->add(Fir);
    Fir->setPosition(bot_color==0?vec3(1,-1,-9):vec3(1,-1,11));
    if(bot_color==1) Fir->faceTo(vec3(0,-1,0));
    auto bot = make<Thread>();
    bot->run([&](ScriptContext& ctx){
        if(turn_color==bot_color) {
            bot_turn = true;
            Move m = findBestMove(max_depth,turn_color);
            if(m.id!=-1) {
                makeMove(m);
                madeMoves << m;
                turn_color = turn_color==0?1:0;
            }
            else
                bot->pause();
            bot_turn = false;
        }
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

    
    bool auto_turn = true;
    if(auto_turn) bot->start();
    int last_col = turn_color;

    //debug_hash();

    start::run(window,d,[&]{
        vec3 mousePos = scene->getMousePos(0);
        if(mousePos.x()>8.0f) mousePos.setX(8.0f);
        if(mousePos.x()<-6.0f) mousePos.setX(-6.0f);
        if(mousePos.z()>8.0f) mousePos.setZ(8.0f);
        if(mousePos.z()<-6.0f) mousePos.setZ(-6.0f);

        if(last_col!=turn_color) {
            text::setText(turn_color==0?"White":"Black",scene->getSlot("turn")[0]);
        }
        last_col = turn_color;

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
            if(free_camera){print("Locked camera"); scene->camera.lock = true; free_camera=false;}
            else{print("Free camera"); scene->camera.lock = false; free_camera=true;}
        }
        if(pressed(T)) {
            if(held(LSHIFT)) {
                //generateMoves(turn_color);
                if(!auto_turn) {auto_turn = true; print("Auto turn");}
                else {auto_turn=false; print("Disabled auto turn");}
            }
            else {
                Move m = findBestMove(max_depth,turn_color);
                if(m.id!=-1) {
                    makeMove(m);
                    madeMoves << m;
                }
            }
        }
        if(pressed(Y)) {
            vec3 clickPos = num_grid->snapToGrid(scene->getMousePos());
            if(in_bounds(world_to_board(clickPos)))
                takePiece(square(world_to_board(clickPos)));
        }
        if(pressed(R)) if(!madeMoves.empty()) unmakeMove(madeMoves.pop());
        if(pressed(NUM_1)) scene->camera.toOrbit();
        if(pressed(NUM_2)) scene->camera.toIso();
        if(pressed(NUM_3)) scene->camera.toFirstPerson();

        if(pressed(E)) {
            auto clickPos = num_grid->snapToGrid(scene->getMousePos());
            ivec2 v = world_to_board(clickPos);
            if(in_bounds(v)) {
            //grid->toIndex(clickPos)
            print("----",bts(v),"----");
           if(square(v)!=-1) {
                print("E:",dtypes[square(v)]," I:",square(v));
           }
           else print("EMPTY");
            }
        }
        if(pressed(G)) {
            print(isKingInCheck(turn_color)==0?"No check":"In check");
        }
        if(pressed(MOUSE_LEFT)) {
            if(!selected) {
                 auto clickPos = num_grid->snapToGrid(scene->getMousePos());
                 ivec2 v = world_to_board(clickPos);
                if(in_bounds(v))
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
              if(in_bounds(v))
              {
              update_num_grid(selected,newPos);
              bool castling = false;
              if(validate_castle(s_id,v)) {
                castling = true;
              } 
              Move move;
              move.id = s_id;
              move.from = start_pos;
              move.to = v;
              if(castling) move.rule = 2;
              if(validate_move(move)||debug_move||castling) {
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
                    //print("Move repeated: ",history.getOrDefault(current_hash,0));
                    madeMoves << move;
                    if(auto_turn) {
                        turn_color = 1-turn_color;
                    }
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
         }

         if(selected&&!bot_turn)
         {
            vec3 targetPos = num_grid->snapToGrid(mousePos).addY(1);
            vec3 direction = targetPos - selected->getPosition();
            float distance = direction.length();
            float moveSpeed = distance>=6.0f?distance*2:6.0f;
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
                if(bot->runningTurn) {bot->pause(); bot_turn=false;}
                else {bot->start();}
            }
        }

        if(pressed(P)) {
            if(phys_thread->runningTurn) {phys_thread->pause();}
            else {phys_thread->start();}
        }

    });

return 0;
}