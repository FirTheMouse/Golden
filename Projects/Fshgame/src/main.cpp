#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <rendering/renderer.hpp>
#include <rendering/scene.hpp>
#include <rendering/model.hpp>
#include <rendering/single.hpp>
#include <core/scriptable.hpp>
#include <extension/simulation.hpp>
#include <util/util.hpp>
#include <fstream>
#include <string>
#include <fsh/fsh_scene.hpp>
#include <fsh/f_object.hpp>

using namespace Golden;

vec3 input2D()
{
    float x = 0.0f;
    float y = 0.0f;
    Input& input = Input::get();

    if(input.keyPressed(UP)) y-=0.3f;
    if(input.keyPressed(DOWN)) y+=0.3f;
    if(input.keyPressed(D)) x+=0.3f;
    if(input.keyPressed(A)) x-=0.3f;
    return vec3(x,0,0);
}

glm::vec3 getWorldMousePosition(g_ptr<Scene> scene) {
    Window& window = scene->window;
    Camera& camera = scene->camera;
    int windowWidth = window.width;
    int windowHeight = window.height;
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);

    // Flip y for OpenGL
    float glY = windowHeight - float(ypos);

    // Get view/proj
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix();
    glm::ivec4 viewport(0, 0, windowWidth, windowHeight);

    // Unproject near/far points
    glm::vec3 winNear(float(xpos), glY, 0.0f);   // depth = 0
    glm::vec3 winFar(float(xpos), glY, 1.0f);    // depth = 1
    

    glm::vec3 worldNear = glm::unProject(winNear, view, projection, viewport);
    glm::vec3 worldFar  = glm::unProject(winFar,  view, projection, viewport);

    // Ray from near to far
    glm::vec3 rayOrigin = worldNear;
    glm::vec3 rayDir = glm::normalize(worldFar - worldNear);

     // Intersect with Z = -5 plane (Z-up)
    float targetZ = 0.0f;
    if (fabs(rayDir.z) < 1e-6f) return glm::vec3(0); // Ray is parallel to plane

    float t = (targetZ - rayOrigin.z) / rayDir.z;
    return rayOrigin + rayDir * t;
}

glm::vec2 mousePos(g_ptr<Scene> scene)
{
    Window& window = scene->window;
    int windowWidth = window.width;
    int windowHeight = window.height;
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);
    return glm::vec2(xpos,ypos);
}


int main() {
    std::string ROOT = "../Projects/Fshgame/storage/";

    int width = 1280;
    int height = 768;

    Window window = Window(width, height, "Fshgame 0.0.1");
    glfwSwapInterval(1); //Vsync 1=on 0=off
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return 0;
    }
    glEnable(GL_DEPTH_TEST);

    glm::vec4 BLUE = glm::vec4(0,0,1,1);
    glm::vec4 RED = glm::vec4(1,0,0,1);
    glm::vec4 BLACK = glm::vec4(0,0,0,1);

    Input& input = Input::get();

    g_ptr<fsh_scene> scene = make<fsh_scene>(window,2);
    scene->camera = Camera(60.0f, 1280.0f/768.0f, 0.1f, 1000.0f, 4);
    scene->camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    scene->relight(glm::vec3(0.0f,0.1f,0.9f),glm::vec4(1,1,1,1));
    scene->setupShadows();

    auto line = make<Quad>();
    line->setupQuad();
    scene->add(line);
    line->setPosition(vec3(300, 450, 0));
    line->scale(100,4,0);
    line->color = glm::vec4(0.2f,0,0,1);

    auto otherLine = make<Quad>();
    otherLine->setupQuad();
    scene->add(otherLine);
    otherLine->setPosition(vec3(300, 450, 0));
    otherLine->scale(100,3,1);
    otherLine->color = glm::vec4(0.2f,0,0,1);

    auto hook = make<Quad>();
    hook->setupQuad();
    scene->add(hook);
    hook->scale(5);
    hook->color = glm::vec4(0,0,0,1);
    hook->setPosition(vec3(300,475,0));

    auto man = make<Quad>();
    man->setupQuad();
    scene->add(man);
    man->scale(25);
    man->color = glm::vec4(0.8f,0.3f,0.3f,1);
    man->setPosition(vec3(300,475,0));


    list<g_ptr<Quad>> fshs;
    for(int i=0;i<50;i++)
    {
        auto fsh = make<Quad>();
        fsh->setupQuad();
        fsh->data.set<float>("speed",randf(6,12));
        scene->add(fsh);
        float s = randf(15,25);
        fsh->scale(s*randf(0.8f,1.8f),s,0);
        fsh->color = glm::vec4(randf(0,1),randf(0,1),randf(0,1),1);
        fsh->setPosition(vec3(randf(300,2800),randf(550,1600),0));
        fshs << fsh;
    }

    auto q = make<Quad>();
    q->setupQuad();
    scene->add(q);
    q->setPosition(vec3(0,500,0));
    q->scale(3000,1200,0);
    q->color = glm::vec4(0.3f,0.4f,0.6f,1);


    float tpf = 0.1; float frametime = 0; int frame = 0;
    auto last = std::chrono::high_resolution_clock::now();
    bool flag = true;
    float pause = 0.0f;

    bool reeling = false;

    while (!window.shouldClose()) {
        if(Input::get().keyPressed(KeyCode::K)) break;
        if(pause>0) pause -=tpf;

        man->move(input2D()*30.0f);
        fshs([scene,width](g_ptr<Quad> fsh){
            fsh->move(-fsh->data.get<float>("speed"),0,0);
            if(fsh->getPosition().x()<0) fsh->setPosition(vec3(
                width*2,fsh->getPosition().y(),0
            ));
        });


        double xpos, ypos;
        glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);
        glm::vec2 dir(
            xpos*2 - man->getPosition().x(),
            ypos*2 - man->getPosition().y()
        );
        float length = glm::length(dir);
        float angle = atan2(dir.y, dir.x);

        line->setPosition(man->getPosition()-vec3(
            xpos*2>man->getPosition().x()?-25:0,-2,0));

        line->rotate(angle,vec3(0,0,1));

        if(Input::get().keyPressed(SPACE)&&pause<=0.0f)
        {
            pause=0.3f;
            reeling=!reeling;
        }  

        otherLine->setPosition(line->getPosition()+(vec3(cos(angle), sin(angle), 0.0f)*100));
        
        vec3 tip = line->getPosition() + vec3(cos(angle), sin(angle), 0.0f) * 100.0f;

        vec3 toTip = tip - hook->getPosition();
        float dist = toTip.length();

        if (dist > 1.0f) {
            vec3 dir = toTip.normalized();
            float speed = glm::clamp(dist * 0.05f, 0.5f, 10.0f);
            hook->move(dir * speed);
        }

        if(reeling)
        {
            hook->move(0,0.8f,0);
        }
        else
        {
            hook->move(0,hook->getPosition().y()<475?0:-0.8f,0);
        }

        vec3 aD = line->getPosition().direction(hook->getPosition());
        glm::vec2 aDir(aD.x(),aD.y());
        float aLength = glm::length(aDir);
        float aAngle = atan2(aDir.y, aDir.x);
        otherLine->scale(hook->getPosition().distance(line->getPosition()),4,0);
        otherLine->rotate(aAngle,vec3(0,0,1));

        scene->updateScene(tpf);
        auto end = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(end - last);
        tpf = delta.count(); last = end; frame++; 

        window.swapBuffers();
        window.pollEvents();
    }

    
    glfwTerminate();
    return 0;
}
