#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<core/grid2d.hpp>
#include<util/meshBuilder.hpp>
#include<util/color.hpp>
#include<util/files.hpp>

using namespace Golden;

void start_flotsam(g_ptr<Scene> scene, g_ptr<Grid2D> level = nullptr) {
    map<std::string,std::string> type_modelPath;

    auto is_model = [](const std::string& filename) -> std::string{ 
        auto split = split_str(filename,'.');
        if(split.length()>0)
        {
            if(split[split.length()-1]=="png") {
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
    process_directory_recursive("../Projects/StudentSystem/assets/images/");

    std::string data_string = "NONE";
    try {
    data_string = readFile("../Projects/StudentSystem/assets/images/Flotsam - data.csv");
    }
    catch(std::exception e)
    {
        print("No - data.csv provided for StudentSystem this is required for object definitions to work");
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
        scene->set<std::string>("_"+type+"_image",path);
        auto data = make<q_data>();
        int has_definition = 2;
        for(int i=2;i<lines.length();i++)
        {
            list<std::string> values = split_str(lines[i],',');
            if(values[0]==type) {
                for(int t = 0;t<values.length();t++)
                {
                    std::string t_type = types[t];
                    if(values[t]=="") continue;
                    if(t_type=="string") data->add<std::string>(headers[t],values[t]);
                    else if (t_type=="int") data->add<int>(headers[t],std::stoi(values[t]));
                    else if (t_type=="float") data->add<float>(headers[t],std::stof(values[t]));
                }
            }
            else {
                has_definition++;
            }
        }
        if(has_definition>lines.length()-1) {
            print("Missing defintion for ",type," in Flotsam - data.csv");
        }
        scene->set<g_ptr<q_data>>("_"+type+"_data",data);
        Script<> make_part("make_"+type,[scene,level,type](ScriptContext& ctx){
            auto path = scene->get<std::string>("_"+type+"_image");
            auto data = scene->get<g_ptr<q_data>>("_"+type+"_data");
            auto part = scene->makeImageQuad(path,1.0f);
            part->data = data;
            if(level)
            {
               //Put some stuff here for passes
            }
            ctx.set<g_ptr<Object>>("toReturn",part);
    });
    scene->define(type,make_part);
    }
}

int main()  {
    using namespace helper;

    std::string MROOT = "../Projects/StudentSystem/assets/models/";
    std::string IROOT = "../Projects/StudentSystem/assets/images/";

    Window window = Window(1280, 768, "Flotsam 0.1");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);

    float cellSize = 100.0f;
    float mapSize = 1000.0f;
    auto grid = make<Grid2D>(cellSize,mapSize);
    grid->y_level = 0;

    auto background = make<Quad>();
    scene->add(background);
    background->setColor(Color::BLUE);
    background->scale(vec2(4000,4000));
    background->stop();
    background->flagOff("valid");

    // auto square = make<Quad>();
    // scene->add(square);
    // square->setColor(Color::RED);
    // square->scale(vec2(300,300));
    // square->setCenter(vec2(300,300));

    vec2 center(500,500);
    vec2 lastpos(500,500);

    start_flotsam(scene,grid);
    for(int i=0;i<300;i++)
    {
       auto a = scene->create<Quad>("plank");
       a->scale(vec2(cellSize,cellSize));
       vec2 newpos = grid->snapToGrid(lastpos+vec2(cellSize*randi(-1,1),cellSize*randi(-1,1)));
       int g = 0;
       while(newpos.x()>=2560&&newpos.y()>=1536&&
            newpos.x()<0&&newpos.y()<0
       &&g<300) {
        newpos = grid->snapToGrid(lastpos+vec2(cellSize*randi(-1,1),cellSize*randi(-1,1)));
        g++;
       }
       g = 0;
    //    while(!grid->getCell(newpos)->empty()&&g<30) {
    //     newpos = grid->snapToGrid(lastpos+vec2(cellSize*randi(-1,1),cellSize*randi(-1,1)));
    //     g++;
    //    }
    //    if(g>=30) print("AH");
       grid->getCell(newpos)->push(a);
       a->setCenter(newpos);
       lastpos = newpos;
       //a->setCenter(grid->snapToGrid(vec2(randf(-mapSize,mapSize),randf(-mapSize,mapSize))));
    }

    g_ptr<Quad> sel = nullptr;
    start::run(window,d,[&]{
        if(pressed(MOUSE_LEFT)) {
            scene->mousePos2d().print();
            if(sel) sel = nullptr;
            else sel = scene->nearestElement();
        }

        if(sel)
            sel->setCenter(grid->snapToGrid(scene->mousePos2d()));
       
    });

    return 0;
}