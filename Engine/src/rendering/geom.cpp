#include<rendering/geom.hpp>
#include<rendering/scene.hpp>

namespace Golden {
    void Geom::setupGeom()
    {
        //The default square
        float quadVertices[] = {
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f
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

    void Geom::draw()
    {
        if(instance) {
            glBindVertexArray(instancedVAO);
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instanceCount);
            glBindVertexArray(0);
        } else {
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    void Geom::setupInstancedVAO(const list<glm::mat4>& instanceTransforms, const list<vec4>& instanceData, const list<vec4>& colors) {
        if (instancedVAO == 0) glGenVertexArrays(1, &instancedVAO);
        if (instanceVBO == 0) glGenBuffers(1, &instanceVBO);
        if (instanceDataVBO == 0) glGenBuffers(1, &instanceDataVBO);
        if (instanceColorVBO == 0) glGenBuffers(1, &instanceColorVBO);

        std::vector<glm::vec4> std_data;
        for(auto i : instanceData) {
            std_data.push_back(i.toGlm());
        }

        std::vector<glm::vec4> std_colors;
        for(auto c : colors) {
            std_colors.push_back(c.toGlm());
        }
    
        glBindVertexArray(instancedVAO);
    
        // Base quad attributes (pos + uv)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        
        // Attribute 0: position (vec2)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        
        // Attribute 1: UV (vec2)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
        // Instance transforms (mat4 = 4x vec4) - attributes 2, 3, 4, 5
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4), 
                     instanceTransforms.data(), GL_STATIC_DRAW);
    
        std::size_t vec4Size = sizeof(glm::vec4);
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(2 + i);
            glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
            glVertexAttribDivisor(2 + i, 1); // Per-instance
        }
    
        // Instance data (vec4) - attribute 6
        glBindBuffer(GL_ARRAY_BUFFER, instanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, std_data.size() * sizeof(glm::vec4), 
                    std_data.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);  // stride = 0 means tightly packed
        glVertexAttribDivisor(6, 1);

        // Instance colors (vec4) - attribute 7
        glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
        glBufferData(GL_ARRAY_BUFFER, std_colors.size() * sizeof(glm::vec4), 
                    std_colors.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);  // stride = 0
        glVertexAttribDivisor(7, 1);

        glBindVertexArray(0);
        instance = true;
        instanceCount = instanceTransforms.size();
    }
    
    void Geom::fullInstanceUpdate(const list<glm::mat4>& instanceTransforms, const list<vec4>& instanceData, const list<vec4>& colors) {

        std::vector<glm::vec4> std_data;
        for(auto i : instanceData) {
            std_data.push_back(i.toGlm());
        }

        std::vector<glm::vec4> std_colors; 
        for(auto c : colors) {
            std_colors.push_back(c.toGlm());
        }

        glBindVertexArray(instancedVAO);

        // Update transform buffer
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4),
                     instanceTransforms.data(), GL_DYNAMIC_DRAW);
        
        // Update data buffer
        glBindBuffer(GL_ARRAY_BUFFER, instanceDataVBO);
        glBufferData(GL_ARRAY_BUFFER, std_data.size() * sizeof(glm::vec4),
                     std_data.data(), GL_DYNAMIC_DRAW);

        // Update color buffer 
        glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
        glBufferData(GL_ARRAY_BUFFER, std_colors.size() * sizeof(glm::vec4),
                     std_colors.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        instanceCount = instanceTransforms.size();
    }
    
    void Geom::enableInstanced() {
        instance = true;
    }
    void Geom::disableInstanced() {
        instance = false;
    }
}