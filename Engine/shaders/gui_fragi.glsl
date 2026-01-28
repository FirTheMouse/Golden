//gui instanced fragment shader

// #version 410 core

// in vec2 uv;
// in vec4 data;

// uniform sampler2D uTex;
// uniform bool useTexture;

// out vec4 FragColor;

// void main() {
//     FragColor = useTexture ? texture(uTex, uv) : data;
// }


// GUI instanced fragment shader
#version 410 core

in vec2 uv;
in vec4 rColor;

uniform sampler2D uTex;
uniform bool useTexture;

out vec4 FragColor;

void main() {
       if(useTexture) {
        vec4 texColor = texture(uTex, uv);
        if(texColor.a < 0.01) discard;  // Discard fully transparent pixels
        FragColor = texColor * rColor;
    } else {
        FragColor = rColor;
    }
}