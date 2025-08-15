#version 410 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 quad;
uniform vec2 uResolution;
uniform vec4 uvRect;

out vec2 uv;

void main() {
    vec4 scaled = quad * vec4(aPos, 0.0, 1.0);
    vec2 ndc = (scaled.xy / uResolution) * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y axis for screen space
    gl_Position = vec4(ndc, 0.0, 1.0);
    uv = uvRect.xy + aUV * uvRect.zw;
}