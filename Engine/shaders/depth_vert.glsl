#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aBoneWeights; 

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

uniform mat4 boneMatrices[100];
uniform bool hasSkeleton = false;

void main()
{
  vec4 localPos = vec4(aPos, 1.0);
    if (hasSkeleton) {
        mat4 boneTransform = 
            boneMatrices[aBoneIds[0]] * aBoneWeights[0] +
            boneMatrices[aBoneIds[1]] * aBoneWeights[1] +
            boneMatrices[aBoneIds[2]] * aBoneWeights[2] +
            boneMatrices[aBoneIds[3]] * aBoneWeights[3];
        
        localPos = boneTransform * localPos;
    }
    
    gl_Position = lightSpaceMatrix * model * localPos;
}