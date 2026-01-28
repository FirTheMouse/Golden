// #version 410 core

// in vec2 uv;

// uniform vec4 data;
// uniform sampler2D uTex;
// uniform bool useTexture;

// out vec4 FragColor;

// void main() {
//     FragColor = useTexture ? texture(uTex, uv) : data;
// }

//GUI uninstanced fragment shader

// #version 410 core

// in vec2 uv;
// in vec2 rawUV;

// uniform vec4 data;
// uniform sampler2D uTex;
// uniform bool useTexture;

// uniform int subdivisions;
// uniform vec4 subData[32];
// uniform int subSizes[32];
// uniform vec2 subVerts[256];
// uniform int subOffsets[32];

// out vec4 FragColor;

// bool pointInPolygon(vec2 point, int offset, int count) {
//     bool inside = false;
//     for(int i = 0; i < count; i++) {
//         int j = (i == 0) ? count - 1 : i - 1;
//         vec2 pi = subVerts[offset + i];
//         vec2 pj = subVerts[offset + j];
        
//         if(((pi.y > point.y) != (pj.y > point.y)) &&
//            (point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)) {
//             inside = !inside;
//         }
//     }
//     return inside;
// }

// void main() {
//     if (subdivisions == 0) {
//         FragColor = useTexture ? texture(uTex, uv) : data;
//         return;
//     }
    
//     // Use rawUV for region testing (0-1 range)
//     for(int r = 0; r < subdivisions; r++) {
//         if(pointInPolygon(rawUV, subOffsets[r], subSizes[r])) {
//             FragColor = subData[r];
//             return;
//         }
//     }
    
//     FragColor = data;
// }

// GUI uninstanced fragment shader
#version 410 core

in vec2 uv;
in vec2 rawUV;

uniform vec4 color;
uniform sampler2D uTex;
uniform bool useTexture;

uniform int subdivisions;
uniform vec4 subData[32];
uniform int subSizes[32];
uniform vec2 subVerts[256];
uniform int subOffsets[32];

out vec4 FragColor;

bool pointInPolygon(vec2 point, int offset, int count) {
    bool inside = false;
    for(int i = 0; i < count; i++) {
        int j = (i == 0) ? count - 1 : i - 1;
        vec2 pi = subVerts[offset + i];
        vec2 pj = subVerts[offset + j];
        
        if(((pi.y > point.y) != (pj.y > point.y)) &&
           (point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

void main() {
    vec4 baseColor = color;
    
    // Check subdivisions for region-specific colors
    if (subdivisions > 0) {
        for(int r = 0; r < subdivisions; r++) {
            if(pointInPolygon(rawUV, subOffsets[r], subSizes[r])) {
                baseColor = subData[r];
                break;
            }
        }
    }
    
    // Apply texture if present
    if(useTexture) {
        vec4 texColor = texture(uTex, uv);
        FragColor = texColor * baseColor;
    } else {
        FragColor = baseColor;
    }
}