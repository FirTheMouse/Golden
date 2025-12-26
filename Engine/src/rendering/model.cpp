#include <rendering/model.hpp>
// #include "scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../ext/tinygltf/tiny_gltf.h"
namespace Golden {

void Mesh::setupMesh() {
    // Generate buffers and a vertex array object (VAO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the VAO (this saves all subsequent buffer/attribute state)
    glBindVertexArray(VAO);

    // Vertex buffer (positions, normals, colors)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Element/index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0
    );

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)
    );

    // Color attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color)
    );

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));


    // Unbind the VAO (good practice)
    glBindVertexArray(0);
}



void Mesh::setupInstancedVAO(const list<glm::mat4>& instanceMatrices) {
    if (instancedVAO == 0) glGenVertexArrays(1, &instancedVAO);
    if (instanceVBO == 0) glGenBuffers(1, &instanceVBO);

    glBindVertexArray(instancedVAO);

    // Base mesh attributes
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    // Instance transforms (mat4 = 4x vec4)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_STATIC_DRAW);

    std::size_t vec4Size = sizeof(glm::vec4);
    for (unsigned int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
        glVertexAttribDivisor(3 + i, 1); // Per-instance
    }

    glBindVertexArray(0);
    instance = true;
    instanceCount = instanceMatrices.size();
}



void Mesh::draw(unsigned int shaderProgram) const {
    unsigned int baseColorLoc = glGetUniformLocation(shaderProgram, "material.baseColor");
    unsigned int metallicLoc = glGetUniformLocation(shaderProgram, "material.metallic");
    unsigned int roughnessLoc = glGetUniformLocation(shaderProgram, "material.roughness");
    glUniform4fv(baseColorLoc, 1, glm::value_ptr(material.baseColor));
    glUniform1f(metallicLoc, material.metallic);
    glUniform1f(roughnessLoc, material.roughness);

    if(instance)
    {
        glBindVertexArray(instancedVAO);
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
    }
    else {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}


Model::Model(const std::string& path) {
    loadModel(path);
}


 void Model::loadModel(const std::string& path) {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        
        bool ret;
        if (path.substr(path.size() - 4) == ".glb") {
            ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
        } else if (path.substr(path.size() - 5) == ".gmdl") {
            loadBinary(path);
            return;
        }
        else {
            ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
        }
        
        if (!warn.empty()) {
            printf("Warning: %s\n", warn.c_str());
        }
        
        if (!err.empty()) {
            printf("Error: %s\n", err.c_str());
        }
        
        if (!ret) {
            printf("Failed to load GLTF file: %s\n", path.c_str());
            return;
        }
        
        processModel(gltfModel);
        preComputeAABB();
    }
    
    void Model::processModel(const tinygltf::Model& model) {
        for (size_t i = 0; i < model.meshes.size(); i++) {
            const tinygltf::Mesh& gltfMesh = model.meshes[i];
            
            for (size_t j = 0; j < gltfMesh.primitives.size(); j++) {
                Mesh mesh;
                const tinygltf::Primitive& primitive = gltfMesh.primitives[j];
                
                // Extract position data
                const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
                const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
                const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);
                
                // Extract normal data if available
                const float* normals = nullptr;
                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                    const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                    const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];
                    normals = reinterpret_cast<const float*>(&normBuffer.data[normView.byteOffset + normAccessor.byteOffset]);
                }
                
                // Process vertices
                for (size_t v = 0; v < posAccessor.count; v++) {
                    Vertex vertex;
                    
                    // Position
                    vertex.position.x = positions[v * 3 + 0];
                    vertex.position.y = positions[v * 3 + 1];
                    vertex.position.z = positions[v * 3 + 2];
                    
                    // Normal (if available)
                    if (normals) {
                        vertex.normal.x = normals[v * 3 + 0];
                        vertex.normal.y = normals[v * 3 + 1];
                        vertex.normal.z = normals[v * 3 + 2];
                    } else {
                        vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f); // Default normal
                    }

                    //Set this up better later, right now we just default loaded models to having vert colors of white
                    vertex.color = glm::vec4(1.0f,1.0f,1.0f,1.0f);
                    
                    mesh.vertices.push_back(vertex);
                }
                
                // Extract indices
                if (primitive.indices >= 0) {
                    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
                    const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
                    
                    // Handle different index types
                    switch (indexAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                            const unsigned short* indices = reinterpret_cast<const unsigned short*>(
                                &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
                            for (size_t i = 0; i < indexAccessor.count; i++) {
                                mesh.indices.push_back(static_cast<unsigned int>(indices[i]));
                            }
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                            const unsigned int* indices = reinterpret_cast<const unsigned int*>(
                                &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
                            for (size_t i = 0; i < indexAccessor.count; i++) {
                                mesh.indices.push_back(indices[i]);
                            }
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                            const unsigned char* indices = reinterpret_cast<const unsigned char*>(
                                &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
                            for (size_t i = 0; i < indexAccessor.count; i++) {
                                mesh.indices.push_back(static_cast<unsigned int>(indices[i]));
                            }
                            break;
                        }
                    }
                }
                
                // Extract material
                if (primitive.material >= 0) {
                    const tinygltf::Material& material = model.materials[primitive.material];
                    
                    // PBR metallic roughness
                    if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
                        mesh.material.baseColor = glm::vec4(
                            material.pbrMetallicRoughness.baseColorFactor[0],
                            material.pbrMetallicRoughness.baseColorFactor[1],
                            material.pbrMetallicRoughness.baseColorFactor[2],
                            material.pbrMetallicRoughness.baseColorFactor[3]
                        );
                    }
                    
                    mesh.material.metallic = material.pbrMetallicRoughness.metallicFactor;
                    mesh.material.roughness = material.pbrMetallicRoughness.roughnessFactor;
                }
                
                //mesh.setupMesh();
                meshes.push_back(std::move(mesh));
            }
        }

        for (const auto& node : model.nodes) {
            bool isEmpty = (node.mesh == -1);
            if (isEmpty && !node.name.empty()) {
                glm::vec3 position(0.0f);
        
                if (!node.translation.empty()) {
                    position = glm::vec3(
                        node.translation[0],
                        node.translation[1],
                        node.translation[2]
                    );
                } else if (!node.matrix.empty()) {
                    glm::mat4 m(1.0f);
                    for (int i = 0; i < 16; ++i)
                        m[i / 4][i % 4] = node.matrix[i];
                    position = glm::vec3(m[3]); // Translation is in last column
                }
                markers.put(node.name.substr(0,node.name.find_last_of('.')),position);
            }
        }

        extractAnimations(model);
        
    }

    void Model::extractAnimations(const tinygltf::Model& model) {
        //print("Found ", model.skins.size(), " skins and ", model.animations.size(), " animations");
        
        // STEP 1: Extract skeleton data from skins
        for (size_t skinIndex = 0; skinIndex < model.skins.size(); skinIndex++) {
            const tinygltf::Skin& skin = model.skins[skinIndex];
            // print("Processing skin ", skinIndex, " with ", skin.joints.size(), " bones");
            
            size_t numBones = skin.joints.size();
            if (numBones == 0) continue;
            
            // Clear any existing bone data
            boneLocalPosition.clear();
            boneLocalScale.clear();
            boneLocalRotation.clear();
            boneWorldTransform.clear();
            boneDirty.clear();
            boneParents.clear();
            boneVerticies.clear();
            boneWeight.clear();
            
            for (size_t i = 0; i < numBones; i++) {
                int nodeIndex = skin.joints[i];
                if (nodeIndex < 0 || nodeIndex >= model.nodes.size()) {
                    print("Warning: Invalid joint node index ", nodeIndex);
                    continue;
                }
                
                const tinygltf::Node& node = model.nodes[nodeIndex];
                
                // Push bone data to auto-resizing q_lists
                boneLocalPosition.push(vec3(
                    node.translation.empty() ? 0 : node.translation[0], 
                    node.translation.empty() ? 0 : node.translation[1], 
                    node.translation.empty() ? 0 : node.translation[2]
                ));
                
                if (!node.rotation.empty() && node.rotation.size() >= 4) {
                    boneLocalRotation.push(glm::quat(
                        node.rotation[3],  // w
                        node.rotation[0],  // x
                        node.rotation[1],  // y
                        node.rotation[2]   // z
                    ));
                } else {
                    boneLocalRotation.push(glm::quat(1, 0, 0, 0)); // Identity
                }
                
                boneLocalScale.push(vec3(
                    node.scale.empty() ? 1 : node.scale[0], 
                    node.scale.empty() ? 1 : node.scale[1], 
                    node.scale.empty() ? 1 : node.scale[2]
                ));

                // Check if skin has inverse bind matrices
                if (skin.inverseBindMatrices >= 0) {
                    const tinygltf::Accessor& ibmAccessor = model.accessors[skin.inverseBindMatrices];
                    const tinygltf::BufferView& ibmBufferView = model.bufferViews[ibmAccessor.bufferView];
                    const tinygltf::Buffer& ibmBuffer = model.buffers[ibmBufferView.buffer];
                    const float* matrices = reinterpret_cast<const float*>(
                        &ibmBuffer.data[ibmBufferView.byteOffset + ibmAccessor.byteOffset]);

                    // Add inverse bind matrix array to your Model class
                    boneInverseBindMatrix.clear();
                    for (size_t i = 0; i < numBones; i++) {
                        glm::mat4 ibm;
                        for (int j = 0; j < 16; j++) {
                            ibm[j/4][j%4] = matrices[i*16 + j];
                        }
                        boneInverseBindMatrix.push(ibm);
                    }
                } else {
                    print("No inverse bind matrices found - using identity matrices");
                    boneInverseBindMatrix.clear();
                    for (size_t i = 0; i < numBones; i++) {
                        boneInverseBindMatrix.push(glm::mat4(1.0f)); // Identity matrix
                    }
                }
                
                // Initialize other bone data
                boneWorldTransform.push(glm::mat4(1.0f));
                boneDirty.push(false);
                boneParents.push(list<size_t>{}); // Empty parent list initially
                boneVerticies.push(list<size_t>{}); // Empty vertex list initially
                boneWeight.push(1.0f);
                
                // print("Bone ", i, " (", node.name, "): pos(", 
                //       boneLocalPosition[i].x(), ",", boneLocalPosition[i].y(), ",", boneLocalPosition[i].z(), ")");
            }
            
            
            // Create a mapping from node index to bone index
            std::map<int, size_t> nodeToBone;
            for (size_t i = 0; i < numBones; i++) {
                nodeToBone[skin.joints[i]] = i;
            }
            
            // Find parent relationships
            for (size_t i = 0; i < numBones; i++) {
                int nodeIndex = skin.joints[i];
                const tinygltf::Node& node = model.nodes[nodeIndex];
                
                // Check if any other bones have this bone as a child
                for (size_t j = 0; j < numBones; j++) {
                    if (i == j) continue;
                    
                    int otherNodeIndex = skin.joints[j];
                    const tinygltf::Node& otherNode = model.nodes[otherNodeIndex];
                    
                    // Check if otherNode has this node as a child
                    for (int childIndex : otherNode.children) {
                        if (childIndex == nodeIndex) {
                            // otherNode (bone j) is parent of node (bone i)
                            boneParents[i].push(j);
                            break;
                        }
                    }
                }
                
            }
            
            
            for (size_t meshIndex = 0; meshIndex < meshes.size(); meshIndex++) {
                Mesh& mesh = meshes[meshIndex];
                
                // Find the corresponding GLTF mesh and primitive
                if (meshIndex < model.meshes.size()) {
                    const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];
                    
                    for (size_t primIndex = 0; primIndex < gltfMesh.primitives.size() && primIndex < 1; primIndex++) {
                        const tinygltf::Primitive& primitive = gltfMesh.primitives[primIndex];
                        
                        // Extract joint indices (which bones affect each vertex)
                        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                            const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.at("JOINTS_0")];
                            const tinygltf::BufferView& jointBufferView = model.bufferViews[jointAccessor.bufferView];
                            const tinygltf::Buffer& jointBuffer = model.buffers[jointBufferView.buffer];
                            
                            // Extract weight data (how much each bone influences each vertex)
                            const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.at("WEIGHTS_0")];
                            const tinygltf::BufferView& weightBufferView = model.bufferViews[weightAccessor.bufferView];
                            const tinygltf::Buffer& weightBuffer = model.buffers[weightBufferView.buffer];
                            
                            //print("Found skinning data for mesh ", meshIndex, " with ", jointAccessor.count, " vertices");
                            
                            // Process vertex skinning data
                            for (size_t vertIndex = 0; vertIndex < jointAccessor.count && vertIndex < mesh.vertices.size(); vertIndex++) {
                                // Extract joint indices (4 bone IDs per vertex)
                                if (jointAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                                    const unsigned short* joints = reinterpret_cast<const unsigned short*>(
                                        &jointBuffer.data[jointBufferView.byteOffset + jointAccessor.byteOffset + vertIndex * 8]);
                                    
                                    mesh.vertices[vertIndex].boneIds = glm::ivec4(joints[0], joints[1], joints[2], joints[3]);
                                } else if (jointAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                                    const unsigned char* joints = reinterpret_cast<const unsigned char*>(
                                        &jointBuffer.data[jointBufferView.byteOffset + jointAccessor.byteOffset + vertIndex * 4]);
                                    
                                    mesh.vertices[vertIndex].boneIds = glm::ivec4(joints[0], joints[1], joints[2], joints[3]);
                                }
                                
                                // Extract weights (4 weights per vertex)
                                const float* weights = reinterpret_cast<const float*>(
                                    &weightBuffer.data[weightBufferView.byteOffset + weightAccessor.byteOffset + vertIndex * 16]);
                                
                                mesh.vertices[vertIndex].boneWeights = glm::vec4(weights[0], weights[1], weights[2], weights[3]);
                                
                                // Build bone->vertex mapping for your SoA system
                                for (int boneSlot = 0; boneSlot < 4; boneSlot++) {
                                    int boneIndex = mesh.vertices[vertIndex].boneIds[boneSlot];
                                    float weight = mesh.vertices[vertIndex].boneWeights[boneSlot];
                                    
                                    if (boneIndex >= 0 && boneIndex < numBones && weight > 0.001f) {
                                        boneVerticies[boneIndex].push(vertIndex);
                                        // Note: You might want to store weights separately if you need them on CPU side
                                    }
                                }
                            }
                            
                            // Regenerate VAO with new vertex data
                            mesh.setupMesh();
                        } else {
                            print("Mesh ", meshIndex, " has no skinning data");
                        }
                    }
                }
            }


            // Clear existing animation data
            animationNames.clear();
            animationDuration.clear();
            animationBoneIndex.clear();
            animationProperty.clear();
            animationTime.clear();
            animationPosition.clear();
            animationRotation.clear();
            animationScale.clear();
            animationInterpolation.clear();
            animationOwner.clear();
    
            for (size_t animIndex = 0; animIndex < model.animations.size(); animIndex++) {
                const tinygltf::Animation& gltfAnim = model.animations[animIndex];
                
                std::string animName = gltfAnim.name.empty() ? ("Animation_" + std::to_string(animIndex)) : gltfAnim.name;
                float maxTime = 0.0f;
                
               // print("Processing animation: ", animName, " with ", gltfAnim.channels.size(), " channels");
                
                // Map animation name to its index for quick lookup
                animationNames.put(animName, animIndex);
                
                // Process each animation channel
                for (size_t chanIndex = 0; chanIndex < gltfAnim.channels.size(); chanIndex++) {
                    const tinygltf::AnimationChannel& gltfChannel = gltfAnim.channels[chanIndex];
                    const tinygltf::AnimationSampler& sampler = gltfAnim.samplers[gltfChannel.sampler];
                    
                    // Find which bone this targets
                    int targetNodeIndex = gltfChannel.target_node;
                    size_t boneIndex = SIZE_MAX;
                    for (size_t i = 0; i < numBones; i++) {
                        if (skin.joints[i] == targetNodeIndex) {
                            boneIndex = i;
                            break;
                        }
                    }
                    
                    if (boneIndex == SIZE_MAX) {
                        print("Warning: Animation targets non-bone node ", targetNodeIndex);
                        continue;
                    }
                    
                    // Convert interpolation type
                    InterpolationType interpType = InterpolationType::LINEAR;
                    if (sampler.interpolation == "STEP") interpType = InterpolationType::STEP;
                    else if (sampler.interpolation == "CUBICSPLINE") interpType = InterpolationType::CUBIC;
                    
                    // Extract keyframe times
                    const tinygltf::Accessor& timeAccessor = model.accessors[sampler.input];
                    const tinygltf::BufferView& timeBufferView = model.bufferViews[timeAccessor.bufferView];
                    const tinygltf::Buffer& timeBuffer = model.buffers[timeBufferView.buffer];
                    const float* times = reinterpret_cast<const float*>(
                        &timeBuffer.data[timeBufferView.byteOffset + timeAccessor.byteOffset]);
                    
                    // Extract keyframe values
                    const tinygltf::Accessor& valueAccessor = model.accessors[sampler.output];
                    const tinygltf::BufferView& valueBufferView = model.bufferViews[valueAccessor.bufferView];
                    const tinygltf::Buffer& valueBuffer = model.buffers[valueBufferView.buffer];
                    const float* values = reinterpret_cast<const float*>(
                        &valueBuffer.data[valueBufferView.byteOffset + valueAccessor.byteOffset]);
                    
                    // Add keyframes to SoA
                    for (size_t keyIndex = 0; keyIndex < timeAccessor.count; keyIndex++) {
                        float keyTime = times[keyIndex];
                        maxTime = std::max(maxTime, keyTime);
                        
                        // Add keyframe data to SoA
                        animationBoneIndex.push(boneIndex);
                        animationProperty.push(gltfChannel.target_path);
                        animationTime.push(keyTime);
                        animationInterpolation.push(interpType);
                        animationOwner.push(animName);
                        
                        // Add type-specific data
                        if (gltfChannel.target_path == "translation") {
                            animationPosition.push(vec3(values[keyIndex*3], values[keyIndex*3+1], values[keyIndex*3+2]));
                            animationRotation.push(glm::quat(1,0,0,0)); // Default values
                            animationScale.push(vec3(1,1,1));
                        } else if (gltfChannel.target_path == "rotation") {
                            animationPosition.push(vec3(0,0,0)); // Default values
                            animationRotation.push(glm::quat(values[keyIndex*4+3], values[keyIndex*4], values[keyIndex*4+1], values[keyIndex*4+2]));
                            animationScale.push(vec3(1,1,1));
                        } else if (gltfChannel.target_path == "scale") {
                            animationPosition.push(vec3(0,0,0)); // Default values
                            animationRotation.push(glm::quat(1,0,0,0));
                            animationScale.push(vec3(values[keyIndex*3], values[keyIndex*3+1], values[keyIndex*3+2]));
                        }
                    }
                    
                    // print("  Channel ", chanIndex, ": bone ", boneIndex, " ", gltfChannel.target_path, 
                    //     " (", timeAccessor.count, " keyframes)");
                }
                
                // Store animation duration
                animationDuration.push(maxTime);
               // print("Animation ", animName, " duration: ", maxTime, " seconds");
            }
    
            updateBoneHierarchy();
            
            // print("Root bones: ");
            for (size_t i = 0; i < numBones; i++) {
                if (boneParents[i].empty()) {
                    // print("  Bone ", i, " at world pos (", 
                    //       boneWorldTransform[i][3][0], ", ", 
                    //       boneWorldTransform[i][3][1], ", ", 
                    //       boneWorldTransform[i][3][2], ")");
                }
            }
            
            break; // Only process first skin for now
        }

    }

    void Model::playAnimation(const std::string& animName) {
        if (animationNames.hasKey(animName)) {
            currentAnimation = animName;
            currentAnimTime = 0.0f;
            isPlaying = true;
        } else {
            print("Animation not found: ", animName,", valid animations are ");
            for(auto e : animationNames.keySet()) {
                print("     ",e);
            }
        }
    }
    
    void Model::updateAnimation(float deltaTime) {
        if (!isPlaying || currentAnimation.empty()) return;
        
        currentAnimTime += deltaTime;
        
        // Get animation duration
        size_t animIndex = animationNames.get(currentAnimation);
        float duration = animationDuration[animIndex];
        
        // Loop animation
        if (currentAnimTime >= duration) {
            currentAnimTime = fmod(currentAnimTime, duration);
        }
        
        // Apply animation to skeleton
        applyAnimationFrame(currentAnimTime);
    }
    
    void Model::applyAnimationFrame(float time) {
        if (currentAnimation.empty()) return;
        
        // Find all unique bones that are animated in current animation
        list<size_t> animatedBones;
        for (size_t i = 0; i < animationOwner.length(); i++) {
            if (animationOwner[i] == currentAnimation) {
                if(!animatedBones.has(animationBoneIndex[i])) animatedBones << (animationBoneIndex[i]);
            }
        }
        
        // Update each animated bone
        for (size_t boneIndex : animatedBones) {
            // Interpolate position
            size_t key1, key2;
            float alpha;
            findKeyframes(currentAnimation, boneIndex, "translation", time, key1, key2, alpha);
            if (key1 != SIZE_MAX) {
                if (key2 != SIZE_MAX) {
                    // Interpolate between two keyframes
                    vec3 pos1 = animationPosition[key1];
                    vec3 pos2 = animationPosition[key2];
                    boneLocalPosition[boneIndex] = vec3(
                        pos1.x() + alpha * (pos2.x() - pos1.x()),
                        pos1.y() + alpha * (pos2.y() - pos1.y()),
                        pos1.z() + alpha * (pos2.z() - pos1.z())
                    );
                } else {
                    // Use single keyframe
                    boneLocalPosition[boneIndex] = animationPosition[key1];
                }
            }
            
            // Interpolate rotation
            findKeyframes(currentAnimation, boneIndex, "rotation", time, key1, key2, alpha);
            if (key1 != SIZE_MAX) {
                if (key2 != SIZE_MAX) {
                    // Spherical linear interpolation
                    boneLocalRotation[boneIndex] = glm::slerp(animationRotation[key1], animationRotation[key2], alpha);
                } else {
                    boneLocalRotation[boneIndex] = animationRotation[key1];
                }
            }
            
            // Interpolate scale
            findKeyframes(currentAnimation, boneIndex, "scale", time, key1, key2, alpha);
            if (key1 != SIZE_MAX) {
                if (key2 != SIZE_MAX) {
                    vec3 scale1 = animationScale[key1];
                    vec3 scale2 = animationScale[key2];
                    boneLocalScale[boneIndex] = vec3(
                        scale1.x() + alpha * (scale2.x() - scale1.x()),
                        scale1.y() + alpha * (scale2.y() - scale1.y()),
                        scale1.z() + alpha * (scale2.z() - scale1.z())
                    );
                } else {
                    boneLocalScale[boneIndex] = animationScale[key1];
                }
            }
        }
    }
    
    void Model::findKeyframes(const std::string& animName, size_t boneIndex, const std::string& property, 
                             float time, size_t& keyframe1, size_t& keyframe2, float& alpha) {
        keyframe1 = SIZE_MAX;
        keyframe2 = SIZE_MAX;
        alpha = 0.0f;
        
        // Find keyframes for this bone and property
        std::vector<size_t> matchingKeys;
        for (size_t i = 0; i < animationOwner.length(); i++) {
            if (animationOwner[i] == animName && 
                animationBoneIndex[i] == boneIndex && 
                animationProperty[i] == property) {
                matchingKeys.push_back(i);
            }
        }
        
        if (matchingKeys.empty()) return;
        
        // Find the keyframes to interpolate between
        for (size_t i = 0; i < matchingKeys.size(); i++) {
            size_t keyIndex = matchingKeys[i];
            float keyTime = animationTime[keyIndex];
            
            if (time <= keyTime) {
                keyframe2 = keyIndex;
                if (i > 0) {
                    keyframe1 = matchingKeys[i-1];
                    float t1 = animationTime[keyframe1];
                    float t2 = animationTime[keyframe2];
                    alpha = (time - t1) / (t2 - t1);
                } else {
                    keyframe1 = keyIndex; // Use first keyframe
                }
                return;
            }
        }
        
        // Time is beyond all keyframes, use last one
        keyframe1 = matchingKeys.back();
    }

    void Model::updateBone(size_t boneIndex) {
        if (boneIndex >= boneLocalPosition.length()) return;
        
        // Compose local transform matrix from components
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), boneLocalPosition[boneIndex].toGlm());
        glm::mat4 rotation = glm::mat4_cast(boneLocalRotation[boneIndex]);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), boneLocalScale[boneIndex].toGlm());
        
        // Combine in TRS order (Translation * Rotation * Scale)
        glm::mat4 localTransform = translation * rotation * scale;
        
        // Apply parent transform if this bone has parents
        if (boneParents[boneIndex].length()>0) {
            // For now, assume single parent (most common case)
            size_t parentId = boneParents[boneIndex][0];
            
            // World transform = parent's world transform * this bone's local transform
            boneWorldTransform[boneIndex] = boneWorldTransform[parentId] * localTransform;
        } else {
            // Root bone - local transform becomes world transform
            boneWorldTransform[boneIndex] = localTransform;
        }

        boneWorldTransform[boneIndex] = boneWorldTransform[boneIndex] * boneInverseBindMatrix[boneIndex];
    }

    void Model::updateBoneHierarchy() {
        if (boneLocalPosition.length()<=0) return;
    
        for (size_t i = 0; i < boneDirty.length(); i++) {
            boneDirty[i] = false;
        }
        
        for (size_t i = 0; i < boneLocalPosition.length(); i++) {
            if (boneDirty[i]) continue; 
            
            for (size_t p=0;p<boneParents.get(i).length();p++) {
                size_t parentId = boneParents.get(i).get(p);
                // if(parentId<0) continue;
                if (!boneDirty[parentId]) {
                    // Parent not processed yet - process it first
                    updateBone(parentId);
                    boneDirty[parentId] = true;
                }
            }
            
            // Now process this bone (all parents guaranteed to be ready)
            updateBone(i);
            boneDirty[i] = true;
        }
    }

    void Model::uploadBoneMatrices(unsigned int shaderProgram) {
        if (!hasBones()) return;
        
        // Upload bone matrices to shader
        GLint boneMatricesLoc = glGetUniformLocation(shaderProgram, "boneMatrices");
        if (boneMatricesLoc != -1 && boneWorldTransform.length()>0) {
            glUniformMatrix4fv(boneMatricesLoc, 
                            boneWorldTransform.length(), 
                            GL_FALSE, 
                            glm::value_ptr(boneWorldTransform.get(0))); //This may be problmatic, try another version or make this const.
        }
    }

    void Model::saveBinary(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: " + path);
        saveBinary(out);
        out.close();
    }

    void Model::loadBinary(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read from file: " + path);
        loadBinary(in);
        in.close();
    }

    void Model::saveBinary(std::ostream& out) const {

        // Optional: Write file header "GMDL"
        const char magic[] = "GMDL";
        out.write(magic, 4);

        uint32_t meshCount = meshes.size();
        out.write(reinterpret_cast<const char*>(&meshCount), sizeof(meshCount));

        for (const auto& mesh : meshes) {
            uint32_t vertexCount = mesh.vertices.size();
            uint32_t indexCount = mesh.indices.size();

            out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            out.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));

            out.write(reinterpret_cast<const char*>(mesh.vertices.data()), vertexCount * sizeof(Vertex));
            out.write(reinterpret_cast<const char*>(mesh.indices.data()), indexCount * sizeof(unsigned int));

            // Write material (optional, just color for now)
            out.write(reinterpret_cast<const char*>(&mesh.material.baseColor), sizeof(glm::vec4));
            out.write(reinterpret_cast<const char*>(&mesh.material.metallic), sizeof(float));
            out.write(reinterpret_cast<const char*>(&mesh.material.roughness), sizeof(float));
        }
    }

    void Model::loadBinary(std::istream& in) {

        // Optional: check magic header
        char magic[4];
        in.read(magic, 4);
        if (std::strncmp(magic, "GMDL", 4) != 0)
            throw std::runtime_error("Invalid file format");

        uint32_t meshCount;
        in.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount));
        meshes.clear();
        meshes.reserve(meshCount);

        for (uint32_t i = 0; i < meshCount; ++i) {
            uint32_t vertexCount, indexCount;
            in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
            in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

            Mesh mesh;
            mesh.vertices.resize(vertexCount);
            mesh.indices.resize(indexCount);

            in.read(reinterpret_cast<char*>(mesh.vertices.data()), vertexCount * sizeof(Vertex));
            in.read(reinterpret_cast<char*>(mesh.indices.data()), indexCount * sizeof(unsigned int));

            // Read material (baseColor, metallic, roughness)
            in.read(reinterpret_cast<char*>(&mesh.material.baseColor), sizeof(glm::vec4));
            in.read(reinterpret_cast<char*>(&mesh.material.metallic), sizeof(float));
            in.read(reinterpret_cast<char*>(&mesh.material.roughness), sizeof(float));

            mesh.setupMesh();  // Setup VAO/VBO
            meshes.push_back(std::move(mesh));
        }

        preComputeAABB();  // Recompute bounds
    }



    // void Model::addInstance(g_ptr<S_Object> instance) {
    //     instance->ID = instances.size();
    //     instances.push_back(instance);
    //     instanceTransforms.push_back(glm::mat4(1.0f));

    //     static bool checkedInstanced = false;
    //     if(!checkedInstanced) 
    //     {
    //         this->instance=true;
    //         for (auto& mesh : meshes)
    //         {
    //             mesh.instance = true;
    //         }
    //         checkedInstanced=true;
    //     }
        
    //     if (!meshes.empty() && meshes[0].instanceVBO != 0) {
    //         //requestInstanceUpdate();
    //         updateInstanceBuffers();
    //     } else if (!instanceTransforms.empty()) {
    //         setupInstancing(instanceTransforms);
    //     }
    // }

    // void Model::removeInstance(size_t uid) {
    //     Object* obj = scene->uidobjects.get(uid).get();
    //     size_t index = obj->id;
    //     size_t lastIdx = instanceTransforms.size() - 1;
    //     if (index >= instanceTransforms.size()) {
    //         std::cerr << "Invalid instance index: " << index << std::endl;
    //         return;
    //     }

    //     // Swap with last if it's not already last
    //     if (index != lastIdx) {
    //         std::swap(instanceTransforms[index], instanceTransforms[lastIdx]);
    //         instances[lastIdx]->id = obj->id;
    //         std::swap(instances[index], instances[lastIdx]);
    //     }

    //     instanceTransforms.pop_back();
    //     instances.pop_back();

    //     for (auto& mesh : meshes) {
    //         if (mesh.instanceVBO == 0) {
    //             std::cerr << "Error: instanceVBO not initialized\n";
    //             continue;
    //         }

    //         glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVBO);
    //         glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4), 
    //                     instanceTransforms.data(), GL_DYNAMIC_DRAW);
    //         glBindBuffer(GL_ARRAY_BUFFER, 0);
    //         mesh.instanceCount = instanceTransforms.size();
    //         }
    // }

    void Model::requestInstanceUpdate()
    {
        if(!waitingForInstanceUpdate)
        {
            waitingForInstanceUpdate = true;
            // Sim::get().queueMainThreadCall([this](){
            //         for (auto& mesh : meshes) mesh.fullInstanceUpdate(instanceTransforms);
            //         waitingForInstanceUpdate = false;
            // });
        }
    }
}


