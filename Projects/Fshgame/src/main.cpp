#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<util/color.hpp>
#include<util/string_generator.hpp>
#include<util/logger.hpp>
#include<core/physics.hpp>
#include<util/meshBuilder.hpp>

#include<gui/twig.hpp>
#include<util/DSL_util.hpp>

using namespace Golden;

g_ptr<Scene> scene;
ivec2 win(2560,1536);


int sampleForSubdivision(g_ptr<Quad> quad, vec2 screenPoint) {
    if (quad->getGeom()->subdivisions.empty()) return -1;
    
    glm::mat4 invTransform = glm::inverse(quad->getTransform());
    vec4 localPoint4 = invTransform * glm::vec4(screenPoint.toGlm(), 0, 1);
    vec2 localPoint(localPoint4.x(), localPoint4.y());
    
    list<list<vec2>>& subs = quad->getGeom()->subdivisions;
    for(int r = 0; r < subs.length(); r++) {
        list<vec2>& poly = subs[r];
        bool inside = false;
        for(int i = 0, j = poly.length() - 1; i < poly.length(); j = i++) {
            if(((poly[i].y() > localPoint.y()) != (poly[j].y() > localPoint.y())) &&
               (localPoint.x() < (poly[j].x() - poly[i].x()) * 
                (localPoint.y() - poly[i].y()) / 
                (poly[j].y() - poly[i].y()) + poly[i].x())) {
                inside = !inside;
            }
        }
        if(inside) return r;
    }
    return -1;
}


//Deosn't work for sampling
list<std::pair<list<vec2>, vec4>> generateSimpleSubdivisions(int numRegions, unsigned int seed = 0) {
    if(seed != 0) srand(seed);
    
    list<std::pair<list<vec2>, vec4>> result;
    
    // Simple approach: recursive binary space partitioning
    struct Cell {
        vec2 min, max;
        vec4 color;
    };
    
    list<Cell> cells;
    cells << Cell{vec2(0, 0), vec2(1, 1), vec4(randf(0.3, 1), randf(0.3, 1), randf(0.3, 1), 1)};
    
    // Keep splitting until we have enough regions
    while(cells.length() < numRegions) {
        // Pick a random cell to split
        int idx = rand() % cells.length();
        Cell& c = cells[idx];
        
        vec2 size = c.max - c.min;
        bool splitHorizontal = size.x() > size.y();
        
        // Split point (with some randomness, not exactly center)
        float splitPos = randf(0.35, 0.65);
        
        Cell a, b;
        if(splitHorizontal) {
            float splitX = c.min.x() + size.x() * splitPos;
            a = {c.min, vec2(splitX, c.max.y()), vec4(randf(0.3, 1), randf(0.3, 1), randf(0.3, 1), 1)};
            b = {vec2(splitX, c.min.y()), c.max, vec4(randf(0.3, 1), randf(0.3, 1), randf(0.3, 1), 1)};
        } else {
            float splitY = c.min.y() + size.y() * splitPos;
            a = {c.min, vec2(c.max.x(), splitY), vec4(randf(0.3, 1), randf(0.3, 1), randf(0.3, 1), 1)};
            b = {vec2(c.min.x(), splitY), c.max, vec4(randf(0.3, 1), randf(0.3, 1), randf(0.3, 1), 1)};
        }
        
        // Replace old cell with two new ones
        cells.removeAt(idx);
        cells << a << b;
    }
    
    // Convert cells to boundary lists
    for(auto& c : cells) {
        list<vec2> boundary;
        boundary << c.min;
        boundary << vec2(c.max.x(), c.min.y());
        boundary << c.max;
        boundary << vec2(c.min.x(), c.max.y());
        
        result << std::pair{boundary, c.color};
    }
    
    return result;
}



int main()  {
    using namespace helper;

    std::string MROOT = root()+"/Projects/Fshgame/assets/models/";
    std::string IROOT = root()+"/Projects/Fshgame/assets/images/";
    Window window = Window(win.x()/2, win.y()/2, "GUI Testing");
    scene = make<Scene>(window,2);
    Data d = make_config(scene,NUM_0);
    scene->tickEnvironment(0);

    g_ptr<Text> twig = nullptr;

    list<g_ptr<Quad>> boxes;
    list<g_ptr<Single>> cubes;

    scene->enableInstancing();
    Physics phys(scene);

    g_ptr<Thread> thread = make<Thread>();
    // thread->logSPS = true;
    // thread->run([&](ScriptContext& ctx){
    //     phys.updatePhysics();
    //     return ctx;
    // },0.008f);
    // thread->pause();

    int TESTING = 0;
    if(TESTING == 0) {
        g_ptr<Font> font = make<Font>(root()+"/Engine/assets/fonts/source_code.ttf",8);
        twig = make<Text>(font,scene);
        boxes << twig->makeText(
        "There is no single elected official or leader of the Republic, instead power is distributed to three bodies:"
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
        "\nThe Administrative Council deals with immediate and federal functions, and typically abstains from the majority of" 
        "\nlaws except in special or contested cases. It is rare for the full Administrative Council to even assemble,"
        "\nand of all the bodies they are the least active."
        "\nThe Popular Assembly is responsible for representing the will of the people, they tend to be more invested"
        "\nin the approval of laws than the proposal, yet they do propose. Laws can be passed to the Popular Assembly"
        "\nvia petition, or by any single member. Representatives of the assembly are elected by popular election in electoral"
        "\nsubdivisions of the districts."
        "\nThe Legislature is the primary and most active body of the government, responsible for the proposal of most laws."
        "\nIts members are a mix of elected and unelected, representatives from the academy are selected by the academies"
        "\n(some using an electoral process), city representatives are chosen by city government (sometimes electorally),"
        "\nand district representatives are elected by popular vote. Some cities or academies have representatives to the "
        "\nLegislature as a separate role, some are represented by a mayor or dean."
        "\nthe Administrative Council, composed of the 7 heads of each of the major ministries, the Popular Assembly," 
        "\ncomposed of 137 elected representatives by population, and the Legislature, composed of 61 representatives"
        "\nfrom the major districts, academies, and cities."
        "\nThe three bodies all operate together to pass laws, with any one being able to propose a law,"
        "\nbut needing the approval of all others to pass it (except in special cases), There are certain"
        "\njurisdictions where approval is weighted, for instance, laws relating to a specific city are given greater"
        "\nweight for disapproval by the representing minority of the Legislature. The general functions of the bodies are each separate:"
            ,{100,100});
        // auto g = make<Geom>();
        // for(int i=0;i<10;i++) {
        //     auto q = make<Quad>(g);
        //     scene->add(q);
        //     q->scale({100,100});
        //     q->setPosition(vec2(500+(i*10.0f),i*50.0f));
        //     if(i<5) {
        //         scene->guiColor[q->ID] = vec4(0.0f,0.3f,0.0f,1);
        //         //scene->guiTransforms[q->ID][3][3] = 0.1f;
        //         q->setDepth(0.1f);
        //     }
        //     else {
        //             scene->guiColor[q->ID] = vec4(0.0f,0.0f,0.8f,1);
        //             // scene->guiTransforms[q->ID][3][3] = 1.0f;
        //             q->setDepth(1.0f);
        //         }
        // }
    } else if (TESTING==1) {
        g_ptr<Geom> geom = make<Geom>();
        for(int i=0;i<10000;i++) {
            g_ptr<Quad> box = make<Quad>(geom);
            scene->add(box);
            box->setColor(i%3==0?Color::RED:i%3==1?Color::BLUE:Color::GREEN);
            box->scale({10,10});
            box->setPosition({randf(0,win.x()),randf(0,win.y())});
            box->setLinearVelocity({randf(-40,40),randf(-40,40)});
            box->setPhysicsState(P_State::GHOST);

            // box->setLinearVelocity(vec2(100, 50));  // Move right and down
            // box->getVelocity().rotation = vec3(0, 0, 1.0f);  // Rotate 1 rad/sec
            // box->getVelocity().scale = vec3(0.01f, 0.01f, 0);  // Grow 50% per second
            boxes << box;
        }
        phys.quadCollisonMethod = Physics::AABB;
    } else if (TESTING==2) {
        g_ptr<Quad> box = make<Quad>();
        scene->add(box);
        box->setColor(Color::GREEN);
        box->scale({100,100});
        box->setPosition({randf(0,win.x()),randf(0,win.y())});
        boxes << box;

        for(int i=0;i<10000;i++) {
            g_ptr<Quad> box2 = make<Quad>();
            scene->add(box2);
            box2->setColor(Color::RED);
            box2->scale({10,15});
            box2->setPosition({randf(0,win.x()),randf(0,win.y())});
            boxes << box2;

            box2->parent = box;
            box->children << box2;

            box2->joint = [box2]() {
                    g_ptr<Quad> parent = box2->parent;
                    vec2 offset = box2->getPosition() - parent->getPosition();
                    box2->position = vec2(parent->position) + offset;
                    return true;
                };
        }

                // g_ptr<Quad> box2 = make<Quad>();
        // scene->add(box2);
        // box2->setColor(Color::RED);
        // box2->scale({80,150});
        // box2->setPosition({randf(0,win.x()),randf(0,win.y())});
        // boxes << box2;

        // box2->parent = box;
        // box->children << box2;

        // box2->joint = [box2]() {
        //         g_ptr<Quad> parent = box2->parent;
        //         vec2 offset = box2->getPosition() - parent->getPosition();
        //         box2->position = vec2(parent->position) + offset;
        //         return true;
        //     };


    } else if (TESTING==3) {
        g_ptr<Quad> base = scene->makeImageQuad(IROOT+"plank.png",10);
        base->setPosition({randf(0,win.x()),randf(0,win.y())});
        boxes << base;
        for(int i=0;i<10;i++) {
            g_ptr<Quad> box = make<Quad>(base->getGeom());
            scene->add(box);
            box->scale(base->getScale());
            box->setData(base->getData());
            box->setPosition({randf(0,win.x()),randf(0,win.y())});
            boxes << box;
        }
    } else if(TESTING==4) {
        g_ptr<Quad> box = make<Quad>();
        scene->add(box);
        box->scale({50,50});
        box->setColor(Color::RED);
        box->setPosition({500,500});
        box->setPhysicsState(P_State::ACTIVE);
        boxes << box;

        g_ptr<Quad> line = make<Quad>();
        scene->add(line);
        line->scale({100,1});
        line->setPosition({500,200});
        line->setPhysicsState(P_State::FREE);
        boxes << line;
    } else if (TESTING==5) {
        
        Log::rig r;

        // Test data
        glm::mat4 testMatrix = glm::mat4(1.0f);
        vec2 cachedPos = vec2(100, 200);
        vec2 cachedScale = vec2(50, 50);
        float cachedRot = 0.5f;
        vec2 extractedPos, extractedScale;
        float extractedRot;
        
        r.add_process("clean", [&](int i) {
            if(i == 0) {
                testMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(100, 200, 0));
                testMatrix = glm::rotate(testMatrix, 0.5f, glm::vec3(0, 0, 1));
                testMatrix = glm::scale(testMatrix, glm::vec3(50, 50, 1));
            }
        });
        
        // === Composition costs ===
        r.add_process("compose_full", [&](int i) {
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(cachedScale.toGlm(), 1));
            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), cachedRot, glm::vec3(0, 0, 1));
            glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(cachedPos.toGlm(), 0));
            testMatrix = transMat * rotMat * scaleMat;
        });
        
        r.add_process("compose_no_rotation", [&](int i) {
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(cachedScale.toGlm(), 1));
            glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(cachedPos.toGlm(), 0));
            testMatrix = transMat * scaleMat;
        });
        
        // === Extraction costs ===
        r.add_process("extract_position", [&](int i) {
            extractedPos = vec2(glm::vec3(testMatrix[3]));
        });
        
        r.add_process("extract_scale", [&](int i) {
            extractedScale = vec2(
                glm::length(glm::vec3(testMatrix[0])),
                glm::length(glm::vec3(testMatrix[1]))
            );
        });
        
        r.add_process("extract_rotation", [&](int i) {
            // Assuming 2D rotation around Z-axis
            glm::vec2 right = glm::normalize(glm::vec2(testMatrix[0]));
            extractedRot = atan2(right.y, right.x);
        });
        
        r.add_process("extract_all", [&](int i) {
            extractedPos = vec2(glm::vec3(testMatrix[3]));
            extractedScale = vec2(
                glm::length(glm::vec3(testMatrix[0])),
                glm::length(glm::vec3(testMatrix[1]))
            );
            glm::vec2 right = glm::normalize(glm::vec2(testMatrix[0]));
            extractedRot = atan2(right.y, right.x);
        });
        
        // === Cache access costs (baseline) ===
        r.add_process("cache_read", [&](int i) {
            volatile vec2 p = cachedPos;
            volatile vec2 s = cachedScale;
            volatile float r = cachedRot;
        });
        
        r.add_process("cache_write", [&](int i) {
            cachedPos = vec2(i, i);
            cachedScale = vec2(i, i);
            cachedRot = (float)i;
        });
        
        // Comparisons
        r.add_comparison("compose_full", "cache_write");
        r.add_comparison("compose_no_rotation", "cache_write");
        r.add_comparison("extract_position", "cache_read");
        r.add_comparison("extract_scale", "cache_read");
        r.add_comparison("extract_all", "cache_read");
        
        r.run(10000, true, 100);

        return 0;
    }
    else if (TESTING==6) {
        // list<g_ptr<Quad>> ropeSegments;
        // vec2 startPos = vec2(win.x()/2, 100);
        // int segmentCount = 300;
        // float segmentLength = 10.0f;
    
        // for(int i = 0; i < segmentCount; i++) {
        //     g_ptr<Quad> segment = make<Quad>();
        //     scene->add(segment);
        //     segment->setColor(i == 0 ? Color::RED : Color::BLUE);
        //     segment->scale({15, segmentLength});
        //     segment->setPosition(startPos + vec2(0, i * segmentLength));
        //     segment->setPhysicsState(P_State::PARTICLE);
            
        //     ropeSegments << segment;
        //     if(i==0) boxes << segment;
            
        //     if(i > 0) {
        //         g_ptr<Quad> parent = ropeSegments[i-1];
        //         segment->parent = parent;
        //         parent->children << segment;
                
        //         segment->joint = [segment, segmentLength,ropeSegments]() {
        //             g_ptr<Quad> parent = segment->parent;
                    
        //             vec2 toParent = parent->getPosition() - segment->getPosition();
        //             float dist = toParent.length();
                    
        //             if(dist > segmentLength) {
        //                 vec2 targetPos = parent->getPosition() - toParent.normalized() * segmentLength;
        //                 segment->position = segment->getPosition() * 0.3f + targetPos * 0.7f;
        //             }
                    
        //             vec2 dir = toParent.normalized();
        //             float targetAngle = atan2(dir.x(), -dir.y());
        //             segment->rotation = targetAngle;
                    
        //             if(segment == ropeSegments[1]) {
        //                 parent->rotation = targetAngle;
        //                 parent->updateTransform(false); 
        //             }
                    
        //             return true;
        //         };
        //     }
        // }
        // phys.quadGravity = 100.0f;

        
        list<g_ptr<Quad>> ropeSegments;
        vec2 startPos = vec2(win.x()/2, 100);
        int segmentCount = 100;
        float segmentLength = 10.0f;

        for(int i = 0; i < segmentCount; i++) {
            g_ptr<Quad> segment = make<Quad>();
            scene->add(segment);
            segment->setColor(i == 0 ? Color::RED : Color::BLUE);
            segment->scale({15, segmentLength});
            segment->setPosition(startPos + vec2(0, i * segmentLength));
            segment->setPhysicsState(P_State::PARTICLE);
            
            ropeSegments << segment;
            if(i==0) boxes << segment;
            
            if(i > 0) {
                g_ptr<Quad> parent = ropeSegments[i-1];
                segment->parent = parent;
                parent->children << segment;
                
                // physicsJoint maintains distance constraint via forces
                segment->physicsJoint = [segment, segmentLength]() {
                    g_ptr<Quad> parent = segment->parent;
                    
                    vec2 toParent = parent->getPosition() - segment->getPosition();
                    float dist = toParent.length();
                    
                    if(dist > 0.001f) {
                        float error = dist - segmentLength;
                        vec2 direction = toParent / dist;
                        float stiffness = 50.0f;
                        vec2 force = direction * error * stiffness;
                        vec2 childVel = vec3_to_vec2(segment->getVelocity().position);
                        vec2 parentVel = vec3_to_vec2(parent->getVelocity().position);
                        vec2 relativeVel = childVel - parentVel;
                        float damping = 5.0f;
                        vec2 dampingForce = (relativeVel * damping)*-1;
                        
                        vec2 totalForce = force + dampingForce;
                        segment->setLinearVelocity(childVel + totalForce * 0.05f);
                        parent->setLinearVelocity(parentVel - totalForce * 0.05f);
                    }
                    return true;
                };
                
                // Regular joint handles rotation
                segment->joint = [segment, segmentLength, ropeSegments]() {
                    g_ptr<Quad> parent = segment->parent;
                    
                    vec2 toParent = parent->getPosition() - segment->getPosition();
                    float dist = toParent.length();
                    
                    if(dist > 0.001f) {
                        vec2 dir = toParent / dist;
                        float targetAngle = atan2(dir.x(), -dir.y());
                        segment->rotation = targetAngle;
                        
                        // First segment rotates head
                        if(segment == ropeSegments[1]) {
                            parent->rotation = targetAngle;
                            parent->updateTransform(false);
                        }
                    }
                    
                    return true;
                };
            }
        }

        phys.quadGravity = 100.0f;
        phys.quadDragCof = 0.98f;

    }
    else if (TESTING== 7) {
        g_ptr<Quad> map = make<Quad>();
        scene->add(map);
        map->setPosition({100, 100});
        map->setScale({400, 400});
        boxes << map;

        map->getGeom()->addSubdivision({vec2(0, 0),vec2(0.5, 0),vec2(0.5, 1),vec2(0, 1)},vec4(1,0,0,1),"left");
        map->getGeom()->addSubdivision({vec2(0.5, 0),vec2(1, 0),vec2(1, 1),vec2(0.5, 1)},vec4(0,0,1,1),"right");

        // auto regions = generateSimpleSubdivisions(15);
        // for(auto& [boundary, color] : regions) {
        //     map->getGeom()->addSubdivision(boundary, color);
        // } 


        for(int i=0;i<3;i++) {
            g_ptr<Quad> box2 = make<Quad>(map->getGeom());
            scene->add(box2);
            box2->scale({100,100});
            box2->setPosition({randf(0,win.x()),randf(0,win.y())});
            boxes << box2;
        }
    }
    else if (TESTING==8) {
        GDSL::helper_test_module::initialize();
        GDSL::add_keyword("make_quad",[](GDSL::exec_context& ctx){
            g_ptr<Quad> box = make<Quad>();
            scene->add(box);
            box->scale({100,100});
            box->setColor(vec4(1,0,0,1));
            box->setPosition({500,500});
            return ctx.node;
        });

        GDSL::add_function("make_quad_pos",[](GDSL::exec_context& ctx){
            g_ptr<Quad> box = make<Quad>();
            scene->add(box);
            box->scale({100,100});
            box->setColor(vec4(1,0,0,1));
            if(ctx.node->children.length()<2) print("WARNING: at least two args required for make_quad_pos!");
            // float x = get_arg<int>(ctx, 0);
            // float y = get_arg<int>(ctx, 1);
            // box->setPosition({x, y});
            return ctx.node;
        });

        g_ptr<GDSL::Frame> frame = GDSL::compile(root()+"/Projects/Testing/src/test.gld");
        Log::Line l; l.start();
        for(int i=0;i<4;i++) {
            g_ptr<Quad> box = make<Quad>();
            scene->add(box);
            box->scale({100,100});
            box->setColor(vec4(1,0,0,1));
            box->setPosition({100, 800});
        }
        print("Time taken: ",l.end()/1000000,"ms"); l.start();
        GDSL::execute_r_nodes(frame);
        print("Time taken: ",l.end()/1000000,"ms"); l.start();
        for(int i=0;i<4;i++) {
            g_ptr<Quad> box = make<Quad>();
            scene->add(box);
            box->scale({100,100});
            box->setColor(vec4(1,0,0,1));
            box->setPosition({100, 800});
        }
        print("Time taken: ",l.end()/1000000,"ms"); l.start();
        GDSL::execute_r_nodes(frame);
        print("Time taken: ",l.end()/1000000,"ms");

        print("Done!");
    }
    else if(TESTING==9) {
        // g_ptr<Quad> box = make<Quad>();
        // scene->add(box);
        // box->scale({50,50});
        // box->setCenter({200,200});
        // boxes << box;


        // g_ptr<Font> font = make<Font>(root()+"/Engine/assets/fonts/source_code.ttf",50);
        // twig = make<Text>(font,scene);
        // g_ptr<Quad> txt = twig->makeText(
        // "0/0",{100,100})[0];
        // boxes << txt;

        // box->children << txt;
        // txt->parent = box;

        // Create arm segments
        g_ptr<Quad> hand = make<Quad>();
        scene->add(hand);
        hand->scale({10, 10});
        hand->setColor(vec4(0, 0, 1, 1));
        boxes << hand;  // hand is boxes[0] - follows cursor with G

        g_ptr<Quad> shoulder = make<Quad>();
        scene->add(shoulder);
        shoulder->scale({20, 20});
        shoulder->setCenter({400, 300});
        shoulder->setColor(vec4(1, 0, 0, 1));
        boxes << shoulder;

        g_ptr<Quad> elbow = make<Quad>();
        scene->add(elbow);
        elbow->scale({15, 15});
        elbow->setColor(vec4(0, 1, 0, 1));
        boxes << elbow;

        // Setup hierarchy
        shoulder->children << elbow;
        elbow->parent = shoulder;
        elbow->children << hand;
        hand->parent = elbow;

        // Store bone lengths
        float upperArmLen = 100.0f;
        float forearmLen = 80.0f;
        elbow->opt_float = upperArmLen;
        hand->opt_float = forearmLen;

        // Hand joint - no constraint, moves freely (you control it with G)
        hand->joint = [hand](){
            if(hand->parent) {
                hand->unlockJoint = true;
                hand->parent->updateTransform();
                hand->unlockJoint = false;
            }
            return true;
        };

        // Elbow joint - solves 2-bone IK
        elbow->joint = [elbow](){
            g_ptr<Quad> shoulder = elbow->parent;
            g_ptr<Quad> hand = elbow->children[0];
            
            shoulder->updateTransform(false);
            hand->updateTransform(false);
            
            vec2 shoulderPos = shoulder->getPosition();
            vec2 handPos = hand->getPosition();
            
            float upperArmLen = elbow->opt_float;
            float forearmLen = hand->opt_float;
            
            vec2 toHand = handPos - shoulderPos;
            float distToHand = toHand.length();
            
            // Clamp if unreachable
            if(distToHand > upperArmLen + forearmLen) {
                distToHand = upperArmLen + forearmLen;
                toHand = toHand.normalized() * distToHand;
            }
            
            // Handle fully extended case
            if(distToHand >= upperArmLen + forearmLen - 0.01f) {
                elbow->position = shoulderPos + toHand.normalized() * upperArmLen;
                return true;
            }
            
            // Law of cosines for elbow angle
            float a = upperArmLen;
            float b = forearmLen;
            float c = distToHand;
            
            float shoulderAngle = acos((a*a + c*c - b*b) / (2.0f*a*c));
            
            vec2 direction = toHand.normalized();
            float baseAngle = atan2(direction.y(), direction.x());
            
            // Place elbow (using positive angle for "elbow down" configuration)
            float elbowAngle = baseAngle + shoulderAngle;
            elbow->position = shoulderPos + vec2(cos(elbowAngle), sin(elbowAngle)) * upperArmLen;
            
            return true;
        };
        
    }
    else if (TESTING==10) {
        g_ptr<Single> cube = make<Single>(make<Model>(makeTestBox(1.0f)));
        scene->add(cube);

        g_ptr<Single> cube2 = make<Single>(make<Model>(makeTestBox(1.0f)));
        scene->add(cube2);
        cube2->setPosition(vec3(10,0,0));

        cubes << cube2;
        cubes << cube;

        cube->parent = cube2;
        cube2->children << cube;

        cube->joint = [cube]() {
                g_ptr<Single> parent = cube->parent;
                vec3 offset = cube->getPosition() - parent->getPosition();
                cube->position = parent->position + offset;
                return true;
            };
    }

    if(TESTING==6) {
        thread->start();
    }

    // phys.thread->logSPS = true;
    // phys.thread->run([&](ScriptContext& ctx){
    //     phys.updatePhysics();
    //     return ctx;
    // },0.008f);
    // phys.thread->start();

    // phys.quadGravity = 300;

    Log::Line timer; timer.start();
    int instance_accumulator = 0;

    S_Tool s_tool;
    s_tool.log_fps = true;
    double m = 0;
    //The parent of selected
    g_ptr<Quad> sel = nullptr;
    //The actual selected
    g_ptr<Quad> sub_sel = nullptr;

    g_ptr<TextEditor> editor = make<TextEditor>();

    if(!boxes.empty()) sel = boxes[0];

    // s_tool.log = [](){
    //     print("Joints/sec: ",std::to_string(totalJointCalls.load()));
    //     totalJointCalls = 0;
    // };

    bool block_input = false;
    start::run(window,d,[&]{
        if(TESTING==0) {
            editor->tick(s_tool.tpf);
            if(pressed(ESCAPE)) {
                editor->close();
                block_input = false;
            }

            if(editor->twig) block_input = true;
        }

        // m = std::sin(s_tool.frame)*100;
        // //test->setPosition(vec2(m,m));
        //    box->move(vec2(m,m));
        if(TESTING==2) {
            //phys.updatePhysics();
        } 
        else if(TESTING==1) {
            // for(auto b : boxes) {
            //     vec2 mov(randf(-10,10),randf(-10,10));
            //     vec2 n_pos = b->getCenter()+mov;
            //     if(n_pos.x()>win.x()) mov.setX(-800);
            //     if(n_pos.x()<=0) mov.setX(800);
            //     if(n_pos.y()>win.y()) mov.setY(-800);
            //     if(n_pos.y()<=0) mov.setY(800);
            //     b->move(mov);
            // }
            phys.updatePhysics();
            for(auto b : boxes) {
                Velocity& mov = b->getVelocity();
                vec2 n_pos = b->getCenter();
                int amt = 400;
                if(n_pos.x()>win.x()) mov.position.setX(randf(-amt,amt));
                if(n_pos.x()<=0) mov.position.setX(randf(-amt,amt));
                if(n_pos.y()>win.y()) mov.position.setY(-randf(-amt,amt));
                if(n_pos.y()<=0) mov.position.setY(randf(-amt,amt));
            }
        } 
        else if(TESTING==4) {
            phys.updatePhysics();
            g_ptr<Quad> line = boxes[1];
            g_ptr<Quad> box = boxes[0];

            vec2 dir = box->getCenter().direction(scene->mousePos2d());
            float angle = std::atan2(dir.y(),dir.x());
            line->rotate(angle);

            line->setPosition(box->getCenter());
        } else if(TESTING==6) {
            phys.updatePhysics();
        }

        if(pressed(MOUSE_RIGHT)||(TESTING==0&&held(MOUSE_LEFT))) {
            sel = scene->nearestElement();
            sub_sel = sel;
            if(sel) {
                // print("Selected at: ",sel->getPosition().to_string());
                if(sel->parent) {
                    while(sel->parent) sel = sel->parent;
                    //print("Transfered to parent at: ",sel->getPosition().to_string());
                }
            }

            if(sub_sel&&TESTING==0) {
                if(!editor->twig) {
                    editor->open(twig,sub_sel);
                    block_input = true;
                }
                else {
                    if(pressed(MOUSE_LEFT)) {
                        editor->click_move(sub_sel->parent);
                    } else { //If just held
                        editor->move_cursour(sub_sel->parent);
                    }
                }
            }
        }

        if(!block_input) {
            if(held(G)) {
                if(TESTING==2) {
                    //vec2 diff = boxes[1]->getPosition() - boxes[0]->getPosition(); 
                    boxes[0]->setCenter(scene->mousePos2d());
                    //boxes[1]->setPosition(boxes[0]->getPosition() + diff);

                    //boxes[0]->setLinearVelocity(boxes[0]->direction(scene->mousePos2d()));
                } 
                else if(TESTING==6) {
                    boxes[0]->setLinearVelocity(boxes[0]->direction(scene->mousePos2d()));
                } 
                else if(TESTING==10) {
                    cubes[0]->setPosition(scene->getMousePos());
                }
                else if (!boxes.empty()||sel) {
                    if(sel) 
                        sel->setCenter(scene->mousePos2d());
                    else
                        boxes[0]->setCenter(scene->mousePos2d());
                }
            }

            if(pressed(MOUSE_LEFT)) {
                if(TESTING==7) {
                    int sub_idx = sampleForSubdivision(boxes[0],scene->mousePos2d());
                    if(sub_idx!=-1) {
                        vec4& data = boxes[0]->getGeom()->subData[sub_idx];
                        if(data == vec4(1,0,0,1))
                            data = vec4(0,0,1,1);
                        else
                            data = vec4(1,0,0,1);
                        print("Clicked: ",boxes[0]->getGeom()->subSlots[sub_idx]);
                    }
                }
            }

            if(held(R)) {
                if(TESTING==2) {
                    boxes[0]->rotateCenter(m+=0.01);

                    // vec2 boxCenter = boxes[0]->getCenter();
                    // vec2 mousePos = scene->mousePos2d();
                    // vec2 dir = boxCenter.direction(mousePos);
                    // float targetAngle = std::atan2(dir.y(), dir.x());
                    // boxes[0]->getVelocity().rotation = vec3(0, 0, targetAngle);
                } else if (sel) {
                    sel->rotateCenter(held(LSHIFT) ? m+=0.01 : m-=0.01);
                }
            }

            if(held(S)) {
                if(TESTING==2) {
                    vec2 center = boxes[0]->getCenter();
                    vec2 pos = scene->mousePos2d();
                    float n = center.distance(pos) * 2.0f;
                    boxes[0]->scaleCenter(vec2(n,n));

                    // vec2 center = boxes[0]->getCenter();
                    // vec2 pos = scene->mousePos2d();
                    // float distance = center.distance(pos);
                    // float targetScale = distance * 2.0f;
                    // float currentScale = boxes[0]->getScale().x();
                    // float scaleRate = (targetScale - currentScale) * 0.005f; // Rate of growth
                    
                    // boxes[0]->getVelocity().scale = vec3(scaleRate, scaleRate, 0);
                } else if (sel) {
                    vec2 center = sel->getCenter();
                    vec2 pos = scene->mousePos2d();
                    float n = center.distance(pos) * 2.0f;
                    sel->scaleCenter(vec2(n,n));
                }
            }

            if(pressed(G)) {
                if(TESTING==2) {
                    if(boxes[0]->getWorldBounds().intersects(boxes[1]->getWorldBounds())) {
                        print("INTERSECTS!");
                    } else {
                        print("No intersection");
                    }
                }
            }

            if(pressed(R)||pressed(S)||pressed(MOUSE_LEFT)) {
                instance_accumulator++;
            }

            if(pressed(E)) {
                if(TESTING==2) {
                    print("----------------------");
                    printnl("Box 0 pos: "); boxes[0]->getPosition().print();
                    printnl("Box 1 pos: "); boxes[1]->getPosition().print();
                    print("Toggle Box2 joint lock ");
                    boxes[1]->unlockJoint = !boxes[1]->unlockJoint;
                } else if(TESTING==0) {
                    double duration = timer.end();
                    print("Accumulated times over ",duration/1000000000,"s: ");
                    list<float> final_times;
                    for(auto b : boxes) {
                        for(int c=0;c<b->opt_list_float.length();c++) {
                            if(c>=final_times.length()) final_times << 0;
                            final_times[c]+=b->opt_list_float[c];
                            b->opt_list_float[c] = 0;
                        }
                    }
                    for(int t=0;t<final_times.length();t++) {
                        print("time ",t,": ",(final_times[t]/boxes.length())/instance_accumulator);
                    }
                    instance_accumulator = 0;
                }
            }

            if(pressed(H)) {
                // if(held(LSHIFT))
                //     twig->removeText(0,twig->chars.length());
                // else
                //     twig->setText("womble");

                

                int anchors = 0;
                list<g_ptr<Quad>> u_anchors;
                for(auto c : twig->chars) {
                    if(c->isAnchor) anchors+=1;
                    if(c->opt_ptr) {
                        if(!u_anchors.has(c->opt_ptr))
                            u_anchors << c->opt_ptr;
                    }
                }
                print("ACHORS: ",anchors);
                print("UANCHORS: ",u_anchors.length());

                // if(s_tool.frame%2==0)
                //     twig->setText("womble");
                // else
                //     twig->setText("wibles");
                // print(twig->chars.length());
                // print(scene->quads.length());
            }
        }
       s_tool.tick();
    });

    return 0;
}