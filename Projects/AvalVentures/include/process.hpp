#pragma once

#include<core/helper.hpp>
#include<util/files.hpp>
#include<materials.hpp>

namespace Golden {



    // class p_object : public Object {
    //     public:
    //         json& info;
    //         std::string id;
    //         p_object(json& _info,std::string _id) : info(_info), id(_id) {}
    
    //         // void move(ivec2 to) {
    //         //     grid->move(id,to);
    //         //     info["pos"][0] = to.x();
    //         //     info["pos"][1] = to.y();
    //         // }
    
    //         // ivec2 get_pos() {
    //         //     return {info["pos"][0],info["pos"][1]};
    //         // }
    
    //         // std::string get_action() {
    //         //     return info.value("action","nothing");
    //         // }
    //         // void set_action(const std::string& to) {
    //         //     info["action"] = to;
    //         // }
            
    //         // std::string get_with() {
    //         //     return info.value("with","none");
    //         // }
    //         // void set_with(const std::string& to) {
    //         //     info["with"] = to;
    //         // }
    
    //         // std::string get_state() {
    //         //     return info.value("state","none");
    //         // }
    //         // void set_state(const std::string& to) {
    //         //     info["state"] = to;
    //         // }
    
    //         // int get_energy() {
    //         //     return info.value("energy",0);
    //         // }
    //         // void set_energy(int to) {
    //         //     info["energy"] = to;
    //         // }
    //         // void change_energy(int by) {
    //         //     info["energy"] = info.value("energy",0)+by;
    //         // }
    
    //         // float get_injury() {
    //         //     return info.value("injury",0.0f);
    //         // }
    //         // void set_injury(float to) {
    //         //     info["injury"] = to;
    //         // }
    //         // void change_injury(float by) {
    //         //     info["injury"] = info.value("injury",0.0f)+by;
    //         // }
    
    //         // void kill() {set_state("dead");}
    //         // bool is_dead() {return get_state()=="dead";}
    //     };


    //Stamper
        //Heat [gauge]
        //Contents [group]

    //Event: 

    json template_region(list<std::string> region_tags) {
        json r = {
            {"tags",json::array()},
            {"exposure",0},
            {"tick",0},
            {"progress",0}
        };

        for(auto g : region_tags) r["tags"].push_back(g);

        return r;
    }

    json template_object(list<std::string> tags, list<std::string> region_names, list<list<std::string>> region_tags) {
        if(region_names.size()!=region_tags.size()) print("WARNING! region_names and region_tags are diffrent sizes!!");

        json t = {
            {"tags",json::array()},
            {"pos",0},
            {"regions",json::object()}
        };

        for(auto g : tags) t["tags"].push_back(g);

        for(int i=0;i<region_names.size();i++) {
            t["regions"][region_names[i]] = template_region(region_tags[i]);
        }

        return t;
    }

    json template_section() {
        json s = {
            {"size",10},
            {"exposure_modifier",1},
            {"contents",json::object()}
        };
        return s;
    }

  

    #define init 0
    #define flipbook 0
    #define save 1
    int file_num = 1;

    json j;

    void run_process() {
        std::ifstream in_meta(IROOT+"process_meta.json");
        if (in_meta.good()) {
            json meta;
            in_meta >> meta;
            int flogic_num = meta.value("process_num", 1);
            file_num = flogic_num;
        } else {
            file_num = 1;
        }
        in_meta.close();

        std::ifstream file(IROOT+"process"+(file_num<2?"":std::to_string(file_num))+".json");
        file >> j;
        #if init
            j["stamper"] = {
                {"heat",0},
                {"length",2},
                {"tick_rate",0.5f},
                {"sections",json::object()},
                {"contents",json::object()}
            };
            j["melter"] = {
                {"heat",0},
                {"length",10},
                {"tick_rate",3.0f},
                {"contents",json::object()}
            };

            list<std::string> things = {"ingot","bar","rail"};
            for(auto thing : things) {
                j["melter"]["contents"][thing] = { {"id", thing} };
            }

            j["melter"]["contents"]["obj"] =
             template_object(
                    {"soft"},
                    {"branch","trunk","roots"},
                    {
                        {"thin"}, //For branch
                        {"thick"}, //For trunk
                        {"deep","rooted"} //For roots
                    });




            // for(auto i : c_inits) {
            //     ivec2 pos(10,10);
            //     json& jc = j["chars"][i];
            //     chars.put(i,init_fobj(jc,i,pos));
            // }
            // for(int i=0;i<7;i++) {
            //     std::string id = "C";
            //     if(i<9) id.append("0"+std::to_string(i));
            //     else id.append(std::to_string(i));
            //     ivec2 pos(randi(0,s_x-1),randi(0,s_y-1));
            //     json& jc = j["animals"][id];
            //     animals.put(id,init_fobj(jc,id,pos));
            // }
            // j["flipbook"].clear();
        #else

        for(auto& [pkey,part] : j.items()) { //Iterate through the parts of the system
            float part_tick_rate = part.value("tick_rate",1.0f);
            if(part.contains("contents")) {
                for(auto& [ckey,content] : part["contents"].items()) { //Iterate through the contents of each part

                    //Move things


                    if(content.contains("regions")) {
                        for(auto& [key,item] : content["regions"].items()) { //Iterate through the regions of each content
                            //Do exposure calculations here... eventually
                            if(item.contains("tick")) {
                                item["tick"] = item.value("tick",0)+(item.value("exposure",1.0f)); //Tick each region based on exposure
                                if(item["tick"]>=part_tick_rate) {
                                    item["tick"] = 0;
                                    item["progress"] = item.value("progress",0) + 1;
                                }
                            }
                        }
                    }
                }
            }
        }






            if(j["melter"]["heat"]>4) {
                j["melter"]["heat"] = j["melter"].value("heat",0)-3;
                auto& melter_contents = j["melter"]["contents"];
                if(!j["melter"]["contents"].empty()) {
                    json new_stamper_in = json::object();
                    for (auto& [key, item] : melter_contents.items()) {
                        new_stamper_in[key] = item;
                    }
                    auto& stamper_contents = j["stamper"]["contents"];
                    for (auto& [k, v] : new_stamper_in.items()) {
                        stamper_contents["melted_"+k] = std::move(v);
                    }
                    melter_contents.clear();
                } 
            } else  {
                j["melter"]["heat"] = j["melter"].value("heat",0)+1;
            }

            if(j["stamper"]["heat"]<3) {
                j["stamper"]["heat"] = j["stamper"].value("heat",0)+1;
                if(!j["stamper"]["contents"].empty()) {
                    for(auto f : j["stamper"]["contents"]) {
                        f["id"] = "stamped_"+f.value("id","ingot");
                    }
                }
            } else  {
                j["stamper"]["heat"] = j["stamper"].value("heat",0)-2;
            }

            // for(auto& cinfo : j["chars"]) {
            //     g_ptr<f_object> c = make<f_object>(cinfo,cinfo["id"]);
            //     chars.put(c->id,c);
            //     grid->place(c->get_pos(),c->id);
            // }
            // for(auto& ainfo : j["animals"]) {
            //     g_ptr<f_object> a = make<f_object>(ainfo,ainfo["id"]);
            //     animals.put(a->id,a);
            //     grid->place(a->get_pos(),a->id);
            // }
        #endif

        #if flipbook
            //Do nothing
        #else
            //Do nothing
        #endif

        #if save
            file_num += 1;
            if(file_num>3) file_num = 1;
            print("Saving to: ",file_num);
            std::ofstream o_file(IROOT+"process"+(file_num<2?"":std::to_string(file_num))+".json");
            o_file << j.dump(4); 
            o_file.close();

            std::ofstream out_meta(IROOT+"process_meta.json");
            out_meta << json{{"process_num", file_num}};
            out_meta.close();
        #endif
    }

}