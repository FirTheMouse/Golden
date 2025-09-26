#pragma once

#include<rendering/scene.hpp>
#include<core/thread.hpp>

namespace Golden
{

struct Collider
{
    BoundingBox bb;
    g_ptr<S_Object> obj;
    Collider(g_ptr<Model> model,g_ptr<Single> sobj) {
        // std::string from = "physics::collider::15";
        // if(sobj->has("from")) from = sobj->get<std::string>("from")+" | "+from;
        // sobj->set<std::string>("from",from);
        bb = model->localBounds;
        bb.transform(sobj->getTransform()); 
        obj = sobj;
    }

    bool collides(const Collider& other)
    {
        // print("A "); 
        // vec3(bb.getCenter()).print();
        // print("B "); 
        // vec3(other.bb.getCenter()).print();
        // print(bb.intersects(other.bb));
        return bb.intersects(other.bb);
    }

};



class Physics : public Object
{
public:
    g_ptr<Scene> scene;
    g_ptr<Thread> thread;

    Physics() {
        thread = make<Thread>();
    }
    Physics(g_ptr<Scene> _scene) : scene(_scene) {
        thread = make<Thread>();
    }

    void updatePhysics()
    {       
        //DELETE THIS LATER: ONLY FOR TEMPORARY TESTING
        std::vector<Collider> collisonBuffer;
        //3d physics pass
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if(i>=scene->transforms.length()) continue;
            if(!scene->active.get(i,"physics::updatePhysics::103")) continue;
            P_State p = scene->physicsStates.get(i,"physics::updatePhysics::116");

            if(p==P_State::DETERMINISTIC)
            {
                AnimState& a = scene->animStates.get(i,"physics::updatePhysics::109");
    
                if (a.duration <= 0.0f) goto c_check; // No animation active
        
                a.elapsed = std::min(a.elapsed + thread->getSpeed(), a.duration);
                float alpha = a.elapsed / a.duration;
        
                scene->transforms.get(i,"physics::updatePhysics::116") = interpolateMatrix(
                    scene->transforms.get(i,"physics::updatePhysics::117"),
                    scene->endTransforms.get(i,"physics::updatePhysics::118"),
                    alpha
                );
        
                if (a.elapsed >= a.duration) {
                    scene->transforms.get(i,"physics::updatePhysics::123") = scene->endTransforms.get(i,"physics::updatePhysics::123::2");
                    a.duration = 0.0f; // Animation complete
                    a.elapsed = 0.0f;
                }
            }
            else if(p==P_State::ACTIVE)
            {
                glm::mat4& transform = scene->transforms.get(i,"physics::141");
                Velocity& velocity = scene->velocities.get(i,"physics::142");
                float velocityScale = 1.0f;

                if(velocity.length()==0) goto c_check;

                //Add checking for if the velocity in this regard is just all zero in the future
                //To optimize

                // --- Position ---
                glm::vec3 pos = glm::vec3(transform[3]);
                pos += vec3(velocity.position * thread->getSpeed() * velocityScale).toGlm();
                transform[3] = glm::vec4(pos, 1.0f);
                
                // --- Rotation ---
                glm::vec3 rotVel = vec3(velocity.rotation * thread->getSpeed() * velocityScale).toGlm();
                if (glm::length(rotVel) > 0.0001f) {
                    glm::mat4 rotationDelta = glm::mat4(1.0f);
                    rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                    rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                    rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                    transform = transform * rotationDelta; // Apply rotation in object space
                }
                
                // --- Scale ---
                glm::vec3 scaleChange = vec3(velocity.scale * thread->getSpeed() * velocityScale).toGlm();
                if (glm::length(scaleChange) > 0.0001f) {
                    glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                    transform = transform * scaleDelta; // Apply after rotation
                }
            }

            c_check:;

            if(p!=P_State::NONE)
            {
                if(scene->transforms.length()<=i)
                {
                    // std::cerr << "[WARN Physics 134] Attempted to retrive a deleted object!";
                    // if(!scene->getObject(scene->models[i]->UUIDPTR)->isActive()) std::cerr << " but caught tombstone";
                    // std::cerr << "\n";
                }
                else 
                {
            Collider c(scene->physicsModels.get(i,"physics::185"),
            scene->singles[i]);
            collisonBuffer.push_back(c);
                }
            }
        }

        // for(Collider cA : collisonBuffer) {
        //     for(Collider cB : collisonBuffer)
        //     {
        //         if(cA.obj==cB.obj) continue;
        //         if(cA.collides(cB))
        //         {
        //             scene->velocities[cA.obj->ID] = Velocity();
        //         }
        //     }
        // }

        for (size_t i = 0; i < collisonBuffer.size(); ++i) {
            for (size_t j = i + 1; j < collisonBuffer.size(); ++j) {
                Collider cA = collisonBuffer[i];
                Collider cB = collisonBuffer[j];
                if(cA.collides(cB))
                {
                    // ScriptContext ctx;
                    // if(cA.obj->isActive())
                    // if(auto sA = g_dynamic_pointer_cast<Scriptable>(cA.obj))
                    // {
                    //     ctx.set<g_ptr<S_Object>>("with",cB.obj);
                    //     sA->run("onCollide",ctx);
                    // }

                    // if(cB.obj->isActive()) 
                    // if(auto sB = g_dynamic_pointer_cast<Scriptable>(cB.obj))
                    // {
                    //     ctx.set<g_ptr<S_Object>>("with",cA.obj);
                    //     sB->run("onCollide",ctx);
                    // }

                    // scene->velocities[cA.obj->ID] = Velocity();
                    // scene->velocities[cB.obj->ID] = Velocity();
                }
            }
        }

        //2d physics pass
        for (size_t i = 0; i < scene->quadActive.length(); ++i) {
            if(i>=scene->quadActive.length()) continue;
            if(!scene->quadActive.get(i,"physics::updatePhysics::227")) continue;
            P_State p = scene->quadPhysicsStates.get(i,"physics::updatePhysics::228");

            if(p==P_State::DETERMINISTIC)
            {
                AnimState& a = scene->quadAnimStates.get(i,"physics::updatePhysics::232");
    
                if (a.duration <= 0.0f) goto c_check2; // No animation active
        
                a.elapsed = std::min(a.elapsed + thread->getSpeed(), a.duration);
                float alpha = a.elapsed / a.duration;
        
                scene->guiTransforms.get(i,"physics::updatePhysics::239") = interpolateMatrix(
                    scene->guiTransforms.get(i,"physics::updatePhysics::240"),
                    scene->guiEndTransforms.get(i,"physics::updatePhysics::241"),
                    alpha
                );
        
                if (a.elapsed >= a.duration) {
                    scene->guiTransforms.get(i,"physics::updatePhysics::246") = scene->guiEndTransforms.get(i,"physics::updatePhysics::246::2");
                    a.duration = 0.0f; // Animation complete
                    a.elapsed = 0.0f;
                }
            }
            else if(p==P_State::ACTIVE)
            {
                glm::mat4& transform = scene->guiTransforms.get(i,"physics::153");
                Velocity& velocity = scene->quadVelocities.get(i,"physics::254");
                float velocityScale = 1.0f;

                //Add checking for if the velocity in this regard is just all zero in the future
                //To optimize

                // --- Position ---
                glm::vec3 pos = glm::vec3(transform[3]);
                pos += vec3(velocity.position * thread->getSpeed() * velocityScale).toGlm();
                transform[3] = glm::vec4(pos, 1.0f);
                
                // --- Rotation ---
                glm::vec3 rotVel = vec3(velocity.rotation * thread->getSpeed() * velocityScale).toGlm();
                if (glm::length(rotVel) > 0.0001f) {
                    glm::mat4 rotationDelta = glm::mat4(1.0f);
                    rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                    rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                    rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                    transform = transform * rotationDelta; // Apply rotation in object space
                }
                
                // --- Scale ---
                glm::vec3 scaleChange = vec3(velocity.scale * thread->getSpeed() * velocityScale).toGlm();
                if (glm::length(scaleChange) > 0.0001f) {
                    glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                    transform = transform * scaleDelta; // Apply after rotation
                }
            }

            c_check2:;

            if(p!=P_State::NONE)
            {
                //Final physics pass for whatever quads need
            }
        }


    

    }


};

}