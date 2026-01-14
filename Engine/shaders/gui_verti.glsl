#version 410 core
layout(location = 0) in vec2 aPos;    
layout(location = 1) in vec2 aUV; 

// Per-instance attributes
layout(location = 2) in mat4 iTransform;  // Instance transform (takes locations 2,3,4,5)
layout(location = 6) in vec4 iData;       // Instance data (color or uvRect)

uniform vec2 uResolution;
uniform bool useTexture;

out vec2 uv;
out vec4 data;

void main() {
    // Apply instance transform to vertex
    vec4 scaled = iTransform * vec4(aPos, 0.0, 1.0);
    vec2 ndc = (scaled.xy / uResolution) * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y axis for screen space
    gl_Position = vec4(ndc, 0.0, 1.0);
    
    // Calculate UV using instance data
    uv = iData.xy + aUV * iData.zw;
    data = iData;  // Pass through for fragment shader
}