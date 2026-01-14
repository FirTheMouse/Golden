#pragma once

#include<rendering/scene.hpp>
#include<core/thread.hpp>

namespace Golden
{

class aabb_node : public Object {
public:
    g_ptr<aabb_node> left = nullptr;
    g_ptr<aabb_node> right = nullptr;
    g_ptr<Single> entity = nullptr;
    BoundingBox aabb;
    bool mark = false;
};


class Physics : public Object
{
public:
    g_ptr<Scene> scene;
    g_ptr<Thread> thread;

    g_ptr<aabb_node> treeRoot;
    bool treeBuilt = false;

    Physics() {
        thread = make<Thread>();
    }
    Physics(g_ptr<Scene> _scene) : scene(_scene) {
        thread = make<Thread>();
    }

    void rebuildTree() { treeBuilt = false; }

    g_ptr<aabb_node> populateTree(list<g_ptr<Single>>& entities) {
        if(entities.length() == 0) return nullptr;  

        g_ptr<aabb_node> node = make<aabb_node>();

        if(entities.length() == 1) {
            node->entity = entities[0];
            node->aabb = entities[0]->getWorldBounds();
            return node;
        }

        BoundingBox totalBounds;
        for (auto e : entities) {
            totalBounds.expand(e->getWorldBounds());
        }

        char ax = totalBounds.getLongestAxis();

        //This is because I never added a sort to my list...
        std::vector<g_ptr<Single>> temp;
        for (auto e : entities) temp.push_back(e);
        std::sort(temp.begin(), temp.end(), [ax](g_ptr<Single> a, g_ptr<Single> b) {
            vec3 posA = a->getPosition();
            vec3 posB = b->getPosition();
            if(ax=='x')  return posA.x() < posB.x();
            else if(ax=='y')  return posA.y() < posB.y();
            else if(ax=='z')  return posA.z() < posB.z();
            else return false;
        });

        size_t mid = temp.size() / 2;
        std::vector<g_ptr<Single>> leftHalf(temp.begin(), temp.begin() + mid);
        std::vector<g_ptr<Single>> rightHalf(temp.begin() + mid, temp.end());

        list<g_ptr<Single>> left;
        list<g_ptr<Single>> right;
        for(auto l : leftHalf) left << l;
        for(auto r : rightHalf) right << r;

        node->aabb = totalBounds;
        node->left = populateTree(left);
        node->right = populateTree(right);

        return node;
    }

    void buildTree() {
        list<g_ptr<Single>> entities;
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if(i>=scene->transforms.length()) continue;
            if(!scene->active[i]) continue;
            if(p_state_collides(scene->physicsStates[i])) {
                entities << scene->singles[i];
            }
        }
        treeRoot = populateTree(entities);
    }

    void printTree(g_ptr<aabb_node> node,std::string indent = "",int depth = 0) {
        print(" ",depth);
        indent = indent.append(" ");
        depth++;
        if(!node) {
            print(indent,"NULL");
            return;
        }
        if(node->entity) {printnl(indent); node->entity->getPosition().print(); }
        if(node->left) {printnl(indent,"LEFT: "); printTree(node->left,indent,depth);}
        if(node->right) {printnl(indent,"RIGHT: ");printTree(node->right,indent,depth);}
    }

    float calculateInsertionCost(g_ptr<aabb_node> node, BoundingBox& entityBounds) {
        BoundingBox expanded = node->aabb;
        expanded.expand(entityBounds);
        return expanded.volume() - node->aabb.volume();
    }
    
    bool shouldGoLeft(g_ptr<aabb_node> parent, BoundingBox& entityBounds) {
        if(!parent->left) {
            return false;
        } else if(!parent->right) {
            return true;
        }
        float leftCost = calculateInsertionCost(parent->left, entityBounds);
        float rightCost = calculateInsertionCost(parent->right, entityBounds);
        return leftCost < rightCost;
    }

    void updateBoundsAfterRemoval(g_ptr<aabb_node> node) {
        if (!node) return;
        
        if (node->entity) {
            // Leaf node - bounds should match entity
            node->aabb = node->entity->getWorldBounds();
            return;
        }
        
        // Internal node - recalculate bounds from children
        bool hasLeftChild = node->left && (node->left->entity || node->left->left || node->left->right);
        bool hasRightChild = node->right && (node->right->entity || node->right->left || node->right->right);
        
        if (hasLeftChild && hasRightChild) {
            node->aabb = node->left->aabb;
            node->aabb.expand(node->right->aabb);
        } else if (hasLeftChild) {
            node->aabb = node->left->aabb;
        } else if (hasRightChild) {
            node->aabb = node->right->aabb;
        } else {
            node->mark = true;
        }
    }

    bool removeEntityFromTree(g_ptr<aabb_node> node, g_ptr<Single> entity) {
        if (!node) return false;
        
        if (node->entity == entity) {
            node->entity = nullptr;
            return true;
        }
        
        if (removeEntityFromTree(node->left, entity) || 
            removeEntityFromTree(node->right, entity)) {
            updateBoundsAfterRemoval(node);
            return true;
        }
        
        return false;
    }

    g_ptr<aabb_node> cleanupMarkedNodes(g_ptr<aabb_node> node) {
        if (!node) return nullptr;
        
        if (node->left) {
            node->left = cleanupMarkedNodes(node->left);
        }
        if (node->right) {
            node->right = cleanupMarkedNodes(node->right);
        }
        
        if (node->mark) {
            if (node->left && !node->right) {
                return node->left;
            } else if (node->right && !node->left) {
                return node->right;
            } else {
                return nullptr; 
            }
        }
        
        if (!node->entity && !node->left && !node->right) {
            return nullptr;
        }
        
        if (!node->entity && node->left && !node->right) {
            return node->left;
        }
        
        if (!node->entity && !node->left && node->right) {
            return node->right;
        }
        
        return node;
    }

    void insertIntoSubtree(g_ptr<aabb_node> targetNode, g_ptr<Single> entity) {
        if(!targetNode) {
            print("INVALID INSERTION POINT");
            return;
        }
        BoundingBox entityBounds = entity->getWorldBounds();
        if(targetNode->entity) {
            g_ptr<Single> existingEntity = targetNode->entity;
            targetNode->entity = nullptr;
            
            targetNode->left = make<aabb_node>();
            targetNode->left->entity = existingEntity;
            targetNode->left->aabb = existingEntity->getWorldBounds();
            
            targetNode->right = make<aabb_node>();
            targetNode->right->entity = entity;
            targetNode->right->aabb = entityBounds;
            
            targetNode->aabb = targetNode->left->aabb;
            targetNode->aabb.expand(targetNode->right->aabb);
        } else {
            if (shouldGoLeft(targetNode, entityBounds)) {
                insertIntoSubtree(targetNode->left, entity);
            } else {
                if(targetNode->right) {
                    insertIntoSubtree(targetNode->right, entity);
                } else {
                    targetNode->entity = entity;
                    targetNode->aabb = entityBounds;
                }
            }
            if(targetNode->left) {
                targetNode->aabb = targetNode->left->aabb;
                if(targetNode->right) 
                    targetNode->aabb.expand(targetNode->right->aabb);
            } else {
                if(targetNode->right) 
                    targetNode->aabb = targetNode->right->aabb;
            }
        
        }
    }

    void collectMoved(g_ptr<aabb_node> node,list<g_ptr<Single>>& moved) {
        if(!node) return;
        if(node->entity) {
            BoundingBox entityBounds = node->entity->getWorldBounds();
            if(node->aabb.contains(entityBounds.getCenter())) {
                return; 
            } else {
                moved << node->entity;
            }
        } else {
            if(node->left) collectMoved(node->left, moved);
            if(node->right) collectMoved(node->right, moved);
        }
    }

    void relocateEntity(g_ptr<Single> entity) {
        removeEntityFromTree(treeRoot, entity);
        insertIntoSubtree(treeRoot, entity);
    }
 
    void updateTreeInPlace() {
        list<g_ptr<Single>> movedEntities;
        collectMoved(treeRoot, movedEntities);
        
        for (auto entity : movedEntities) {
            relocateEntity(entity);
        }

        treeRoot = cleanupMarkedNodes(treeRoot);
    }

    void queryTree(g_ptr<aabb_node> node, BoundingBox& queryBounds, list<g_ptr<Single>>& results) {
        if (!node) return;

        // Early rejection - if query doesn't intersect this node, skip entire subtree
        if (!node->aabb.intersects(queryBounds)) return;

        if (node->entity) {
            results.push(node->entity);
        } else {
            queryTree(node->left, queryBounds, results);
            queryTree(node->right, queryBounds, results);
        }
    }

    struct collisionData3d {
        g_ptr<Single> first;
        g_ptr<Single> second;
        vec3 mtv;
    };
    list<std::pair<g_ptr<Single>, g_ptr<Single>>> AABB_generate3dCollisonPairs() {
        list<std::pair<g_ptr<Single>, g_ptr<Single>>> collisionPairs;
        
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if (!scene->active[i] || !p_state_collides(scene->physicsStates[i])) continue;
            
            g_ptr<Single> entityA = scene->singles[i];
            BoundingBox boundsA = entityA->getWorldBounds();
            
            // Query tree for potential collision candidates
            list<g_ptr<Single>> candidates;
            queryTree(treeRoot, boundsA, candidates);
            
            // Check collision layers and do narrow phase collision
            for (auto entityB : candidates) {
                if (entityA == entityB) continue; // Don't collide with self
                
                // Check collision layers
                CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
                CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
                if (!layersA.canCollideWith(layersB)) continue;
                
                if (boundsA.intersects(entityB->getWorldBounds())) {
                    collisionPairs.push(std::pair{entityA,entityB});
                }
            }
        }
        return collisionPairs;
    }
    list<std::pair<g_ptr<Single>, g_ptr<Single>>> naive_generate3dCollisonPairs() {
        list<std::pair<g_ptr<Single>, g_ptr<Single>>> collisionPairs;
        
        // Collect all active physics entities
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if (!scene->active[i] || !p_state_collides(scene->physicsStates[i])) continue;
            for(int j = 0; j<scene->transforms.length(); ++j) {
                if (!scene->active[j] ||  !p_state_collides(scene->physicsStates[j])) continue;
                g_ptr<Single> entityA = scene->singles[i];
                g_ptr<Single> entityB = scene->singles[j];
                
                // Check collision layers
                CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
                CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
                if (!layersA.canCollideWith(layersB)) continue;
                
                // Narrow phase collision detection
                BoundingBox boundsA = entityA->getWorldBounds();
                BoundingBox boundsB = entityB->getWorldBounds();
                
                if (boundsA.intersects(boundsB)) {
                    collisionPairs.push(std::pair{entityA,entityB});
                }
            }
        }
        return collisionPairs;
    }
    void handle3dCollison(const list<std::pair<g_ptr<Single>, g_ptr<Single>>>& pairs) {
        for(auto& p : pairs) {
            int i = p.first->ID;
            int s = p.second->ID;
            vec3 thisCenter = scene->singles[i]->getPosition();
            vec3 otherCenter = scene->singles[s]->getPosition();
            
            auto thisBounds = scene->singles[i]->getWorldBounds();
            auto otherBounds = scene->singles[s]->getWorldBounds();
            
            vec3 overlap = vec3(
                std::min(thisBounds.max.x, otherBounds.max.x) - std::max(thisBounds.min.x, otherBounds.min.x),
                std::min(thisBounds.max.y, otherBounds.max.y) - std::max(thisBounds.min.y, otherBounds.min.y),
                std::min(thisBounds.max.z, otherBounds.max.z) - std::max(thisBounds.min.z, otherBounds.min.z)
            );
            
            vec3 normal;
            if(overlap.x() < overlap.y() && overlap.x() < overlap.z()) {
                normal = vec3(thisCenter.x() > otherCenter.x() ? 1 : -1, 0, 0);
            } else if(overlap.y() < overlap.z()) {
                normal = vec3(0, thisCenter.y() > otherCenter.y() ? 1 : -1, 0);
            } else {
                normal = vec3(0, 0, thisCenter.z() > otherCenter.z() ? 1 : -1);
            }
            
            Velocity& velocity = GET(scene->velocities,i);
            vec3 currentVel = velocity.position;
            float velocityAlongNormal = currentVel.dot(normal);
            //cs++;
            if(velocityAlongNormal < 0) {
                velocity.position = currentVel - (normal * velocityAlongNormal);
                velocity.position = velocity.position*0.78; //Drag
            }
        }
    }
    
    struct collisionData2d {
        g_ptr<Quad> first;
        g_ptr<Quad> second;
        vec2 mtv;
    };
    list<std::pair<g_ptr<Quad>, g_ptr<Quad>>> naive_generate2dCollisionPairs() {
        list<std::pair<g_ptr<Quad>, g_ptr<Quad>>> collisionPairs;
        
        for (size_t i = 0; i < scene->guiTransforms.length(); ++i) {
            if (!scene->quadActive[i] || !p_state_collides(scene->quadPhysicsStates[i])) continue;
            for(int j = 0; j<scene->guiTransforms.length(); ++j) {
                if (!scene->quadActive[j] || !p_state_collides(scene->quadPhysicsStates[j])) continue;
                if(i==j) continue;
                g_ptr<Quad> entityA = scene->quads[i];
                g_ptr<Quad> entityB = scene->quads[j];
                
                CollisionLayer& layersA = scene->quadCollisonLayers.get(entityA->ID);
                CollisionLayer& layersB = scene->quadCollisonLayers.get(entityB->ID);
                if (!layersA.canCollideWith(layersB)) continue;

                if (entityA->intersects(entityB)) {
                    collisionPairs.push(std::pair{entityA,entityB});
                }
            }
        }
        return collisionPairs;
    }
    list<collisionData2d> naive_generate2dCollisonData() {
        list<collisionData2d> collisionPairs;
        
        for (size_t i = 0; i < scene->guiTransforms.length(); ++i) {
            if (!scene->quadActive[i] || !p_state_collides(scene->quadPhysicsStates[i])) continue;
            for(int j = 0; j<scene->guiTransforms.length(); ++j) {
                if (!scene->quadActive[j] || !p_state_collides(scene->quadPhysicsStates[j])) continue;
                if(i==j) continue;
                g_ptr<Quad> entityA = scene->quads[i];
                g_ptr<Quad> entityB = scene->quads[j];
                
                CollisionLayer& layersA = scene->quadCollisonLayers.get(entityA->ID);
                CollisionLayer& layersB = scene->quadCollisonLayers.get(entityB->ID);
                if (!layersA.canCollideWith(layersB)) continue;

                vec2 mtv;
                if (entityA->intersects(entityB,mtv)) {
                    collisionData2d cd;
                    cd.first = entityA;
                    cd.second = entityB;
                    cd.mtv = mtv;
                    collisionPairs.push(cd);
                }
            }
        }
        return collisionPairs;
    }
    void handle2dCollision(g_ptr<Quad> g,vec2 mtv) {
        vec3& vel3 = g->getVelocity().position;
        vec2 push = mtv*-1;
        g->setCenter(g->getCenter() + push);
        vec2 n;
        if (push.x() != 0.0f) n = vec2((push.x() < 0.0f) ? -1.0f : 1.0f, 0.0f);
        else if (push.y() != 0.0f) n = vec2(0.0f, (push.y() < 0.0f) ? -1.0f : 1.0f);
        else n = vec2(0.0f, 0.0f);
        vec2 v(vel3.x(), vel3.y());
        float vn = v.dot(n);
        if (vn < 0.0f) v -= n * vn;
        vel3.x(v.x());
        vel3.y(v.y());
    }
    void handle2dCollision(collisionData2d p) {
        handle2dCollision(p.first,p.mtv);
    }

  

    bool enableCollisons = true;
    float dragCof = 0.99f;
    float gravity = 4.8f;

    bool enableQuadCollisons = true;
    float quadDragCof = 0.99f;
    float quadGravity = 48.0f;

    enum SAMPLE_METHOD {
        NCOL, NAIVE, AABB //GRID <- Add this later!
    };

    SAMPLE_METHOD collisonMethod = SAMPLE_METHOD::AABB;
    SAMPLE_METHOD quadCollisonMethod = SAMPLE_METHOD::NCOL;

    void enableSinglePhysics() {
        dragCof = 0.99f;
        gravity = 48.0f;
        enableCollisons = true;
    }

    void disableSinglePhysics() {
        dragCof = 0;
        gravity = 0;
        enableCollisons = false;
    }

    void enableQuadPhysics() {
        quadDragCof = 0.99f;
        quadGravity = 48.0f;
        enableQuadCollisons = true;
    }

    void disableQuadPhysics() {
        quadDragCof = 0;
        quadGravity = 0;
        enableQuadCollisons = false;
    }

    void updatePhysics()
    {       

        //This is the pre-loop check for global varients, like gravity and drag and such

        //3d preloop
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if(i>=scene->transforms.length()) continue;
            if(!GET(scene->active,i)) continue;
            Velocity& velocity = GET(scene->velocities,i);
            if(p_state_preloops(GET(scene->physicsStates,i)))
            {
                if(gravity!=0) {
                    velocity.position.addY(-gravity*thread->getSpeed()); //Somtimes always multiply by 0.016f as a constant: experiment with it.
                }
                if(dragCof!=0) {
                    velocity.position = velocity.position*dragCof;
                }
            }
        }

        //2d preloop
        for (size_t i = 0; i < scene->guiTransforms.length(); ++i) {
            if(i>=scene->guiTransforms.length()) continue;
            if(!GET(scene->quadActive,i)) continue;
            Velocity& velocity = GET(scene->quadVelocities,i);
            if(p_state_preloops(GET(scene->quadPhysicsStates,i)))
            {
                if(quadGravity!=0) {
                    velocity.position.addY(quadGravity*thread->getSpeed()); //Somtimes always multiply by 0.016f as a constant: experiment with it.
                }
                if(quadDragCof!=0) {
                    velocity.position = velocity.position*quadDragCof;
                }
            }
        }


        if(collisonMethod==SAMPLE_METHOD::AABB) {
            if(!treeBuilt) {
                buildTree();
                treeBuilt = true;
            } else {
                updateTreeInPlace();
                handle3dCollison(AABB_generate3dCollisonPairs());
            }
        } 
        else if (collisonMethod==SAMPLE_METHOD::NAIVE) {
            handle3dCollison(naive_generate3dCollisonPairs());
        }

        if(quadCollisonMethod==SAMPLE_METHOD::NAIVE) {
            for(auto& c : naive_generate2dCollisonData()) {
                handle2dCollision(c);
            }
        } else if (quadCollisonMethod==SAMPLE_METHOD::AABB) {
            print("AABB is not yet implmented for quad collisons!");
        }

        //3d physics pass
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if(i>=scene->transforms.length()) continue;
            if(!GET(scene->active,i)) continue;
            P_State p = GET(scene->physicsStates,i);

            if(p==P_State::DETERMINISTIC)
            {
                AnimState& a = GET(scene->animStates,i);
    
                if (a.duration <= 0.0f) goto c_check; // No animation active
        
                a.elapsed = std::min(a.elapsed + thread->getSpeed(), a.duration);
                float alpha = a.elapsed / a.duration;
        
               GET(scene->transforms,i) = interpolateMatrix(
                    GET(scene->transforms,i),
                    GET(scene->endTransforms,i),
                    alpha
                );
        
                if (a.elapsed >= a.duration) {
                    GET(scene->transforms,i) = GET(scene->endTransforms,i);
                    a.duration = 0.0f; // Animation complete
                    a.elapsed = 0.0f;
                }
            }
            else if(p_state_uses_velocity(p))
            {
                glm::mat4& transform = GET(scene->transforms,i);
                Velocity& velocity = GET(scene->velocities,i);
                float velocityScale = 1.0f;

                if(velocity.length()==0) goto c_check;

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
        }

        

        //2d physics pass
        for (size_t i = 0; i < scene->quadActive.length(); ++i) {
            if(i>=scene->quadActive.length()) continue;
            if(!GET(scene->quadActive,i)) continue;
            P_State p = GET(scene->quadPhysicsStates,i);

            if(p==P_State::DETERMINISTIC)
            {
                AnimState& a = GET(scene->quadAnimStates,i);
    
                if (a.duration <= 0.0f) goto c_check2; // No animation active
        
                a.elapsed = std::min(a.elapsed + thread->getSpeed(), a.duration);
                float alpha = a.elapsed / a.duration;
                
                GET(scene->guiTransforms,i) = interpolateMatrix(
                    GET(scene->guiTransforms,i),
                    GET(scene->guiEndTransforms,i),
                    alpha
                );
        
                if (a.elapsed >= a.duration) {
                    GET(scene->guiTransforms,i) = GET(scene->guiEndTransforms,i);
                    a.duration = 0.0f; // Animation complete
                    a.elapsed = 0.0f;
                }
            }
            else if(p_state_uses_velocity(p))
            {
                glm::mat4& transform = GET(scene->guiTransforms,i);
                Velocity& velocity = GET(scene->quadVelocities,i);
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
            scene->quads.get(i)->getPosition();
            c_check2:;

            if(p!=P_State::NONE)
            {
                //Final physics pass for whatever quads need
            }
        }


    

    }


};

}



    //This should be moved into Single's own methods eventually, just like it is for Quad.
    // collisionData3d generate3dCollisonDataPoint(g_ptr<Single> entityA, g_ptr<Single> entityB) {
    //     collisionData3d cd; 
    //     cd.first = nullptr;
    
    //     // Check collision layers
    //     CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
    //     CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
    //     if (!layersA.canCollideWith(layersB)) return cd;
        
    //     BoundingBox boundsA = entityA->getWorldBounds();
    //     BoundingBox boundsB = entityB->getWorldBounds();
        
    //     if (boundsA.intersects(boundsB)) {
    //         vec3 thisCenter = entityA->getPosition();
    //         vec3 otherCenter = entityB->getPosition();
            
    //         //BROKEN:
    //         vec3 overlap = vec3(
    //             std::min(boundsA.max.x, boundsB.max.x) - std::max(boundsA.min.x, boundsB.min.x),
    //             std::min(boundsA.max.y, boundsB.max.y) - std::max(boundsA.min.y, boundsB.min.y),
    //             std::min(boundsA.max.z, boundsB.max.z) - std::max(boundsA.min.z, boundsB.min.z)
    //         );
            
    //         vec3 mtv;
    //         if(overlap.x() < overlap.y() && overlap.x() < overlap.z()) {
    //             float sign = thisCenter.x() > otherCenter.x() ? 1.0f : -1.0f;
    //             mtv = vec3(overlap.x() * sign, 0, 0);
    //         } else if(overlap.y() < overlap.z()) {
    //             float sign = thisCenter.y() > otherCenter.y() ? 1.0f : -1.0f;
    //             mtv = vec3(0, overlap.y() * sign, 0);
    //         } else {
    //             float sign = thisCenter.z() > otherCenter.z() ? 1.0f : -1.0f;
    //             mtv = vec3(0, 0, overlap.z() * sign);
    //         }
    
    //         cd.first = entityA;
    //         cd.second = entityB;
    //         cd.mtv = mtv;
    //     }
    //     return cd;
    // }

    // list<collisionData3d> generate3dCollisionDataNaive() {
    //     list<collisionData3d> collisionPairs;
        
    //     for (size_t i = 0; i < scene->transforms.length(); ++i) {
    //         if (!scene->active[i] || !p_state_collides(scene->physicsStates[i])) continue;
    //         for(int j = 0; j<scene->transforms.length(); ++j) {
    //             if (!scene->active[j] || !p_state_collides(scene->physicsStates[j])) continue;
    //             if(i==j) continue;
    //             collisionData3d point = generate3dCollisonDataPoint(GET(scene->singles,i),GET(scene->singles,j));
    //             if(point.first) { //i.e if we returned a valid data point
    //                 collisionPairs << point;
    //             }
    //         }
    //     }
    //     return collisionPairs;
    // }
    // list<collisionData3d> generate3dCollisonDataAABB() {
    //     list<collisionData3d> collisionPairs;
        
    //     for (size_t i = 0; i < scene->transforms.length(); ++i) {
    //         if (!scene->active[i] || !p_state_collides(scene->physicsStates[i])) continue;
            
    //         g_ptr<Single> entityA = scene->singles[i];
    //         BoundingBox boundsA = entityA->getWorldBounds();
            
    //         // Query tree for potential collision candidates
    //         list<g_ptr<Single>> candidates;
    //         queryTree(treeRoot, boundsA, candidates);
            
    //         // Check collision layers and do narrow phase collision
    //         for (auto entityB : candidates) {
    //             if (entityA == entityB) continue; // Don't collide with self
                
    //             // // Check collision layers
    //             // CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
    //             // CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
    //             // if (!layersA.canCollideWith(layersB)) continue;
                
    //             // if (boundsA.intersects(entityB->getWorldBounds())) {
    //             //     collisionPairs.push(std::pair{entityA,entityB});
    //             // }

    //             collisionData3d point = generate3dCollisonDataPoint(entityA,entityB);
    //             if(point.first) { //i.e if we returned a valid data point
    //                 collisionPairs << point;
    //             }

    //         }
    //     }
    //     return collisionPairs;
    // }
    // void handle3dCollision(g_ptr<Single> g, vec3 mtv) {
    //     vec3 n = mtv.normalized();
        
    //     Velocity& velocity = scene->velocities.get(g->ID);
    //     vec3 currentVel = velocity.position;
    //     float velocityAlongNormal = currentVel.dot(n);
        
    //     if(velocityAlongNormal < 0) {
    //         velocity.position = currentVel - (n * velocityAlongNormal);
    //         velocity.position = velocity.position * 0.78f;
    //     }
    // }
    // void handle3dCollision(collisionData3d cd) {
    //     handle3dCollision(cd.first, cd.mtv);
    // }
    // void handle3dCollision(collisionData3d cd) {
    //     int i = cd.first->ID;
    //     int s = cd.second->ID;
    //     vec3 thisCenter = scene->singles[i]->getPosition();
    //     vec3 otherCenter = scene->singles[s]->getPosition();
        
    //     auto thisBounds = scene->singles[i]->getWorldBounds();
    //     auto otherBounds = scene->singles[s]->getWorldBounds();
        
    //     vec3 overlap = vec3(
    //         std::min(thisBounds.max.x, otherBounds.max.x) - std::max(thisBounds.min.x, otherBounds.min.x),
    //         std::min(thisBounds.max.y, otherBounds.max.y) - std::max(thisBounds.min.y, otherBounds.min.y),
    //         std::min(thisBounds.max.z, otherBounds.max.z) - std::max(thisBounds.min.z, otherBounds.min.z)
    //     );
        
    //     vec3 normal;
    //     if(overlap.x() < overlap.y() && overlap.x() < overlap.z()) {
    //         normal = vec3(thisCenter.x() > otherCenter.x() ? 1 : -1, 0, 0);
    //     } else if(overlap.y() < overlap.z()) {
    //         normal = vec3(0, thisCenter.y() > otherCenter.y() ? 1 : -1, 0);
    //     } else {
    //         normal = vec3(0, 0, thisCenter.z() > otherCenter.z() ? 1 : -1);
    //     }
        
    //     Velocity& velocity = GET(scene->velocities, i);
    //     vec3 currentVel = velocity.position;
    //     float velocityAlongNormal = currentVel.dot(normal);
        
    //     if(velocityAlongNormal < 0) {
    //         velocity.position = currentVel - (normal * velocityAlongNormal);
    //         velocity.position = velocity.position * 0.78;
    //     }
    // }