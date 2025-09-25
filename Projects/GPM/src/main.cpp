#include<util/util.hpp>
#include<rendering/scene.hpp>
#include <filesystem>
#include<gui/text.hpp>

namespace fs = std::filesystem;


void startProject(std::string name)
{
    std::string dirPath = "../Projects/" + name+"/";

    if (!fs::exists(dirPath)) {
        fs::create_directories(dirPath);
        fs::create_directories(dirPath+"src");
        fs::create_directories(dirPath+"include");
        fs::create_directories(dirPath+"assets");
        fs::create_directories(dirPath+"assets/fonts");
        fs::create_directories(dirPath+"assets/images");
        fs::create_directories(dirPath+"assets/models");
        std::ofstream outcsv(dirPath + "assets/models/"+name+" - data.csv", std::ios::binary);
        std::string csvstart = "dtype,name\n[TYPE],string";
        uint32_t csvlen = csvstart.length();
        outcsv.write(csvstart.data(), csvlen);
        outcsv.close();
        fs::create_directories(dirPath+"assets/gui");       
        fs::create_directories(dirPath+"assets/shaders");

        std::string makeFile = ""
        "\nfile(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS"
        "\n    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "\n)"
        "\nadd_executable("+name+" ${GAME_SOURCES})"
        "\n"
        "\ntarget_include_directories("+name+" PRIVATE"
        "\n${CMAKE_SOURCE_DIR}/Engine/include"
        "\n    ${CMAKE_SOURCE_DIR}/Projects/"+name+"/include"
        "\n)                                          "
        "\ntarget_link_libraries("+name+" PRIVATE GoldenEngine)";

        std::ofstream out(dirPath + "CMakeLists.txt", std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: "+dirPath + "CMakeLists.txt");

        uint32_t len = makeFile.length();
        out.write(makeFile.data(), len);

        out.close();
        std::string lower = "";
        std::transform(name.begin(),name.end(),lower.begin(),::tolower);
        std::string main = ""
        "#include<core/helper.hpp>"
        "\n"
        "\n"
        "\nint main() {"
        "\nusing namespace Golden;"
        "\nusing namespace helper;"
        "\n"
        "\n     print(\"hello world\");"
        "\n"
        "\n     Window window = Window(1280, 768, \""+name+" 0.1"+"\");"
        "\n     g_ptr<Scene> scene = make<Scene>(window,2);"
        "\n     Data d = make_config(scene,K);"
        "\n     //load_gui(scene, \""+name+"\", \""+lower+"gui"+".fab\");"
        "\n"
        "\n"
        "\n     start::run(window,d,[&]{"
        "\n     });"
        "\n     return 0;"
        "\n}";

        out = std::ofstream (dirPath + "src/main.cpp", std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: "+dirPath + "src/main.cpp");

        len = main.length();
        out.write(main.data(), len);

        out.close();
    }
    else
    {
        print("Project already exists!");
    }
}

void addProjectToCMake(const std::string& name) {
    const std::string cmakePath = "../CMakeLists.txt";
    std::ifstream inFile(cmakePath);
    if (!inFile) {
        std::cerr << "Error: Cannot open " << cmakePath << '\n';
        return;
    }

    std::string line;
    std::ostringstream modified;
    bool foundMarker = false;
    bool replaced = false;

    while (std::getline(inFile, line)) {
        if (foundMarker && !replaced) {
            // Overwrite the line after the marker
            modified << "add_subdirectory(Projects/" << name << ")\n";
            replaced = true;
        } else {
            modified << line << '\n';
            if (line.find("# ACTIVE_PROJECT_HERE") != std::string::npos) {
                foundMarker = true;
            }
        }
    }

    inFile.close();

    if (!foundMarker || !replaced) {
        std::cerr << "Error: Couldn't find project marker in " << cmakePath << '\n';
        return;
    }

    std::ofstream outFile(cmakePath);
    outFile << modified.str();
    outFile.close();

    std::cout << "Updated active project to: " << name << '\n';
}


int launchProject(std::string name)
{
    std::string path = "../Projects/"+name;
    std::string build = "../build";
    std::cout << "Reconfiguring with cmake...\n";
    int result = std::system("cmake -S .. -B ../build");
    if (result != 0) return result;

    std::cout << "Building project...\n";
    result = std::system("cmake --build ../build");
    if (result != 0) return result;

    // Step 3: Run project binary
    std::string binPath = "../build/Projects/" + name + "/" + name;
    std::cout << "Running binary: " << binPath << "\n";
    return std::system(binPath.c_str());
}


int main()
{
    using namespace Golden;

    //Window window = Window(1280, 768, "GPM 0.1.0");
    Window window = Window(100, 70, "GPM 0.1.0");
    auto scene = make<Scene>(window,1);
    scene->camera.speedMod = 0.01f;


    // std::string name = "ThymeLoop";
    // startProject(name);
    // addProjectToCMake(name);
    std::string p1 = "StudentSystem";
    std::string p2 = "GUIDE";
    std::string p3 = "GDSL";
    std::string p4 = "Testing";
    
    bool runningProject = false;

    while (!window.shouldClose()) {
        if(Input::get().keyPressed(KeyCode::K)&&Input::get().keyPressed(KeyCode::LSHIFT)) break;
        scene->updateScene(1.0f);

        if(Input::get().keyJustPressed(KeyCode::NUM_1))
        {
            addProjectToCMake(p1);
            launchProject(p1);
        }
        else if(Input::get().keyJustPressed(KeyCode::NUM_2))
        {
            addProjectToCMake(p2);
            launchProject(p2);
        }
        else if(Input::get().keyJustPressed(KeyCode::NUM_3))
        {
            addProjectToCMake(p3);
            launchProject(p3);
        }
        else if(Input::get().keyJustPressed(KeyCode::NUM_4))
        {
            addProjectToCMake(p4);
            launchProject(p4);
        }

        window.swapBuffers();
        window.pollEvents();
    }
    glfwTerminate();

    return 0;
}

