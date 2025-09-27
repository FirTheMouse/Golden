#ifndef MODEL_H
#define MODEL_H
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <util/util.hpp>
#include <limits>
#include <algorithm>
#include <utility>
#include <core/object.hpp>
//#include <rendering/scene_object.hpp>

namespace tinygltf {
    class Model;  // Just declaring that this class exists somewhere
}

namespace Golden {

enum class InterpolationType : int {
    LINEAR = 0,
    STEP = 1,
    CUBIC = 2
};

class BoundingBox {
public:
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox()
        : min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest()) {}

    BoundingBox(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {}

    void expandToInclude(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    vec3 extent() const {
        return vec3(
            max.x - min.x,
            max.y - min.y,
            max.z - min.z
        );
    }

    char getLongestAxis() const {
        glm::vec3 size = getSize(); 
        
        if (size.x >= size.y && size.x >= size.z) return 'x';
        if (size.y >= size.z) return 'y';
        return 'z';
    }

    float volume() const {
    glm::vec3 size = max - min;
    return size.x * size.y * size.z;
    }

    void transform(const glm::mat4& matrix) {
        glm::vec3 newMin(std::numeric_limits<float>::max());
        glm::vec3 newMax(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; ++i) {
            glm::vec3 corner = getCorner(i);
            glm::vec3 transformed = glm::vec3(matrix * glm::vec4(corner, 1.0f));
            newMin = glm::min(newMin, transformed);
            newMax = glm::max(newMax, transformed);
        }

        min = newMin;
        max = newMax;
    }

    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 getSize() const {
        return max - min;
    }

    glm::vec3 getCorner(int index) const {
        return glm::vec3(
            (index & 1) ? max.x : min.x,
            (index & 2) ? max.y : min.y,
            (index & 4) ? max.z : min.z
        );
    }

    BoundingBox& expand(float scalar) {
        glm::vec3 center = getCenter();
        glm::vec3 halfSize = getSize() * 0.5f;
        glm::vec3 newHalfSize = halfSize * scalar;
        
        min = center - newHalfSize;
        max = center + newHalfSize;
        return *this;
    }

    BoundingBox& expand(const BoundingBox& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
        return *this;
    }

    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y &&
                point.z >= min.z && point.z <= max.z);
    }
};

class CollisionShape {
public:

};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;

    glm::ivec4 boneIds = glm::ivec4(0);
    glm::vec4 boneWeights = glm::vec4(1,0,0,0);

    Vertex() {}

    Vertex(glm::vec3 pos, glm::vec3 norm, glm::vec4 col) :
    position(pos),normal(norm),color(col) {}
};

struct Material {
    glm::vec4 baseColor = glm::vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
};

class Mesh : virtual public Object {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // std::vector<glm::mat4> instanceTransforms;
    Material material;
    bool instance = false;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int instancedVAO = 0, instanceVBO = 0;
    size_t instanceCount = 0;

    Mesh() {}

    Mesh(const Mesh& other)
    : vertices(other.vertices),
      indices(other.indices),
      material(other.material),
      instance(other.instance),
      VAO(0), VBO(0), EBO(0), instancedVAO(0), instanceVBO(0), instanceCount(other.instanceCount)
    {
        
        // Don't copy OpenGL buffers blindly — recreate them if needed
        // setupMesh() if needed, or leave it uninitialized
        setupMesh();
    }

    Mesh& operator=(const Mesh& other) {
        if (this != &other) {
            vertices = other.vertices;
            indices = other.indices;
            material = other.material;
            instance = other.instance;
            instanceCount = other.instanceCount;
    
            // Do NOT copy VAO/VBO/EBO directly — recreate them if needed
            VAO = VBO = EBO = instancedVAO = instanceVBO = 0;
            // Possibly: setupMesh();
        }
        return *this;
    }

    void copy(const Mesh& other)
    {
        this->vertices = other.vertices;
        this->indices = other.indices;
        this->material = other.material;
        this->VAO = other.VAO;this->VBO = other.VBO;this->EBO = other.EBO;
        this->instancedVAO = other.instancedVAO; this->instanceVBO = other.instanceVBO;
        this->instanceCount = other.instanceCount;
        this->instance = other.instance;
    }

    void setupMesh();
    void setupInstancedVAO(const std::vector<glm::mat4>& instanceMatrices);
    void draw(unsigned int shaderProgram) const;

    inline void updateInstanceTransform(size_t idx, const glm::mat4& mat) {
         if (instanceVBO == 0) {
        std::cerr << "Error: Attempted to update instance transform, but instanceVBO is 0." << std::endl;
        return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(mat));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    inline void fullInstanceUpdate(const std::vector<glm::mat4>& instanceTransforms) {
      glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
      glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4),
             instanceTransforms.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};


class Scene;

/// @todo add markers to the save/load binary!
class Model : virtual public Object {
public:
    std::vector<Mesh> meshes;
    bool instance = false;
    //std::vector<g_ptr<S_Object>> instances;
   // std::vector<glm::mat4> instanceTransforms;
    Scene* scene = nullptr;
    size_t id;
    size_t Iid;
    size_t UUIDPTR;
    BoundingBox localBounds;
    map<std::string,vec3> markers;

    q_list<vec3> boneLocalPosition;
    q_list<vec3> boneLocalScale;
    q_list<glm::quat> boneLocalRotation;
    q_list<glm::mat4> boneWorldTransform;
    q_list<bool> boneLocked; //Not working right now, but in the future will let us lock specific bones 
    q_list<bool> boneDirty;
    q_list<glm::mat4> boneInverseBindMatrix;
    q_list<list<size_t>> boneParents;
    q_list<list<size_t>> boneVerticies;
    q_list<float> boneWeight;

    bool hasBones() const { return boneLocalPosition.length()>0; }
    void updateBoneHierarchy();
    void uploadBoneMatrices(unsigned int shaderProgram);

    private:
    void updateBone(size_t boneIndex);
    void extractAnimations(const tinygltf::Model& model);
    void findKeyframes(const std::string& animName, size_t boneIndex, const std::string& property, 
        float time, size_t& keyframe1, size_t& keyframe2, float& alpha);
    public:

    map<std::string, size_t> animationNames; 
    q_list<float> animationDuration;
    q_list<size_t> animationBoneIndex;
    q_list<std::string> animationProperty;   // "translation", "rotation", "scale"
    q_list<float> animationTime;
    q_list<vec3> animationPosition; 
    q_list<glm::quat> animationRotation; 
    q_list<vec3> animationScale;  
    q_list<InterpolationType> animationInterpolation; 
    q_list<std::string> animationOwner; 

    std::string currentAnimation = "";
    float currentAnimTime = 0.0f;
    bool isPlaying = false;
    
    // Animation methods
    void playAnimation(const std::string& animName);
    void updateAnimation(float deltaTime);
    void applyAnimationFrame(float time);
        
    Model(const std::string& path);
    Model(Mesh&& m) {meshes.push_back(std::move(m)); preComputeAABB();}
    Model(std::vector<Mesh>&& ms) : meshes(std::move(ms)) {preComputeAABB();}
    Model() {};

    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    void saveBinary(const std::string& path) const;
    void loadBinary(const std::string& path);

    void saveBinary(std::ostream& out) const;
    void loadBinary(std::istream& in);


    void copy(const Model& other)
    {
        // Copy meshes (existing)
        for (const auto& mesh : other.meshes)
        {
            Mesh newMesh;
            newMesh.copy(mesh);
            meshes.push_back(std::move(newMesh));
        }
        
        localBounds = other.localBounds;
        markers = other.markers;
        
       // boneLocalPosition << other.boneLocalPosition;
        // boneLocalScale << other.boneLocalScale;
        // boneLocalRotation << other.boneLocalRotation;
        // boneWorldTransform << other.boneWorldTransform;
        // boneLocked << other.boneLocked;
        // boneDirty << other.boneDirty;
        // boneInverseBindMatrix << other.boneInverseBindMatrix;
        // boneParents << other.boneParents;
        // boneVerticies << other.boneVerticies;
        // boneWeight << other.boneWeight;
        
        // // Copy animation data
        // animationNames = other.animationNames;
        // animationDuration << other.animationDuration;
        // animationBoneIndex << other.animationBoneIndex;
        // animationProperty << other.animationProperty;
        // animationTime << other.animationTime;
        // animationPosition << other.animationPosition;
        // animationRotation << other.animationRotation;
        // animationScale << other.animationScale;
        // animationInterpolation << other.animationInterpolation;
        // animationOwner << other.animationOwner;
                
        preComputeAABB();
    }
    
    void copy(const g_ptr<Model>& other)
    {
        copy(*other);
    }

    void draw(unsigned int shaderProgram) const {
        for (const auto& mesh : meshes)
            mesh.draw(shaderProgram);
    }

    // void addInstance(g_ptr<S_Object> instance);

    // void removeInstance(size_t uid);

    // void setupInstancing(const std::vector<glm::mat4>& instanceMatrices) {
    //     for (auto& mesh : meshes)
    //     {  
    //         instanceTransforms = instanceMatrices;
    //         mesh.setupInstancedVAO(instanceMatrices);
    //         mesh.instance=instance;
    //     }
    // }

    // inline void updateInstanceTransform(size_t idx, const glm::mat4& mat) {    
    //     if (Iid < 0 || Iid >= instanceTransforms.size()) {
    //         std::cerr << "Invalid Iid: " << Iid << " (size: " << instanceTransforms.size() << ")\n";
    //         std::abort();
    //     }  
    //     for (auto& mesh : meshes) 
    //     {
    //         instanceTransforms[idx] = mat;
    //         mesh.updateInstanceTransform(idx,mat);
    //     }
    // }

    // void updateInstanceBuffers() {
    //     for (auto& mesh : meshes) {
    //         if (mesh.instanceVBO == 0) {
    //             mesh.setupInstancedVAO(instanceTransforms);
    //         } else {
    //             glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVBO);
    //             glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4),
    //                         instanceTransforms.data(), GL_DYNAMIC_DRAW);
    //             glBindBuffer(GL_ARRAY_BUFFER, 0);
    //             mesh.instanceCount = instanceTransforms.size();
    //         }
    //     }
    // }


    bool waitingForInstanceUpdate = false;
    void requestInstanceUpdate();

    void setColor(const glm::vec4& color)
    {
        for (auto& mesh : meshes) {
            for(auto& vert : mesh.vertices) {
                vert.color = color;
            }
            mesh.setupMesh();
        }
    }

    glm::vec3 localMin = glm::vec3(FLT_MAX);
    glm::vec3 localMax = glm::vec3(-FLT_MAX);

    void preComputeAABB()
    {
        for (const auto& mesh : meshes) {
            for (const auto& v : mesh.vertices) {
                localBounds.expandToInclude(v.position);
            }
        }
    }

    
private:

    void loadModel(const std::string& path);
    void processModel(const class tinygltf::Model& model);

    vec3 position;
};
}


#endif