#define CRUMB_ROWS 2
#define CRUMB_COLS 2

const int POS = (0 << 16) | 1;
const int VAL = (1 << 16) | 1;


#include<core/helper.hpp>
#include<core/numGrid.hpp>
#include<util/meshBuilder.hpp>
#include<core/type.hpp>
#include<core/thread.hpp>
#include<util/logger.hpp>
#include<util/nodenet.hpp>

using namespace Golden;
using namespace Log;

#define EVALUATE 1
#define LOG 0
#define TURN_TAGS 0
static int max_depth = 2;
#define ENABLE_AB 1
#define ENABLE_TT 1
#define ENABLE_PENALTY 1
#define ENABLE_TACTEXT 1
#define ENABLE_BOOK 1
#define ENABLE_BOARDSPLIT 0

#define USE_NODENET 1
#define LOG_MATCHING 0

bool debug_move = false;
bool free_camera = true;
bool bot_turn = false;


struct Move {
    int id, score;
    ivec2 from, to;
    int takes=-1;
    bool first_move = false;
    int rule = 0;
    //0 == No rule
    //1 == Promotions
    //2 == Castling
    //3 == Pawn Double Move
    //4 == En Passante
    
    int c_id = -1;
    ivec2 c_to,c_from;

    ivec2 e_square = {-1,-1};

    void saveBinary(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&id), sizeof(int));
        out.write(reinterpret_cast<const char*>(&score), sizeof(int));
        out.write(reinterpret_cast<const char*>(&takes), sizeof(int));
        out.write(reinterpret_cast<const char*>(&first_move), sizeof(bool));
        out.write(reinterpret_cast<const char*>(&rule), sizeof(int));
        out.write(reinterpret_cast<const char*>(&c_id), sizeof(int));

        out.write(reinterpret_cast<const char*>(&from), sizeof(ivec2));
        out.write(reinterpret_cast<const char*>(&to), sizeof(ivec2));
        out.write(reinterpret_cast<const char*>(&c_to), sizeof(ivec2));
        out.write(reinterpret_cast<const char*>(&c_from), sizeof(ivec2));
        out.write(reinterpret_cast<const char*>(&e_square), sizeof(ivec2));      
    }

    void loadBinary(std::istream& in) {
        in.read(reinterpret_cast<char*>(&id), sizeof(int));
        in.read(reinterpret_cast<char*>(&score), sizeof(int));
        in.read(reinterpret_cast<char*>(&takes), sizeof(int));
        in.read(reinterpret_cast<char*>(&first_move), sizeof(bool));
        in.read(reinterpret_cast<char*>(&rule), sizeof(int));
        in.read(reinterpret_cast<char*>(&c_id), sizeof(int));

        in.read(reinterpret_cast<char*>(&from), sizeof(ivec2));
        in.read(reinterpret_cast<char*>(&to), sizeof(ivec2));
        in.read(reinterpret_cast<char*>(&c_to), sizeof(ivec2));
        in.read(reinterpret_cast<char*>(&c_from), sizeof(ivec2));
        in.read(reinterpret_cast<char*>(&e_square), sizeof(ivec2));     
    }
};

g_ptr<Scene> scene = nullptr;
g_ptr<NumGrid> num_grid = nullptr;

list<g_ptr<Single>> white_losses;
list<g_ptr<Single>> black_losses;

uint64_t turn_zobrist_key;
map<uint64_t,int> history;

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

int turn_color = 0; //0 = White, 1 = Black
int turn_number = 0;
list<g_ptr<Single>> drop;
list<Move> madeMoves;

map<uint64_t, list<Move>> opening_book;

uint64_t random64bit() {
    return ((uint64_t)rand() << 32) | rand();
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
    process_directory_recursive(root()+"/Projects/"+project_name+"/assets/models/");

    std::string data_string = "NONE";
    try {
    data_string = readFile(root()+"/Projects/"+project_name+"/assets/models/"+project_name+" - data.csv");
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

void update_num_grid(g_ptr<Single> selected,const vec3& pos) {
    num_grid->getCell(selected->getPosition()).erase(selected->ID);
    num_grid->getCell(pos) << selected->ID;
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

bool in_bounds(const vec3& pos) {
    return(pos.x()<=8.0f&&pos.x()>=-6.0f&&pos.z()<=8.0f&&pos.z()>=-6.0f);
}

bool in_bounds(const ivec2& pos) {
    return(pos.x()<=8&&pos.x()>=1&&pos.y()<=8&&pos.y()>=1);
}


void save_opening_book() {
    std::string path = root()+"/Projects/FirChess/assets/games/book.odc";
    std::ofstream out(path, std::ios::binary);
    
    uint32_t size = opening_book.size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    
    for(auto& entry : opening_book.entrySet()) {
        uint64_t hash = entry.key;
        out.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
        
        uint32_t moveCount = entry.value.length();
        out.write(reinterpret_cast<const char*>(&moveCount), sizeof(moveCount));
        
        for(auto& move : entry.value) {
            move.saveBinary(out);
        }
    }
    out.close();
}

void load_opening_book() {
    std::string path = root()+"/Projects/FirChess/assets/games/book.odc";
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Can't read opening book: " + path);
    opening_book.clear();
    uint32_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    for(uint32_t i = 0; i < size; i++) {
        uint64_t hash;
        in.read(reinterpret_cast<char*>(&hash), sizeof(hash));
        
        uint32_t moveCount;
        in.read(reinterpret_cast<char*>(&moveCount), sizeof(moveCount));
        
        list<Move> moves;
        for(uint32_t j = 0; j < moveCount; j++) {
            Move move;
            move.loadBinary(in);
            moves << move;
        }
        
        opening_book.put(hash, moves);
    }
    in.close();
}

static int tt_hits = 0;
static int tt_misses = 0;

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

class Board : public Object {
public:
    Board() {}
    ivec2 enpassant_square{-1,-1};

    g_ptr<Single> selected = nullptr;
    int s_id = 0;
    ivec2 start_pos{0,0};

    list<list<int>> grid;
    list<std::string> dtypes;
    list<g_ptr<Single>> ref;
    list<bool> captured;
    list<int> colors;
    list<int> values;
    list<int> specialRules;
    list<ivec2> cells;
    list<list<ivec2>> moves;
    list<bool> hasMoved;

    list<Move> to_process;

    uint64_t zobrist_table[32][64];
    uint64_t current_hash = 0;

void operator=(g_ptr<Board> board) {
    enpassant_square = board->enpassant_square;
    grid = board->grid;
    dtypes = board->dtypes;
    ref = board->ref;
    captured = board->captured;
    colors = board->colors;
    values = board->values;
    specialRules = board->specialRules;
    cells = board->cells;
    moves = board->moves;
    hasMoved = board->hasMoved;
    for(int a = 0;a<32;a++) {
        for(int b = 0;b<64;b++) {
            zobrist_table[a][b] = board->zobrist_table[a][b];
        }
    }
    current_hash = board->current_hash;
}

void sync_with(g_ptr<Board> board) {
    enpassant_square = board->enpassant_square;
    grid = board->grid;
    dtypes = board->dtypes;
    ref = board->ref;
    captured = board->captured;
    colors = board->colors;
    values = board->values;
    specialRules = board->specialRules;
    cells = board->cells;
    moves = board->moves;
    hasMoved = board->hasMoved;
    for(int a = 0;a<32;a++) {
        for(int b = 0;b<64;b++) {
            zobrist_table[a][b] = board->zobrist_table[a][b];
        }
    }
    current_hash = board->current_hash;
}
// Piece to string
std::string pts(int id) {
    return dtypes[id]+"-"+std::to_string(id);
}

// Move to string
std::string mts(Move move) {
    return pts(move.id)+" from "+bts(move.from)+" to "+bts(move.to)+(move.takes!=-1?" takes "+pts(move.takes):"")+(move.rule==2?" castle ":move.rule==1?" promote ":"");
}

void print_move(Move move) {
    print(mts(move));
}

inline ivec2 pos_of(int piece) {
    return cells[piece];
}

vec3 world_pos_of(int piece) {
    return board_to_world(pos_of(piece));
}

inline int& square(const ivec2& c) {
    return grid[c.x()][c.y()];
}

inline void update_grid(int id,const ivec2& to) {
    ivec2 from = cells[id];
    int old_square = (from.y() - 1) * 8 + (from.x() - 1);
    int new_square = (to.y() - 1) * 8 + (to.x() - 1);

    if(id==-1) {
        print("update_grid::327 attempted to update nonexistent id piece from ",bts(from)," to ",bts(to));
        return;
    }

    if(square(from) != id) {
        print("update_grid::331 piece ", id, " not at expected position ", bts(from));
        return; 
    }

    current_hash ^= zobrist_table[id][old_square];  // Remove from old square
    current_hash ^= zobrist_table[id][new_square];  // Add to new square


    square(cells[id]) = -1;
    square(to) = id;
    cells[id] = to;
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
    if(color == 0) hash ^= turn_zobrist_key;
    return hash;
}

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

void makeMove(Move& move,bool real = true,bool is_linked = false) {
    if(!hasMoved[move.id]) {
        move.first_move = true;
        hasMoved[move.id] = true;
    }
    if(move.rule != 2 || move.rule==4) {
        //Damned en passant...
        int t = move.rule==4?square(move.to-(colors[move.id]==0?ivec2(0,1):ivec2(0,-1))):square(move.to);
        if(t!=-1) {
            takePiece(t,real);
            move.takes = t;
        }
    }

    move.e_square = enpassant_square;
    
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
        makeMove(linked,real,true);
    } //Pawn double move
    if(move.rule == 3) {
        enpassant_square = move.to-(colors[move.id]==0?ivec2(0,1):ivec2(0,-1));
    } else {
        enpassant_square = ivec2(-1,-1);
    }

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

    enpassant_square = move.e_square;

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

    ivec2 return_to = move.to;
    if(move.rule==4) {
        return_to = move.to-(colors[move.id]==0?ivec2(0,1):ivec2(0,-1));
    }
    if(move.takes!=-1) {
        captured[move.takes] = false;
        int new_square = (return_to.y() - 1) * 8 + (return_to.x() - 1);
        current_hash ^= zobrist_table[move.takes][new_square];
        square(cells[move.takes]) = -1;
        square(return_to) = move.takes;
        //update_grid(move.takes,move.to);
        if(real) {
            update_num_grid(ref[move.takes],board_to_world(return_to));
            ref[move.takes]->setPosition(board_to_world(return_to));
        }
    }

    if(real) {
        history.getOrPut(current_hash,0)--;
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

                  if(enpassant_square!=ivec2(-1,-1)) {
                    if(move.to==enpassant_square) {
                        meets_special = true;
                        can_take = true;
                        move.rule = 4;
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

list<Move> generateMovesForPiece(int i) {
    list<Move> result;
    bool castling = false;
    int color = (i<16?0:1);
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
        if(enpassant_square!=ivec2(-1,-1)) {
            if(d_r==enpassant_square) {
                special_moves << d_r;
            } 
            if(d_l==enpassant_square) {
                special_moves << d_l;
            }
        }
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

    if(specialRules[i]!=2) { //Don't evaluate normal moves for sliding peices
        for(int m=0;m<moves[i].length();m++) {
            Move move;
            move.id = s_id;
            move.from = start_pos;
            move.to = start_pos+moves[i][m];
            if(validate_move(move)) {
                result << move;
            }
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
    selected = nullptr;
    return result;
}

list<Move> sortMoveList(list<Move> result) {
    list<Move> good_captures;
    list<Move> neutral_captures; 
    list<Move> bad_captures;
    list<Move> quiet_moves;
    for(auto& m : result) {
        makeMove(m, false);
        bool kingInCheck = isKingInCheck((m.id<16?0:1));
        unmakeMove(m, false);
        if(!kingInCheck) {
            if(check_promotion(m)) m.rule = 1;
            if(m.takes==-1) 
                quiet_moves << m;
            else {
                int cap_val = capture_value(m,(m.id<16?0:1));
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
    return good_captures;
}

list<Move> generateMoves(int color) {
    // Line total;
    // total.start();
    // Line s;
    // s.start();
    list<Move> result;
    //if(captured[color==0?white_king_id:black_king_id]) return result;
    for(int i=(color==0?0:16);i<(color==0?16:32);i++) {
        if(captured[i]) continue;
        result << generateMovesForPiece(i);
    }
    return sortMoveList(result);
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

int calcs = 0;

int minimax(int depth, int color, int alpha, int beta) {
    if(depth == 0) return evaluate();
    test_hash_consistency();
    uint64_t hash = get_search_hash(color);
    #if ENABLE_TT
    int tt_score = tt_lookup(hash, depth, alpha, beta);
    if(tt_score != INT_MIN) {
        return tt_score;
    }
    #endif

    auto moves = generateMoves(color);
    #if ENABLE_BOOK
    if(opening_book.hasKey(hash)) {
        moves = opening_book.get(hash);
    }
    #endif
    if(moves.empty()) {
        return color==0?-10000:10000;
    }
    calcs+=2;
    int bestEval = color==0?-9999:9999;
    int original_alpha = alpha;
    int original_beta = beta;
    for(auto move : moves) {
        int extension = 0;
        #if ENABLE_TACTEXT
        if(move.takes != -1 && depth < max_depth+6) {
            int cap_val = capture_value(move, color);
            if(cap_val > 200) {
                extension = 1;
            }
        }
        #endif
        makeMove(move, false);
        #if ENABLE_TACTEXT
        if(isKingInCheck(1-color) && depth < max_depth+6) {
            extension = 1;
        }
        #endif
        #if ENABLE_PENALTY
        int repeats = history.getOrDefault(current_hash,1)-1;
        int penalty = 50 * (repeats * repeats);
        #endif
        int eval = minimax(depth+extension-1, 1-color, alpha, beta);
        #if ENABLE_PENALTY
        if(color==0) eval -= penalty;
                else eval += penalty;
        #endif
        unmakeMove(move, false);

        if(color==0)
            bestEval = std::max(bestEval, eval);
        else
            bestEval = std::min(bestEval, eval);

        #if ENABLE_AB
        if(color==0)
            alpha = std::max(alpha, eval);
        else
            beta = std::min(beta, eval);
        if(alpha >= beta) break;
        #endif
    }

    #if ENABLE_TT
    int flag = TT_EXACT;
    if(color==0) {
        if(bestEval <= original_alpha) flag = TT_ALPHA;
        if(bestEval >= beta) flag = TT_BETA;
    }
    else {
        if(bestEval >= original_beta) flag = TT_BETA;
        if(bestEval <= alpha) flag = TT_ALPHA;
    }
    
    tt_store(hash, depth, bestEval, flag);
    #endif
    return bestEval;
}

Move best;
int progress = 0;
void test_hash_consistency() {
    uint64_t hash1 = current_hash;
    uint64_t hash2 = hash_board();
    if(hash1 != hash2) {
        print("HASH MISMATCH! Incremental:", hash1, "Recalc:", hash2);
    }
}

void process_moves(int depth,int color) {
    Line s; s.start();
    uint64_t hash = get_search_hash(color);
    Move bestMove = best;
    int bestScore = color == 0 ? -9999 : 9999;
    list<Move> equal_moves;
    bool new_equal = false;
    #if LOG
    //print("-----Finding move for ",color==0?"white":"black","-----");
    #endif
    int alpha = -9999;
    int beta = 9999;
    for(int i=0;i<to_process.length();i++) {
        if(i>=to_process.length()) break;
        Move& move = to_process[i];
        progress = i;
        makeMove(move,false);
        int repeats = history.getOrDefault(current_hash,1)-1;
        int penalty = 50 * (repeats * repeats);
        int score = minimax(depth-1, 1-color, alpha, beta);
        if(color == 0) {
            score -= penalty;
        } else {
            score += penalty;
        }
        //if(penalty>50) print(mts(move)," Score: ",score);
        move.score = score;
        unmakeMove(move,false);
        calcs+=2;
        
        
        bool isBetter = color == 0 ? (score > bestScore) : (score < bestScore);
        if(isBetter) {
            bestScore = score;
            #if LOG
            // print("New best: ",bestScore);
            // print_move(move);
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
        //print("Checkmate!");
        best = bestMove;
    }

    if(!equal_moves.empty()) {
        //bestMove = equal_moves[randi(0,equal_moves.length()-1)];
    }
    best = bestMove;
}

};

g_ptr<Board> global;
g_ptr<Thread> global_thread;

class chess_nodenet : public nodenet {
public:
    float salience(g_ptr<Crumb> c) override {
        return c->mat[1][0]; // just value for now
    }

    g_ptr<Crumb> piece_to_crumb(int id, int perspective_color, ivec2 anchor = ivec2(0,0)) {
        g_ptr<Crumb> c = clone(ZERO);
        c->id = id;
        ivec2 pos = global->pos_of(id) - anchor;
        c->mat[0][0] = pos.x();
        c->mat[0][1] = pos.y();
        c->mat[1][0] = (global->colors[id] == perspective_color ? 1 : -1) * (global->values[id] / 1000.0f);
        return c;
    }

    list<g_ptr<Crumb>> gather_states_from_move(Move& move, int perspective_color) {
        list<g_ptr<Crumb>> states;
        ivec2 anchor = move.to;
        states << piece_to_crumb(move.id, perspective_color,anchor);
        
        if(move.takes != -1) {
            states << piece_to_crumb(move.takes, perspective_color,anchor);
            
            for(auto id : global->is_attacking(move.to, 1 - global->colors[move.id])) {
                if(states.length() >= cognitive_focus) break;
                //if(id==white_king_id||id==black_king_id) continue;
                states << piece_to_crumb(id, perspective_color,anchor);
            }
            for(auto id : global->is_attacking(move.to, global->colors[move.id])) {
                if(states.length() >= cognitive_focus) break;
                //if(id==white_king_id||id==black_king_id) continue;
                if(id == move.id) continue;
                states << piece_to_crumb(id, perspective_color,anchor);
            }
        }

        // states << piece_to_crumb(white_king_id,perspective_color,anchor);
        // states << piece_to_crumb(black_king_id,perspective_color,anchor);
        
        return states;
    }

    Move derive_move(g_ptr<Episode> ep) {
        Move mov;
        mov.id = -1;
        if(!ep->states.empty()) {
            g_ptr<Crumb> anchor = ep->states[0];
            if(!is_mutable(anchor) || anchor->id == -1) return mov;
            mov.id = anchor->id;
            mov.from = global->pos_of(mov.id);
            mov.to = mov.from - ivec2((int)anchor->mat[0][0], (int)anchor->mat[0][1]);
        }
        return mov;
    }

    void propagate(g_ptr<Episode> start, float energy, std::function<bool(g_ptr<Episode>, float, int)> visit, int depth = 0) override {
        //This implmentation is chess specific, but most will be encouraged to use the action struct instead.
        //I do this because A. I want to get it working in chess without worrying about what's working
        //and B. because a lot of people will want to implment onto existing infrastructure.
        if(energy <= 1.0f) return;
        Move mov = derive_move(start);
        if(mov.id != -1) {
            global->makeMove(mov, false);
        }

        nodenet::propagate(start, energy, visit, depth);
    
        if(mov.id != -1) {
            list<g_ptr<Crumb>> after = gather_states_from_move(mov, global->colors[mov.id]);
            g_ptr<Episode> new_ep = form_episode(start->states, after, mov.rule, -1); //<- no timestamp for now, but could slot depth here
            if(consolidate_episode(new_ep,recent_episodes,ALL,0.9f,0.1f)>4) { //If it has enough hits to consolidate properly
                recent_episodes.erase(new_ep);
                consolidated_episodes << new_ep;
            }

            while(recent_episodes.length() > max_recent_episode_window) //May or may not go here
                recent_episodes.removeAt(0);
            global->unmakeMove(mov, false);
        }
    }

    //There's a wrapper in nodnet that calls this with episode->states for crumbs
    list<g_ptr<Episode>> potential_reactions(list<g_ptr<Crumb>> crumbs) override {
        list<g_ptr<Episode>> reactions;
        if(crumbs.empty()) return reactions;
        int id = crumbs[0]->id;
        if(id==-1) return reactions;
        for(auto& state : crumbs) {
            if(!is_mutable(state)) continue;
            if(state->id == -1 || global->captured[state->id]) continue;
            for(auto& mov : global->sortMoveList(global->generateMovesForPiece(id))) {
                auto states = gather_states_from_move(mov, global->colors[id]);

                float best_match = 0.0f;
                for(auto& ep : consolidated_episodes)
                    best_match = std::max(best_match, match_episode(ep, ALL, states, ALL));
                
                float novelty = 1.0f;
                for(auto& ep : recent_episodes)
                    novelty = std::min(novelty, 1.0f - match_episode(ep, ALL, states, ALL));
                
                float combined = best_match + (novelty * 0.5f);
                
                if(combined > 0.2f || std::abs(mov.score) >= 200) {
                    // print("PIECE: ",global->pts(mov.id)," COMBINED: ",combined," SCORE: ",mov.score);
                    g_ptr<Episode> reaction = make<Episode>();
                    reaction->states = states;
                    reaction->action_id = mov.id;
                    reaction->score = (std::abs(mov.score) >= 200 ? std::abs(mov.score)/200.0f : best_match);
                    reactions << reaction;
                }
            }
        }
        return reactions;
    }


    Move pick_move(int color) {
        Move to_return;
        to_return.id = -1;
        if(madeMoves.empty()) {
            list<Move> fallback_moves = global->generateMoves(color);
            if(!fallback_moves.empty()) {
                return fallback_moves[0];
            } else {
                print(red("NO FALLBACK MOVE!!!"));
            }
        }
        Move& last_move = madeMoves.last();

        list<std::pair<g_ptr<Episode>, float>> candidates;
        float energy = 10.0f;

        for(auto potential_move : potential_reactions(gather_states_from_move(last_move, color))) {
            //print("ATTEMPTING TO PROPAGATE THROUGH A MOVE, SCORE: ",potential_move->score," ID: ",potential_move->states[0]->id);
            candidates << std::make_pair(potential_move, 0.0f);
            propagate(potential_move, energy, [&](g_ptr<Episode> ep, float remaining_energy, int depth) {
                if(ep->score > 0.2f) {
                    g_ptr<Crumb> anchor = ep->states[0];
                    if(is_mutable(anchor)&&anchor->id != -1&&!global->captured[anchor->id]&&global->colors[anchor->id] == color) {
                        candidates.last().second = std::max(candidates.last().second, ep->score * remaining_energy);
                    }
                    return true;
                }
                return false;
            });
        }
        
        // Winner take all sweep
        float best_score = 0.0f;
        g_ptr<Episode> winner = nullptr;
        for(auto& candidate : candidates) {
            if(candidate.second > best_score) {
                best_score = candidate.second;
                winner = candidate.first;
            }
        }
        
        if(winner != nullptr) {
            to_return = derive_move(winner);
        }
        
        if(to_return.id == -1 && !global->generateMoves(color).empty())
            to_return = global->generateMoves(color)[0];
        
        return to_return;
    }
};

#if USE_NODENET
    g_ptr<chess_nodenet> net[2];
#endif

void setup_piece(const std::string& type,int file,int rank) {
    auto piece = scene->create<Single>(type);
    piece->setPosition(board_to_world(file,rank));
    ivec2 mySquare{file,rank};
    global->square(mySquare) = piece->ID;
    global->cells << mySquare;

    //update_cells(piece);
    std::string dtype = piece->get<std::string>("dtype");
    if(dtype=="king_white") white_king_id = global->dtypes.length();
    if(dtype=="king_black") black_king_id = global->dtypes.length();
    if(dtype=="queen_white") white_queen_id = global->dtypes.length();
    if(dtype=="queen_black") black_queen_id = global->dtypes.length();
    if(dtype=="rook_white") {
        if(white_rook_ids[0]==-1) white_rook_ids[0] = global->dtypes.length();
        else white_rook_ids[1] = global->dtypes.length();
    }
    if(dtype=="rook_black") {
        if(black_rook_ids[0]==-1) black_rook_ids[0] = global->dtypes.length();
        else black_rook_ids[1] = global->dtypes.length();
    }
    global->dtypes << dtype;
    global->colors << piece->get<int>("color");
    int value = piece->get<int>("value");
    global->values << value;
    int specialRule = piece->get<int>("specialRule");
    global->specialRules << specialRule;
    list<ivec2> n_moves = piece->get<list<ivec2>>("moves");
    global->moves << n_moves;
    global->captured << false;
    global->ref << piece;
    global->hasMoved << false;
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

void save_game(const std::string& file) {
    std::string path = root()+"/Projects/FirChess/assets/games/"+file+".gdc";
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Can't write to file: " + path);
    out.write(reinterpret_cast<const char*>(&turn_color), sizeof(turn_color));
    uint32_t moveLen = madeMoves.length();
    out.write(reinterpret_cast<const char*>(&moveLen), sizeof(moveLen));
    for (const auto& m : madeMoves) {
        m.saveBinary(out);
    }
    out.close();
}

void load_game(const std::string& file) {
    std::string path = root()+"/Projects/FirChess/assets/games/"+file+".gdc";
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Can't read from file: " + path);
    in.read(reinterpret_cast<char*>(&turn_color), sizeof(turn_color));
    uint32_t moveLen;
    in.read(reinterpret_cast<char*>(&moveLen), sizeof(moveLen));
    madeMoves.clear();
    for (uint32_t i = 0; i < moveLen; i++) {
        Move m;
        m.loadBinary(in);
        global->makeMove(m);
        madeMoves << m;
    }
    in.close();
}

constexpr int board_count = 4;
static g_ptr<Thread> threads[board_count];
static g_ptr<Board> boards[board_count];

Move findBestMove(int depth,int color) {
    Line s; s.start();
    auto moves = global->generateMoves(color);
    if(moves.empty()) print("Out of moves");
    uint64_t hash = global->get_search_hash(color);
    #if ENABLE_BOOK
    bool using_book = false;
    if(opening_book.hasKey(hash)) {
        using_book = true;
        moves = opening_book.get(hash);
        #if !EVALUATE
            print("Using ",moves.length()," book move(s)");
            for(auto m : moves) {
                print(" -> ",global->mts(m));
            }
        #endif
    }
    #endif
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
    #if ENABLE_BOOK
    if(using_book) {
        bestMove = moves[randi(0,moves.length()-1)];
        return bestMove;
    }
    #endif

    int total_calcs = 0;
    list<Move> eval;

    #if ENABLE_BOARDSPLIT
        list<list<Move>> board_moves;
        for(int i = 0;i<board_count;i++) {
            board_moves << list<Move>{};
        }
        for(int i = 0;i<moves.length();i++) {
            board_moves[i%board_count] << moves[i];
        }

        int complete = 0;
        bool completed[board_count];
        bool stolen[board_count];
        for(int i = 0;i<board_count;i++) {
            completed[i] = false;
            stolen[i] = false;
            threads[i]->queueTask([&,i](){
                boards[i]->to_process = board_moves[i];
                Move new_move;
                new_move.id = -1;
                new_move.score = color == 0 ? -9999 : 9999;
                boards[i]->best = new_move;
                boards[i]->calcs = 0;
                boards[i]->progress=0;
                boards[i]->process_moves(max_depth,color);
                complete++;
                completed[i] = true;
                while(complete!=board_count) {
                    for(int j=0;j<board_count;j++) {
                        if(j==i) continue;
                        if(!completed[j]&&!stolen[j]) {
                            //print(i," stealing from ",j);
                            stolen[j] = true;
                            completed[i] = false;
                            complete--;
                            int l = boards[j]->to_process.length();
                            int halfway = (l-boards[j]->progress)/2;
                            for(int k=l;k>=(l-halfway);k--) {
                                boards[i]->to_process.push(boards[j]->to_process.pop());
                            }
                            boards[i]->process_moves(max_depth,color);
                            //print(i," finished ",halfway," from ",j);
                            complete++;
                            completed[i] = true;
                            break;
                        }
                    }
                }
            });
        }

        auto lastTime = std::chrono::high_resolution_clock::now();
        while(complete!=board_count) {
            auto currentTime = std::chrono::steady_clock::now();
            float delta = std::chrono::duration<float>(currentTime - lastTime).count();
            if(delta>=0.3) {
                for(int i=0;i<board_count;i++) {
                    print(i," calcs: ",boards[i]->calcs,stolen[i]?" STOLEN ":"",completed[i]?" DONE ":"");
                }
                lastTime=std::chrono::high_resolution_clock::now();
            }
        
        }

        for(int i=0;i<board_count;i++) {
            total_calcs+=boards[i]->calcs;
        }
        for(int i = 0;i<board_count;i++) {
            eval << boards[i]->best;
        }
    #else
        global->to_process = moves;
        Move new_move;
        new_move.id = -1;
        new_move.score = color == 0 ? -9999 : 9999;
        global->best = new_move;
        global->calcs = 0;
        global->progress=0;
        bool complete = false;

        global_thread->queueTask([&](){
            global->process_moves(max_depth,color);
            complete = true;
        });

        auto lastTime = std::chrono::high_resolution_clock::now();
        while(!complete) {
            auto currentTime = std::chrono::steady_clock::now();
            float delta = std::chrono::duration<float>(currentTime - lastTime).count();
            if(delta>=0.3) {
                #if !EVALUATE
                    print(" calcs: ",global->calcs," (",global->progress," out of ",global->to_process.length(),") at max_depth ",max_depth);
                #endif
                lastTime=std::chrono::high_resolution_clock::now();
            }
        
        }

        total_calcs = global->calcs; 
        eval << global->best;
    #endif

    
    for(auto& move : eval) {
        bool isBetter = color == 0 ? (move.score > bestScore) : (move.score < bestScore);
        if(isBetter) {
            bestScore = move.score;
            #if LOG
            print("New best: ",bestScore);
            global->print_move(move);
            #endif
            bestMove = move;
            new_equal = true;
        } else if (move.score==bestScore) {
            if(new_equal) {
                new_equal = false;
                equal_moves.clear();
                equal_moves << bestMove;
            }
            equal_moves << move;
        }

        if(color == 0) {
            alpha = std::max(alpha, move.score);
        } else {
            beta = std::min(beta, move.score);
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
    print("From depth: ",depth," Cacls performed: ",total_calcs," time: ",s.end()/1000000000,"s (c/s: ",total_calcs/(s.end()/1000000000),") Moves: ",equal_moves.length()," Chosen score: ",bestMove.score);
    #if ENABLE_TT
    print("TT hits: ", tt_hits, " misses: ", tt_misses, " hit rate: ", (float)tt_hits/(tt_hits+tt_misses));
    #endif
    #endif
    // calcs = 0;
    return bestMove;
}

Move parse_move(const std::string& pat) {
    auto s = split_str(pat, '-');
    Move result;
    result.from = ivec2(ctf(s[0][0]),std::stoi(s[0].substr(1,1)));
    result.to = ivec2(ctf(s[1][0]),std::stoi(s[1].substr(1,1)));
    result.id = global->square(result.from);
    if(s.length()>2) {
        if(s[2]=="P") { //Promotion
            result.rule = 1;
        }
        else if(s[2]=="C") { // Castle
            result.rule = 2;
        }
        else if(s[2]=="DD") { // Pawn double move
            result.rule = 3;
        }
        else if(s[2]=="EP") { // En passante
            result.rule = 3;
        }
    }
    return result;
}

#define LOG_OPEN 0

void build_opening(const std::string& pattern,int color = 0,std::string indent = "  ") {
    //print("Processing patern: ",pattern);
    // auto seq = //Split into balanced chunks

    int depth = 0;
    auto it = pattern.begin();
    std::string seq = "";
    std::string sub_seq = "";
    list<std::string> seqs;
    bool in_sub = false;
    while(it!=pattern.end()) {
        char c = *it;

        if(c=='(') {
            if(depth==0) {
                if(!seq.empty()) seqs << seq;
                seq = "";
                in_sub = true;
            }
            depth++;
            if(depth > 1) sub_seq += c;
        } 
        else if (c==')') {
            depth--;
            if(depth > 0) sub_seq += c;
            if(depth==0) {
                in_sub = false;
                if(!sub_seq.empty()) seqs << sub_seq;
                sub_seq = "";
            }
        }  else {
            if(in_sub) sub_seq += c;
            else seq += c;
        }

        ++it;
    }

    if(!seq.empty()) seqs << seq;

    list<Move> to_unmake;
    
    for(int s = 0;s<seqs.length();s++) {
        if(seqs[s].find('(') != std::string::npos) {
            #if LOG_OPEN
            print(indent,"R: ",seqs[s]);
            #endif
            build_opening(seqs[s],color,indent+"  "); 
        } else {
            #if LOG_OPEN
            print(indent,"P: ",seqs[s]);
            #endif
            auto pats = split_str(seqs[s], ',');
            list<Move> made;
            for(int i = 0; i < pats.length(); i++) {
                uint64_t hash = global->get_search_hash(color);
                Move current = parse_move(pats[i]);
                global->makeMove(current, false);
                #if LOG_OPEN
                print(indent," ",color==0?"W:":"B:",mts(current)," [",pats[i],"]");
                #endif
                opening_book.getOrPut(hash, list<Move>{}) << current;
                made << current;
                color = 1-color;
            }
            
            for(int i = pats.length() - 1; i >= 0; i--) {
                if(s!=0) {
                    global->unmakeMove(made[i], false);
                    color = 1-color;
                    #if LOG_OPEN
                    print(indent,"-",mts(made[i]));
                    #endif
                }
                else {
                    to_unmake << made[i];
                }
            }
        }
    }

    for(auto m : to_unmake) {
        #if LOG_OPEN
        print(indent,"-",mts(m));
        #endif
        global->unmakeMove(m,false);
        color = 1-color;
    }
}

// -- How to code an opening --
// 1. All moves are square to square seperated by a dash, like a2-a3
// 2. Each move is seperates by a comma
// 3. Special rules add an additional character:
//      P == Promotion
//      C == Castle
//      DD = Double Move
//      EP = En passante
// These are appended with a dash, like a2-a4-DD
// 4. Castles are notated by moving the king onto the rook with -CC
// So black castle kingside would be e8-h8-C
// 5. When an opening can branch, you enclose it in parens ()
// Put all together, Italian Game (Giuoco Piano vs Two Knights), could look like this:
// "e2-e4,e7-e5(g1-f3(b8-c6,f1-c4(g8-f6))(g8-f6,d2-d3,f8-c5,c2-c3))"
// Refer to examples for formatting


void play_book() {
    //King's Pawn Openings
    build_opening(
        "e2-e4"
            "(e7-e5"
                "(g1-f3"
                    "(b8-c6"
                        "(f1-c4" //Italian Game
                            "(f8-c5" //Giuoco Piano
                                "(c2-c3,g8-f6,d2-d4,e5-d4,c3-d4,c5-b4,b1-c3,f6-e4,e1-h1-C)"
                            ")"
                            "(g8-f6" //Two Knights Defense
                                "(b1-c3,f8-c5,d2-d3,d7-d6,c1-g5,h7-h6,g5-h4,e8-h8-C)"
                            ")"
                        ")"
                        "(f1-b5" //Ruy Lopez
                            "(a7-a6"
                                "(b5-a4"
                                    "(g8-f6"
                                        "(e1-h1-C,f8-e7,f1-e1,b7-b5,a4-b3,d7-d6,c2-c3,e8-h8-C,h2-h3,c8-b7)"
                                        "(e1-h1-C,f8-c5" //Marshall Attack attempt
                                            "(c2-c3,f7-f5,e4-f5,d7-d6,d2-d4,f6-d5)"
                                        ")"
                                    ")"
                                    "(b7-b5,a4-b3,f8-c5,c2-c3,d7-d6)"
                                ")"
                            ")"
                            "(g8-f6" //Berlin Defense
                                "(e1-h1-C,f8-e7,f1-e1,b7-b5,b5-b3,d7-d6,c2-c3,e8-h8-C)"
                            ")"
                        ")"
                        "(d2-d4" //Scotch Game
                            "(e5-d4,f3-d4,g8-f6,b1-c3,f8-b4,d4-c6,b7-c6,f1-d3)"
                        ")"
                        "(b1-c3" //Four Knights
                            "(g8-f6,f1-b5,f8-b4,e1-h1-C,e8-h8-C,d2-d3)"
                        ")"
                    ")"
                    "(g8-f6" //Petrov Defense
                        "(f3-e5,d7-d6,e5-f3,f6-e4,d2-d4,d6-d5,f1-d3,f8-e7,e1-h1-C)"
                    ")"
                    "(d7-d6" //Philidor Defense
                        "(d2-d4,b8-d7,f1-c4,c7-c6,b1-c3,f8-e7)"
                    ")"
                ")"
                "(f2-f4" //King's Gambit
                    "(e5-f4,g1-f3,g7-g5,f1-c4,g5-g4,e1-h1-C,g8-f6,d2-d4)"
                ")"
            ")"
            "(c7-c5" //Sicilian Defense
                "(g1-f3"
                    "(d7-d6" //Najdorf setup
                        "(d2-d4,c5-d4,f3-d4,g8-f6,b1-c3"
                            "(a7-a6" //Najdorf
                                "(c1-e3,e7-e5,d4-b3,f8-e7" //English attack
                                    "(f2-f3,b7-b5,d1-d2,c8-b7,e1-a1-C)"
                                ")"
                            ")"
                            "(g7-g6" //Dragon
                                "(c1-e3,f8-g7,f2-f3,b8-c6,d1-d2,e8-h8-C,e1-a1-C,d6-d5)"
                            ")"
                        ")"
                    ")"
                    "(b8-c6" //Accelerated Dragon/Closed setups
                        "(d2-d4,c5-d4,f3-d4"
                            "(g7-g6,c2-c4,f8-g7,c1-e3,g8-f6,b1-c3)" //Accelerated Dragon
                        ")"
                    ")"
                    "(g7-g6" //Hyperaccelerated Dragon
                        "(d2-d4,c5-d4,f3-d4,f8-g7,c2-c4,b8-c6,c1-e3)"
                    ")"
                ")"
                "(b1-c3" //Closed Sicilian
                    "(b8-c6,g2-g3,g7-g6,f1-g2,f8-g7,d2-d3,d7-d6)"
                ")"
            ")"
            "(e7-e6" //French Defense
                "(d2-d4,d7-d5"
                    "(b1-c3"
                        "(f8-b4" //Winawer
                            "(e4-e5,c7-c5,a2-a3,b4-c3,b2-c3,g8-e7,d1-g4)"
                        ")"
                        "(g8-f6" //Classical
                            "(c1-g5,f8-e7,e4-e5,f6-d7,g5-e7,d8-e7,f2-f4)"
                        ")"
                        "(d5-c4" //McCutcheon/other
                            "(g1-f3,g8-f6,f1-c4)"
                        ")"
                    ")"
                    "(e4-d5" //Exchange
                        "(e6-d5,f1-d3,b8-c6,c2-c3,f8-d6)"
                    ")"
                    "(e4-e5" //Advance
                        "(c7-c5,c2-c3,b8-c6,g1-f3,d8-b6,a2-a3)"
                    ")"
                ")"
            ")"
            "(c7-c6" //Caro-Kann
                "(d2-d4,d7-d5"
                    "(b1-c3" //Two Knights
                        "(d5-e4,c3-e4,c8-f5,e4-g3,f5-g6,h2-h4,h7-h6,g1-f3)"
                    ")"
                    "(e4-d5" //Exchange
                        "(c6-d5,c2-c4,g8-f6,b1-c3,b8-c6)"
                    ")"
                    "(e4-e5" //Advance
                        "(c8-f5,g1-f3,e7-e6,f1-e2,c6-c5)"
                    ")"
                ")"
            ")"
            "(d7-d5" //Scandinavian
                "(e4-d5,d8-d5,b1-c3,d5-a5,d2-d4,g8-f6,g1-f3)"
            ")"
            "(d7-d6" //Pirc Defense
                "(d2-d4,g8-f6,b1-c3,g7-g6,f2-f4,f8-g7,g1-f3,e8-h8-C)"
            ")"
            "(g7-g6" //Modern Defense
                "(d2-d4,f8-g7,b1-c3,d7-d6,f2-f4,g8-f6,g1-f3)"
            ")"
            "(g8-f6" //Alekhine Defense
                "(e4-e5,f6-d5,d2-d4,d7-d6,c2-c4,d5-b6,f2-f4)"
            ")"
    );

    //Queen's Pawn Openings
    build_opening(
        "d2-d4"
            "(d7-d5"
                "(c2-c4" //Queen's Gambit
                    "(e7-e6"
                        "(b1-c3"
                            "(g8-f6"
                                "(c1-g5" //Orthodox QGD
                                    "(f8-e7,e2-e3,e8-h8-C,g1-f3,h7-h6,g5-h4"
                                        "(b7-b6" //Tartakower
                                            "(d1-c2,c8-b7,c2-d1,b8-d7,f1-d3,c7-c5)"
                                        ")"
                                        "(b8-d7" //Lasker
                                            "(h4-e7,d8-e7,c4-d5,e6-d5,f1-d3)"
                                        ")"
                                    ")"
                                ")"
                                "(g1-f3" //QGD without Bg5
                                    "(c8-e6,c1-g5,h7-h6,g5-h4,f8-e7)"
                                ")"
                            ")"
                            "(c7-c5" //Tarrasch Defense
                                "(c4-d5,e6-d5,g1-f3,b8-c6,g2-g3,g8-f6)"
                            ")"
                        ")"
                    ")"
                    "(d5-c4" //Queen's Gambit Accepted
                        "(e2-e3,g8-f6,f1-c4,e7-e6,g1-f3,c7-c5,e1-h1-C)"
                    ")"
                    "(c7-c6" //Slav Defense
                        "(g1-f3,g8-f6,b1-c3"
                            "(d5-c4" //Slav Accepted
                                "(a2-a4,c8-f5,e2-e3,e7-e6,f1-c4)"
                            ")"
                            "(e7-e6" //Semi-Slav
                                "(e2-e3,b8-d7,f1-d3,d5-c4,d3-c4)"
                            ")"
                        ")"
                    ")"
                    "(b8-c6" //Chigorin Defense
                        "(b1-c3,d5-c4,g1-f3,g8-f6,e2-e3)"
                    ")"
                ")"
                "(g1-f3" //London System/Colle
                    "(g8-f6"
                        "(c1-f4" //London
                            "(e7-e6,e2-e3,c7-c5,c2-c3,f8-d6,f4-d6,d8-d6)"
                        ")"
                        "(e2-e3" //Colle
                            "(e7-e6,f1-d3,c7-c5,c2-c3,b8-c6)"
                        ")"
                    ")"
                ")"
            ")"
            "(g8-f6"
                "(c2-c4"
                    "(g7-g6"
                        "(b1-c3"
                            "(f8-g7"
                                "(e2-e4" //King's Indian Defense
                                    "(d7-d6"
                                        "(f2-f3" //Classical/Sämisch
                                            "(e8-h8-C,c1-e3,e7-e5,g1-e2,b8-d7,d1-d2,c7-c6)"
                                        ")"
                                        "(g1-f3" //Classical
                                            "(e8-h8-C,f1-e2,e7-e5,e1-h1-C,b8-c6"
                                                "(d4-d5" //Mar del Plata
                                                    "(c6-e7,b2-b4,f6-h5,f1-e1,f7-f5,c1-g5)"
                                                ")"
                                            ")"
                                        ")"
                                    ")"
                                ")"
                                "(g1-f3" //King's Indian Fianchetto
                                    "(e8-h8-C,g2-g3,d7-d6,f1-g2,b8-c6,e1-h1-C)"
                                ")"
                            ")"
                            "(d7-d5" //Grünfeld Defense
                                "(c4-d5,f6-d5,e2-e4,d5-c3,b2-c3,f8-g7,g1-f3)"
                            ")"
                        ")"
                    ")"
                    "(e7-e6"
                        "(b1-c3"
                            "(f8-b4" //Nimzo-Indian
                                "(e2-e3" //Rubinstein
                                    "(e8-h8-C,f1-d3,d7-d5,g1-f3,c7-c5,e1-h1-C,b8-c6)"
                                ")"
                                "(d1-c2" //Classical
                                    "(d7-d5,a2-a3,b4-c3,c2-c3,b8-e4,c3-c2)"
                                ")"
                            ")"
                            "(b7-b6" //Queen's Indian
                                "(g1-f3,c8-b7,g2-g3,f8-e7,f1-g2,e8-h8-C)"
                            ")"
                        ")"
                    ")"
                    "(c7-c5" //Benoni Defense
                        "(d4-d5,e7-e6,b1-c3,e6-d5,c4-d5,d7-d6,e2-e4,g7-g6)"
                    ")"
                ")"
                "(c1-g5" //Trompowsky
                    "(e7-e6,e2-e4,h7-h6,g5-f6,d8-f6,b1-c3)"
                ")"
            ")"
            "(f7-f5" //Dutch Defense
                "(c2-c4,g8-f6,g2-g3,e7-e6,f1-g2,f8-e7,g1-f3,e8-h8-C)"
            ")"
    );

    build_opening(
        "c2-c4" //English Opening
            "(e7-e5"
                "(b1-c3"
                    "(b8-c6"
                        "(g1-f3,g8-f6"
                            "(d2-d4" //Four Knights
                                "(e5-d4,f3-d4,f8-b4,c1-g5,h7-h6,g5-h4)"
                            ")"
                            "(g2-g3" //Closed English
                                "(d7-d5,c4-d5,f6-d5,f1-g2,d5-c3,d2-c3)"
                            ")"
                        ")"
                    ")"
                ")"
            ")"
            "(g8-f6"
                "(b1-c3,e7-e6,e2-e4,d7-d5,e4-e5,f6-e4)" //King's English
            ")"
        ")"
    );

    save_opening_book();
}

void initialize_board() {
    for(int a=0;a<9;a++) {
        global->grid << list<int>();
        for(int b=0;b<9;b++) {
            global->grid[a] << -1;
        }
    }

    for(int k = 0;k<2;k++) {
    std::string col = k==0?"white":"black";
    int rank = k==0?1:8;
        setup_piece("queen_"+col,ctf('d'),rank);
        for(int i=1;i<9;i++) setup_piece("pawn_"+col,i,k==0?2:7);
        for(int i=0;i<2;i++) setup_piece("bishop_"+col,ctf(i==0?'c':'f'),rank);
        for(int i=0;i<2;i++) setup_piece("rook_"+col,ctf(i==0?'a':'h'),rank);
        for(int i=0;i<2;i++) setup_piece("knight_"+col,ctf(i==0?'b':'g'),rank);
        setup_piece("king_"+col,ctf('e'),rank);
    }

    srand(12345);
    for(int piece = 0; piece < 32; piece++) {
        for(int square = 0; square < 64; square++) {
            global->zobrist_table[piece][square] = random64bit();
        }
    }
    turn_zobrist_key = random64bit();
    global->current_hash = global->hash_board();
    for(int i = 0; i < TT_SIZE; i++) {
        transposition_table[i].valid = false;
    }
}

int main() {
    using namespace helper;

    std::string MROOT = root()+"/Projects/FirChess/assets/models/";

    Window window = Window(1280, 768, "FirChess 0.1.2");
    scene = make<Scene>(window,2);
    scene->camera.toOrbit();
    //scene->camera.lock = true;
    Data d = make_config(scene,K);
    // load_gui(scene, "FirChess", "firchessgui.fab");

    num_grid = make<NumGrid>(2.0f,21.0f);
    global = make<Board>();
    
    //Define the objects, this pulls in the models and uses the CSV to code them
    // start::define_objects(scene,"FirChess",num_grid);
    type_define_objects(num_grid);

    initialize_board();

    play_book();

    #if USE_NODENET
        init_nodenet(scene);
        net[0] = make<chess_nodenet>();
        net[0]->scene = scene;
        net[1] = make<chess_nodenet>();
        net[1]->scene = scene;

       // global->inject_movement_episodes();
    #endif

    try {
        load_opening_book();
    } catch(std::exception& e) {
        print("No opening book found, playing without book");
    }

    #if ENABLE_BOARDSPLIT
        for(int i = 0;i<board_count;i++) {
            threads[i] = make<Thread>();
            threads[i]->run([&,i](ScriptContext& ctx){
                threads[i]->flushTasks();
            },0.01f);
            threads[i]->start();
            boards[i] = make<Board>();
            boards[i]->sync_with(global);
        }
    #else
        global_thread = make<Thread>();
        global_thread->run([&](ScriptContext& ctx){
           global_thread->flushTasks();
        },0.01f);
        global_thread->start();
    #endif

    //load_game("auto");

    //Make the chess board and offset it so it works with the grid
    auto board = make<Single>(scene->get<g_ptr<Model>>("_board_model"));
    scene->add(board);
    board->move(vec3(1,-1.3,1));


    //Setting up the lighting, ticking environment to 0 just makes it night
    scene->tickEnvironment(0);
    auto l1 = make<Light>(Light(glm::vec3(0,10,0),glm::vec4(300,300,300,1)));
    scene->lights.push_back(l1);

    bool game_over = false;
    int wining_color = 0;
    int wins[2] = {0,0};

    bool bot_color = 0;
    //Make the little mouse to reperesnt the bot (Fir!)
    auto Fir = make<Single>(make<Model>(root()+"/Engine/assets/models/agents/Snow.glb"));
    scene->add(Fir);
    Fir->setPosition(bot_color==0?vec3(1,-1,-9):vec3(1,-1,11));
    if(bot_color==1) Fir->faceTo(vec3(0,-1,0));
    auto bot = make<Thread>();
    auto nnet_bot = make<Thread>();
    bot->run([&](ScriptContext& ctx){
        if(turn_color==bot_color) {
            bot_turn = true;
            save_game("auto");
            #if ENABLE_BOARDSPLIT
                for(int i = 0;i<board_count;i++) {
                    boards[i]->sync_with(global);
                }
            #endif
            #if TURN_TAGS
                Log::Line l; l.start();
            #endif
            Move m = findBestMove(max_depth,turn_color);
            //Move m = net[turn_color]->pick_move(turn_color); 
            if(m.id!=-1) {
                global->makeMove(m);
                madeMoves << m;
                #if TURN_TAGS
                    print("--FINSHED BOT TURN--\nTIME: ",ftime(l.end()));
                #endif
                turn_color = 1-turn_color;
                turn_number++;
            }
            else { //If the game is over
                wining_color = 1-bot_color;
                bot->pause();
                nnet_bot->pause();
                game_over = true;
            }
            bot_turn = false;
        } 
    },0.02f);

    nnet_bot->run([&](ScriptContext& ctx){
        if(turn_color!=bot_color) {
            bot_turn = true;
            save_game("auto");
            #if ENABLE_BOARDSPLIT
                for(int i = 0;i<board_count;i++) {
                    boards[i]->sync_with(global);
                }
            #endif
            #if TURN_TAGS
                Log::Line l; l.start();
            #endif
            //Move m = findBestMove(max_depth,turn_color);
            Move m = net[turn_color]->pick_move(turn_color); 
            if(m.id!=-1) {
                global->makeMove(m);
                madeMoves << m;
                #if TURN_TAGS
                    print("==FINISHED NNET TURN==\nTIME: ",ftime(l.end()));
                #endif
                turn_color = 1-turn_color;
                turn_number++;
            }
            else { //If the game is over
                wining_color = bot_color;
                nnet_bot->pause();
                bot->pause();
                game_over = true;
            }
            bot_turn = false;
        } 
    },0.02f);

    // auto phys_thread = make<Thread>();
    // phys_thread->run([&](ScriptContext& ctx){
    //     update_physics();
    // },0.016);
    // phys_thread->start();

    
    bool auto_turn = true;
    if(auto_turn) {
        bot->start();
        nnet_bot->start();
    }
    int last_col = 1-turn_color;

    //load_game("auto");
    Log::Line game_timer; game_timer.start();
    start::run(window,d,[&]{

        if(game_over||turn_number>100) {
            if(turn_number>100) {
                nnet_bot->pause();
                bot->pause();

                nnet_bot->waitForIdle();
                bot->waitForIdle();
            } else {
                wins[wining_color]++;
            }
            turn_number = 0;
            print("\n====GAME FINISHED====");
            print("TIME: ",ftime(game_timer.end())," WHITE WINS: ",wins[0]," BLACK WINS: ",wins[1]);
            print("MADE MOVES: ",madeMoves.length()," RECENT EPISODES: ",net[1]->recent_episodes.length()," CONSOLIDATED EPISODES: ",net[1]->consolidated_episodes.length());

            print("Moves from black's perspective: ");
            for(int i=0;i<madeMoves.length();i++) { 
                print(global->mts(madeMoves[i]));
            }
            print("Episodes from black's perspective: ");
            for(int i=0;i<net[1]->recent_episodes.length();i++) { 
                print(net[1]->recent_episodes[i]->to_string());
            }
            print("==========");



            for(int i=madeMoves.length()-1;i>=0;i--) {
                global->unmakeMove(madeMoves[i]);
            }
            madeMoves.clear();
            for(int i=0;i<2;i++) {
                for(int e=net[i]->consolidated_episodes.length()-1;e>=0;e--) {
                    g_ptr<Episode> to_consolidate = net[i]->consolidated_episodes[e];
                    net[i]->consolidated_episodes.removeAt(e);
                    net[i]->consolidate_episode(to_consolidate,net[i]->consolidated_episodes, ALL, 0.7f, -1.0f);
                }
            }

            print("Black's consolidated episodes: ");
            for(int i=0;i<net[1]->consolidated_episodes.length();i++) { 
                print(net[1]->consolidated_episodes[i]->to_string());
            }
            print("Left over after sleep: ",net[1]->consolidated_episodes.length());
            print("Made moves left: ",madeMoves.length()," Empty? ",madeMoves.empty()?"yes":"no");

            game_over = false;
            game_timer.start();
            print("==========");
            bot->start();
            nnet_bot->start();
        }

        vec3 mousePos = scene->getMousePos(0);
        if(mousePos.x()>8.0f) mousePos.setX(8.0f);
        if(mousePos.x()<-6.0f) mousePos.setX(-6.0f);
        if(mousePos.z()>8.0f) mousePos.setZ(8.0f);
        if(mousePos.z()<-6.0f) mousePos.setZ(-6.0f);

        // if(last_col!=turn_color) {
        //     text::setText(turn_color==0?"White":"Black",scene->getSlot("turn")[0]);
        // }
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
                bot->start();
                // Move m = findBestMove(max_depth,turn_color);
                // if(m.id!=-1) {
                //     global->makeMove(m);
                //     madeMoves << m;
                // }
            }
        }
        if(pressed(Y)) {
            vec3 clickPos = num_grid->snapToGrid(scene->getMousePos());
            if(in_bounds(world_to_board(clickPos)))
                if(global->square(world_to_board(clickPos))!=-1)
                    global->takePiece(global->square(world_to_board(clickPos)));
        }
        if(pressed(R)) if(!madeMoves.empty()) global->unmakeMove(madeMoves.pop());
        if(pressed(NUM_1)) scene->camera.toOrbit();
        if(pressed(NUM_2)) scene->camera.toIso();
        if(pressed(NUM_3)) scene->camera.toFirstPerson();

        if(pressed(E)) {
            auto clickPos = num_grid->snapToGrid(scene->getMousePos());
            ivec2 v = world_to_board(clickPos);
            if(in_bounds(v)) {
            //grid->toIndex(clickPos)
            print("----",bts(v),"----");
           if(global->square(v)!=-1) {
                print("E:",global->dtypes[global->square(v)]," I:",global->square(v));
           }
           else print("EMPTY");
            }
        }
        if(pressed(G)) {
            // print(isKingInCheck(turn_color)==0?"No check":"In check");
            print(global->get_search_hash(turn_color));
        }
        if(pressed(MOUSE_LEFT)) {
            if(!global->selected) {
                 auto clickPos = num_grid->snapToGrid(scene->getMousePos());
                 ivec2 v = world_to_board(clickPos);
                if(in_bounds(v))
                if(global->square(v)!=-1) {
                    int t_s_id = global->square(v);
                     if(auto g = global->ref[t_s_id]) {
                        if(global->colors[t_s_id]==turn_color||debug_move) {
                            global->select_piece(t_s_id);
                            global->start_pos = world_to_board(clickPos);
                        }
                     }
                 }
             }
             else
             {
              //End the move here
              vec3 newPos = num_grid->snapToGrid(mousePos).addY(global->selected->getPosition().y());
              ivec2 v = world_to_board(newPos);
              if(in_bounds(v))
              {
              update_num_grid(global->selected,newPos);
              bool castling = false;
              if(global->validate_castle(global->s_id,v)) {
                castling = true;
              } 
              Move move;
              move.id = global->s_id;
              move.from = global->start_pos;
              move.to = v;
              if(castling) move.rule = 2;
              if(global->validate_move(move)||debug_move||castling) {
                global->makeMove(move,false);
                if(global->isKingInCheck(turn_color)) {
                    global->unmakeMove(move,false);
                    global->selected->setPosition(board_to_world(global->start_pos));
                }
                else {
                    global->unmakeMove(move,false);
                    if(global->check_promotion(move)) {
                        move.rule = 1;
                    }
                    global->makeMove(move);
                    //print("Move repeated: ",history.getOrDefault(current_hash,0));
                    madeMoves << move;
                    if(auto_turn) {
                        turn_color = 1-turn_color;
                        turn_number++;
                    }
                }
              }
              else 
              global->selected->setPosition(board_to_world(global->start_pos));

              global->selected->setLinearVelocity(vec3(0,-2.5f,0));
              drop << global->selected;
              //print("Moving ",selected->ID," to "); v.print();
              //update_grid(selected->ID,v);
              global->selected = nullptr;
             }
            }
         }

         if(global->selected&&!bot_turn)
         {
            vec3 targetPos = num_grid->snapToGrid(mousePos).addY(1);
            vec3 direction = targetPos - global->selected->getPosition();
            float distance = direction.length();
            float moveSpeed = distance>=6.0f?distance*2:6.0f;
            if (distance > 0.1f) {
                global->selected->setLinearVelocity((direction / vec3(distance,distance,distance)) * moveSpeed);
            } else {
                global->selected->setLinearVelocity(vec3(0,0,0));
            }
         }

         for(auto d : drop) {
            if(d->getPosition().y()<=0) {
                d->setLinearVelocity(vec3(0,0,0)); 
                drop.erase(d);
            }
         }

         if(pressed(Q)) {
            if(held(LSHIFT)) global->unmakeMove(madeMoves.last());
            else {
                if(bot->runningTurn) {bot->pause(); bot_turn=false;}
                else {bot->start();}
            }
        }

        // if(pressed(P)) {
        //     if(phys_thread->runningTurn) {phys_thread->pause();}
        //     else {phys_thread->start();}
        // }
        update_physics();

    });

return 0;
}