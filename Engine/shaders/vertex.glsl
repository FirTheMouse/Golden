#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in ivec4 aBoneIds;
layout(location = 4) in vec4 aBoneWeights;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform vec4 color;

uniform mat4 boneMatrices[100];
uniform bool hasSkeleton = false;

out vec3 FragPos;
out vec3 Normal;
out vec4 vertexColor;
out vec4 FragPosLightSpace;
void main() {
    vec4 localPos = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;
    
    if (hasSkeleton) {
        mat4 boneTransform = 
            boneMatrices[aBoneIds[0]] * aBoneWeights[0] +
            boneMatrices[aBoneIds[1]] * aBoneWeights[1] +
            boneMatrices[aBoneIds[2]] * aBoneWeights[2] +
            boneMatrices[aBoneIds[3]] * aBoneWeights[3];
        
        localPos = boneTransform * localPos;
        mat3 normalMatrix = mat3(transpose(inverse(boneTransform)));
        localNormal = normalMatrix * localNormal;
    }
    
    // Continue with your existing transform pipeline
    vec4 worldPos = model * localPos;
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * localNormal;
    vertexColor = aColor * color;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}



