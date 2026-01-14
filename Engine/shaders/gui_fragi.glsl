#version 410 core

in vec2 uv;
in vec4 data;

uniform sampler2D uTex;
uniform bool useTexture;

out vec4 FragColor;

void main() {
    FragColor = useTexture ? texture(uTex, uv) : data;
}