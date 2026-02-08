#include <util/meshBuilder.hpp>

namespace Golden
{

Mesh makeBox(float width, float height, float depth, glm::vec4 color) {
    Mesh mesh;

    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    glm::vec3 positions[8] = {
        {-w, -h, -d}, { w, -h, -d},
        { w,  h, -d}, {-w,  h, -d},
        {-w, -h,  d}, { w, -h,  d},
        { w,  h,  d}, {-w,  h,  d}
    };

    int faceIndices[6][4] = {
        {4, 5, 6, 7}, // Front  (+Z)
        {1, 0, 3, 2}, // Back   (-Z)
        {5, 1, 2, 6}, // Right  (+X)
        {0, 4, 7, 3}, // Left   (-X)
        {3, 7, 6, 2}, // Top    (+Y)
        {0, 1, 5, 4}  // Bottom (-Y)
    };

    glm::vec3 normals[6] = {
        {0, 0,  1},
        {0, 0, -1},
        {1, 0,  0},
        {-1, 0, 0},
        {0, 1,  0},
        {0,-1,  0}
    };

    for (int i = 0; i < 6; ++i) {
        int a = faceIndices[i][0];
        int b = faceIndices[i][1];
        int c = faceIndices[i][2];
        int d = faceIndices[i][3];

        glm::vec3 n = normals[i];

        mesh.vertices.push_back({positions[a], n, color});
        mesh.vertices.push_back({positions[b], n, color});
        mesh.vertices.push_back({positions[c], n, color});
        mesh.vertices.push_back({positions[d], n, color});

        unsigned int base = i * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
        mesh.indices.push_back(base + 0);
    }

    mesh.setupMesh();
    return mesh;
}

Mesh makeTestBox(float size) {return makeBox(size,size,size,glm::vec4(1,1,1,1));}

Mesh makeBox(vec3 v,glm::vec4 color) {return makeBox(v.x(),v.y(),v.z(),color);}

Mesh makePhysicsBox(const BoundingBox& b) {
    return makeBox(b.extent(),glm::vec4(1,0,0,1));
}

Mesh makeTerrainGrid(int size, float cellSize) {
    Mesh mesh;
    glm::vec4 colorA = glm::vec4(0.6, 0.2, 0.6, 1.0);
    glm::vec4 colorB = glm::vec4(0.2, 0.2, 0.2, 1.0);
    int subdivisions = static_cast<int>(size / cellSize);
    float step = static_cast<float>(size) / subdivisions;

    // Vertex grid
    for (int z = 0; z <= subdivisions; ++z) {
        for (int x = 0; x <= subdivisions; ++x) {
            float xpos = x * step - size * 0.5f;
            float zpos = z * step - size * 0.5f;

            // Checkerboard color
            bool dark = ((x % 2 == 0 && z % 2 == 0) || (x % 2 == 1 && z % 2 == 1));
            glm::vec4 color = dark ? colorA : colorB;

            mesh.vertices.push_back({
                glm::vec3(xpos, 0.0f, zpos),    // position
                glm::vec3(0, 1, 0),             // normal (up)
                color                           // color
            });
        }
    }

    // Indices (2 triangles per grid cell)
    for (int z = 0; z < subdivisions; ++z) {
        for (int x = 0; x < subdivisions; ++x) {
            int row = subdivisions + 1;
            int bottomLeft  = z * row + x;
            int bottomRight = bottomLeft + 1;
            int topLeft     = (z + 1) * row + x;
            int topRight    = topLeft + 1;

            // First triangle (bottomLeft, topLeft, topRight)
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(topRight);

            // Second triangle (bottomLeft, topRight, bottomRight)
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomRight);
        }
    }

    mesh.setupMesh();
    return mesh;
}

}

