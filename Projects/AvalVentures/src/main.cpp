#include<core/helper.hpp>
#include<util/files.hpp>
#include<materials.hpp>
using namespace Golden;

const std::string IROOT = "../Projects/AvalVentures/assets/images/";

class v_object;
g_ptr<v_object> ROOT = nullptr;
list<json*> decode_path(const std::string& path); 
json& thing_at(const std::string& path);

static inline float clampf(float x, float a, float b) {
     return x < a ? a : (x > b ? b : x);
 }

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
          list<json*> empty_path;
          if(info->contains("name")) {
              update_part((*info)["name"].get<std::string>(), *info, empty_path);
          } else {
              update_part("root", *info, empty_path);
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

     static void process_links(json& link,list<json*> path) {
          if(link["type"] == "cover") {
               // print((*path[path.length()-1])["name"]," is ",calculate_accessibility(*path[path.length()-1],*path[path.length()-2])," acessible ");
               // std::string t_str = path_to_string(path);
               // print("UNCODE: ",t_str);
               // print("DECODE: ",path_to_string(decode_path(t_str)));
          }
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
               json actions = event["on_active"];
     
               for(auto& action : actions) {
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
          else if(action == "impact") {
               execute_impact(event, path);
          }
          else if(action == "grab") {
               execute_grab(event, path);
          }
      }
      
      static void execute_grab(json& event, list<json*>& path) {
          auto target_path = decode_path(event["target"]);
          auto from_path   = decode_path(event["from"]);
          
          json* target = target_path.empty() ? nullptr : target_path.last();
          json* hand   = from_path.empty() ? nullptr : from_path.last();
          
          if (!target || !hand) {
               print("Grab failed: invalid paths");
               return;
          }
          
          // Find and remove target from its current parent
          // (Need to find parent in target_path)
          if (target_path.length() >= 2) {
               json* parent = target_path[target_path.length() - 2];
               
               // Remove from parent's contents or parts
               if (parent->contains("contents")) {
                    auto& contents = (*parent)["contents"];
                    for (int i = 0; i < contents.size(); i++) {
                         if (contents[i]["name"] == (*target)["name"]) {
                              contents.erase(contents.begin() + i);
                              break;
                         }
                    }
               }
          }
          
          // Add to hand
          if (!hand->contains("contents")) {
               (*hand)["contents"] = json::array();
          }
          (*hand)["contents"].push_back(*target);
          
          print("Grabbed ", (*target)["name"], " with ", (*hand)["name"]);
     }

     static void execute_impact(json& event, list<json*>& path) {
     //     json& d = event["data"];
     //     double area = d["area"];
     //     int force = d["force"];

     //     print("Impacting of area ",area," force ",force," from ",event["name"]," path: ",path_to_string(path));
         resolve_impact(event,path);

     }

     static float calculate_volume_cm3(json& geometry) {
          auto& base = geometry["base"];
          float w = base["dims"][0].get<float>() / 10.0;  // mm to cm
          float h = base["dims"][1].get<float>() / 10.0;
          float d = base["dims"][2].get<float>() / 10.0;
          
          float rx = base["round"][0].get<float>();
          float ry = base["round"][1].get<float>();
          float rz = base["round"][2].get<float>();
          
          float base_volume = 0;
          
          if(rx >= 0.9 && ry >= 0.9 && rz >= 0.9) {
              // Sphere: V = 4/3 π r³
              float r = (w + h + d) / 6.0;
              base_volume = (4.0/3.0) * 3.14159 * r * r * r;
          } 
          else if(rx >= 0.9 && ry >= 0.9 && rz < 0.5) {
              // Cylinder: V = π r² h
              float r = (w + h) / 4.0;
              base_volume = 3.14159 * r * r * d;
          }
          else {
              // Ellipsoid: V = 4/3 π abc
              float a = w / 2.0;
              float b = h / 2.0;
              float c = d / 2.0;
              base_volume = (4.0/3.0) * 3.14159 * a * b * c;
          }
          
          // Subtract diffs (CSG subtraction)
          if(geometry.contains("diffs")) {
              for(auto& diff : geometry["diffs"]) {
                  json diff_geom = {{"base", diff}, {"diffs", json::array()}};
                  base_volume -= calculate_volume_cm3(diff_geom);
              }
          }
          
          return std::max(0.0f, base_volume);
      }
      
      static float calculate_masss_g(json& part) {
          if(!part.contains("geometry") || !part.contains("mat")) {
              return 0.0;
          }
          
          // Base volume from geometry
          float base_volume_cm3 = calculate_volume_cm3(part["geometry"]);
          float density = part["mat"]["density"].get<float>();
          
          return std::max(0.0f, base_volume_cm3 * density);
      }

      // Calculate shell thickness by comparing base and first diff
     static float calculate_thickness_mm(json& geometry) {
          if(!geometry.contains("base")) {
          return 1.0;  // Default fallback
          }
          
          auto& base = geometry["base"];
          
          // If no diffs, it's solid - use smallest dimension as "thickness"
          if(!geometry.contains("diffs") || geometry["diffs"].empty()) {
          float w = base["dims"][0].get<float>();
          float h = base["dims"][1].get<float>();
          float d = base["dims"][2].get<float>();
          return std::min({w, h, d});
          }
          
          // Shell thickness = difference between base and first diff
          auto& first_diff = geometry["diffs"][0];
          
          // Compare dimensions
          float base_w = base["dims"][0].get<float>();
          float base_h = base["dims"][1].get<float>();
          float base_d = base["dims"][2].get<float>();
          
          float diff_w = first_diff["dims"][0].get<float>();
          float diff_h = first_diff["dims"][1].get<float>();
          float diff_d = first_diff["dims"][2].get<float>();
          
          // Thickness is half the difference (symmetric shell)
          float thickness_w = (base_w - diff_w) / 2.0;
          float thickness_h = (base_h - diff_h) / 2.0;
          float thickness_d = (base_d - diff_d) / 2.0;
          
          // Return average thickness (or could be direction-specific later)
          float avg_thickness = (thickness_w + thickness_h + thickness_d) / 3.0;
          
          return std::max(0.1f, avg_thickness);  // Minimum 0.1mm
     }
      
     static float calculate_surface_area_mm2(json& geometry) {
          auto& base = geometry["base"];
          float w = base["dims"][0].get<float>();
          float h = base["dims"][1].get<float>();
          float d = base["dims"][2].get<float>();
          
          float rx = base["round"][0].get<float>();
          float ry = base["round"][1].get<float>();
          float rz = base["round"][2].get<float>();
          
          float surface_area = 0;
          
          if(rx >= 0.9 && ry >= 0.9 && rz >= 0.9) {
              // Sphere: A = 4πr²
              float r = (w + h + d) / 6.0;
              surface_area = 4.0 * M_PI * r * r;
          }
          else if(rx >= 0.9 && ry >= 0.9 && rz < 0.5) {
              // Cylinder: A = 2πr² + 2πrh
              float r = (w + h) / 4.0;
              surface_area = 2.0 * M_PI * r * r + 2.0 * M_PI * r * d;
          }
          else {
              // Box approximation: A = 2(wh + wd + hd)
              surface_area = 2.0 * (w*h + w*d + h*d);
          }
          
          return surface_area;
      }

      static float calculate_opening_area_mm2(json& geometry) {
          float w = geometry["dims"][0].get<float>();
          float h = geometry["dims"][1].get<float>();
          
          float rx = geometry["round"][0].get<float>();
          float ry = geometry["round"][1].get<float>();
          
          if(rx >= 0.8 && ry >= 0.8) {
              // Circular opening
              float r = (w + h) / 4.0;
              return 3.14159 * r * r;
          } else {
              // Rectangular opening
              return w * h;
          }
      }

      struct Opening {
          vec3 position;     // Center of opening
          vec3 dimensions;   // Size of opening
          float area_mm2;    // Surface area exposed
      };

      // Check if diff reaches outer surface
      static bool breaches_surface(json& base, json& diff, float shell_thickness) {
          vec3 base_dims = {
              base["dims"][0].get<float>(),
              base["dims"][1].get<float>(),
              base["dims"][2].get<float>()
          };
          
          vec3 diff_pos = {
              diff.value("pos", json::array({0,0,0}))[0].get<float>(),
              diff.value("pos", json::array({0,0,0}))[1].get<float>(),
              diff.value("pos", json::array({0,0,0}))[2].get<float>()
          };
          
          vec3 diff_dims = {
              diff["dims"][0].get<float>(),
              diff["dims"][1].get<float>(),
              diff["dims"][2].get<float>()
          };
          
          // Calculate distance from center to edge of diff
          float dist_from_center = sqrt(
              diff_pos.x() * diff_pos.x() + 
              diff_pos.y() * diff_pos.y() + 
              diff_pos.z() * diff_pos.z()
          );
          
          float diff_extent = (diff_dims.x() + diff_dims.y() + diff_dims.z()) / 6.0;
          float base_radius = (base_dims.x() + base_dims.y() + base_dims.z()) / 6.0;
          float inner_radius = base_radius - shell_thickness;
          
          // Diff breaches surface if it extends beyond inner radius
          return (dist_from_center + diff_extent) > inner_radius;
      }
      
      // Find all diffs that breach the surface = openings
      static list<Opening> calculate_openings(json& part) {
          list<Opening> openings;
          
          if(!part.contains("geometry")) return openings;
          
          auto& geom = part["geometry"];
          if(!geom.contains("base") || !geom.contains("diffs")) return openings;
          
          auto& base = geom["base"];
          float shell_thickness = calculate_thickness_mm(geom);
          
          // Check each diff (after the first, which is the hollow interior)
          int diff_count = geom["diffs"].size();
          int start_idx = (diff_count > 1) ? 1 : 0;  // Skip first diff if it's the interior
          
          for(int i = start_idx; i < diff_count; i++) {
              auto& diff = geom["diffs"][i];
              
              // Check if this diff penetrates the outer surface
              if(breaches_surface(base, diff, shell_thickness)) {
                  Opening opening;
                  
                  opening.position = {
                      diff.value("pos", json::array({0,0,0}))[0].get<float>(),
                      diff.value("pos", json::array({0,0,0}))[1].get<float>(),
                      diff.value("pos", json::array({0,0,0}))[2].get<float>()
                  };
                  
                  opening.dimensions = {
                      diff["dims"][0].get<float>(),
                      diff["dims"][1].get<float>(),
                      diff["dims"][2].get<float>()
                  };
                  
                  opening.area_mm2 = calculate_opening_area_mm2(diff);
                  
                  openings << opening;
              }
          }
          
          return openings;
      }

      // Nearest point ON the surface of an AABB (works for inside & outside).
      static vec3 nearest_surface_point(json* target, const vec3& origin) {
          // center and half-extents
          const vec3 center((*target)["pos"][0].get<float>(),
                            (*target)["pos"][1].get<float>(),
                            (*target)["pos"][2].get<float>());
      
          const json& base = (*target)["geometry"]["base"];
          const vec3 half(base["dims"][0].get<float>() * 0.5f,
                          base["dims"][1].get<float>() * 0.5f,
                          base["dims"][2].get<float>() * 0.5f);
      
          // point in target-local space
          vec3 p = origin - center;
      
          // distance outside along each axis (<=0 means inside along that axis)
          const float dx = std::fabs(p.x()) - half.x();
          const float dy = std::fabs(p.y()) - half.y();
          const float dz = std::fabs(p.z()) - half.z();
      
          // Case 1: point is OUTSIDE the box on at least one axis → clamp to boundary.
          if (dx > 0.f || dy > 0.f || dz > 0.f) {
              vec3 q(clampf(p.x(), -half.x(), half.x()),
                     clampf(p.y(), -half.y(), half.y()),
                     clampf(p.z(), -half.z(), half.z()));
              return center + q;
          }
      
          // Case 2: point is INSIDE → push to nearest face (minimum distance to a face).
          const float sx = half.x() - std::fabs(p.x()); // distance to ±X face
          const float sy = half.y() - std::fabs(p.y()); // distance to ±Y face
          const float sz = half.z() - std::fabs(p.z()); // distance to ±Z face
      
          vec3 q = p; // start from the inside point
      
          if (sx <= sy && sx <= sz) {
              q.setX((p.x() >= 0.f ? half.x() : -half.x()));
          } else if (sy <= sx && sy <= sz) {
              q.setY((p.y() >= 0.f ? half.y() : -half.y()));
          } else {
              q.setZ((p.z() >= 0.f ? half.z() : -half.z()));
          }
      
          return center + q;
      }

      static vec3 derive_impact_velocity(const json& event, const vec3& surface_point,const vec3& origin) {
          vec3 velocity(0, 0, 0);
          if (event["data"].contains("velocity")) {
              velocity = vec3(event["data"]["velocity"][0].get<float>(),
                              event["data"]["velocity"][1].get<float>(),
                              event["data"]["velocity"][2].get<float>());
          }
      
          // If velocity magnitude is missing, derive from direction to surface
          if (velocity.length() <= 0) {
              velocity = (surface_point - origin);
          }
      
          return velocity;
      }

      // Unit normal at hit_point on target's surface (AABB with optional fillet radii in mm).
     static vec3 estimate_surface_normal(json& target, const vec3& hit_point) {
          const auto& base = target["geometry"]["base"];
          const vec3 center(base.contains("pos") ? vec3(base["pos"][0], base["pos"][1], base["pos"][2])
                                             : vec3(target["pos"][0], target["pos"][1], target["pos"][2]));
     
          const vec3 half(base["dims"][0].get<float>() * 0.5f,
                         base["dims"][1].get<float>() * 0.5f,
                         base["dims"][2].get<float>() * 0.5f);
     
          // Use a single effective fillet radius for blending (min of per-axis), clamped.
          float rx = base["round"][0].get<float>();
          float ry = base["round"][1].get<float>();
          float rz = base["round"][2].get<float>();
          float r  = std::max(0.0f, std::min(rx, std::min(ry, rz)));
     
          // Core box after removing the fillet band.
          vec3 core(std::max(0.0f, half.x() - r),
                    std::max(0.0f, half.y() - r),
                    std::max(0.0f, half.z() - r));
     
          // Local point
          vec3 p = hit_point - center;
     
          // q = how far beyond the core box along each axis (<=0 means inside the core slab on that axis)
          vec3 ap(std::fabs(p.x()), std::fabs(p.y()), std::fabs(p.z()));
          vec3 q(ap.x() - core.x(), ap.y() - core.y(), ap.z() - core.z());
     
          // If outside the core (in the fillet/edge region), normal points from nearest core point to p.
          vec3 d(std::max(q.x(), 0.0f), std::max(q.y(), 0.0f), std::max(q.z(), 0.0f));
          if (d.x() > 0.0f || d.y() > 0.0f || d.z() > 0.0f) {
          vec3 n(d.x() * (p.x() >= 0.0f ? 1.0f : -1.0f),
                    d.y() * (p.y() >= 0.0f ? 1.0f : -1.0f),
                    d.z() * (p.z() >= 0.0f ? 1.0f : -1.0f));
          float L = n.length();
          if (L > 1e-6f) return (n / L);
          }
     
          // Otherwise we're on a face of the core box: pick the nearest face (largest q, i.e., least negative).
          float ax = q.x(), ay = q.y(), az = q.z(); // all <= 0 here
          if (ax >= ay && ax >= az) return vec3(p.x() >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
          if (ay >= ax && ay >= az) return vec3(0.0f, p.y() >= 0.0f ? 1.0f : -1.0f, 0.0f);
          return vec3(0.0f, 0.0f, p.z() >= 0.0f ? 1.0f : -1.0f);
     }
     
     // --- helpers ---
     static inline vec3 world_to_local(const json& target, const vec3& p_world) {
          return vec3(
          p_world.x() - target["pos"][0].get<float>(),
          p_world.y() - target["pos"][1].get<float>(),
          p_world.z() - target["pos"][2].get<float>());
     }
     static inline int argmax3(float ax, float ay, float az) {
          return (ax >= ay && ax >= az) ? 0 : (ay >= ax && ay >= az) ? 1 : 2;
     }
     static constexpr float kEpsDepth = 0.05f; // mm; don't create "zero" holes

     static void resolve_impact(json& event, list<json*>& /*path*/) {
          // Decode objects
          auto target_path = decode_path(event["target"]);
          auto from_path   = decode_path(event["from"]);
          json* from   = target_path.empty() ? nullptr : from_path.last();
          json* target = target_path.empty() ? nullptr : target_path.last();
          if (!target || !from) { print("Impact: bad paths"); return; }
      
          // Positions & velocity
          vec3 origin_pos(event["data"]["pos"][0], event["data"]["pos"][1], event["data"]["pos"][2]);
          vec3 hit_point = nearest_surface_point(target, origin_pos);                   // world
          vec3 velocity  = derive_impact_velocity(event, hit_point, origin_pos);        // mm per action (dt)
      
          // Mass → kg
          float mass_g = event["data"].value("mass", -1.0f);
          if (mass_g <= 0.f) mass_g = calculate_masss_g(thing_at(event["from"]));
          print("Mass calculated at ", mass_g, "g");
          const float m_kg = mass_g * 0.001f;
      
          // Time step (defaults to 2 s per action)
          const float dt = event["data"].value("dt", 2.0f);
      
          // Velocity → m/s (velocity is mm per action)
          const float v_mm_per_action = velocity.length();
          const float v_mps = (v_mm_per_action * 0.001f);  // mm to m (velocity is already per-action)
          const float momentum_kg_m_s = m_kg * v_mps;
          float force_n = momentum_kg_m_s / dt;
      
          print("Calculated momentum force: ", force_n, "n");
          force_n += event["data"].value("force", 0.0f); // add user-specified force if any
          print("Final force: ", force_n, "n");
      
          // Surface/pressure gating uses the NORMAL component only
          float impact_area_mm2 = event["data"].value("profile", 0.0f);
          if (impact_area_mm2 <= 0.f) { print("Impact: no area/profile"); return; }
      
          vec3 n = estimate_surface_normal(*target, hit_point); // unit
          float v_len = v_mm_per_action;
          float v_n   = (v_len > 1e-6f) ? std::max(0.0f, -velocity.dot(n)) : 0.0f; // mm/action toward surface
          float F_n   = (v_len > 1e-6f && v_n > 1e-6f) ? force_n * (v_n / v_len) : force_n;
          float P_mpa = F_n / impact_area_mm2; // N/mm^2 == MPa
      
          if (!target->contains("mat")) { print("Impact: target has no material"); return; }
          json& mat = (*target)["mat"];
          float yield_mpa = mat.value("yield", 0.0f);
          float break_mpa = mat.value("break", std::max(1.0f, yield_mpa + 1e-3f));
          float thickness_mm = calculate_thickness_mm((*target)["geometry"]);
      
          print("Before impact, ", (*target)["name"], " had ", calculate_openings(*target).length(), " openings");
      
          // Route by normal pressure
          if (P_mpa > break_mpa) {
              // SHARP/penetrative
              apply_penetration(target, from, target_path, from_path, event,
                                F_n, impact_area_mm2, /*excess*/ 0.0f,
                                thickness_mm, hit_point, velocity);
          } else if (P_mpa > yield_mpa) {
              // Plastic deformation only (no hole)
              float deformation_ratio = (P_mpa - yield_mpa) / yield_mpa;
              apply_deformation(target, deformation_ratio);
          } else {
              print("Impact resisted");
          }
      
          print("After impact, ", (*target)["name"], " has ", calculate_openings(*target).length(), " openings");
      }
      static void apply_penetration(
          json* target, json* from, list<json*>& target_path, list<json*>& from_path, json& event,
          float force_n_normal, float area_mm2, float /*excess_mpa*/,
          float thickness_mm, const vec3& hit_point_world, const vec3& velocity_mm_per_action)
      {
          if (!target || area_mm2 <= 0.f || thickness_mm <= 0.f) return;
      
          json& mat = (*target)["mat"];
          const float yield_mpa = mat.value("yield", 0.0f);
          const float break_mpa = std::max(mat.value("break", yield_mpa + 1e-3f), yield_mpa + 1e-3f);
      
          // Normal & tangential components
          vec3 n = estimate_surface_normal(*target, hit_point_world); // unit (world==local if axis aligned)
          const float v_len = velocity_mm_per_action.length();
          const float v_n   = (v_len > 1e-6f) ? std::max(0.0f, -velocity_mm_per_action.dot(n)) : 0.0f;
          vec3  v_t_v       = velocity_mm_per_action - n * velocity_mm_per_action.dot(n);
      
          // ---- FORCE BUDGET APPROACH ----
      
          // Step 1: Check if we can even start penetrating (yield gate)
          const float P_mpa = force_n_normal / area_mm2;
          if (P_mpa <= yield_mpa) {
              print("Impact resisted - pressure ", P_mpa, " MPa below yield ", yield_mpa, " MPa");
              return;
          }
      
         // Step 2: Force above YIELD drives penetration
          const float F_yield = yield_mpa * area_mm2; 
          const float fracture_force_available = std::max(0.0f, force_n_normal - F_yield);
          print("Fracture force available: ", fracture_force_available, " N"); 

          // Step 3: But resistance per mm is based on BREAK strength
          const float resistance_per_mm = break_mpa * area_mm2;   
          const float toughness = mat.value("toughness", 1.0f);
          const float effective_resistance = resistance_per_mm * toughness;
          print("Resistance: ", effective_resistance, " N/mm");
      
          // Step 4: Depth from force budget
          float depth_from_force = (effective_resistance > 1e-6f)
                                 ? (fracture_force_available / effective_resistance)
                                 : 0.0f;
          print("Depth from force budget: ", depth_from_force, " mm");
      
          // Step 5: Kinematic cap (can’t advance faster than motion)
          const float depth_kinematic = 0.5f * v_n; // conservative: ~v_n*dt/2 with dt baked in
      
          // Step 6: Geometric cap (impactor reach)
          float depth_geometric = thickness_mm;     // default: full layer
          if (from && (*from).contains("geometry")) {
              auto& from_base = (*from)["geometry"]["base"];
              float impactor_length = std::max({
                  from_base["dims"][0].get<float>(),
                  from_base["dims"][1].get<float>(),
                  from_base["dims"][2].get<float>()
              });
              depth_geometric = std::min(impactor_length, thickness_mm);
          }
      
          // Step 7: Take the minimum of all constraints
          float penetration_depth = std::min({
              depth_from_force,
              depth_kinematic,
              depth_geometric,
              thickness_mm
          });
          print("Depth caps - kinematic: ", depth_kinematic,
                ", geometric: ", depth_geometric,
                ", thickness: ", thickness_mm);
          print("Final penetration depth: ", penetration_depth, " mm");
      
          // Step 8: Fraction through this layer
          const float frac_actual = (thickness_mm > 1e-6f) ? (penetration_depth / thickness_mm) : 0.0f;
          const bool  full = (frac_actual >= 0.95f);
      
          // ---- Opening footprint ----
          float opening_w = std::sqrt(area_mm2);
          float opening_h = opening_w;
          float elong     = std::min(opening_w * 0.5f, 0.1f * v_t_v.length()); // mm
          opening_w      += elong;
      
          // ---- Local placement & axis selection ----
          vec3 hit_local = world_to_local(*target, hit_point_world);
          vec3 n_local   = n; // if you add rotations, rotate n into local here
          vec3 diff_center_local = hit_local - n_local * (penetration_depth * 0.5f);
      
          int axis = argmax3(std::fabs(n_local.x()), std::fabs(n_local.y()), std::fabs(n_local.z()));
          float dx = opening_w, dy = opening_h, dz = penetration_depth;
          if (axis == 0) { dx = penetration_depth; dy = opening_w;  dz = opening_h; }
          if (axis == 1) { dy = penetration_depth; dx = opening_w;  dz = opening_h; }
          if (axis == 2) { dz = penetration_depth; dx = opening_w;  dy = opening_h; }
      
          print("Penetration: ", penetration_depth, " mm (", 100.f * frac_actual,
                "%) at local [", diff_center_local.x(), ", ", diff_center_local.y(), ", ", diff_center_local.z(), "]");
      
          // ---- Force spending based on ACTUAL depth achieved ----
          const float force_spent = penetration_depth * effective_resistance; // N
          float remaining_force   = std::max(0.0f, force_n_normal - force_spent);
          print("Force spent: ", force_spent, " N, remaining: ", remaining_force, " N");
      
          // ---- Create geometry; fragment only if (near) full through ----
          if (penetration_depth > kEpsDepth) {
              if (!target->contains("geometry")) { print("ERROR: Target has no geometry"); return; }
              if (!(*target)["geometry"].contains("diffs")) { (*target)["geometry"]["diffs"] = json::array(); }
      
              json damage_diff = {
                  {"dims",  json::array({dx, dy, dz})},
                  {"round", json::array({0.8f, 0.8f, 0.5f})},
                  {"pos",   json::array({diff_center_local.x(), diff_center_local.y(), diff_center_local.z()})}
              };
              (*target)["geometry"]["diffs"].push_back(damage_diff);
      
              if (full) {
                  json fragment = create_fragment(*target, damage_diff);
                  if (!from->contains("contents")) { (*from)["contents"] = json::array(); }
                  (*from)["contents"].push_back(fragment);
                  print("Fragment (", fragment["name"], ") attached to ", (*from)["name"]);
              } else {
                  print("Partial cut: no fragment (skin still attached).");
              }
          }
      
          // ---- Propagate only if fully penetrated AND force remains ----
          if (full && remaining_force > 10.f &&
              target->contains("covers") && !(*target)["covers"].empty()) {
      
              std::string next_layer = (*target)["covers"][0].get<std::string>();
              print("FULLY PENETRATED through ", mat["name"]);
              print("Propagating to ", next_layer, " with ", remaining_force, " N");
      
              std::string new_path = path_to_string(target_path) + ">p>" + next_layer;
              event["target"]        = new_path;
              event["data"]["force"] = remaining_force; // pass remaining
              resolve_impact(event, target_path);
          } else if (!full) {
              print("Stopped inside ", mat["name"], " at ", 100.f * frac_actual, "% depth");
          }
      }
          
      static void apply_fracture(json* target, float force_n, float area_mm2, 
                                float excess_mpa, json* event = nullptr) {
          json& mat = (*target)["mat"];
          float density = mat["density"].get<float>();
          float thickness_mm = calculate_thickness_mm((*target)["geometry"]);
          float brittleness = mat.value("brittle", 0.5);
          
          // More excess pressure = more fragments
          int fragment_count = 1 + (int)(brittleness * (excess_mpa / 10.0));
          fragment_count = std::min(fragment_count, 10);  // Cap at 10 fragments
          
          print("Fracturing into ", fragment_count, " fragments");
          
          // Create opening
          float opening_size = sqrt(area_mm2);
          
          json opening = {
              {"pos", json::array({0, 0, 0})},
              {"geometry", {
                  {"dims", json::array({opening_size, opening_size, thickness_mm})},
                  {"round", json::array({0.8, 0.8, 0.5})}
              }},
              {"contents", json::array()}
          };
          
          if(!target->contains("openings")) {
              print("451 ", (*target)["name"], " is missing an openings field");
              (*target)["openings"] = json::array();
          }
          
          // Create multiple fragments
          json base_fragment = create_fragment(*target, opening);
          float total_mass = base_fragment["mass"].get<float>();
          
          for(int i = 0; i < fragment_count; i++) {
              json fragment = base_fragment;
              
              // Divide mass among fragments (with variation)
              float mass_fraction = (1.0 / fragment_count) * (0.8 + (rand() % 40) / 100.0);
              fragment["mass"] = total_mass * mass_fraction;
              
              // Scale geometry proportionally
              float scale = std::pow(mass_fraction, 1.0/3.0);  // Cube root for volume
              fragment["geometry"]["dims"][0] = 
                  fragment["geometry"]["dims"][0].get<float>() * scale;
              fragment["geometry"]["dims"][1] = 
                  fragment["geometry"]["dims"][1].get<float>() * scale;
              
              // Attach to impactor or opening
              if(event && event->contains("from")) {
                  json& impactor = thing_at((*event)["from"]);
                  if(!impactor.contains("contents")) {
                      impactor["contents"] = json::array();
                  }
                  impactor["contents"].push_back(fragment);
              } else {
                  opening["contents"].push_back(fragment);
              }
          }
          
          (*target)["openings"].push_back(opening);
          
          float opening_area = calculate_opening_area_mm2(opening["geometry"]);
          print("Created opening: ", opening_area, "mm² with ", fragment_count, " fragments");
      }

      static void apply_deformation(json* target, float deformation_ratio) {
          print("TO DO: add defformation");
          // json& mat = (*target)["mat"];
          
          // // Check elasticity
          // float elasticity = mat.value("elastic", 0.0);
          // float permanent_deformation = deformation_ratio * (1.0 - elasticity);
          
          // if(!target->contains("deformation")) {
          //      (*target)["deformation"] = 0.0;
          // }
          
          // float current_deform = (*target)["deformation"].get<float>();
          // (*target)["deformation"] = current_deform + permanent_deformation;
          
          // print("Deformed", mat["name"], "by", permanent_deformation * 100, "%");
          
          // if((*target)["deformation"].get<float>() > 0.5) {
          //      (*target)["impaired"] = true;
          //      print(mat["name"], "is severely deformed");
          // }
     }

     static json create_fragment(json& source_part, json& opening) {
          // Copy opening geometry - this is the shape of what was removed
          
          // Get source properties
          json source_material = source_part["mat"];
          
          // Calculate fragment mass
          json frag_geom_wrapper = {
              {"base", opening},
              {"diffs", json::array()}
          };

          std::string name = source_part["name"].get<std::string>();
          if(name.find("_fragment")==std::string::npos) {
               name.append("_fragment");
          }
          // Create fragment
          json fragment = {
              {"name", name },
              {"geometry", frag_geom_wrapper},
              {"mat", source_material},
          };
          
          return fragment;
      }
      
      static void check_impactor_damage(json& event, float reflected_pressure_mpa) {
          json& impactor_mat = event["data"]["impactor_mat"];
          float impactor_yield = impactor_mat["yield"];
          
          if(reflected_pressure_mpa > impactor_yield) {
              print("Impactor damaged by !?!");
              
              // Queue damage event to impactor
              std::string origin = event["data"]["origin_path"];
              // ... send damage back to origin
          }
      }

};

json make_event(const std::string& action,const std::string& target,const std::string& from, json data = json::object()) {
     json& origin =  thing_at(from);
     if(origin.contains("pos"))
          data["pos"] = origin["pos"];
     if(origin.contains("geometry")) {
          auto base = origin["geometry"]["base"];
          int use = 0;
    if (data.contains("use_profile"))
     int use = data.value("use_profile", 1); // 0 = sharpest, 1 = middle, 2 = bluntest (default middle)
        if (base.contains("profiles") && base["profiles"].is_array()) {
            std::vector<float> vals;
            for (auto& p : base["profiles"])
                vals.push_back(p.get<float>());
            if (vals.size() == 3) {
                std::vector<float> sorted = vals;
                std::sort(sorted.begin(), sorted.end());
    
                float chosen = 0.f;
                switch (use) {
                    case 0: chosen = sorted[0]; break; // smallest area = sharpest
                    case 1: chosen = sorted[1]; break; // middle area
                    case 2: chosen = sorted[2]; break; // largest area = bluntest
                    default: chosen = sorted[1]; break;
                }
    
                data["profile"] = chosen;
            } else {
                print("WARNING: expected 3 profiles (x,y,z), got ", vals.size());
            }
        } else {
            print("WARNING: no profiles array in geometry");
        }
     }
     return {
         {"name", action+" event"},
         {"cur", 0},
         {"max", 0},
         {"active", true},
         {"active_for", 0},
         {"active_dur", 1},
         {"on_active", json::array({action})},
         {"from", from},
         {"target", target},
         {"data",data}
     };
 }
 
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

json& thing_at(const std::string& path){
    return (*decode_path(path).last());
}

 void add_event(const std::string& action,const std::string& target,const std::string& from,json data = json::object()) {
     thing_at(target)["events"].push_back(make_event(action,target,from,data));
 }

class m_table : public Object {
     public:
     int width = 0;
     list<std::string> headers;
     list<std::string> contents;

     m_table() {}
     m_table(const std::string& table) {
          list<std::string> sections = split_str(table,'|');
          for (auto& s : sections) {
               while (!s.empty() && s.front() == ' ')
                   s.erase(s.begin());
               while (!s.empty() && s.back() == ' ')
                   s.pop_back();
           }
          int spacer_start = -1;
          for(int i = 0;i<sections.length();i++) {
               if(sections[i]=="-----") {
                    width++;
                    if(spacer_start==-1) 
                         spacer_start = i;
               } else if (spacer_start!=-1) {
                    for(int w = 0; w<width;w++) {
                         sections.removeAt(spacer_start);
                    }
                    break;
               }
          }
          for(int i = 0; i<sections.length();i++) {
               if(i<width) {
                    headers << sections[i];
               } else {
                    contents << sections[i];
               }
          }
     }
     m_table(int _width) {
          width = _width;
     }

     std::string to_string() {
          std::string result = "";
          for(auto h : headers) result.append(h+"|");
          for(int i=0;i<contents.length();i++) {
               if(i%width==0) result.append("\n");
               result.append(contents[i]+"|");
          }
          return result;
     }

     std::string to_markdown() {
          std::string result = "";
          for(auto h : headers) result.append(" "+h+" |");
          result.append("\n");
          for(int w = 0;w<width;w++) result.append(" ------ |");
          for(int i=0;i<contents.length();i++) {
               if(i%width==0) result.append("\n");
               result.append(" "+contents[i]+" |");
          }
          return result;
     }

     std::string to_terminal() {
          std::string result = "\"";
          for(auto h : headers) result.append(" "+h+" |");
          result.append("\"\n\"");
          for(int w = 0;w<width;w++) result.append(" ------ |");
          for(int i=0;i<contents.length();i++) {
               if(i%width==0) result.append("\"\n\"");
               result.append(" "+contents[i]+" |");
          }
          result.append("\"");
          return result;
     }

     void print_out() {
          print("headers: "); 
          for(auto h : headers) {
               print("   "+h);
          }
          print("Contents: "); 
          for(int i=0;i<contents.length();i++) {
               if(i%width==0) print("BREAK");
               print("  "+headers[i%width]+": "+contents[i]);
          }
     }
     
     list<std::string> column_contents(int column) {
          list<std::string> result;
          for(int i=0;i<contents.length();i++) { 
               if(i%width==column) result << contents[i];
          }
          return result;
     }

     list<std::string> row_contents(int row) {
          list<std::string> result;
          int on_row = -1;
          for(int i=0;i<contents.length();i++) { 
               if(i%width==0) on_row++;
               if(on_row==row) {
                    result << contents[i];
               } else if (on_row > row) break;
          }
          return result;
     }

     size_t find_and_infer(const std::string& section,const std::string& thing,int infer,int depth = 0) {
          int at = section.find(thing);
          if(at!=std::string::npos) {
               return at;
          } else {
               std::string s = section;
               std::string t = thing;
               switch(infer) {
                    case 1: 
                    {
                         std::transform(s.begin(), s.end(), s.begin(),[](unsigned char c){ return std::tolower(c);});
                         std::transform(t.begin(), t.end(), t.begin(),[](unsigned char c){ return std::tolower(c);});
                         return find_and_infer(s,t,2);
                    }
                    break;

                    case 2:
                    {
                         if(depth==0) return find_and_infer(s,split_str(t,'/')[0],2,1);
                         else if(depth==1) return find_and_infer(s,split_str(t,'(')[0],3,0);
                    }
                    break;

                    default: //Also 0 or No inference
                         return std::string::npos;
               }
          }
          return std::string::npos;
     }
     int column_from_string(const std::string& in_column,int infer = 1) {
          for(int i=0;i<width;i++) {
               if(find_and_infer(headers[i],in_column,infer)!=std::string::npos) {
                    return i;
               }
          }
          return -1;
     }
     int row_from_string(const std::string& in_row,int infer = 1) {
          for(int i=0;i<contents.length()/width;i++) {
               if(find_and_infer(contents[i*width],in_row,infer)!=std::string::npos) {
                    return i;
               }
          }
          return -1;
     }
     //Inference level controls how much auto-correct the procceser will add
     //0 = None at all, read as written
     //
     void process_command(const std::string& command,int infer = 1) {
          list<std::string> sections = split_str(command,'|');
          for(auto s : sections) {
               list<std::string> parts = split_str(s,';');
               if(parts.length()<3) {
                    print("Malformed command section: ",s);
               } else {
                    execute_command(parts[0],parts[1],parts[2],parts.length()>3?parts[3]:"");
               }
          }
     }     
     void execute_command(const std::string& in_column, const std::string& in_row,const std::string& thing, 
                          std::string arg,int infer = 1) {
          int column_num = column_from_string(in_column,infer);
          int row_num = row_from_string(in_row,infer);
          
          if(column_num == -1) {
               print("No such column found: ",in_column);
               return;
          }
          if (row_num == -1) {
               print("No such row found: ",in_row);
               return;
          }
          execute_command(column_num,row_num,thing,arg);
     }
     const std::string command_args[5] = {"ADD_IN","REMOVE_IN","MOVE_TO","INC","NEW_DESC"};
     bool is_command_arg(const std::string& arg) {
          for(int i = 0;i<4;i++) {
               if(arg==command_args[i]) return true;
          }
          return false;
     }
     void execute_command(int in_column, int in_row,const std::string& thing, std::string arg, int infer = 1) { 
          int id = (in_row*width)+in_column;
          auto sub_arg = split_str(arg,':');
          std::string s = contents[id];
          if(sub_arg.length()==0) {
               contents[id].append(thing);
          } else {
               arg = sub_arg[0];
               std::string loc = "";
               if(sub_arg.length()>1) loc = sub_arg[1];
               if(arg=="ADD_IN") {
                    int at = find_and_infer(s,loc,infer);
                    if(at==std::string::npos) {
                         print("Unable to find arg loc: ",loc," In:\n",s);
                    } else
                         contents[id].insert(at+loc.length(),thing);
               }
               else if (arg=="REMOVE_IN") {
                    int at = find_and_infer(s,thing,infer);
                    if(at==std::string::npos) {
                         print("Unable to find thing to remove: ",thing," In:\n",s);
                    } else {
                         int next = s.find('*',at);
                         contents[id].erase(at,at-next);
                    }
               } else if (arg=="MOVE_TO") {
                    int at = find_and_infer(s,thing,infer);
                    std::string full_thing = "";
                    if(at==std::string::npos) {
                         print("Unable to find thing for move: ",thing," In:\n",s);
                    } else {
                         int next = s.find('*',at);
                         full_thing = s.substr(at,at-next);
                    }
                    std::string new_cmd = "";
                    for(int a = 1;a<sub_arg.length();a++) {
                         if(a==3) {
                              new_cmd.append(full_thing+";"+"ADD_IN:");
                         }
                         new_cmd.append(sub_arg[a]);
                         if(a<sub_arg.length()-1) {
                              new_cmd.append(";");
                         } 
                    }
                    execute_command(in_column,in_row,thing,"REMOVE_IN:");
                    process_command(new_cmd);
               } else if(arg=="INC") {
                    int at = find_and_infer(s,thing,infer);
                    if(at==std::string::npos) {
                         print("Unable to find thing for inc: ",thing," In:\n",s);
                    } else {
                         int ex = s.find('x',at)+1;
                         if(ex!=std::string::npos) {
                              int num = ex+1;
                              while(std::isdigit(s.at(num))) {
                                   num++;
                              } 
                              std::string ns = s.substr(ex,num-ex);
                              contents[id].erase(ex,ns.length());
                              int inc_num = std::stoi(ns);
                              int inc_val = 1;
                              if(loc!="") inc_val = std::stoi(loc);
                              contents[id].insert(ex,std::to_string(inc_num+inc_val));
                         }
                    }
               } else if(arg=="NEW_DESC") {
                    int at = find_and_infer(s,thing,infer);
                    if(at==std::string::npos) {
                         print("Unable to find thing for new_desc: ",thing," In:\n",s);
                    } else {
                         int ex = s.find('-',at)+1;
                         if(ex!=std::string::npos) {
                              int next = s.find('*',at);
                              contents[id].erase(ex,(ex-next)-4);
                              contents[id].insert(ex,loc);
                         }
                    }
               }
               else {
                    print("Unrecognized arg: ",arg);
               }
          }
     }
};




int main() {
using namespace helper;

     Window window = Window(1280, 768, "AvalVentures 0.4");
     g_ptr<Scene> scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     //load_gui(scene, "AvalVentures", "gui.fab");

     auto table = make<m_table>(
 
"Region | Gear | Trivial | Minor | Normal | Major | Severe | Permanent |"
"----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |"
"**Head** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
"**Face / Neck** | None | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
"**Chest / Belly** | None | **Injuries** **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** |"
"**Back / Shoulders** | Satchel | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
"**Arms / Paws** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
"**Legs / Feet** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
"**Tail / Base** | None | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
          );

     



     // table->process_command("trivial;face; *Dirt bit* x1 - a bit of dirt ;ADD_IN:**Filth**");
     // table->process_command("trivial;face; *Dirt bit* ;REMOVE_IN:**Filth**");
     //table->process_command("trivial;face; *Dirt bit* ;MOVE_TO:minor:face:**Filth**");
     //table->process_command("trivial;head; *Dirt bit* ;INC:4");
     //table->process_command("trivial;head; *Dirt bit* ;NEW_DESC: whatever you want! ");
     // table->process_command("trivial;head; *Scratch* ;INC:-2|trivial;head; *Scratch* ;NEW_DESC: A bit worse now |trivial;head; *Scratch* ;MOVE_TO:minor:head:**Injuries**");
     print(table->to_markdown());
     std::string cmd = "echo \"" + table->to_markdown() + "\" | pbcopy";
     std::system(cmd.c_str());
     //print(table->to_markdown());

     // ROOT = make<v_object>("char1.json");
     // auto mira = ROOT->get_chunk("mira");
     // auto apple = ROOT->get_chunk("apple");
     // auto floor = ROOT->get_chunk("floor");

     // randi(0,0);

     // // json c = apple->ref("parts")["core"];
     // // apple->ref("contents").push_back(c);

     // //floor->ref("parts")["0,0"]["contents"].push_back((*apple->info));

     // // float flesh_accessible = calculate_accessibility(
     // //      apple->ref("parts")["skin"]["parts"]["flesh"],
     // //      apple->ref("parts")["skin"]  // parent context
     // // );
     // // add_event("impact","apple>p>skin","mira>p>head>p>face>p>mouth>p>teeth",
     // // {
     // // {"use_profile",0},
     // // {"mass",190},
     // // {"force",800}
     // // }
     // // );

     // add_event("grab","apple","mira>p>torso>p>shoulder_right>p>arm_right>p>hand",
     // {
     // }
     // );

     // //thing_at("mira>p>head>p>face>p>mouth>p>teeth")["mat"] = mat_tooth_enamel;
     // //thing_at("apple>p>skin")["mat"] = mat_apple_skin;
 
     // apple->update();
     // mira->update();

     // // add_event("impact","apple>p>skin","mira>p>head>p>face>p>mouth>p>teeth",
     // // {
     // // {"force",100},
     // // {"area",50},
     // // }
     // // );

     // // apple->update();
     // // mira->update();

     // //ROOT->sync_with_file();
     // print("Done");

     start::run(window,d,[&]{
     });
     return 0;
}