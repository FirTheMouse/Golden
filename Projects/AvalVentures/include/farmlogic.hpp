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

    json j;
    bool impact = false;
    auto grid = make<FarmLogic>(s_x,s_y);

    class f_object : public Object {
    public:
        json& info;
        std::string id;
        f_object(json& _info,std::string _id) : info(_info), id(_id) {}

        void move(ivec2 to) {
            grid->move(id,to);
            info["pos"][0] = to.x();
            info["pos"][1] = to.y();
        }

        ivec2 get_pos() {
            return {info["pos"][0],info["pos"][1]};
        }

        std::string get_action() {
            return info.value("action","nothing");
        }
        void set_action(const std::string& to) {
            info["action"] = to;
        }
        
        std::string get_with() {
            return info.value("with","none");
        }
        void set_with(const std::string& to) {
            info["with"] = to;
        }

        std::string get_state() {
            return info.value("state","none");
        }
        void set_state(const std::string& to) {
            info["state"] = to;
        }

        int get_energy() {
            return info.value("energy",0);
        }
        void set_energy(int to) {
            info["energy"] = to;
        }
        void change_energy(int by) {
            info["energy"] = info.value("energy",0)+by;
        }

        float get_injury() {
            return info.value("injury",0.0f);
        }
        void set_injury(float to) {
            info["injury"] = to;
        }
        void change_injury(float by) {
            info["injury"] = info.value("injury",0.0f)+by;
        }

        void kill() {set_state("dead");}
        bool is_dead() {return get_state()=="dead";}
    };

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

    map<std::string,g_ptr<f_object>> chars;
    map<std::string,g_ptr<f_object>> animals;
    map<std::string,ivec2> f_pos;
    list<std::string> c_inits = {"R","L","G","A","Y","K","C","D","M"};
    list<std::string> c_names = {"Ro","Luis","Grace","Anton","Yara","Kells","Mina","Daniel","Milo"};
    
    map<std::string,map<std::string,list<std::string>>> action_map;

    void init_action_map_for_cows() {
        if(action_map.size()==0) {
            map<std::string,list<std::string>> new_map;

            auto append_map = [](map<std::string,list<std::string>>& map){  
                map.put("avoid",{
                    " was out of the way", //0
                    " wasn't close to it" //1
                });
                map.put("strong_save",{
                    " deftly backpedaled", //0
                    " swiftly sidestepped", //1
                    " fluidly dodged" //2
                });
                map.put("normal_save",{
                    " dove out of the way", //0
                    " leapt away", //1
                    " ran away just in time" //2
                });
                map.put("weak_save",{
                    " desperately dove away just in time", //0
                    " frantically leapt out of the way", //1
                    " narrowly rolled away" //2
                });
            };
            

            append_map(new_map);
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
            new_map.clear();
            append_map(new_map);

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
                " noticed the hoof and failed to react, being caught across the side and taking a nasty gash and sticking them to the hoof", //0
                " attempted to dive away and failed, their chest being struck, sticking them to the hoof", //1
                " couldn't get away in time and was struck in the leg, snapping it and sticking them to the hoof" //2
            });
            new_map.put("repeat_0",{
                " was taken with the hoof" //0
            });
            new_map.put("repeat_1",{
                " shudders as the next step lands" //0
            });
            new_map.put("repeat_2",{
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
            new_map.clear();
            append_map(new_map);

            new_map.put("weak_fail",{
                " frantically dove away, but got their leg bruised", //0
                " attempted to roll away, but got coated in muck" //1 
            });
            new_map.put("normal_fail",{
                " attempted to dive away, but got their legs pinned, squeezing out", //0
                " ran and tripped, getting covered in muck", //1
                " tried to step away, but got pinned, struggling out", //2
                " tried to run, but got hit in the back of the leg, pulling it free at the last momment" //3
            });
            new_map.put("strong_fail",{
                " tried to dodge, but got buried under flesh", //0
                " attempted to dive away, only to be slammed into the ground by the body", //1
                " couldn't get away in time and was firmly pinned under thousands of pounds" //2
            });
            new_map.put("repeat_0",{
                " was worked under the flesh" //0
            });
                new_map.put("static_0", {
                    " is struggling under the body ", //0
                    " is trying to pull themselves out before they get worked deeper " //1
                });
                new_map.put("react_shift_0",{
                    " is worked deeper by a lazy shift " //0
                });
            new_map.put("repeat_1",{
                " is pulled in up to their belly" //0
            });
                new_map.put("react_shift_1",{
                    " is worked up to their belly by a shift " //0
                });
            new_map.put("repeat_2",{
                " is fully pulled under the resting body", //0
                " is worked under the body with a nasty sound" //1
            });
                new_map.put("static_2", {
                    " is screaming under thousands of pounds of flesh ", //0
                    " squirms against the body ", //1
                    " is gasping at the stench " //2
                });
                new_map.put("react_shift_2",{
                    " can't breath as the body shifts on top of them " //0
                });
            new_map.put("repeat_3",{
                " is being smoothered under thousands of pounds of flesh" //0
            });
                new_map.put("react_shift_3",{
                    " can feel their ribs creak as the body shifts" //0
                });
            new_map.put("repeat_4",{
                " can only breath the stench of a body" //0
            });
                new_map.put("react_shift_4",{
                    " cries as a shift catches their arm" //0
                });
            new_map.put("repeat_5",{
                " screams as their arm relents under the weight" //0
            });
                new_map.put("static_5", {
                    " whimpers in pain, one shift from being crushed ", //0
                    " screams as they struggle, futile ", //1
                    " is gasping at the stench and only breathing flesh " //2
                });
                new_map.put("react_shift_5",{
                    " screams as their arm is torn from the socket" //0
                });
            new_map.put("repeat_6",{
                " takes one last gasp of stench and flesh, then fades" //0
            });
                new_map.put("react_shift_6",{
                   " is reduced to a mangled mess by a final shift" //0
                });
            action_map.put("rest",new_map);
            new_map.clear();
            append_map(new_map);

            new_map.put("weak_fail",{
                " frantically dove away, but got slobber on their leg", //0
                " attempted to roll away, but got clipped by the lip and slimed", //1
                " tried to run, but got hit in the back of the leg, getting covered in spit" //2
            });
            new_map.put("normal_fail",{
                " attempted to dive away, but got their legs taken in, pulling them out at the last momment", //0
                " ran and tripped, getting covered in muck", //1
                " tried to step away, but got their head slurped up, squirming free at the last momment" //2
            });
            new_map.put("strong_fail",{
                " tried to dodge, but got caught in the lips", //0
                " attempted to dive away, only to be slurped up to the waist", //1
                " couldn't get away in time and was firmly taken up to the waist" //2
            });
            new_map.put("repeat_0",{
                " is taken in deeper, up to the belly" //0
            });
                new_map.put("static_0", {
                    " is dangling from the lips " //0
                });
            new_map.put("repeat_1",{
                " is slurped up to the shoulders" //0
            });
            new_map.put("repeat_2",{
                " is fully taken into the mouth" //0
            });
                new_map.put("static_2", {
                    " is struggling inside the mouth " //0
                });
            new_map.put("repeat_3",{
                " is caught in the incisors as the next bite is taken, lacerating their arms ;t/injury:+4", //0
                " is pushed back with the next bite ;t/state:eat_impacted_4", //1
                " is integrated into the bolus and taken deeper ;t/state:eat_impacted_4", //2
                " fights like hell and manages to resist being integrated with the bolus ;t/energy:-6" //2
            });
            new_map.put("repeat_4",{
                " is bludgoned by an incoming bite and pushed towards the throat; t/state:eat_impacted_5", //0
                "'s head is crushed under the tounge as the mass of food pushes down; t/injury:+2", //1
                "'s legs are trapped and twisted by the passing bolus; t/injury:+2" //2
            });
            new_map.put("repeat_5",{
                " is blended with food, their world becoming nothing but mash and stink ", //0
                " is worked onto the molars, their lower body pulvurized ;t/injury:+10", //1
                "'s arm hits a molar, before being obliterated to the shoulder ;t/injury:+10", //2
                "'s leg is caught on a molar during a chew and torn open ;t/injury:+6", //3
                " struggles and manages to avoid being chewed or worked deeper ;t/energy:-6;t/state:eat_impacted_4"
            });
                new_map.put("static_5", {
                    " is just waiting for the gulp", //0
                    " is fighting to stay away from the molars" //1
                });
            new_map.put("repeat_6",{
                " is sent to the stomach with a single gulp", //0
                " is pushed back to the molars ;t/state:eat_impacted_4"
            });
            list<std::string> ars = {"rest","move","shift"};
            for(int i=0;i<6;i++) {
                std::string i_str = std::to_string(i);
                for(auto a : ars) {
                    if (i<3) {
                        new_map.put("react_"+a+"_"+i_str,{
                            " is lazily worked deeper by the lips ", //0
                            " is drawn a bit deeper by the lips ", //1
                            "'s body is lazily slurped up ;t/state:eat_impacted_2", //2
                            " slips from the lips a bit ;t/state:eat_impacted_0" //3
                        });
                    } else if (i==3) {
                        new_map.put("react_"+a+"_"+i_str,{
                            " is shifted back past the incisors", //0
                            " is caught and worked around by the inner lips ;t/state:eat_impacted_2", //1
                            " is bullied to the back of the mouth", //2
                            " is shoved all the way to the molars by the tounge ;t/state:eat_impacted_4", //3
                            " struggles to pull themselves out of the lips ;t/energy:-6;t/state:eat_impacted_1" //4
                        });
                    } else if (i==4) {
                        new_map.put("react_"+a+"_"+i_str,{
                            " is being wrangled by the tounge towards the back of the mouth", //0
                            " is collected by the tounge and pushed towards the molars", //1
                            " is pushed back towards the lips by chance ;t/state:eat_impacted_2", //2
                            " struggles towards the lips, fighting against the tounge ;t/energy:-6;t/state:eat_impacted_2" //3
                        });
                    } else if (i==5) {
                        new_map.put("react_"+a+"_"+i_str,{
                            " can only scream as they're worked around the back of the mouth ;t/state:eat_impacted_4", //0
                            " is hardly able to breath as saliva and tounge surround them ;t/state:eat_impacted_4", //1
                            " fights their way away from the throat and molars ; t/energy:-6 ; t/state:eat_impacted_3", //2
                            "'s legs scrape against a molar ;t/injury:+1 ; t/state:eat_impacted_4", //3
                            "'s arm is caught in a lazy grind of teeth, and pulvurized ;t/injury:+5 ; t/state:eat_impacted_4", //4
                            " is pushed back towards the lips by chance ;t/state:eat_impacted_2", //5
                            " is gulped down with a lazy sweep of the tounge ;t/state:eat_impacted_7" //6
                        });
                    }
                }
            }
            new_map.put("repeat_7",{
                " is hit by fresh boluses of food ;t/injury:+2.0;t/state:eat_impacted_6", //0
                " is mixed under the new food ;t/injury:+1.8;t/state:eat_impacted_6", //1
                " can hear more food join them ;t/injury:+1.2;t/state:eat_impacted_6" //2
            });
            new_map.put("react_rest_7",{
                " is being churned away as they rest ;t/injury:+1.0;t/state:eat_impacted_6" //0
            });
            new_map.put("react_move_7",{
                " is jostled as they walk ;t/injury:+1.2;t/state:eat_impacted_6" //0
            });
                new_map.put("static_7",{
                    " is being worked on by the stomach ;t/injury:+0.8;t/state:eat_impacted_6", //0
                    " is slowly digesting ;t/injury:+0.8;t/state:eat_impacted_6", //1
                    " is regurgitated to be chewed with cud ;t/state:eat_impacted_4;t/injury:+0.8" //2
                });
            action_map.put("eat",new_map);
            new_map.clear();
            append_map(new_map);


            new_map.put("weak_fail",{
                " frantically dove away, but got their leg bruised", //0
                " attempted to roll away, but got coated in muck" //1 
            });
            new_map.put("normal_fail",{
                " attempted to dive away, but got their legs pinned, squeezing out", //0
                " ran and tripped, getting covered in muck", //1
                " tried to step away, but got pinned, struggling out", //2
                " tried to run, but got hit in the back of the leg, pulling it free at the last momment" //3
            });
            new_map.put("strong_fail",{
                " tried to dodge, but got buried under flesh", //0
                " attempted to dive away, only to be slammed into the ground by the body", //1
                " couldn't get away in time and was firmly pinned under thousands of pounds" //2
            });
            new_map.put("repeat_0",{
                " is taken in deeper, up to the belly" //0
            });
                new_map.put("static_0", {
                    " is dangling from the lips " //0
                });
            new_map.put("repeat_1",{
                " is slurped up to the shoulders" //0
            });
            new_map.put("repeat_2",{
                " is fully taken into the mouth" //0
            });
                new_map.put("static_2", {
                    " is struggling inside the mouth " //0
                });
            action_map.put("bad curiosity",new_map);
            new_map.clear();
            append_map(new_map);
        }
    }


    std::string process_action_desc(g_ptr<f_object> f, g_ptr<f_object> cf, const std::string& action_desc) {
        auto asplit = split_str(action_desc,';');
        if(asplit.length()>1) {
            for(int i=1;i<asplit.length();i++) {
                list<g_ptr<f_object>> targets;  
                list<std::string> cmds;
                std::string value = "";

                std::string a = asplit[i];
    print("\x1b[34;1mPROCESSING: \x1b[0m",a);
                auto osplit = split_str(a,':');
                for(int o=0;o<osplit.length();o++) {
                    auto lsplit = split_str(osplit[o],'/');
                    if(lsplit.length()>1) { //Treat as a property modification (i.e t_injury)
                        if(lsplit[0]=="s") targets << f;
                        else if(lsplit[0]=="t") targets << cf;
                        else { 
                            if(chars.hasKey(lsplit[0])) {
                                targets << chars.get(lsplit[0]);
                            } else if(animals.hasKey(lsplit[0])) {
                                targets << animals.get(lsplit[0]);
                            } else {
                                print("Unable to find ID for target in action_desc: ",a);
                            }
                        }
                        cmds << lsplit[1];
                    } else if(o==1) { //Treat as a command (i.e say)
                        value = lsplit[0];
                    } else {
                        cmds << lsplit[0];
                    }
                }

    print("\x1b[32mFINISHED SLICING\x1b[0m");
    print("TARGETS:");
    int z = 0;
    for(auto t : targets) {
        print(" ",z,"| ",t->id);
        z++;
    }
    print("CMDS:");
    z=0;
    for(auto c : cmds) {
        print(" ",z,"| ",c);
        z++;
    }
    print("VALUE: \n  |",value);

                //Succsefully extracted a target, or, is not an assignment opperator
                if(!targets.empty()&&!cmds.empty()) {
                    std::string opp = value.substr(0,1);
                    if(std::isalpha(opp.front())) opp = ""; //So opps are only - + = etc.. and not just the first letter
                    else value = value.substr(1); //Clipping off the opperator

                    if(targets.length()>1) { //Need to make sure this is a string somehow
                        value = targets[1]->info.value(cmds[1],"0");
                    }

            //This is shit for performance, but noted for later revision if bottlenecks are ever hit
                    try { //Is float?
                        float val = std::stof(value);
                        if(opp=="+")
                            targets[0]->info[cmds[0]] = targets[0]->info.value(cmds[0],0)+val;
                        else if(opp=="-")
                            targets[0]->info[cmds[0]] = targets[0]->info.value(cmds[0],0)-val;
                        else
                            targets[0]->info[cmds[0]] = val;
                    } 
                    catch(...) { 
                        try { //Is int?
                            float val = std::stoi(value);
                            if(opp=="+")
                                targets[0]->info[cmds[0]] = targets[0]->info.value(cmds[0],0)+val;
                            else if(opp=="-")
                                targets[0]->info[cmds[0]] = targets[0]->info.value(cmds[0],0)-val;
                            else
                                targets[0]->info[cmds[0]] = val;
                        } 
                        catch(...) { //Is string
                            targets[0]->info[cmds[0]] = value;
                        }
                    }
                } else {
                    print("FAILED DUE TO");
                    if(targets.empty()) print("NO TARGET");
                    print("STAT: ",cmds[0]);
                    print("VALUE: ",value);
                }

    print("\x1b[32mFINISHED PROCESSING\x1b[0m");
            }
        }
        return asplit[0];
    }

    ivec2 roll_move(ivec2 from,ivec2 exclude = {-1,-1}) {
        ivec2 n_pos = from+ivec2(randi(-1,1),randi(-1,1));
        while(!in_bounds(n_pos)||n_pos==exclude) {
            n_pos = from+ivec2(randi(-1,1),randi(-1,1));
        }
        return n_pos;
    }


    std::string col_name(const std::string& e) {
        std::string ename = "";
        int enumber = std::stoi(e.substr(1,2));
        std::string col_code = "\x1b[";
        if(enumber<=14) {
            int col_num = enumber+(enumber<=7?30:90);
            col_code.append(std::to_string(col_num)+"m");
        }

        ename = col_code+e+"\x1b[0m";
        return ename;
    }

    void cow_logic(const std::string& e) {
        auto f = animals.get(e);
        std::string ename = col_name(e);
        ivec2 pos = f->get_pos();

        int mvs_max = std::max(f->get_energy(),1);
        int mvs = randi(1,mvs_max);
        if(f->get_action()=="rest") { //If we're resting we're more likley to stay
            mvs = std::max(1,randi(0,10)>8?2:1);
            if(f->get_energy()<6&&mvs==1) f->change_energy(2); //Increase energy for a good nap!
        } else if(f->get_action()=="eat") { //Same goes for eating 
            mvs = std::max(1,randi(0,10)>6?2:1);
            if(f->get_energy()>-6)  //Eating is tiring work!
                f->change_energy(-1);
        } else {
            int decrease = -1;
            if(f->get_action()=="shift") decrease = 0;
            if(f->get_energy()>-4)
                f->change_energy(decrease);
        }

        int move_kind = 0; //Stationary
        for(int n=0;n<mvs;n++) {
            move_kind = 0;
            if(mvs>1) {
                ivec2 n_pos = roll_move(pos,pos);
                //Advantage for 'cosmic randomness' rolls, to keep things interesting!
                for(int i=0;i<2;i++) {
                    ivec2 n_roll = roll_move(pos,pos);
                    for(auto c : chars.entrySet()) {

                        if(c.value->is_dead()) continue;
                        if(c.value->get_with()==e) continue;
                        if((c.value->get_pos()-pos).length()<6) {
                            // if(a.key=="C02") print("Choosing closer: ",n_roll.to_string()," ",n_pos.to_string());
                            if((c.value->get_pos()-n_roll).length()<(c.value->get_pos()-n_pos).length()) {
                                n_pos = n_roll;
                                break;
                            }
                        }
                    }
                }
                //print(a.key," Moved from ",a.value.to_string()," to ",n_pos.to_string(),": ",(n_pos-a.value).to_string());
                f->move(n_pos);
                for(auto c : chars.getAll())
                    if(c->get_with()==e) c->move(n_pos);
                if(n_pos!=pos) {
                    if(n==mvs-1) move_kind = 1; //Moved to
                    else         move_kind = 2; //Moving through
                }
                pos = n_pos;
            }

            list<std::string> on;
            std::string at = grid->grid[f->get_pos().x()][f->get_pos().y()];

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
            f->set_action(action);

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
                            auto cf = chars.get(c);
                            if(cf->get_with()!="none"&&cf->get_with()!=e) continue;
                            std::string cstate = cf->get_state();
                            std::string cname = c_names[c_inits.find(c)];
                            int cstrength = cf->get_energy()-(int)cf->get_injury();
                            auto split = split_str(cstate,'_');
                            auto repeat_num = 0;
                            if(split.length()>2) {
                                cstate= split[0]+"_"+split[1];
                                repeat_num = std::stoi(split[2]);
                            }
                            if(cstate=="none") {
                                if(!grouped_string.empty()) {
                                    grouped_string.append(", "+cname);
                                } else {
                                    grouped_string.append(cname);
                                }
                            } else if (cstate=="dead") {
                                if(cf->get_with()==e) {
                                    if(!state_string.empty())
                                        state_string.append("and ");
                                    if(randi(1,10)>=8) {
                                        state_string.append("\x1b[31m\x1b[2m"+cname+"'s corpse falling off \x1b[0m");
                                        cf->set_with("none");
                                    } else {
                                        state_string.append("\x1b[31m\x1b[2m"+cname+"'s corpse \x1b[0m");
                                    } 
                                } // v Should maybe do an impacted check here?
                            } else if(cstate!="none") {
                                    if(!state_string.empty())
                                        state_string.append("and ");

                                    int save_roll = randi(1,10)+cstrength;
                                    int dc = repeat_num+f->get_energy();

                                    if(cstate=="shift_impacted") {
                                        bool s = action=="move";
                                        if(save_roll>s?6:8+dc) {
                                            state_string.append("\x1b[32m"+cname+" squirming free from their hoof "+(s?"as they start moving":"")+" \x1b[0m");
                                            cf->set_with("none");
                                            cf->set_state("none");
                                            to_remove << c;
                                            continue;
                                        } else if(s) {
                                            cstate = "move_impacted";
                                            cf->set_state("move_impacted");
                                        } else if(action=="rest") {
                                            cstate = "rest_impacted";
                                            cf->set_state("rest_impacted");
                                        }
                                        else {
                                            state_string.append(cname+" under their hoof ");
                                        }
                                    }
                                    else if(cstate=="move_impacted") {
                                        bool s = action=="shift";
                                        if(save_roll>s?6:8+dc) {
                                            state_string.append("\x1b[32m"+cname+" peeling off the hoof "+(s?"as they stop":"")+" \x1b[0m");
                                            cf->set_with("none");
                                            cf->set_state("none");
                                            to_remove << c;
                                            continue;
                                        } else if(s) {
                                            cstate = "shift_impacted";
                                            cf->set_state("shift_impacted");
                                        } else if(action=="rest") {
                                            cstate = "rest_impacted";
                                            cf->set_state("rest_impacted");
                                        }
                                        else {
                                            state_string.append(cname+" stuck to their hoof ");
                                        }
                                    } else if(cstate=="rest_impacted") {
                                        if(save_roll>move_kind!=0?4:8+dc) {
                                            if(move_kind==0)
                                                state_string.append("\x1b[32m"+cname+" squirming out from under them \x1b[0m");
                                            else 
                                                state_string.append("\x1b[32m"+cname+" peeling off as they move \x1b[0m");
                                            cf->set_with("none");
                                            cf->set_state("none");
                                            to_remove << c;
                                            continue;
                                        }
                                        else {
                                            if(move_kind==0)
                                                state_string.append(cname+" under their body ");
                                            else
                                                state_string.append(cname+" stuck to their body ");
                                            auto_repeat << c;
                                        }
                                    }
                                    else if(cstate=="eat_impacted") {
                                        print("ROLLED: ",save_roll," AGAINST: ",8+dc);
                                        if(save_roll>8+dc) {
                                            state_string.append("\x1b[32m"+cname+" pulling free from their mouth \x1b[0m");
                                            cf->set_with("none");
                                            cf->set_state("none");
                                            to_remove << c;
                                            continue;
                                        } else {
                                            if(repeat_num<7) state_string.append(cname+" in their mouth ");
                                            else state_string.append(cname+" in their belly ");
                                            auto_repeat << c;
                                        }
                                    } else {
                                        state_string.append(cname+" state "+cstate);
                                    }
                            } 
                        }

                        if(!grouped_string.empty()) {
                            print(ename,
                                move_kind==0?" is "+action_to_verb(action)+" at "+f->get_pos().to_string()+", the same space as "+grouped_string:
                                move_kind==1?" has moved to "+f->get_pos().to_string()+", the location of "+grouped_string+", to "+action:
                                " has moved through "+f->get_pos().to_string()+", impacting "+grouped_string,
                                "\x1b[0m",state_string.empty()?"":" with "+state_string);
                        } else if(!state_string.empty()) {
                            print(ename,
                                move_kind==0?" is "+action_to_verb(action)+" at "+f->get_pos().to_string():
                                move_kind==1?" has moved to "+f->get_pos().to_string()+" to "+action:
                                " moved through "+f->get_pos().to_string(),
                                " with ",state_string);
                        }
                        
                        for(auto t : to_remove) on.erase(t);
                        for(auto c : on) {
                            auto cf = chars.get(c);
                            if(cf->is_dead()) continue;
                            std::string cstate = cf->get_state();
                            std::string cname = c_names[c_inits.find(c)];
                            std::string action_desc = " [missing action desc] ";
                            int cstrength = cf->get_energy()-(int)cf->get_injury();
                            if(cf->get_with()!="none"&&cf->get_with()!=e) continue;
                            std::string search_for = action+"_";
                            if(cstate.find(search_for)!=std::string::npos||auto_repeat.has(c)) { //Check if the char is already impacted by this action
                                auto split = split_str(cstate,'_');
                                int repeat_num = 0;
                                if(split.length()>2) {
                                    repeat_num = std::stoi(split[2])+1;
                                    cf->set_state(split[0]+"_"+split[1]+"_"+std::to_string(repeat_num));
                                } else {
                                    cf->set_state(cstate+"_0");
                                }
                                bool is_static = false;
                                //Check if we have a react to use
                                if(action!=split[0]) { //Because some things interact, some things don't
                                    if(action_map.get(split[0]).hasKey("react_"+action+"_"+std::to_string(repeat_num))) {
                                        action_desc = action_map.get(split[0]).get("react_"+action+"_"+std::to_string(repeat_num)).rand();
                                    } else if(repeat_num>0) {
                                        repeat_num--;
                                        is_static = true;
                                        cf->set_state(split[0]+"_"+split[1]+"_"+std::to_string(repeat_num));
                                        int check_num = repeat_num;
                                        //This may need to be optimized as a sort of binary search, because ceartin long duration impacts could make this search *long*.
                                //Profile later if we start having performance problems
                                        while(check_num>=0) {
                                            if(action_map.get(split[0]).hasKey("static_"+std::to_string(check_num))) {
                                                 action_desc = action_map.get(split[0]).get("static_"+std::to_string(check_num)).rand();  
                                                 break; 
                                            }
                                            check_num--;
                                        }
                                        if(check_num<0) {
                                            action_desc = action_map.get(split[0]).get("repeat_"+std::to_string(repeat_num)).rand();
                                        }
                                    } else {
                                        action_desc = action_map.get(split[0]).get("repeat_"+std::to_string(repeat_num)).rand();
                                    }
                                } else {
                                    action_desc = action_map.get(split[0]).get("repeat_"+std::to_string(repeat_num)).rand();
                                }

                                //print("REPEATNUM: ",repeat_num," for ",cname," who is with ",cf->get_with()," and in state ",cstate," actually ",cf->get_state()," checking against ",e," who is ",action_to_verb(split[0]));
                                if(!action_map.get(split[0]).hasKey("repeat_"+std::to_string(repeat_num+1))&&repeat_num>0) {
                                    cf->change_injury(3.0f);
                                    repeat_num--;
                                    is_static = true;
                                    cf->set_state(split[0]+"_"+split[1]+"_"+std::to_string(repeat_num));
                                    action_desc = action_map.get(split[0]).get("repeat_"+std::to_string(repeat_num)).rand();
                                }
                                if(is_static) {
                                    cf->change_energy(-1);
                                } else {
                                    cf->change_energy(-2);
                                }
                                print("\x1b[31m",cf->get_state()=="dead"?"\x1b[2m":is_static?"":"\x1b[1m",c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc),"\x1b[0m");
                            } else {
                                impact = true;
                                int prox = randi(1,40);
                                if(prox<=1) { //Imedietly takes the char to repeat_2, need to make sure our actions actually *have* this though!
                                    cf->set_state(action+"_impacted_2");
                                    if(!action_map.get(action).hasKey("repeat_2")) print("WARNING: missing second repeat for action: ",action,", needed for direct impact");
                                    if(!action_map.get(action).hasKey("repeat_3")) cf->change_injury(3.0f);
                                    action_desc = action_map.get(action).get("repeat_2").rand();
                                    cf->set_with(e);
                                    cf->change_energy(-4);
                                    cf->change_injury(1.4f);
                                    print("\x1b[31m",cf->get_state()=="dead"?"\x1b[2m":"\x1b[1m",c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc),"\x1b[0m");
                                } else if (prox<=20) {
                                    int threshold = 11-prox;
                                    int save_roll = randi(1,10)+cstrength;
                                    int margin = save_roll - threshold;
                                    if(margin<(action=="eat"?-6:-8)) {
                                        cf->set_state(action+"_impacted");
                                        action_desc = action_map.get(action).get("strong_fail").rand();
                                        cf->set_with(e);
                                        cf->change_energy(-4);
                                        cf->change_injury(1.0f);
                                        print("\x1b[31m\x1b[1m",c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc),"\x1b[0m");
                                    } else if (margin<-4) {
                                        action_desc = action_map.get(action).get("normal_fail").rand();
                                        print("\x1b[33m",c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc),"\x1b[0m");
                                        cf->change_energy(-3);
                                        cf->change_injury(0.3f);
                                    } else if (margin<0) {
                                        action_desc = action_map.get(action).get("weak_fail").rand();
                                        print("\x1b[33m",c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc),"\x1b[0m");
                                        cf->change_energy(-2);
                                        cf->change_injury(0.1f);
                                    } else if (margin<4) {
                                        action_desc = action_map.get(action).get("weak_save").rand();
                                        print(c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc));
                                        cf->change_energy(-2);
                                    } else if (margin<8) {
                                        action_desc = action_map.get(action).get("normal_save").rand();
                                        print(c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc));
                                        cf->change_energy(-1);
                                    } else if (margin>=8) {
                                        action_desc = action_map.get(action).get("strong_save").rand();
                                        print(c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc));
                                    }
                                } else {
                                    action_desc = action_map.get(action).get("avoid").rand();
                                    print(c_names[c_inits.find(c)],process_action_desc(f,cf,action_desc));
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
            if(!c.value->is_dead()) {
                if(c.value->get_energy()<4)
                    c.value->change_energy(1);
                if(c.value->get_injury()>20) {
                    c.value->kill();
                    print("\x1b[31;2m",c_names[c_inits.find(c.key)]," has sucumbed to their injuries \x1b[0m");
                }

            }
        }
    }

    g_ptr<f_object> init_fobj(json& jc,const std::string& id,ivec2 pos) {
        jc["id"] = id;
        jc["state"] = "none";
        jc["with"] = "none";
        jc["energy"] = 4;
        jc["injury"] = 0.0f;
        jc["action"] = "nothing";
        jc["pos"][0] = pos.x();
        jc["pos"][1] = pos.y();
        g_ptr<f_object> f = make<f_object>(jc,id);
        grid->place(pos,id);
        return f;
    }

    #define init 1
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
                ivec2 pos(10,10);
                json& jc = j["chars"][i];
                chars.put(i,init_fobj(jc,i,pos));
            }
            for(int i=0;i<7;i++) {
                std::string id = "C";
                if(i<9) id.append("0"+std::to_string(i));
                else id.append(std::to_string(i));
                ivec2 pos(randi(0,s_x-1),randi(0,s_y-1));
                json& jc = j["animals"][id];
                animals.put(id,init_fobj(jc,id,pos));
            }
            j["flipbook"].clear();
        #else
            for(auto& cinfo : j["chars"]) {
                g_ptr<f_object> c = make<f_object>(cinfo,cinfo["id"]);
                chars.put(c->id,c);
                grid->place(c->get_pos(),c->id);
            }
            for(auto& ainfo : j["animals"]) {
                g_ptr<f_object> a = make<f_object>(ainfo,ainfo["id"]);
                animals.put(a->id,a);
                grid->place(a->get_pos(),a->id);
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

            auto f = animals.get("C01");
            auto cf = chars.get("R");
            print("CF ENG: ",cf->get_energy()," CF STATE: ",cf->get_state()," CF INJ: ",cf->get_injury()," CF WITH: ",cf->get_with());
            print("F ENG: ",f->get_energy()," F STATE: ",f->get_state()," F ACTION: ",f->get_action());
            print("Processing action: ",process_action_desc(f,cf,"Test action ;t/energy:+10"));
            print("CF ENG: ",cf->get_energy()," CF STATE: ",cf->get_state()," CF INJ: ",cf->get_injury()," CF WITH: ",cf->get_with());
            print("F ENG: ",f->get_energy()," F STATE: ",f->get_state()," F ACTION: ",f->get_action());


            // init_action_map_for_cows();
            // run_turn();

            // list<std::string> to_move = {};
            // list<ivec2> facing = {};
            // list<bool> look_around = {};

            // // if(!impact) {
            // //     to_move = {"R","C","G","L","D","Y","K","A","M"};
            // //     //facing = {{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0}};
            // //     for(int m=0;m<to_move.length();m++) facing << ivec2(1,0);
            // //     facing[0] = ivec2(1,1); //RO's move
            // //     look_around = {true,true};
            // // }

            // print("---------------------------");


            // for(int m=0;m<to_move.length();m++) {
            //     std::string on_c = to_move[m];
            //     auto c = chars.get(on_c);
            //     if(c->is_dead()) continue;
            //     if(c->get_with()!="none") continue;
            //     ivec2 pos = c->get_pos();
            //     c->move(pos+facing[m]);
            //     if(m<look_around.length()&&look_around[m]) {
            //         list<std::string> around = grid->around(pos,3);
            //         if(grid->is_group(pos)) around << grid->in_group(grid->grid[pos.x()][pos.y()]);
            //         for(auto g : around) {
            //             if(animals.hasKey(g)) {
            //                 auto a = animals.get(g);
            //                 ivec2 a_pos = a->get_pos();
            //                 print(g," is ",(int)(pos-a_pos).length()," units ",grid->relative_dir(pos-a_pos,facing[m])," of ",c_names[c_inits.find(on_c)],"(who is at ",pos.to_string(),") and ",action_to_verb(a->get_action()));
            //             }
            //         }        
            //     }
            // }
            // print("---------------------------");
            // for(auto c : chars.entrySet()) {
            //     if(!c.value->is_dead())
            //         print(c_names[c_inits.find(c.key)]," has ",c.value->get_energy()," energy and ",c.value->get_injury()," injury");
            // }
            // print("---------------------------");
            // print(grid->to_string());
            // print("---------------------------");
            // for(auto a : animals.entrySet()) {
            //     print(a.key," has ",a.value->get_energy()," energy and is ",action_to_verb(a.value->get_action()));
            // }
            // print("---------------------------");
           

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

    // bool string_is_floating(const std::string& s) {
    //     int f = s.find('.');
    //     if(f!=std::string::npos&&f>0&&(s.length()+1)>f+1) {
    //         if(std::isdigit(s.at(f-1))&&std::isdigit(s.at(f+1))) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }
