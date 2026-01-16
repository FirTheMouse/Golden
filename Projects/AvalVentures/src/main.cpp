
 #include<markdown_table.hpp>
// #include<phys_system.hpp>
// #include<farmlogic.hpp>
// #include<util/meshBuilder.hpp>
#include<process.hpp>
using namespace Golden;


int main() {
using namespace helper;
//1280, 768
     Window window = Window(100, 100, "AvalVentures 0.7");
     g_ptr<Scene> scene = make<Scene>(window,2);
     Data d = make_config(scene,K);
     scene->camera.toOrbit();


     run_process();

     //load_gui(scene, "AvalVentures", "gui.fab");

     //test();
     // auto table = make<m_table>( 
     //      "Region | Gear | Trivial | Minor | Normal | Major | Severe | Permanent |"
     //      "----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |"
     //      "**Head** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     //      "**Face** | None | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     //      "**Chest** | None | **Injuries** **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** |"
     //      "**Back** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     //      "**Forelimbs** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     //      "**Hindlimbs** | None | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     //      "**Other** | None | **Injuries**  **Filth** | **Injuries**  **Filth**  | **Injuries**  **Filth**  | **Injuries**  **Filth** | **Injuries**  **Filth** | **Injuries**  **Filth** |"
     // );


     // int game = 0;

     // if(game==0) {
     //      table->process_command("trivial;face; *Dirt bit* x1 - a bit of dirt ;ADD_IN:**Filth**");

     // }

     // std::string cmd = "echo \"" + table->to_markdown() + "\" | pbcopy";
     // std::system(cmd.c_str());

     // table->print_out();

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
     // auto cube = make<Single>(makeTestBox(1.0f));
     // scene->add(cube);
     // cube->setPosition(vec3(0,1,0));
     // start::run(window,d,[&]{
     // });
     return 0;
}