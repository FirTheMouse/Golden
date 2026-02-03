#pragma once

#include<rendering/scene.hpp>
#include<core/thread.hpp>
#include<core/numGrid.hpp>

namespace Golden
{

class aabb_node : public Object {
public:
    g_ptr<aabb_node> left = nullptr;
    g_ptr<aabb_node> right = nullptr;
    g_ptr<S_Object> entity = nullptr;
    BoundingBox aabb;
    bool mark = false;
};


class Physics : public Object
{
public:
    g_ptr<Scene> scene = nullptr;
    g_ptr<Thread> thread = nullptr;

    g_ptr<aabb_node> treeRoot3d = nullptr;
    g_ptr<aabb_node> treeRoot2d = nullptr;
    bool treeBuilt3d = false;
    bool treeBuilt2d = false;

    g_ptr<NumGrid> grid = nullptr;

    Physics() {
        thread = make<Thread>();
        thread->setSpeed(0.016f);
    }
    Physics(g_ptr<Scene> _scene) : scene(_scene) {
        thread = make<Thread>();
        thread->setSpeed(0.016f);
    }

    void rebuildTree() { treeBuilt2d = false; treeBuilt3d = false; }

    g_ptr<aabb_node> populateTree(list<g_ptr<S_Object>>& entities) {
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
        std::vector<g_ptr<S_Object>> temp;
        for (auto e : entities) temp.push_back(e);
        std::sort(temp.begin(), temp.end(), [ax](g_ptr<S_Object> a, g_ptr<S_Object> b) {
            vec3 posA, posB;
            if (auto sEntity = g_dynamic_pointer_cast<Single>(a)) {
                posA = sEntity->getPosition();
            } else if (auto qEntity = g_dynamic_pointer_cast<Quad>(a)) {
                posA = vec3(qEntity->getPosition(), 0.0f); // Convert vec2 to vec3
            }
            
            if (auto sEntity = g_dynamic_pointer_cast<Single>(b)) {
                posB = sEntity->getPosition();
            } else if (auto qEntity = g_dynamic_pointer_cast<Quad>(b)) {
                posB = vec3(qEntity->getPosition(), 0.0f); // Convert vec2 to vec3
            }
            
            if(ax == 'x')  return posA.x() < posB.x();
            else if(ax == 'y')  return posA.y() < posB.y();
            else if(ax == 'z')  return posA.z() < posB.z();
            else return false;
        });

        size_t mid = temp.size() / 2;
        std::vector<g_ptr<S_Object>> leftHalf(temp.begin(), temp.begin() + mid);
        std::vector<g_ptr<S_Object>> rightHalf(temp.begin() + mid, temp.end());

        list<g_ptr<S_Object>> left;
        list<g_ptr<S_Object>> right;
        for(auto l : leftHalf) left << l;
        for(auto r : rightHalf) right << r;

        node->aabb = totalBounds;
        node->left = populateTree(left);
        node->right = populateTree(right);

        return node;
    }

    void buildTree3d() {
        list<g_ptr<S_Object>> entities;
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if(i>=scene->transforms.length()) continue;
            //May need to add these back, but doing this so we can do queries even on free singles.
            // if(!scene->active[i]) continue;
            // if(p_state_collides(scene->physicsStates[i])) {
            //     entities << scene->singles[i];
            // }
            if(GET(scene->physicsStates,i)!=P_State::NONE) {
                entities << scene->singles[i];
            }
        }
        treeRoot3d = populateTree(entities);
    }

    void buildTree2d() {
        list<g_ptr<S_Object>> entities;
        for (size_t i = 0; i < scene->guiTransforms.length(); ++i) {
            if(i>=scene->guiTransforms.length()) continue;
            // if(!scene->quadActive[i]) continue;
            // if(p_state_collides(scene->quadPhysicsStates[i])) {
            //     entities << scene->quads[i];
            // }
            if(GET(scene->quadPhysicsStates,i)!=P_State::NONE) {
                entities << scene->quads[i];
            }
        }
        treeRoot2d = populateTree(entities);
    }

    void printTree(g_ptr<aabb_node> node,std::string indent = "",int depth = 0) {
        print(" ",depth);
        indent = indent.append(" ");
        depth++;
        if(!node) {
            print(indent,"NULL");
            return;
        }
        if(node->entity) {
            if (auto sEntity = g_dynamic_pointer_cast<Single>(node->entity))  {printnl(indent); sEntity->getPosition().print(); } 
            if(auto qEntity = g_dynamic_pointer_cast<Quad>(node->entity))  {printnl(indent); qEntity->getPosition().print(); }
        }
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

    bool removeEntityFromTree(g_ptr<aabb_node> node, g_ptr<S_Object> entity) {
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

    void insertIntoSubtree(g_ptr<aabb_node> targetNode, g_ptr<S_Object> entity) {
        if(!targetNode) {
            print("INVALID INSERTION POINT");
            return;
        }
        BoundingBox entityBounds = entity->getWorldBounds();
        if(targetNode->entity) {
            g_ptr<S_Object> existingEntity = targetNode->entity;
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

    void collectMoved(g_ptr<aabb_node> node,list<g_ptr<S_Object>>& moved) {
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

    void relocateEntity(g_ptr<aabb_node> root, g_ptr<S_Object> entity) {
        removeEntityFromTree(root, entity);
        insertIntoSubtree(root, entity);
    }
 
    void updateTreeInPlace(g_ptr<aabb_node> root) {
        list<g_ptr<S_Object>> movedEntities;
        collectMoved(root, movedEntities);
        
        for (auto entity : movedEntities) {
            relocateEntity(root, entity);
        }

        treeRoot3d = cleanupMarkedNodes(root);
    }

    void queryTree(g_ptr<aabb_node> node, BoundingBox& queryBounds, list<g_ptr<S_Object>>& results) {
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

    float raycast(vec3 origin, vec3 direction, float max_dist, g_ptr<aabb_node> root = nullptr) {
        if(!root) root = treeRoot3d;
        if(!root) {
            return max_dist;
        }
        
        direction = direction.normalized();
        float nearest_hit = max_dist;
        
        int nodes_tested = 0;
        int hits_found = 0;
        
        // Ray-AABB intersection using slab method
        auto ray_aabb_intersect = [](vec3 origin, vec3 dir, BoundingBox box) -> float {
            float tmin = -std::numeric_limits<float>::infinity();
            float tmax = std::numeric_limits<float>::infinity();
            
            // X slab
            if(std::abs(dir.x()) > 0.0001f) {
                float t1 = (box.min.x() - origin.x()) / dir.x();
                float t2 = (box.max.x() - origin.x()) / dir.x();
                tmin = std::max(tmin, std::min(t1, t2));
                tmax = std::min(tmax, std::max(t1, t2));
            } else if(origin.x() < box.min.x() || origin.x() > box.max.x()) {
                return -1.0f; // Ray parallel and outside slab
            }
            
            // Y slab
            if(std::abs(dir.y()) > 0.0001f) {
                float t1 = (box.min.y() - origin.y()) / dir.y();
                float t2 = (box.max.y() - origin.y()) / dir.y();
                tmin = std::max(tmin, std::min(t1, t2));
                tmax = std::min(tmax, std::max(t1, t2));
            } else if(origin.y() < box.min.y() || origin.y() > box.max.y()) {
                return -1.0f;
            }
            
            // Z slab
            if(std::abs(dir.z()) > 0.0001f) {
                float t1 = (box.min.z() - origin.z()) / dir.z();
                float t2 = (box.max.z() - origin.z()) / dir.z();
                tmin = std::max(tmin, std::min(t1, t2));
                tmax = std::min(tmax, std::max(t1, t2));
            } else if(origin.z() < box.min.z() || origin.z() > box.max.z()) {
                return -1.0f;
            }
            
            // No intersection
            if(tmax < tmin || tmax < 0) return -1.0f;
            
            return tmin > 0 ? tmin : tmax;
        };
        
        std::function<void(g_ptr<aabb_node>)> traverse = [&](g_ptr<aabb_node> node) {
            if(!node) return;
            
            nodes_tested++;
            float t = ray_aabb_intersect(origin, direction, node->aabb);
            
            // Early rejection only if ray misses entirely
            if(t < 0) return;
            
            if(node->entity) {
                // Leaf node - check distance limit here
                if(t >= nearest_hit) return;
                
                float hit_t = ray_aabb_intersect(origin, direction, node->entity->getWorldBounds());
                if(hit_t > 0 && hit_t < nearest_hit) {
                    hits_found++;
                    nearest_hit = hit_t;
                }
            } else {
                // Internal node - always traverse children if ray intersects, regardless of distance
                traverse(node->left);
                traverse(node->right);
            }
        };
        
        traverse(root);
        return nearest_hit;
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
            list<g_ptr<S_Object>> candidates;
            queryTree(treeRoot3d, boundsA, candidates);
            
            // Check collision layers and do narrow phase collision
            for (auto entityB_uncasted : candidates) {
                if (auto entityB = g_dynamic_pointer_cast<Single>(entityB_uncasted)) {
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
        }
        return collisionPairs;
    }
  
      list<std::pair<g_ptr<Single>, g_ptr<Single>>> GRID_generate3dCollisionPairs() {
        list<std::pair<g_ptr<Single>, g_ptr<Single>>> collisionPairs;
        
        // Pre-allocate based on typical pair count (reduces reallocation)
        collisionPairs.reserve(6000);
        
        // Reusable deduplication array
        static list<int> seen_flags;
        if(seen_flags.length() < scene->singles.length()) {
            seen_flags = list<int>(scene->singles.length(), -1);
        }
        
        // Cache these arrays to avoid repeated lookups
        auto& active = scene->active;
        auto& physicsStates = scene->physicsStates;
        auto& singles = scene->singles;
        
        for (size_t i = 0; i < scene->transforms.length(); ++i) {
            if (!active[i] || !p_state_collides(physicsStates[i])) continue;
            
            g_ptr<Single> entityA = singles[i];
            BoundingBox boundsA = entityA->getWorldBounds();
            
            // Query grid for cells around this entity's bounds
            list<int> cells = grid->cellsAround(boundsA);
            
            // Early bounds check parameters (hoist out of loop)
            vec3 minA = boundsA.min;
            vec3 maxA = boundsA.max;
            
            // Collect unique candidate IDs using flag-based deduplication
            for(auto cell : cells) {
                auto& cell_contents = grid->cells[cell];
                
                for(auto candidate_id : cell_contents) {
                    // Skip if already processed for this entity
                    if(seen_flags[candidate_id] == (int)i) continue;
                    seen_flags[candidate_id] = (int)i;
                    
                    // Skip self
                    if(candidate_id == (int)i) continue;
                    
                    // Skip inactive or non-colliding entities (bounds check first)
                    if(candidate_id >= singles.length()) continue;
                    if(!active[candidate_id]) continue;
                    if(!p_state_collides(physicsStates[candidate_id])) continue;
                    
                    g_ptr<Single> entityB = singles[candidate_id];
                    
                    // Early AABB rejection (before expensive layer checks)
                    BoundingBox boundsB = entityB->getWorldBounds();
                    if(maxA.x() < boundsB.min.x() || minA.x() > boundsB.max.x()) continue;
                    if(maxA.z() < boundsB.min.z() || minA.z() > boundsB.max.z()) continue;
                    if(maxA.y() < boundsB.min.y() || minA.y() > boundsB.max.y()) continue;
                    
                    // Check collision layers (only if AABB passes)
                    CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
                    CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
                    if (!layersA.canCollideWith(layersB)) continue;
                    
                    // Passed all tests - add pair
                    collisionPairs.push(std::pair{entityA, entityB});
                }
            }
        }
        
        return collisionPairs;
    }
    // list<std::pair<g_ptr<Single>, g_ptr<Single>>> GRID_generate3dCollisionPairs() {
    //     list<std::pair<g_ptr<Single>, g_ptr<Single>>> collisionPairs;
        
    //     // Reusable deduplication array (static to avoid reallocation)
    //     static list<int> seen_flags;
    //     if(seen_flags.length() < scene->singles.length()) {
    //         seen_flags = list<int>(scene->singles.length(), -1);
    //     }
        
    //     for (size_t i = 0; i < scene->transforms.length(); ++i) {
    //         if (!scene->active[i] || !p_state_collides(scene->physicsStates[i])) continue;
            
    //         g_ptr<Single> entityA = scene->singles[i];
    //         BoundingBox boundsA = entityA->getWorldBounds();
            
    //         // Query grid for cells around this entity's bounds
    //         list<int> cells = grid->cellsAround(boundsA);
            
    //         // Collect unique candidate IDs using flag-based deduplication
    //         for(auto cell : cells) {
    //             for(auto candidate_id : grid->cells[cell]) {
    //                 // Skip if already processed for this entity
    //                 if(seen_flags[candidate_id] == (int)i) continue;
    //                 seen_flags[candidate_id] = (int)i;
                    
    //                 // Skip self
    //                 if(candidate_id == (int)i) continue;
                    
    //                 // Skip inactive or non-colliding entities
    //                 if(candidate_id >= scene->singles.length()) continue;
    //                 if(!scene->active[candidate_id]) continue;
    //                 if(!p_state_collides(scene->physicsStates[candidate_id])) continue;
                    
    //                 g_ptr<Single> entityB = scene->singles[candidate_id];
                    
    //                 // Check collision layers
    //                 CollisionLayer& layersA = scene->collisonLayers.get(entityA->ID);
    //                 CollisionLayer& layersB = scene->collisonLayers.get(entityB->ID);
    //                 if (!layersA.canCollideWith(layersB)) continue;
                    
    //                 // Narrow-phase AABB test
    //                 if (boundsA.intersects(entityB->getWorldBounds())) {
    //                     collisionPairs.push(std::pair{entityA, entityB});
    //                 }
    //             }
    //         }
    //     }
        
    //     return collisionPairs;
    // }
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
                std::min(thisBounds.max.x(), otherBounds.max.x()) - std::max(thisBounds.min.x(), otherBounds.min.x()),
                std::min(thisBounds.max.y(), otherBounds.max.y()) - std::max(thisBounds.min.y(), otherBounds.min.y()),
                std::min(thisBounds.max.z(), otherBounds.max.z()) - std::max(thisBounds.min.z(), otherBounds.min.z())
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
    list<std::pair<g_ptr<Quad>, g_ptr<Quad>>> AABB_generate2dCollisonPairs() {
        list<std::pair<g_ptr<Quad>, g_ptr<Quad>>> collisionPairs;
        
        for (size_t i = 0; i < scene->guiTransforms.length(); ++i) {
            if (!scene->quadActive[i] || !p_state_collides(scene->quadPhysicsStates[i])) continue;
            
            g_ptr<Quad> entityA = scene->quads[i];
            BoundingBox boundsA = entityA->getWorldBounds();
            
            // Query tree for potential collision candidates
            list<g_ptr<S_Object>> candidates;
            queryTree(treeRoot2d, boundsA, candidates);
            
            // Check collision layers and do narrow phase collision
            for (auto entityB_uncasted : candidates) {
                if (auto entityB = g_dynamic_pointer_cast<Quad>(entityB_uncasted)) {
                    if (entityA == entityB) continue; // Don't collide with self
                    
                    // Check collision layers
                    CollisionLayer& layersA = scene->quadCollisonLayers.get(entityA->ID);
                    CollisionLayer& layersB = scene->quadCollisonLayers.get(entityB->ID);
                    if (!layersA.canCollideWith(layersB)) continue;
                    
                    if (boundsA.intersects(entityB->getWorldBounds())) {
                        collisionPairs.push(std::pair{entityA,entityB});
                    }
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
    void handle2dCollision(const list<std::pair<g_ptr<Quad>, g_ptr<Quad>>>& pairs) {
        for(auto& p : pairs) {
            int i = p.first->ID;
            int s = p.second->ID;
            vec2 thisCenter = scene->quads[i]->getCenter();
            vec2 otherCenter = scene->quads[s]->getCenter();
            
            auto thisBounds = scene->quads[i]->getWorldBounds();
            auto otherBounds = scene->quads[s]->getWorldBounds();
            
            vec2 overlap = vec2(
                std::min(thisBounds.max.x(), otherBounds.max.x()) - std::max(thisBounds.min.x(), otherBounds.min.x()),
                std::min(thisBounds.max.y(), otherBounds.max.y()) - std::max(thisBounds.min.y(), otherBounds.min.y())
            );
            
            vec2 normal;
            if(overlap.x() < overlap.y()) {
                normal = vec2(thisCenter.x() > otherCenter.x() ? 1 : -1, 0);
            } else {
                normal = vec2(0, thisCenter.y() > otherCenter.y() ? 1 : -1);
            }
            
            Velocity& velocity = GET(scene->quadVelocities, i);
            vec2 currentVel = vec2(velocity.position.x(), velocity.position.y());
            float velocityAlongNormal = currentVel.dot(normal);
            
            if(velocityAlongNormal < 0) {
                vec2 newVel = currentVel - (normal * velocityAlongNormal);
                velocity.position.x(newVel.x());
                velocity.position.y(newVel.y());
                velocity.position = velocity.position * 0.78f; // Drag
            }
        }
    }

  

    bool enableCollisons = true;
    float dragCof = 0.99f;
    float gravity = 4.8f;

    bool enableQuadCollisons = true;
    float quadDragCof = 0.99f;
    float quadGravity = 400.0f;
    bool quadRotateAroundCenter = true;
    bool quadScaleAroundCenter = true;

    enum SAMPLE_METHOD {
        NCOL, NAIVE, AABB, GRID
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

#define PHYSTIMERS 0

    void updatePhysics()
    {  

#if PHYSTIMERS
Log::Line overall; overall.start();
Log::Line l; l.start();
double joints3d_time = 0;
double preloop3d_time = 0;
double tree3d_time = 0;
double update_tree_time = 0;
double generate_pairs_time = 0;
int pairs_len = 0;
double handle_collison_time = 0;
double velocity3d_time = 0;
static int FRAME;
FRAME++;

map<std::string,vec2> dtype_joint_profiles;
#endif

        //Resolve the physics joints first
        for (size_t i = 0; i < scene->active.length(); ++i) {
            g_ptr<Single> single = GET(scene->singles, i);
            if(single->physicsJoint) {
                #if PHYSTIMERS
                Log::Line s; s.start();
                #endif
                single->physicsJoint(); // modifies velocities
                #if PHYSTIMERS
                dtype_joint_profiles.getOrPut(single->dtype,vec2(0,0)) += vec2(1,s.end());
                #endif
            }
        }

#if PHYSTIMERS
joints3d_time += l.end(); l.start();
#endif

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

#if PHYSTIMERS
preloop3d_time += l.end();
#endif

        for (size_t i = 0; i < scene->quadActive.length(); ++i) {
            g_ptr<Quad> quad = GET(scene->quads, i);
            if(quad->physicsJoint) {
                quad->physicsJoint(); // modifies velocities
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

#if PHYSTIMERS
l.start();
#endif
        if(collisonMethod==SAMPLE_METHOD::AABB) {
            if(!treeBuilt3d) {
                buildTree3d();
#if PHYSTIMERS
tree3d_time += l.end(); l.start();
#endif
                treeBuilt3d = true;
            } else {
                updateTreeInPlace(treeRoot3d);
#if PHYSTIMERS
update_tree_time += l.end(); l.start();
#endif
            }
                auto col_pairs = AABB_generate3dCollisonPairs();
#if PHYSTIMERS
generate_pairs_time += l.end(); l.start();
pairs_len = col_pairs.length();
#endif
                handle3dCollison(col_pairs);
#if PHYSTIMERS
handle_collison_time += l.end();
#endif
        } else if(collisonMethod==SAMPLE_METHOD::GRID) {

                auto col_pairs = GRID_generate3dCollisionPairs();
#if PHYSTIMERS
generate_pairs_time += l.end(); l.start();
pairs_len = col_pairs.length();
#endif
                handle3dCollison(col_pairs);
#if PHYSTIMERS
handle_collison_time += l.end();
#endif
        } 
        else if (collisonMethod==SAMPLE_METHOD::NAIVE) {
            handle3dCollison(naive_generate3dCollisonPairs());
        }

        if(quadCollisonMethod==SAMPLE_METHOD::AABB) {
            if(!treeBuilt2d) {
                buildTree2d();
                treeBuilt2d = true;
            } else {
                updateTreeInPlace(treeRoot2d);
                handle2dCollision(AABB_generate2dCollisonPairs());
            }
        }
        else if(quadCollisonMethod==SAMPLE_METHOD::NAIVE) {
            for(auto& c : naive_generate2dCollisonData()) {
                handle2dCollision(c);
            }
        } 

#if PHYSTIMERS
l.start();
#endif
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
                g_ptr<Single> single = GET(scene->singles, i);
                Velocity& velocity = GET(scene->velocities, i);
                float dt = thread->getSpeed();
                
                if(velocity.length() == 0) goto c_check;
                
                // Position
                single->position += velocity.position * dt;
                
                // Rotation
                vec3 rotVel = velocity.rotation * dt;
                if(rotVel.length() > 0.0001f) {
                    glm::quat deltaQuat = glm::quat(rotVel.toGlm()); // Euler to quat
                    single->rotation = single->rotation * deltaQuat;
                }
                
                // Scale
                vec3 scaleChange = velocity.scale * dt;
                if(scaleChange.length() > 0.0001f) {
                    single->scaleVec *= (vec3(1.0f) + scaleChange);
                }                
                single->updateTransform();
            }

            c_check:;
        }

        
#if PHYSTIMERS
velocity3d_time += l.end();
#endif

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
                g_ptr<Quad> quad = GET(scene->quads, i);
                Velocity& velocity = GET(scene->quadVelocities, i);
                float dt = thread->getSpeed();
                
                vec2 positionDelta = vec2(vec3_to_vec2(velocity.position)) * dt;
                
                // Rotation
                if(quadRotateAroundCenter && abs(velocity.rotation.z()) > 0.0001f) {
                    quad->rotateCenter(quad->rotation + velocity.rotation.z() * dt);
                } else {
                    quad->rotation += velocity.rotation.z() * dt;
                }
                
                // Scale
                vec2 scaleVelocity = vec2(vec3_to_vec2(velocity.scale));
                if(quadScaleAroundCenter && glm::length(scaleVelocity.toGlm()) > 0.0001f) {
                    vec2 newScale = quad->scaleVec * (vec2(1.0f) + scaleVelocity * dt);
                    quad->scaleCenter(newScale);
                } else {
                    quad->scaleVec *= (vec2(1.0f) + scaleVelocity * dt);
                }
                
                quad->position += positionDelta;
                quad->updateTransform();
            }
            scene->quads.get(i)->getPosition();
            c_check2:;

            if(p!=P_State::NONE)
            {
                //Final physics pass for whatever quads need
            }
        }

#if PHYSTIMERS
if(FRAME%60==0) {
    print("---------------------\nFrame: ",FRAME);
    double overall_time = overall.end();
    print("joints3d_time: ",joints3d_time/1000000,"ms");
    for(auto e : dtype_joint_profiles.entrySet()) {
        print("     ",e.key,": ",e.value.x()," over ",ftime(e.value.y())," (",ftime(e.value.y()/e.value.x())," per)");
    }
    print("preloop3d_time: ",ftime(preloop3d_time));
    if(tree3d_time!=0) print("tree_build_time: ",ftime(tree3d_time));
    if(update_tree_time!=0) print("update_tree_time: ",ftime(update_tree_time));
    print("generate_pairs_time: ",ftime(generate_pairs_time)," (generated ",pairs_len,")");
    print("handle_collison_time: ",ftime(handle_collison_time));
    print("velocity3d_time: ",ftime(velocity3d_time));
    print("overall_time: ",ftime(overall_time));
}
#endif


    }


};

}

//Manual 3d velocity
// glm::mat4& transform = GET(scene->transforms,i);
                // Velocity& velocity = GET(scene->velocities,i);
                // float velocityScale = 1.0f;

                // if(velocity.length()==0) goto c_check;

                // // --- Position ---
                // glm::vec3 pos = glm::vec3(transform[3]);
                // pos += vec3(velocity.position * thread->getSpeed() * velocityScale).toGlm();
                // transform[3] = glm::vec4(pos, 1.0f);
                
                // // --- Rotation ---
                // glm::vec3 rotVel = vec3(velocity.rotation * thread->getSpeed() * velocityScale).toGlm();
                // if (glm::length(rotVel) > 0.0001f) {
                //     glm::mat4 rotationDelta = glm::mat4(1.0f);
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                //     transform = transform * rotationDelta; // Apply rotation in object space
                // }
                
                // // --- Scale ---
                // glm::vec3 scaleChange = vec3(velocity.scale * thread->getSpeed() * velocityScale).toGlm();
                // if (glm::length(scaleChange) > 0.0001f) {
                //     glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                //     transform = transform * scaleDelta; // Apply after rotation
                // }

    //Manual 2d velocity
                 // glm::mat4& transform = GET(scene->guiTransforms,i);
                // Velocity& velocity = GET(scene->quadVelocities,i);
                // float velocityScale = 1.0f;

                // //Add checking for if the velocity in this regard is just all zero in the future
                // //To optimize

                // // --- Position ---
                // glm::vec3 pos = glm::vec3(transform[3]);
                // pos += vec3(velocity.position * thread->getSpeed() * velocityScale).toGlm();
                // transform[3] = glm::vec4(pos, 1.0f);
                
                // // --- Rotation ---
                // glm::vec3 rotVel = vec3(velocity.rotation * thread->getSpeed() * velocityScale).toGlm();
                // if (glm::length(rotVel) > 0.0001f) {
                //     glm::mat4 rotationDelta = glm::mat4(1.0f);
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.y, glm::vec3(0, 1, 0)); // yaw
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.x, glm::vec3(1, 0, 0)); // pitch
                //     rotationDelta = glm::rotate(rotationDelta, rotVel.z, glm::vec3(0, 0, 1));
                //     transform = transform * rotationDelta; // Apply rotation in object space
                // }
                
                // // --- Scale ---
                // glm::vec3 scaleChange = vec3(velocity.scale * thread->getSpeed() * velocityScale).toGlm();
                // if (glm::length(scaleChange) > 0.0001f) {
                //     glm::mat4 scaleDelta = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f) + scaleChange);
                //     transform = transform * scaleDelta; // Apply after rotation
                // }


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