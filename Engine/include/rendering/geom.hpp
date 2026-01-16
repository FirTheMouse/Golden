#pragma once

#include<core/object.hpp>
#include<util/engine_util.hpp>
#include <glm.hpp>

namespace Golden {

    class Geom : public Object {
    private:
        unsigned int instanceVBO = 0;
        unsigned int instancedVAO = 0;
        unsigned int instanceDataVBO = 0;
        int instanceCount = 0;
    public:
        //Default making just a white square
        Geom() {
            setupGeom();
        }

        unsigned int VAO = 0, VBO = 0, EBO = 0;
        bool instance = false;

        unsigned int texture = 0;

        BoundingBox localBounds = BoundingBox(
            vec3(0.0f, 0.0f, 0.0f), 
            vec3(1.0f, 1.0f, 0.0f), 
            BoundingBox::DIM_2D
        );

        void setupGeom();
        void draw();

        void setupInstancedVAO(const list<glm::mat4>& transforms, const list<vec4>& instanceData);
        void fullInstanceUpdate(const list<glm::mat4>& transforms, const list<vec4>& instanceData);

        void transformInstances(const list<glm::mat4>& transforms, const list<vec4>& instanceData) {
            if (instanceVBO == 0) {
                setupInstancedVAO(transforms,instanceData);
            } else {
                fullInstanceUpdate(transforms,instanceData);
            }
        }

        void enableInstanced();
        void disableInstanced();
    };

}