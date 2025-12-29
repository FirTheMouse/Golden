#pragma once

#include <rendering/model.hpp>
namespace Golden {


Mesh makeBox(float width, float height, float depth, glm::vec4 color);

Mesh makeBox(vec3 v,glm::vec4 color);

Mesh makePhysicsBox(const BoundingBox& b);

Mesh makeTestBox(float size);

Mesh makeTerrainGrid(int size, float cellSize);

}