#pragma once
#include<core/helper.hpp>
#include<materials.hpp>

namespace Golden {

    class FarmLogic : public Object {
    public:
        FarmLogic() {};

        list<list<std::string>> grid;

        struct group {
            ivec2 pos;
            list<std::string> contents;
            std::string id;
        };

        map<std::string,group> groups;
        int size_x = 0;
        int size_y = 0;

        FarmLogic(int _size_x, int _size_y) : size_x(_size_x), size_y(_size_y) {
            for(int i=0;i<size_x;i++) {
                list<std::string> row;
                for(int c=0;c<size_y;c++) {
                    std::string cell = ".";
                    row << cell;
                }
                grid << row;
            }
        };

        bool is_group(ivec2 at) {
            return is_group(grid[at.x()][at.y()]);
        }

        bool is_group(const std::string& thing) {
            return thing.length()==3&&thing.front()=='g';
        }

        list<std::string>& in_group(const std::string& thing) {
           return groups.get(thing).contents;
        }

        

        static std::string face(ivec2 dir) {
            if(dir.length()>1) dir = dir.normalized();
            if(dir==ivec2(1,0)) {
                return "v";
            } else if(dir==ivec2(-1,0)) {
                return "^";
            } else if(dir==ivec2(0,-1)) {
                return "<";
            } else if(dir==ivec2(0,1)) {
                return ">";
            } else if(dir==ivec2(1,1)) {
                return "\\";
            } else if(dir==ivec2(-1,-1)) {
                return "/";
            } else if(dir==ivec2(-1,1)) {
                return "\\";
            } else if(dir==ivec2(1,-1)) {
                return "/";
            } else {
                print("Invalid direction for facing: ",dir.to_string());
                return "-";
            }
        }

        static std::string relative_dir(ivec2 dir, ivec2 facing = {0,-1}) {
            ivec2 d = ivec2((dir.y() > 0) - (dir.y() < 0), (dir.x() > 0) - (dir.x() < 0));
            ivec2 f = ivec2((facing.y() > 0) - (facing.y() < 0), (facing.x() > 0) - (facing.x() < 0));
            if (d == ivec2(0, 0)) return "here";
            static const ivec2 dirs8[8] = {
                {-1,  0}, {-1,  1}, {0,  1}, {1,  1},
                { 1,  0}, { 1, -1}, {0, -1}, {-1, -1}
            };
            int di = -1, fi = -1;
            for (int i = 0; i < 8; ++i) {
                if (d == dirs8[i]) di = i;
                if (f == dirs8[i]) fi = i;
            }
            if (di == -1 || fi == -1) {
                print("Invalid direction(s): dir=", dir.to_string(), " facing=", facing.to_string());
                return "-";
            }
            int rel = (di - fi + 8) % 8;
            static const char* names8[8] = {
                "behind", "behind and to the right", "to the right", "ahead and to the right",
                "ahead", "ahead and to the left", "to the left", "behind and to the left"
            };
            return names8[rel];
        }
  
        std::string to_string() {
            std::string table = "   ";
            for(int i=0;i<grid[0].length();i++) {
                table.append("  "+std::to_string(i)+" ");
                if(i<9) table.append(" ");
            }
            table.append("\n");
            for(int g=0;g<grid.length();g++) {
                list<std::string>& r = grid[g];
                std::string line = std::to_string(g)+" ";
                if(g<10) line.append(" ");
                for(auto c : r) {
                    for(int m=0;m<2-(c.length()/2);m++) {
                        line.append(" ");
                    }
                    line.append(c);
                    for(int m=0;m<2-(c.length()/2);m++) {
                        line.append(" ");
                    }
                }
                if(g<grid.length()-1) line.append("\n");
                table.append(line);
            }
            return table;
        }
    
        ivec2 search(const std::string& thing) {
            for(int g=0;g<grid.length();g++) {
                for(int r=0;r<grid[g].length();r++) {
                    if(grid[g][r]==thing) {
                        return {g,r};
                    }
                }
            }
            for(auto g : groups.getAll()) {
                for(auto c : g.contents) {
                    if(c==thing) {
                        return g.pos;
                    }
                }
            }

            return {-1,-1};
        }
    
        void place(ivec2 loc, const std::string& new_thing) {
            std::string at = grid.get(loc.x()).get(loc.y());
            if(at!=".") {
                if(is_group(at)) {
                    groups.get(at).contents << new_thing;
                } else {
                    group g;
                    std::string n_id = "g00";
                    while(groups.hasKey(n_id)) {
                        int n_num = std::stoi(n_id.substr(1,2))+1;
                        if(n_num<9) n_id = "g0"+std::to_string(n_num);
                        else n_id = "g"+std::to_string(n_num);
                    }
                    g.id = n_id;
                    g.contents << new_thing;
                    g.contents << at;
                    g.pos = loc;
                    groups.put(n_id,g);
                    grid[loc.x()][loc.y()] = n_id;
                }
            } else {
                grid[loc.x()][loc.y()] = new_thing;
            }
        }

        void replace(const std::string& thing,const std::string& new_thing) {
            ivec2 loc = search(thing);
            if(loc!=ivec2(-1,-1)) {
                place(loc,new_thing);
            }
        }

        void remove(ivec2 loc, std::string thing = "") {
            if(thing=="") {
                grid[loc.x()][loc.y()] = ".";
            } else {
                std::string at = grid[loc.x()][loc.y()];
                if(at!=".") {
                    if(is_group(at)) {
                        //for(auto c : groups.get(at).contents) print(c);
                        groups.get(at).contents.erase(thing);
                        if(groups.get(at).contents.empty()) {
                            grid[loc.x()][loc.y()] = ".";
                            groups.remove(at);
                        } else if(groups.get(at).contents.length()==1) {
                            grid[loc.x()][loc.y()] = groups.get(at).contents[0];
                            groups.remove(at);
                        }
                    } else {
                        grid[loc.x()][loc.y()] = ".";
                    }
                }
            }
        }

        void move(ivec2 from, ivec2 to) {
            if(from==to) return;
            std::string thing = grid[from.x()][from.y()];
            remove(from);
            place(to,thing);
        }
    
        void move(const std::string& thing, ivec2 to) {
            ivec2 from = search(thing);
            if(from==to) return;
            if(from!=ivec2(-1,-1)) {
                remove(from,thing);
                place(to,thing);
            } else {
                print("Could not find thing ",thing," in grid search for grid_move");
            }
    
        }
    
        list<std::string> around(ivec2 point, int radius)  {
            radius+=1;
            list<std::string> result;
            for(int x=point.x()-radius;x<point.x()+radius;x++) {
                if(x>=0&&x<grid.length()) {
                    for(int y=point.y()-radius;y<point.y()+radius;y++) {
                        if(y>=0&&y<grid[x].length()) {
                            if(ivec2(x,y)!=point&&grid[x][y]!=".") {
                                    if(is_group(grid[x][y])) {
                                        result << groups.get(grid[x][y]).contents;
                                    } else {
                                        result << grid[x][y];
                                    }
                            }
                        }
                    }
                }
            }
            return result;
        }

        ivec2 operator[](const std::string& thing) {
            return search(thing);
        } 
        std::string operator[](ivec2 at) {
            return grid[at.x()][at.y()];
        } 
    };

    int s_x = 20;
    int s_y = 20;

    bool in_bounds(ivec2 pos,int x = s_x,int y = s_y) {
        return pos.x()>=0&&pos.x()<s_x&&pos.y()>=0&&pos.y()<s_y;
    }

    inline std::string red(const std::string& text) {
        return "\x1b[31m"+text+"\x1b[0m";
    }

    std::string action_to_verb(const std::string& action) {
        if(action.back()=='e') {
            return action.substr(0,action.length()-1)+"ing";
        } 
        return action+"ing";
    }

    auto grid = make<FarmLogic>(s_x,s_y);

    map<std::string,ivec2> chars;
    map<std::string,ivec2> animals;
    map<std::string,ivec2> features;

    //Chars
    //1) R = Rowan “Ro” Patel
    //2) L = Luis Armenta
    //3) G = Grace Ibe
    //4) A = Anton Volkov
    //5) Y = Yara Haddad 
    //6) K = Marcus “Kells” Kelleher
    //7) C = Mina Cho 
    //8) D = Daniel Okoy
    //9) M = Milo A0001

    list<std::string> c_inits = {"R","L","G","A","Y","K","C","D","M"};
    list<std::string> c_names = {"Ro","Luis","Grace","Anton","Yara","Kells","Mina","Daniel","Milo"};
    map<std::string,std::string> c_states;
    map<std::string,std::string> c_with;
    map<std::string,int> c_strength;

    map<std::string,std::string> cow_actions;
    map<std::string,map<std::string,list<std::string>>> action_map;
    map<std::string,std::string> a_states;

    json j;
    bool impact = false;

    void move_char(const std::string& thing,ivec2 to) {
        grid->move(thing,to);
        chars.get(thing) = to;
        j["chars"][thing]["pos"][0] = to.x();
        j["chars"][thing]["pos"][1] = to.y();
    }

    void move_animal(const std::string& thing,ivec2 to) {
        grid->move(thing,to);
        animals.get(thing) = to;
        j["animals"][thing]["pos"][0] = to.x();
        j["animals"][thing]["pos"][1] = to.y();
    }

    void change_char_state(const std::string& thing,const std::string& to) {
        c_states.get(thing) = to;
        j["chars"][thing]["state"] = to;
    }

    void change_animal_state(const std::string& thing,const std::string& to) {
        a_states.get(thing) = to;
        j["animals"][thing]["state"] = to;
    }

    void attatch_char_to(const std::string& thing,const std::string& to) {
        c_with.set(thing,to);
        j["chars"][thing]["with"] = to;
    }

    void unattatch_char(const std::string& thing) {
        c_with.set(thing,"none");
        j["chars"][thing]["with"] = "none";
    }

    void kill_char(const std::string& c) {
        change_char_state(c,"dead");
    }

    bool is_dead(const std::string& c) {
        return c_states.get(c) == "dead";
    }

    void change_char_strength(const std::string& c,int by) {
        c_strength.get(c) = c_strength.get(c)+by;
        j["chars"][c]["strength"] = c_strength.get(c);
    }

    void shift_chars(list<std::string> inits, ivec2 by) {
        for(auto i : inits)
            move_char(i,chars.get(i)+by);
    }


    void init_action_map_for_cows() {

        //This can be tricky, there needs to be increasing repeats for each new entry, so that way we're not saving with extra blank repeats
        //Because we detect if a move is fatal by if there's no repeat_x after it


        if(action_map.size()==0) {
            map<std::string,list<std::string>> new_map;

            new_map.put("avoid",{
                " was out of the way", //0
                " wasn't close to it" //1
            });
            new_map.put("strong_save",{
                " deftly backpedaled", //0
                " swiftly sidestepped", //1
                " fluidly dodged" //2
            });
            new_map.put("normal_save",{
                " dove out of the way", //0
                " leapt away", //1
                " ran away just in time" //2
            });
            new_map.put("weak_save",{
                " desperately dove away just in time", //0
                " frantically leapt out of the way", //1
                " narrowly rolled away" //2
            });
            new_map.put("weak_fail",{
                " frantically dove away, but got clipped across the side", //0
                " attempted to roll away, but got scraped by the edge", //1
                " tried to run, but got hit in the back of the leg" //2
            });
            new_map.put("normal_fail",{
                " attempted to dive away, but got struck in the side", //0
                " ran and tripped, leg lacerated by the edge", //1
                " tried to step away, but got struck in the chest" //2
            });
            new_map.put("strong_fail",{
                " noticed the hoof and failed to react, their arm firmly pinned and crushed", //0
                " attempted to dive away and failed, their leg pinned and crushed by the shift" //1
                "'s lower body was pinned by a hoof, one leg crushed after catching the pressure wrong", //0
                "'s upper body was pinned by a hoof, one arm mangled" //1
            });
            new_map.put("repeat_0",{
                " was worked under the hoof by the shift" //0
            });
            new_map.put("repeat_1",{
                "'s entire body is beneath a hoof" //0
            });
            new_map.put("repeat_2",{
                " was ground to paste beneath a hoof" //0
            });
            action_map.put("shift",new_map);

            new_map.set("weak_fail",{
                " frantically dove away, but got clipped across the side", //0
                " attempted to roll away, but got scraped by the edge", //1
                " tried to run, but got hit in the back of the leg" //2
            });
            new_map.set("normal_fail",{
                " attempted to dive away, but got struck in the side", //0
                " ran and tripped, leg lacerated by the edge", //1
                " tried to step away, but got struck in the chest" //2
            });
            new_map.set("strong_fail",{
                " noticed the hoof and failed to react, being caught across the side and taking a nasty gash and sticking them to the hoof", //0
                " attempted to dive away and failed, their chest being struck, sticking them to the hoof", //1
                " couldn't get away in time and was struck in the leg, snapping it and sticking them to the hoof" //2
            });
            new_map.set("repeat_0",{
                " was taken with the hoof" //0
            });
            new_map.set("repeat_1",{
                " shudders as the next step lands" //0
            });
            new_map.set("repeat_2",{
                " is plastered to the hoof" //0
            });
            new_map.put("repeat_3",{
                " slid beneath the hoof and was crushed", //0
                "'s body couldn't take the force anymore", //1
                " was worked to the edge and split open by the next step", //2
                "'s body was reduced to a mangled mess by the next step", //3
                "'s head hit the hoof too hard and split" //4
            });
            action_map.put("move",new_map);


            new_map.set("weak_fail",{
                " frantically dove away, but got their leg bruised", //0
                " attempted to roll away, but got coated in muck", //1
                " tried to run, but got hit in the back of the leg, pulling it free at the last momment" //2
            });
            new_map.set("normal_fail",{
                " attempted to dive away, but got their legs pinned, squeezing out", //0
                " ran and tripped, getting covered in muck", //1
                " tried to step away, but got pinned, struggling out" //2
            });
            new_map.set("strong_fail",{
                " tried to dodge, but got buried under flesh", //0
                " attempted to dive away, only to be slammed into the ground by the body", //1
                " couldn't get away in time and was firmly pinned under thousands of pounds" //2
            });
            new_map.set("repeat_0",{
                " was worked under the flesh" //0
            });
            new_map.set("repeat_1",{
                " is gasping on the stench" //0
            });
            new_map.set("repeat_2",{
                " is fully pulled under the resting body" //0
            });
            new_map.set("repeat_3",{
                " is being smoothered under thousands of pounds of flesh", //0
                " can feel their ribs creak as the body shifts" //1
            });
            new_map.put("repeat_4",{
                " can only breath the stench of a body" //0
            });
            new_map.put("repeat_5",{
                " screams as their arm is torn from the socket" //0
            });
            new_map.put("repeat_6",{
                " is put out of their misery by a final shift" //0
            });
            action_map.put("rest",new_map);

            new_map.set("weak_fail",{
                " frantically dove away, but got slobber on their leg", //0
                " attempted to roll away, but got clipped by the lip and slimed", //1
                " tried to run, but got hit in the back of the leg, getting covered in spit" //2
            });
            new_map.set("normal_fail",{
                " attempted to dive away, but got their legs taken in, pulling them out at the last momment", //0
                " ran and tripped, getting covered in muck", //1
                " tried to step away, but got their head slurped up, squirming free at the last momment" //2
            });
            new_map.set("strong_fail",{
                " tried to dodge, but got caught in the lips", //0
                " attempted to dive away, only to be slurped up to the waist", //1
                " couldn't get away in time and was firmly taken up to the waist" //2
            });
            new_map.set("repeat_0",{
                " is taken in deeper, up to the belly" //0
            });
            new_map.set("repeat_1",{
                " is slurped up to the shoulders" //0
            });
            new_map.set("repeat_2",{
                " is fully taken into the mouth" //0
            });
            new_map.set("repeat_3",{
                " is shifted back past the incisors" //0
            });
            new_map.set("repeat_4",{
                " is being wrangled by the tounge towards the throat" //0
            });
            new_map.set("repeat_5",{
                " can only scream as they're prepared for a swallow" //0
            });
            new_map.set("repeat_6",{
                " is sent to the stomach with a single gulp" //0
            });
            for(int i = 7;i<120;i++) {
                std::string i_str = std::to_string(i);
                new_map.put("repeat_"+i_str,{
                    " has been worked on by the stomach for the past "+i_str+" turns ", //0
                    " is digesting, they've been in the stomach for "+i_str+" turns " //1
                });
            }
            action_map.put("eat",new_map);


        }
    }

    ivec2 roll_move(ivec2 from,ivec2 exclude = {-1,-1}) {
        ivec2 n_pos = from+ivec2(randi(-1,1),randi(-1,1));
        while(!in_bounds(n_pos)||from==exclude) {
            n_pos = from+ivec2(randi(-1,1),randi(-1,1));
        }
        return n_pos;
    }


    void cow_logic(const std::string& e) {
        ivec2 pos = animals.get(e);

        int mvs = randi(1,4);
        int move_kind = 0; //Stationary
        for(int n=0;n<mvs;n++) {
            move_kind = 0;
            if(mvs>1) {
                ivec2 n_pos = roll_move(pos);
                //Advantage for 'cosmic randomness' rolls, to keep things interesting!
                for(int i=0;i<2;i++) {
                    ivec2 n_roll = roll_move(pos);
                    for(auto c : chars.entrySet()) {
                        if(is_dead(c.key)) continue;
                        if(c_with.get(c.key)==e) continue;
                        if((c.value-pos).length()<6) {
                            // if(a.key=="C02") print("Choosing closer: ",n_roll.to_string()," ",n_pos.to_string());
                            if((c.value-n_roll).length()<(c.value-n_pos).length()) {
                                n_pos = n_roll;
                                break;
                            }
                        }
                    }
                }
                //print(a.key," Moved from ",a.value.to_string()," to ",n_pos.to_string(),": ",(n_pos-a.value).to_string());
                move_animal(e,n_pos);
                for(auto c : chars.keySet())
                    if(c_with.get(c)==e) move_char(c,n_pos);
                if(n_pos!=pos) {
                    if(n==mvs-1) move_kind = 1; //Moved to
                    else         move_kind = 2; //Moving through
                }
                pos = animals.get(e);
            }

            list<std::string> on;
            std::string at = grid->grid[pos.x()][pos.y()];

            ///ADD
            //defacate
            //urinate
            //bad curiosity
            //good curiosity
            std::string action = "nothing";
            switch(move_kind) {
                case 0: //Stationary
                    switch(randi(1,10)) {
                        case 1: action = "eat"; break;
                        case 2: action = "eat"; break;
                        case 3: action = "eat"; break;
                        case 4: action = "eat"; break;
                        case 5: action = "eat"; break;
                        case 6: action = "shift"; break;
                        case 7: action = "rest"; break;
                        case 8: action = "rest"; break;
                        case 9: action = "rest"; break;
                        case 10: action = "rest"; break;
                    }
                break;
                case 1: //Moved to
                    switch(randi(1,10)) {
                        case 1: action = "eat"; break;
                        case 2: action = "eat"; break;
                        case 3: action = "eat"; break;
                        case 4: action = "shift"; break;
                        case 5: action = "shift"; break;
                        case 6: action = "shift"; break;
                        case 7: action = "shift"; break;
                        case 8: action = "shift"; break;
                        case 9: action = "rest"; break;
                        case 10: action = "rest"; break;
                    }
                break;
                case 2: //Moving through
                    switch(randi(1,1)) {
                        case 1: action = "move"; break;
                    }
                break;
                default:
                    print("Nothing");
            }
            if(!cow_actions.hasKey(e))
                cow_actions.put(e,action);
            else
                cow_actions.set(e,action);

                //This entire system will need to be replaced, ideally, by storing functions which describe the behaviour for each action,
                //like in GDSL 0.1.0+
                if(grid->is_group(at)) {
                    for(auto c : chars.entrySet()) {
                        if(grid->in_group(at).has(c.key)) {
                            on << c.key;
                        }
                    }
                    if(!on.empty()) {

                        std::string grouped_string;
                        std::string state_string;
                        list<std::string> to_remove;
                        list<std::string> auto_repeat;
                        for(auto c : on) {
                            if(c_with.get(c)!="none"&&c_with.get(c)!=e) continue;
                            std::string cstate = c_states.get(c);
                            std::string cname = c_names[c_inits.find(c)];
                            int cstrength = c_strength.get(c);
                            auto split = split_str(cstate,'_');
                            auto repeat_num = 0;
                            if(split.length()>2) {
                                cstate= split[0]+"_"+split[1];
                                repeat_num = std::stoi(split[2]);
                            }
                            if(cstate=="none") {
                                impact = true;
                                grouped_string.append(cname+", ");
                            } else if (cstate=="dead"&&c_with.get(c)==e) {
                                    if(!state_string.empty())
                                        state_string.append("and ");
                                    if(randi(1,10)>=8) {
                                        state_string.append("\x1b[31m\x1b[2m"+cname+"'s corpse falling off \x1b[0m");
                                        unattatch_char(c);
                                    } else {
                                        state_string.append("\x1b[31m\x1b[2m"+cname+"'s corpse \x1b[0m");
                                    }
                            } else if(cstate=="shift_impacted"||cstate=="rest_impacted"||cstate=="move_impacted"||cstate=="rest_impacted"||cstate=="eat_impacted") {
                                    if(!state_string.empty())
                                        state_string.append("and ");

                                    if(cstate=="shift_impacted") {
                                        bool s = action=="move";
                                        if(randi(1,10)+cstrength>s?6:8+repeat_num) {
                                            state_string.append("\x1b[32m"+cname+" squirming free from their hoof "+(s?"as they start moving":"")+" \x1b[0m");
                                            unattatch_char(c);
                                            change_char_state(c,"none");
                                            to_remove << c;
                                            continue;
                                        } else if(s) {
                                            cstate = "move_impacted";
                                            change_char_state(c,"move_impacted");
                                        } else {
                                            state_string.append(cname+" under their hoof ");
                                        }
                                    }
                                    else if(cstate=="move_impacted") {
                                        bool s = action=="shift";
                                        if(randi(1,10)+cstrength>s?6:8+repeat_num) {
                                            state_string.append("\x1b[32m"+cname+" peeling off the hoof "+(s?"as they stop":"")+" \x1b[0m");
                                            unattatch_char(c);
                                            change_char_state(c,"none");
                                            to_remove << c;
                                            continue;
                                        } else if(s) {
                                            cstate = "shift_impacted";
                                            change_char_state(c,"shift_impacted");
                                        } else {
                                            state_string.append(cname+" stuck to their hoof ");
                                        }
                                    } else if(cstate=="rest_impacted") {
                                        if(randi(1,10)+cstrength>8+repeat_num) {
                                            state_string.append("\x1b[32m"+cname+" squirming out from under them \x1b[0m");
                                            unattatch_char(c);
                                            change_char_state(c,"none");
                                            to_remove << c;
                                            continue;
                                        } else {
                                            state_string.append(cname+" under their body ");
                                            auto_repeat << c;
                                        }
                                    }
                                    else if(cstate=="eat_impacted") {
                                        if(randi(1,10)+cstrength>8+repeat_num) {
                                            state_string.append("\x1b[32m"+cname+" pulling free from their mouth \x1b[0m");
                                            unattatch_char(c);
                                            change_char_state(c,"none");
                                            to_remove << c;
                                            continue;
                                        } else {
                                            if(repeat_num<7) state_string.append(cname+" in their mouth ");
                                            else state_string.append(cname+" in their belly ");
                                            auto_repeat << c;
                                        }
                                    }
                            } 
                        }

                        if(!grouped_string.empty()) {
                            print(e,move_kind==0?" is standing in the same space as ":move_kind==1?" has moved to the location of ":" has moved through ",grouped_string,"at ",pos.to_string()," and is ",action_to_verb(action)," ",state_string.empty()?"":"with "+state_string);
                        } else if(!state_string.empty()) {
                            print(e,move_kind==0?" is standing around ":move_kind==1?" has moved to ":" moved through ",pos.to_string()," and is ",action_to_verb(action)," with ",state_string);
                        }
                        
                        for(auto t : to_remove) on.erase(t);
                        for(auto c : on) {
                            if(is_dead(c)) continue;
                            std::string cstate = c_states.get(c);
                            std::string cname = c_names[c_inits.find(c)];
                            int cstrength = c_strength.get(c);
                            if(c_with.get(c)!="none"&&c_with.get(c)!=e) continue;
                            std::string search_for = action+"_";
                            if(cstate.find(search_for)!=std::string::npos||auto_repeat.has(c)) { //Check if the char is already impacted by this action
                                auto split = split_str(cstate,'_');
                                int repeat_num = 0;
                                if(split.length()>2) {
                                    repeat_num = std::stoi(split[2])+1;
                                    change_char_state(c,split[0]+"_"+split[1]+"_"+std::to_string(repeat_num));
                                } else {
                                    change_char_state(c,cstate+"_0");
                                }
                                //print("REPEATNUM: ",repeat_num," for ",cname," who is with ",c_with.get(c)," and in state ",cstate," checking against ",e," who is ",action_to_verb(split[0]));
                                if(!action_map.get(split[0]).hasKey("repeat_"+std::to_string(repeat_num+1))) {
                                    kill_char(c);
                                }
                                print("\x1b[31m",c_states.get(c)=="dead"?"\x1b[2m":"\x1b[1m",c_names[c_inits.find(c)],action_map.get(split[0]).get("repeat_"+std::to_string(repeat_num)).rand(),"\x1b[0m");
                                change_char_strength(c,-1);
                            } else {
                                int prox = randi(1,20);
                                if(prox==1) { //Imedietly takes the char to repeat_2, need to make sure our actions actually *have* this though!
                                    change_char_state(c,action+"_impacted_2");
                                    if(!action_map.get(action).hasKey("repeat_2")) print("WARNING: missing second repeat for action: ",action,", needed for direct impact");
                                    if(!action_map.get(action).hasKey("repeat_3")) kill_char(c);
                                    print("\x1b[31m",c_states.get(c)=="dead"?"\x1b[2m":"\x1b[1m",c_names[c_inits.find(c)],action_map.get(action).get("repeat_2").rand(),"\x1b[0m");
                                    attatch_char_to(c,e);
                                    change_char_strength(c,-2);
                                } else if (prox<=10) {
                                    int threshold = 11-prox;
                                    int save_roll = randi(1,10)+cstrength;
                                    int margin = save_roll - threshold;
                                    if(margin<-8) {
                                        change_char_state(c,action+"_impacted");
                                        print("\x1b[31m\x1b[1m",c_names[c_inits.find(c)],action_map.get(action).get("strong_fail").rand(),"\x1b[0m");
                                        attatch_char_to(c,e);
                                        change_char_strength(c,-3);
                                    } else if (margin<-4) {
                                        print(c_names[c_inits.find(c)],action_map.get(action).get("normal_fail").rand());
                                        change_char_strength(c,-2);
                                    } else if (margin<0) {
                                        print(c_names[c_inits.find(c)],action_map.get(action).get("weak_fail").rand());
                                        change_char_strength(c,-1);
                                    } else if (margin<4) {
                                        print(c_names[c_inits.find(c)],action_map.get(action).get("weak_save").rand());
                                    } else if (margin<8) {
                                        print(c_names[c_inits.find(c)],action_map.get(action).get("normal_save").rand());
                                    } else if (margin>=8) {
                                        print(c_names[c_inits.find(c)],action_map.get(action).get("strong_save").rand());
                                    }
                                } else {
                                    print(c_names[c_inits.find(c)],action_map.get(action).get("avoid").rand());
                                }
                            }
                        }
                    }
                }
            }
    }

    void run_turn() {
        for(auto a : animals.entrySet()) {
                cow_logic(a.key);
        }
        for(auto c : chars.entrySet()) {
            if(!is_dead(c.key)) {
                change_char_strength(c.key,1);
            }
        }
    }

    #define init 0
    #define flipbook 0
    #define save 1
    int file_num = 1;

    void test() {

        std::ifstream in_meta(IROOT+"meta.json");
        if (in_meta.good()) {
            json meta;
            in_meta >> meta;
            int flogic_num = meta.value("farmlogic_num", 1);
            file_num = flogic_num;
        } else {
            file_num = 1;
        }
        in_meta.close();

        std::ifstream file(IROOT+"farmlogic"+(file_num<2?"":std::to_string(file_num))+".json");
        file >> j;
        #if init
            for(auto i : c_inits) {
                chars.put(i,ivec2(10,10));
                grid->place(ivec2(10,10),i);
                c_states.put(i,"none");
                c_with.put(i,"none");
                c_strength.put(i,4);
                j["chars"][i]["id"] = i;
                j["chars"][i]["state"] = "none";
                j["chars"][i]["with"] = "none";
                j["chars"][i]["strength"] = 4;
                j["chars"][i]["pos"][0] = 10;
                j["chars"][i]["pos"][1] = 10;
            }
            for(int i=0;i<7;i++) {
                std::string id = "C";
                if(i<9) id.append("0"+std::to_string(i));
                else id.append(std::to_string(i));
                ivec2 pos(randi(0,s_x-1),randi(0,s_y-1));
                animals.put(id,pos);
                a_states.put(id,"none");
                grid->place(pos,id);
                j["animals"][id]["id"] = id;
                j["animals"][id]["state"] = "none";
                j["animals"][id]["pos"][0] = pos.x();
                j["animals"][id]["pos"][1] = pos.y();
            }
            j["flipbook"].clear();
        #else
            for(auto c : j["chars"]) {
                ivec2 pos(c["pos"][0],c["pos"][1]);
                chars.put(c["id"],pos);
                c_states.put(c["id"],c.value("state","none"));
                c_with.put(c["id"],c.value("with","none"));
                c_strength.put(c["id"],c.value("strength",4));
                grid->place(pos,c["id"]);
            }
            for(auto a : j["animals"]) {
                ivec2 pos(a["pos"][0],a["pos"][1]);
                animals.put(a["id"],pos);
                a_states.put(a["id"],a.value("state","none"));
                grid->place(pos,a["id"]);
            }
        #endif

        #if flipbook
            auto start = std::chrono::high_resolution_clock::now();
            int pro = 0;
            while(!helper::pressed(S)) {
                auto end = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()/1000000;
                if(time>500) {
                    if(pro<j["flipbook"].size()) {
                        std::string t = j["flipbook"][pro];
                        print(t);
                        pro++;
                    } else {
                        pro = 0;
                        print("\n----------------------------------------\n");
                    }
                    start = end;
                } else {

                }
            }
        #else
            init_action_map_for_cows();
            run_turn();
            // print(action_map.get("move").get("strong_save").rand());

            list<std::string> to_move = {};
            list<ivec2> facing = {};
            list<bool> look_around = {};

            // if(!impact) {
            //     to_move = {"R","C","G","L","D","Y","K","A","M"};
            //     //facing = {{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0}};
            //     for(int m=0;m<to_move.length();m++) facing << ivec2(0,-1);
            //     look_around = {true};
            // }

            for(int m=0;m<to_move.length();m++) {
                std::string on_c = to_move[m];
                if(is_dead(on_c)) continue;
                move_char(on_c,chars.get(on_c)+facing[m]);
                if(m<look_around.length()&&look_around[m]) {
                    ivec2 pos = chars.get(on_c);
                    list<std::string> around = grid->around(pos,3);
                    if(grid->is_group(pos)) around << grid->in_group(grid->grid[pos.x()][pos.y()]);
                    for(auto g : around) {
                        if(animals.hasKey(g)) {
                            ivec2 a_pos = animals.get(g);
                            if(!cow_actions.hasKey(g)) cow_actions.put(g,"nothing");
                            print(g," is ",(int)(pos-a_pos).length()," units ",grid->relative_dir(pos-a_pos,facing[m])," of ",c_names[c_inits.find(on_c)],"(who is at ",pos.to_string(),") and ",action_to_verb(cow_actions.get(g)));
                        }
                    }        
                }
            }
           
            print(grid->to_string());

            //j["flipbook"].push_back(grid->to_string());
        #endif

        #if save
            file_num += 1;
            if(file_num>3) file_num = 1;
            print("Saving to: ",file_num);
            std::ofstream o_file(IROOT+"farmlogic"+(file_num<2?"":std::to_string(file_num))+".json");
            o_file << j.dump(4); 
            o_file.close();

            std::ofstream out_meta(IROOT+"meta.json");
            out_meta << json{{"farmlogic_num", file_num}};
            out_meta.close();
        #endif
    }

    

}