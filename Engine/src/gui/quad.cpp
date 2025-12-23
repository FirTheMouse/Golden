#include<gui/quad.hpp>
#include<rendering/scene.hpp>
#include<gui/text.hpp>

#define QUAD_DEBUG 1
#define TO_STRING(x) #x
#define TO_STRING_EXPAND(x) TO_STRING(x)

#if QUAD_DEBUG
    #define GET(container, id) \
        container.get(id, debug_trace_path + \
                         std::string(__FUNCTION__) + " at " + std::string(__FILE__) + ":" + TO_STRING_EXPAND(__LINE__))
#else
    #define GET(container, id) container.get(id)
#endif


namespace Golden
{

    bool Quad::checkGet(int typeId)
    {
        if(!scene) {
            print("Quad::checkGet::21 tried to get property ",typeId," but Scene is null!");
            return false;
        }
        // if(!isActive()) {
        //     print("Quad::checkGet::25 tried to get a property ",typeId," of an innactive Object!");
        //     return false;
        // }
        return true;
    }

    void Quad::remove()
    {
        //If it's got a parent relationship, like text...
        //I'll just sweep it under the rug for now and use deactivate, coordinating remove across
        //grouped items is too much of a headache, plus if I add GC later it can clean these deactivated
        //objects in one nice clean pass
        if(lockToParent)
       {
        callingChild=true;
        run("onRemove");
        parent->remove();
        callingChild=false;
        scene->deactivate(this);
        return;
       }
       else if(!children.empty())
       {
        list<g_ptr<Quad>> swaped;
        swaped <= children;
        for(int i=swaped.length()-1;i>=0;i--)
        {
            auto c = swaped[i];
            if(c->lockToParent&&!c->callingChild)
            {
                scene->deactivate(c);
                c->run("onRemove");
                // if(c->ID>=c->scene->quadActive.length()) continue;

                // c->S_Object::remove();
                // c->scene->quadActive.remove(c->ID);
                // c->scene->quads.remove(c->ID);
                // c->scene->guiTransforms.remove(c->ID);
                // c->scene->guiEndTransforms.remove(c->ID);
                // c->scene->slots.remove(c->ID);

                // if(c->ID<scene->quads.length())
                // scene->quads.get(c->ID,"Quad::remove::62")->ID =c->ID;
                
            }
        }
        detatchAll();
        }

        run("onRemove");
        scene->deactivate(this);
        S_Object::remove();
        
        scene->quadActive.remove(ID);
        scene->quads.remove(ID);
        scene->guiTransforms.remove(ID);
        scene->guiEndTransforms.remove(ID);
        scene->quadPhysicsStates.remove(ID);
        scene->quadAnimStates.remove(ID);
        scene->quadVelocities.remove(ID);
        scene->slots.remove(ID);

        if(ID<scene->quads.length())
        scene->quads.get(ID,"Quad::remove::43")->ID = ID;
    }

    void Quad::setupQuad()
    {
        float quadVertices[] = {
            // pos     // uv
            0.0f, 1.0f, 0.0f, 0.0f, // Top-left
            1.0f, 1.0f, 1.0f, 0.0f, // Top-right
            1.0f, 0.0f, 1.0f, 1.0f, // Bottom-right
            0.0f, 0.0f, 0.0f, 1.0f  // Bottom-left
        };
        
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0); // pos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1); // uv
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    void Quad::draw()
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Quad::hide() {
        if(lockToParent)
        {
         callingChild=true;
         parent->hide();
         callingChild=false;
         return;
        }
        else if(!children.empty())
        {
         scene->quadCulled.get(ID)=true;
         for(auto c : children)
         {
             if(c->lockToParent)
             {
                scene->quadCulled.get(c->ID)=true;
             }
         }
         } else {
            scene->quadCulled.get(ID)=true;
         }
    }
    void Quad::show() {
        if(lockToParent)
        {
         callingChild=true;
         parent->show();
         callingChild=false;
         return;
        }
        else if(!children.empty())
        {
         scene->quadCulled.get(ID)=false;
         for(auto c : children)
         {
             if(c->lockToParent)
             {
                scene->quadCulled.get(c->ID)=false;
             }
         }
         } else {
            scene->quadCulled.get(ID)=false;
         }
    }
    bool Quad::culled() {return scene->quadCulled.get(ID);}

    glm::mat4& Quad::getTransform() {
        if(checkGet(0)) {
            return GET(scene->guiTransforms,ID);
        }
        else {
            return GET(scene->guiTransforms,0);
        }
    }
    
    glm::mat4& Quad::getEndTransform() {
        if(checkGet(1)) {
        return GET(scene->guiEndTransforms,ID);
        }
        else {
          return GET(scene->guiEndTransforms,0); }
    }

    AnimState& Quad::getAnimState() {
        if(checkGet(6)) {
        return GET(scene->quadAnimStates,ID);
        }
        else return GET(scene->quadAnimStates,0);
    }
    
    P_State& Quad::getPhysicsState() {
        if (checkGet(2)) {
            return GET(scene->quadPhysicsStates,ID);
        }
        else {
        static P_State dummy = P_State::NONE;
        return dummy; // Safe fallback
        }
    
    }
    
    Velocity& Quad::getVelocity() {
        if (checkGet(3)) {
            return GET(scene->quadVelocities,ID);
        }
        else {
            static Velocity dummy;
            return dummy; // Safe fallback
        }
    }

    vec2 Quad::getPosition() {
        #if QUAD_DEBUG
        debug_trace_path = "Quad::getPosition::185";
        #endif
        position = vec2(glm::vec3(getTransform()[3]));
        return position;
    }

    float Quad::getRotation() {
        return rotation;
    }

    vec2 Quad::getScale() {
        glm::mat4 mat = getTransform();
        vec2 scale = {
            glm::length(glm::vec3(mat[0])),
            glm::length(glm::vec3(mat[1]))
        };
        return scale;
    }

    vec2 Quad::getCenter() {
        glm::mat4 mat = getTransform();
    
        // Extract rotation and scale
        glm::vec3 right = glm::vec3(mat[0]);
        glm::vec3 up    = glm::vec3(mat[1]);
    
        // Half-extents in transformed space
        glm::vec3 halfExtents = (right + up) * 0.5f;
    
        // Position is translation component
        glm::vec3 pos = glm::vec3(mat[3]);
        position = vec2(pos);
    
        // Center is position + half-extents
        vec2 center(pos + halfExtents);
        if(!has("char"))
        {
            return center;
        }
        else
        {
            auto textQuad = text::parent_of(this);
            vec2 visualCenter = text::center_of(textQuad);
            return visualCenter;
        }
    }

    CollisionLayer& Quad::getLayer() {
        if(checkGet(11)) {
            return GET(scene->quadCollisonLayers,ID);
        } else {
            static CollisionLayer dummy;
            return dummy;
        }
    }

    Quad& Quad::setPhysicsState(P_State p)
    {
        getPhysicsState() = p;
        return *this;
    }

    Quad& Quad::setLinearVelocity(const vec2& v) {
        getVelocity().position = vec3(v,0);
        return *this;
    }

    Quad& Quad::impulseL(const vec2& v) {
        if (checkGet(9)) {
            getVelocity().position += vec3(v,0);
        }
        return *this;
    }

    list<std::string>& Quad::getSlots()
    {
        if(checkGet(7)) {
            return scene->slots[ID];
        }
        else {
            return scene->slots[0]; }
    }

    Quad& Quad::addSlot(const std::string& slot)
    {
     if(!getSlots().has(slot))
     {
        getSlots() << slot;
     }
     return *this;
    }

    void Quad::addChild(g_ptr<Quad> q, bool andLock)
    {
        if(!q->parent)
        {
        children << q;
        q->parent = g_ptr<Quad>(this);
            if(andLock)
            {
                q->lockToParent = true;
                q->ogt = Q_Snapshot(
                    q->position-position,
                    q->scaleVec-scaleVec,
                    q->rotation-rotation);
            }
        }
        else {
            print("Quad::AddChild::131 Quad already parented!");
        }
    }

    Quad& Quad::startAnim(float duration) {
        if (checkGet(7)) {
        AnimState& a = getAnimState();
        a.duration=duration;
        a.elapsed = 0.0f;
        }
        return *this;
    }

    Quad& Quad::setPosition(const vec2& pos)
    {
       position = pos;
       if(lockToParent)
       {
        callingChild=true;
        parent->setPosition(pos-vec2(ogt.pos.x(),ogt.pos.y()));
        callingChild=false;
       }
       else if(!children.empty())
       {
        for(auto c : children)
        {
            bool isTextParent = has("char");
            
            if(isTextParent)
            {
                // Get the character list and update the text scale
                auto chars = get<g_ptr<d_list<size_t>>>("chars");
                float textScale = chars->Object::get<float>("scale");
                
                // Recalculate positions using makeText logic
                vec2 start = position; // or however you want to anchor it
                for(auto c : children)
                {
                    if(c->lockToParent && c->has("char"))
                    {
                        vec2 bearing = c->get<vec2>("bearing");
                        c->position = start + (bearing * textScale);
                        float dx = c->get<float>("advance") * textScale;
                        
                        float left = getPosition().x();
                        if(c->get<char>("char")=='\n') {
                            dx-=(start.x()-left);
                            start.addY(50.0f*textScale);}

                        start.addX(dx);
                        c->updateTransform();
                    }
                    else if(c->lockToParent && !c->callingChild)
                    {
                        c->position = pos+c->ogt.pos;
                        c->updateTransform();
                    }
                }
            }
            else if(c->lockToParent&&!c->callingChild)
            {
                c->position = pos+vec2(c->ogt.pos.x(),c->ogt.pos.y());
                c->updateTransform();
            }
        }
       }
       updateTransform();
       return *this;
    }

    Quad& Quad::setCenter(const vec2& pos)
    {
        if (checkGet(10)) {
            if(has("char"))
            {
                auto textQuad = text::parent_of(this);
                vec2 visualCenter = text::center_of(textQuad);
                vec2 currentPos = textQuad->getPosition();
                vec2 offset = pos - visualCenter;
                textQuad->setPosition(currentPos + offset);
            }
            else
            {
            vec2 scale = getScale();
            setPosition(pos-(scale*0.5f));
            }
        } 
        return *this;
    }  
    
    Quad& Quad::rotate(float angle) 
    {
        rotation = angle;
        updateTransform();
        return *this;
    }

    Quad& Quad::scale(const vec2& scale)
    {
        scaleVec = scale;

        if(lockToParent)
       {
        callingChild=true;
        parent->scale(vec2(scaleVec/ogt.scale));
        callingChild=false;
       }
       else if(!children.empty())
        {
            bool isTextParent = has("char");
            
            if(isTextParent)
            {
                // Get the character list and update the text scale
                auto chars = get<g_ptr<d_list<size_t>>>("chars");
                float currentTextScale = chars->Object::get<float>("scale");
                float newTextScale = (scale.x() + scale.y()) / 2.0f;
                chars->Object::set<float>("scale", newTextScale);
                
                // Recalculate positions using makeText logic
                vec2 start = position; // or however you want to anchor it
                for(auto c : children)
                {
                    if(c->lockToParent && c->has("char"))
                    {
                        // Apply makeText positioning logic here
                        vec2 bearing = c->get<vec2>("bearing");
                        c->position = vec2(start + (bearing * newTextScale));
                        
                        float dx = c->get<float>("advance") * newTextScale;
                        float left = getPosition().x();
                        if(c->get<char>("char")=='\n') {
                            dx-=(start.x()-left);
                            start.addY(50.0f*newTextScale);}
                        start.addX(dx);
                        // Scale the visual quad
                        vec2 st = c->get<vec2>("size")*newTextScale;
                        c->scaleVec = st;
                        c->updateTransform();
                    }
                }
            }
            else
            {
                // Regular child scaling logic
                for(auto c : children)
                {
                    if(c->lockToParent&&!c->callingChild)
                    {
                        c->scaleVec = c->ogt.scale+scale;
                        c->updateTransform();
                    }
                }
            }
        }
        updateTransform();
        return *this;
    }

    Quad& Quad::move(const vec2& pos,bool update) {
        position+=pos;
        if(update) updateTransform();
        return *this;
    }

    Quad& Quad::move(const vec2& delta,float animationDuration,bool update) {
        endPosition+=delta;
        if(update) {
            startAnim(animationDuration);
            updateEndTransform();
        }
        return *this;
    }

    void Quad::updateTransform() {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleVec.toGlm(),1));
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f),rotation,glm::vec3(0,0,1));
        glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(position.toGlm(),0));

        getTransform() = transMat * rotMat * scaleMat; 
    }
    
    void Quad::updateEndTransform() {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(endScale.toGlm(),1));
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f),endRotation,glm::vec3(0,0,1));
        glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::vec3(endPosition.toGlm(),0));
        
        getEndTransform() = transMat * rotMat * scaleMat;  
    }

    bool Quad::pointInQuad(vec2 point) {
        glm::mat4 mat = getTransform();
    
        // Extract position (translation), right and up vectors (including rotation + scale)
        glm::vec2 origin = glm::vec2(mat[3]);
        glm::vec2 right  = glm::vec2(mat[0]);
        glm::vec2 up     = glm::vec2(mat[1]);
    
        // Vector from origin to point
        glm::vec2 toPoint = point.toGlm() - origin;
    
        // Project onto local axes
        float projRight = glm::dot(toPoint, glm::normalize(right));
        float projUp    = glm::dot(toPoint, glm::normalize(up));
    
        // Compare against length of local axes (scale)
        float lenRight = glm::length(right);
        float lenUp    = glm::length(up);
    
        return (projRight >= 0 && projRight <= lenRight &&
                projUp    >= 0 && projUp    <= lenUp);
    }
    
}

 // void Quad::remove()
    // {
    //     S_Object::remove();
        
    //     std::vector<g_ptr<Quad>>& quads          = scene->quads;
    //     std::vector<glm::mat4>& guiTransforms    = scene->guiTransforms;
    //     std::vector<glm::mat4>& guiEndTransforms = scene->guiEndTransforms;
    //     std::vector<list<std::string>>& slots    = scene->slots;

    //     size_t last = quads.size() - 1;
    //     size_t idx = ID;

    //     if (idx != last) {
    //         // Swap all vectors to keep indexing consistent
    //         std::swap(quads[idx],           quads[last]);
    //         std::swap(guiTransforms[idx],   guiTransforms[last]);
    //         std::swap(guiEndTransforms[idx],guiEndTransforms[last]);
    //         std::swap(slots[idx],           slots[last]);

    //         // Fix the swapped quad's ID 
    //         size_t uuid = quads[idx]->ID = idx;
    //     }

    //     // Pop off the last elements
    //     quads.pop_back();
    //     guiTransforms.pop_back();
    //     guiEndTransforms.pop_back();
    //     slots.pop_back();
    // }

     // vec2 Quad::getCenter() {
    //     vec2 scale = getScale();
    //     return getPosition() + (scale*0.5f);
    // }

    // S_Object& Quad::rotate(float angle) {
    //     glm::mat4& mat = getTransform();
    //     vec2 position = getPosition();
    //     vec2 scale = getScale();
    //     glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,0,1));
    //     rotation[0] *= scale.x();
    //     rotation[1] *= scale.y();
    //     mat = rotation;
    //     mat[3] = glm::vec4((position).toGlm(),1.0f, 1.0f);
    
    //     if (s_type() == S_TYPE::INSTANCED) {
    //         requestInstanceUpdate();
    //     }
    //     return *this;
    // }