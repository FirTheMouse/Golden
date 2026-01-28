// //GUI uninstanced vertice shader

// #version 410 core
// layout(location = 0) in vec2 aPos;
// layout(location = 1) in vec2 aUV;

// uniform mat4 quad;
// uniform vec2 uResolution;
// uniform vec4 data;
// uniform bool useTexture;

// out vec2 uv;
// out vec2 rawUV;  

// void main() {
//     vec4 scaled = quad * vec4(aPos, 0.0, 1.0);
//     vec2 ndc = (scaled.xy / uResolution) * 2.0 - 1.0;
//     ndc.y = -ndc.y;
//     gl_Position = vec4(ndc, 0.0, 1.0);
//     uv = data.xy + aUV * data.zw;
//     rawUV = aUV;  // untransformed 0-1 coords
// }

// GUI uninstanced vertex shader
#version 410 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 quad;
uniform vec2 uResolution;
uniform vec4 data;

out vec2 uv;
out vec2 rawUV;

void main() {
    vec4 scaled = quad * vec4(aPos, 0.0, 1.0);
    vec2 ndc = (scaled.xy / uResolution) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, quad[3][3], 1.0);
    
    uv = data.xy + aUV * data.zw;
    rawUV = aUV;
}