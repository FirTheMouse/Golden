#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;
layout (location = 3) in mat4 instanceModel;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
out vec3 FragPos;
out vec3 Normal;
out vec4 vertexColor;
out vec4 FragPosLightSpace;

void main() {
    vec4 worldPos = instanceModel * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
    vertexColor = aColor;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}