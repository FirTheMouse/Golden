#include <rendering/scene.hpp>
#include <util/meshBuilder.hpp>
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
namespace Golden
{


list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}

uint loadTexture2D(const std::string& path, bool flipY)
{
    int w,h;
    return loadTexture2D(path,flipY,w,h);
}
uint loadTexture2D(const std::string& path, bool flipY,int& w, int& h)
{
int n;
stbi_set_flip_vertically_on_load(flipY);
unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 4);
if(!data) { std::cerr << "stb_image failed: " << path << "\n"; return 0; }

GLuint tex;
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, data);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
stbi_image_free(data);
return tex;
}


 void Scene::repo(const g_ptr<S_Object>& obj)
    {

    }

void Scene::add(const g_ptr<S_Object>& sobj) {
    sobj->scene = this;
    if(auto quad = g_dynamic_pointer_cast<Quad>(sobj))
    {
        quad->ID = guiTransforms.length();
        quadActive.push(true);
        quads.push(quad);
        //QUUID.push_back(quad->UUID);
        guiTransforms.push(glm::mat4(1.0f));
        guiEndTransforms.push(glm::mat4(1.0f));
        quadAnimStates.push(AnimState());
        quadVelocities.push(Velocity());
        quadPhysicsStates.push(P_State::DETERMINISTIC);
        quadCollisonLayers.push(CollisionLayer());
        quadPhysicsProp.push(P_Prop());
        slots.push(list<std::string>{"all"});
    }
    // else if(auto obj = g_dynamic_pointer_cast<Instanced>(sobj))
    // {
    //         if(obj->getModel())
    //         {
    //         obj->getModel()->addInstance(sobj);
    //          if(!instanceModels.hasKey(obj->type))
    //          setupInstance(obj->type,obj->model);
    //         }
    //         else {
    //             if(instanceModels.hasKey(obj->type))
    //             {
    //                 obj->setModel(instanceModels.get(obj->type));
    //                 obj->getModel()->addInstance(sobj);
    //             }
    //             else {
    //             std::cerr << "No model for instance type in Scene::add" << std::endl;
    //             return;
    //             }
    //         }
    // } 
    else if (auto obj = g_dynamic_pointer_cast<Single>(sobj)) 
    {
        obj->ID = singles.length();
        active.push(true);
        culled.push(false);
        singles.push(obj);
        auto p_model = make<Model>(makePhysicsBox(obj->model->localBounds));
        p_model->UUIDPTR = obj->UUID;
        //physicsModels.push(p_model);
        transforms.push(glm::mat4(1.0f));
        endTransforms.push(glm::mat4(1.0f));
        animStates.push(AnimState());
        velocities.push(Velocity());
        physicsStates.push(P_State::DETERMINISTIC);
        collisonLayers.push(CollisionLayer());
        collisionShapes.push(CollisionShape());
        physicsProp.push(P_Prop());
    }
}

void Scene::updateScene(float tpf)
{
    sceneTime+=tpf;
    Input::get().decayScroll();
    double xpos, ypos;
    glfwGetCursorPos((GLFWwindow*)window.getWindow(), &xpos, &ypos);
    Input::get().updateMouse(xpos, ypos);
    camera.update(tpf);
    Input::get().syncMouse();

    glViewport(0, 0, 4096, 4096);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
    for (size_t i = 0; i < singles.length(); ++i) {
        if(i>=singles.length()) continue;
        if(!active.get(i,"scene::updateScene::129")) continue;
        if(culled.get(i,"scene::updateScene::130")) continue;
        depthShader.setMat4("model", glm::value_ptr(transforms[i]));
        g_ptr<Model> model = singles[i]->model;
        bool hasBones = model->boneDirty.length()>0;
        depthShader.setInt("hasSkeleton", hasBones ? 1 : 0);
        if(hasBones)
        {
            model->updateBoneHierarchy();
            model->uploadBoneMatrices(depthShader.getID());
        }
        model->draw(depthShader.getID());
    }

    instanceDepthShader.use();
    instanceDepthShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
    for (size_t i = 0; i < instanceTypes.size(); ++i) {
        if(!active.get(i,"scene::updateScene::38")) continue;
        instanceDepthShader.setMat4("model", glm::value_ptr(glm::mat4(1.0f)));
        instanceModels.get(instanceTypes[i])->draw(instanceDepthShader.getID());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(sunColor.x*0.5f, sunColor.y*0.8f, sunColor.z*1.3f, 1.0f);
    glViewport(0, 0, window.width*2, window.height*2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(size_t i=0;i<3;i++)
    {
        Shader& shader = (i == 0) ? singleShader :
                 (i == 1) ? instanceShader :
                            depthShader;
        shader.use();
        shader.setVec3("camPos", camera.getPosition().toGlm());
        for (unsigned int j = 0; j < lights.size(); j++) {
            std::string lightNum = "lightPositions[" + std::to_string(j) + "]";
            std::string colorNum = "lightColors[" + std::to_string(j) + "]";
            shader.setVec3(lightNum.c_str(),lights[j]->position);
            shader.setVec3(colorNum.c_str(),lights[j]->color);
        }
        shader.setVec3("sunDirection", sunDir.x, sunDir.y, sunDir.z);
        shader.setVec3("sunColor", sunColor.x, sunColor.y, sunColor.z);
        shader.setVec3("moonDirection", moonDir.x, moonDir.y, moonDir.z);
        shader.setVec3("moonColor", moonColor.x, moonColor.y, moonColor.z);
        shader.setMat4("view", glm::value_ptr(camera.getViewMatrix()));
        shader.setMat4("projection", glm::value_ptr(camera.getProjectionMatrix()));
        shader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
        shader.setInt("shadowMap", 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
    }

    singleShader.use();
    for (size_t i = 0; i < singles.length(); ++i) {
        if(i>=singles.length()) continue;
        if(!active.get(i,"scene::updateScene::184")) continue;
        if(culled.get(i,"scene::updateScene::185")) continue;
        singleShader.setMat4("model", glm::value_ptr(transforms[i]));
        g_ptr<Model> model = singles[i]->model;
        bool hasBones = model->boneDirty.length()>0;
        singleShader.setInt("hasSkeleton", hasBones ? 1 : 0);
        if(hasBones)
        {
            model->uploadBoneMatrices(singleShader.getID());
        }
        singles[i]->model->draw(singleShader.getID());
    }

    instanceShader.use();
    for (size_t i = 0; i < instanceTypes.size(); ++i) {
        if(!active.get(i,"scene::updateScene::83")) continue;
        instanceDepthShader.setMat4("model", glm::value_ptr(glm::mat4(1.0f)));
        instanceModels.get(instanceTypes[i])->draw(instanceShader.getID());
    }

    glDisable(GL_DEPTH_TEST);      // ignore depth
    glDepthMask(GL_FALSE);         // don’t write depth
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    guiShader.use();
    guiShader.setVec2("uResolution", window.resolution);
    for (size_t i = 0; i < quads.length(); ++i) {
        if(i>=quads.length()) continue;
        if (!quads[i]) continue;
        if(!quadActive.get(i,"scene::updateScene::98")) continue;

       quads[i]->run("onUpdate");

        guiShader.setMat4("quad",glm::value_ptr(guiTransforms[i]));
        guiShader.setVec4("uColor", quads[i]->color);
   
        bool useTex = quads[i]->textureGL != 0;
        guiShader.setInt("useTexture", useTex ? 1 : 0);
        guiShader.setVec4("uvRect", quads[i]->uvRect); 
        if (useTex) {
            uint8_t slot = quads[i]->textureSlot;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, quads[i]->textureGL);
            guiShader.setInt("uTex", slot); // Must match glActiveTexture slot
        }
        quads[i]->draw();
    }

    glDepthMask(GL_TRUE);          // restore for 3-D, if needed
    glEnable(GL_DEPTH_TEST);
}

void Scene::tickEnvironment(int time)
{
    // cout << "envtick" << endl;
    const float pi     =  glm::pi<float>();
    float dayCycle = (time) / 3000.0f;  // [0, 1] loop
    
    const float latitude = 40.0f; // degrees
    const float latitudeRad = glm::radians(latitude);
    float dayOfYear = fmod(dayCycle * 365.0f, 365.0f);
    float timeOfDay = fmod(dayCycle, 1.0f);
    float meanAnomaly = 2.0f * pi * (dayOfYear - 4.0f) / 365.0f;
    float declination = 0.4093f * sin(meanAnomaly - 0.0274f * sin(meanAnomaly));
    float equationOfTime = 0.0172f * sin(2.0f * meanAnomaly) - 0.0193f * sin(meanAnomaly + 0.033f * sin(meanAnomaly));
    float hourAngle = pi * (2.0f * timeOfDay - 1.0f) - equationOfTime;
    float sinElevation = sin(latitudeRad) * sin(declination) + 
                         cos(latitudeRad) * cos(declination) * cos(hourAngle);
    float elevation = asin(sinElevation);
    float cosAzimuth = (sin(declination) - sin(latitudeRad) * sinElevation) / 
                       (cos(latitudeRad) * cos(elevation));
    cosAzimuth = glm::clamp(cosAzimuth, -1.0f, 1.0f);
    float azimuth = acos(cosAzimuth);
    if (hourAngle > 0) {
        azimuth = 2.0f * pi - azimuth;
    }
    glm::vec3 sunDirCalc = glm::normalize(glm::vec3(
        cos(elevation) * sin(azimuth),
        sin(elevation),
        cos(elevation) * cos(azimuth)
    ));

    // Moon direction = offset by 180 degrees in azimuth
    glm::vec3 moonDirCalc = glm::vec3(
        -sunDirCalc.x,
        -sunDirCalc.y,
        -sunDirCalc.z
    );

    float sunElev = sunDirCalc.y;
    float moonElev = moonDirCalc.y;
   // cout << sunElev << " S|M " << moonElev << endl;
    float sunIntensity = glm::clamp(sunElev * 1.5f, 0.0f, 1.0f);
    float moonIntensity = glm::clamp(moonElev * 0.5f, 0.0f, 0.3f); // Moonlight dimmer

    glm::vec3 warmSun = glm::mix(
        glm::vec3(1.1f, 0.6f, 0.4f), // Sunrise/sunset
        glm::vec3(1.0f, 0.95f, 0.9f), // Midday
        sunIntensity
    );

    glm::vec3 blueMoon = glm::vec3(0.2f, 0.3f, 0.6f);

    // Final sun/moon values
    sunDir = sunDirCalc;
    moonDir = moonDirCalc;

    //cout << sunDir.y << endl;

    glm::vec4 sunLightColor = glm::vec4(warmSun * sunIntensity, 1.0f);
    glm::vec4 moonLightColor = glm::vec4(blueMoon * moonIntensity, 1.0f);
    sunColor = sunLightColor;
    moonColor = moonLightColor;

    // Decide which light casts shadows (dominant one)
    glm::vec3 shadowCasterDir = sunElev >= moonElev ? sunDir : moonDir;
    //cout << (sunElev >= moonElev) << endl;

    glm::vec3 camPos =  camera.getPosition().nY().toGlm(); //glm::vec3(0,0,0);
    glm::vec3 lightPos = camPos+shadowCasterDir * 50.0f;
    //camera.getPosition().toGlm() Cam pos
    glm::mat4 lightView = glm::lookAt(lightPos,camPos, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 200.0f);
    lightSpaceMatrix = lightProjection * lightView;
}


void Scene::setupShadows()
{
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                4096, 4096, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec3 lightPos = sunDir * 50.0f;
    glm::mat4 lightView = glm::lookAt(lightPos,glm::vec3(0,0,0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 200.0f);
    lightSpaceMatrix = lightProjection * lightView;
}

    void Scene::saveBinary(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: " + path);
        saveBinary(out);
        out.close();
    }

    void Scene::loadBinary(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read from file: " + path);
        loadBinary(in);
        in.close();
    }

    //Gutting serilization for now, come back to this later to fix it up!

    void Scene::saveBinary(std::ostream& out) const {
        // const char magic[] = "GSCN";
        // out.write(magic, 4);
    
        // // Models - these need custom handling since they're g_ptr
        // models.saveBinaryCustom(out, [](std::ostream& stream, const auto& model) {
        //     model->saveBinary(stream);
        // });
        
        // physicsModels.saveBinaryCustom(out, [](std::ostream& stream, const auto& model) {
        //     model->saveBinary(stream);
        // });
    
        // // POD types can use simple serialization
        // transforms.saveBinary(out);
        // endTransforms.saveBinary(out);
        // guiTransforms.saveBinary(out);
        // animStates.saveBinary(out);
        // velocities.saveBinary(out);
        // physicsStates.saveBinary(out);
    
        // // Complex types need custom handling
        // quads.saveBinaryCustom(out, [](std::ostream& stream, const auto& quad) {
        //     quad->saveBinary(stream);
        // });
        
        // slots.saveBinaryCustom(out, [](std::ostream& stream,list<std::string>& strList) {
        //     size_t size = strList.length();
        //     stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
        //     for (const std::string& str : strList) {
        //         uint32_t len = str.length();
        //         stream.write(reinterpret_cast<const char*>(&len), sizeof(len));
        //         stream.write(str.data(), len);
        //     }
        // });
    }
    
    void Scene::loadBinary(std::istream& in) {
        char magic[4];
        in.read(magic, 4);
        if (std::strncmp(magic, "GSCN", 4) != 0)
            throw std::runtime_error("Invalid file format");
    
        // Load with custom deserializers
        singles.loadBinaryCustom(in, [](std::istream& stream) {
            auto model = make<Model>();
            model->loadBinary(stream);
            return model;
        });
        
        // physicsModels.loadBinaryCustom(in, [](std::istream& stream) {
        //     auto model = make<Model>();
        //     model->loadBinary(stream);
        //     return model;
        // });
    
        // POD types load simply
        transforms.loadBinary(in);
        endTransforms.loadBinary(in);
        guiTransforms.loadBinary(in);
        animStates.loadBinary(in);
        velocities.loadBinary(in);
        physicsStates.loadBinary(in);
    
        quads.loadBinaryCustom(in, [](std::istream& stream) {
            auto quad = make<Quad>();
            quad->loadBinary(stream);
            return quad;
        });
        
        slots.loadBinaryCustom(in, [](std::istream& stream) {
            size_t size;
            stream.read(reinterpret_cast<char*>(&size), sizeof(size));
            list<std::string> strList;
            for (size_t j = 0; j < size; ++j) {
                uint32_t len;
                stream.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string str(len, '\0');
                stream.read(&str[0], len);
                strList << str;
            }
            return strList;
        });
    }
}