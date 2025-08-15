#version 410 core

in vec2 uv;

uniform vec4 uColor;
uniform sampler2D uTex;
uniform bool useTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = useTexture ? texture(uTex, uv) : vec4(1.0);
    FragColor = uColor * texColor;
}
