#include<core/helper.hpp>
#include "../../Engine/ext/json/json.hpp"
#include<util/files.hpp>
using json = nlohmann::ordered_json;
using namespace Golden;

const std::string IROOT = "../Projects/AvalVentures/assets/images/";

class v_object;
g_ptr<v_object> ROOT = nullptr;
list<json*> decode_path(const std::string& path); 

class v_object : public virtual Object {
private:
     void set_info(const std::string& filename,list<std::string> path = {}) {
          std::ifstream file(IROOT+filename);
          root_filename = filename;
          root = std::make_shared<json>();
          file >> *root;
          info = root.get();
          if(!path.empty()) {
               for(auto s : path) {
                    if(info->contains(s))
                         info = &(*info)[s];
                    else
                         break;
               }
          }
     }
public:
     v_object() {}
     v_object(const std::string& filename) {
          set_info(filename);
     }
     v_object(const std::string& filename,list<std::string> path) {
          set_info(filename,path);
     }
     v_object(std::shared_ptr<json> shared_root, json* view_ptr, const std::string& filename)
     : root(shared_root), info(view_ptr), root_filename(filename) {}

     std::string root_filename;
     std::shared_ptr<json> root;
     json* info = nullptr;

     g_ptr<v_object> get_chunk(const std::string& chunk_name) {
          if(!info->contains(chunk_name)) {
              return nullptr;
          }
          json* chunk_ptr = &(*info)[chunk_name];
          return make<v_object>(root, chunk_ptr, root_filename);
      }
      
      g_ptr<v_object> get_chunk(list<std::string> path) {
          json* current = info;
          for(auto& s : path) {
              if(!current->contains(s))
                  return nullptr;
              current = &(*current)[s];
          }
          
          return make<v_object>(root, current, root_filename);
     }

     json& get_json() { return *info; }

     bool has_in(const std::string& category,const std::string& thing) {
          if(!info->contains(category))
               return false;
          return (*info)[category].contains(thing);
     }

     template<typename T>
     void set_in(const std::string& category,const std::string& thing,T value) {
          if(info->contains(category))
               (*info)[category][thing] = value;
     }

     template<typename T>
     void set_value(const std::string& thing,T value) {
          (*info)[thing] = value;
     }

     json& ref(const std::string& thing) {
         return (*info)[thing];
     }

     void sync_with_file() {
          std::ofstream file(IROOT + root_filename);
          file << root->dump(4); 
          file.close();
     }

     void update() {
          if(info->contains("parts")) {
               for(auto& [part_name, part_data] : (*info)["parts"].items()) {
                    list<json*> path;
                    path << info;
                    update_part(part_name, part_data,path);
               }
          }
     }

     static void update_part(const std::string name, json& part, list<json*> path) {
          list<json*> new_path; new_path << path << &part;
          if(part.contains("events")) {
               auto& events = part["events"];
               list<int> to_remove;
        
               for(int i = 0; i < events.size(); i++) {
                   process_event(events[i], new_path);
                   if(events[i].value("max", 1) == 0 && 
                      !events[i].value("active", false)) {
                       to_remove << i;
                   }
               }
               
               for(int i = to_remove.length() - 1; i >= 0; i--) {
                   events.erase(events.begin() + to_remove[i]);
               }
           }
          if(part.contains("links")) {
               for(auto& link : part["links"]) {
                    process_links(link,new_path);
               }
          }
          if(part.contains("parts")) {
               for(auto& [part_name, part_data] : part["parts"].items()) {
                    update_part(part_name, part_data,new_path);
               }
          }
     }

     static void process_event(json& event, list<json*> path) {
          int max_val = event.value("max", 1);
          print("Processing ",event["name"]);
          
          if(max_val == 0) {
              event["active"] = true;
              event["active_for"] = 0;
          } else {
              event["cur"] = event.value("cur", 0) + 1;
              int cur_val = event["cur"];
              
              if(cur_val >= max_val) {
                  event["cur"] = 0;
                  if(!event.value("active", false)) {
                      event["active"] = true;
                      event["active_for"] = 0;
                  }
              }
          }
          
          if(event.value("active", false)) {
              event["active_for"] = event.value("active_for", 0) + 1;
              
              if(event.contains("on_active")) {
                  for(auto& action : event["on_active"]) {
                      execute_action(action.get<std::string>(), event, path);
                  }
              }
              
              int active_for = event["active_for"];
              int active_dur = event.value("active_dur", 1);
              
              if(active_for >= active_dur) {
                  event["active"] = false;
                  event["active_for"] = 0;
              }
          }
      }

      static void execute_action(const std::string& action, json& event, list<json*>& path) {
          print("Executing action ",action);
          if(action == "distr") {
              //execute_distr(event, path);
          }
          else if(action == "sound") {
              //execute_sound(event, path);
          }
          // ... more actions
      }

     static void process_links(json& link,list<json*> path) {
          if(link["type"] == "cover") {
               print((*path[path.length()-1])["name"]," is ",calculate_accessibility(*path[path.length()-1],*path[path.length()-2])," acessible ");
               std::string t_str = path_to_string(path);
               print("UNCODE: ",t_str);
               print("DECODE: ",path_to_string(decode_path(t_str)));
          }
     }

     // Calculate how accessible a part is
     static float calculate_accessibility(json& part, json& parent) {
          if(!part.contains("links")) {
              return 1.0; 
          }
          float total_coverage = 0.0;
          for(auto& link : part["links"]) {
              if(link["type"] == "cover") {
                  std::string covering_part = link["to"];
                  auto cover = parent;
                  if(parent["name"]!=covering_part) {
                    if(parent.contains("parts")) {
                         if(parent["parts"].contains(covering_part)) {
                              cover = parent["parts"][covering_part];
                         } else print("172 ",parent["name"]," is missing part ",covering_part);
                    } else print("173 ",parent["name"]," has no parts");
                  }
                  float mass_cur = cover.value("mass_cur", 0.0);
                  float mass_max = cover.value("mass_max", 1.0);
                  float coverage = mass_cur / mass_max;
                  total_coverage += coverage;
              }
          }
          return std::max(0.0f, 1.0f - total_coverage);
     }  

     static std::string path_to_string(list<json*> path) {
          std::string result = "";
          for(int i=0;i<path.length();i++) {
               json& j = *path[i];
               result.append(j["name"]);
               if(i+1<path.length()) {
                    json& j_n = *path[i+1];
                    if(j.contains("parts") && j["parts"].contains(j_n["name"])) {
                         result.append(">p>");
                    } else if(j.contains("contents") && j["contents"].contains(j_n["name"])) {
                         result.append(">c>");
                    } 
                    else {
                         result.append(">!>");
                    }
               }
              
          }
          return result;
     }
};
 
list<json*> decode_path(const std::string& path) {
     list<json*> result;
     auto segments = split_str(path, '>');
     json* on = ROOT->info;
     std::string look_in = "";
     
     for(int i = 0; i < segments.length(); i++) {
         std::string s = segments[i];
         
         if(s == "p") {
             look_in = "parts";
         }
         else if(s == "c") {
             look_in = "contents";
         }
         else if(s == "!") {
             look_in = "";
         }
         else {
             if(look_in == "parts") {
                 json* j = &(*on)["parts"][s];
                 on = j;
                 result << j;
                 look_in = "";
             }
             else if(look_in == "contents") {
                 bool found = false;
                 for(auto& item : (*on)["contents"]) {
                     if(item.contains("name") && item["name"] == s) {
                         on = &item;
                         result << on;
                         found = true;
                         break;
                     }
                 }
                 if(!found) {
                     print("ERROR: Could not find", s, "in contents");
                 }
                 look_in = "";
             }
             else {
                 json* j = &(*on)[s];
                 on = j;
                 result << j;
             }
         }
     }
     return result;
 }


int main() {
using namespace helper;

     Window window = Window(1280, 768, "AvalVentures 0.1");
     g_ptr<Scene> scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     //load_gui(scene, "AvalVentures", "gui.fab");
     ROOT = make<v_object>("char1.json");
     auto mira = ROOT->get_chunk("mira");
     auto apple = ROOT->get_chunk("apple");
     auto floor = ROOT->get_chunk("floor");

     // json c = apple->ref("parts")["core"];
     // apple->ref("contents").push_back(c);

     //floor->ref("parts")["0,0"]["contents"].push_back((*apple->info));

     // float flesh_accessible = calculate_accessibility(
     //      apple->ref("parts")["skin"]["parts"]["flesh"],
     //      apple->ref("parts")["skin"]  // parent context
     // );
 
     apple->update();
     mira->update();

     //ROOT->sync_with_file();
     print("Done");

     start::run(window,d,[&]{
     });
     return 0;
}