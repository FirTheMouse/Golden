#version 410 core
in vec3 FragPos;
in vec3 Normal;
in vec4 vertexColor;
in vec4 FragPosLightSpace;
out vec4 FragColor;
struct Material {
    vec4 baseColor;
    float metallic;
    float roughness;
};
uniform Material material;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 moonDirection;
uniform vec3 moonColor;
uniform vec3 lightPositions[16];
uniform vec3 lightColors[16];
uniform vec3 camPos;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float texelSize = 1.0 / 2048.0;

    for(int x = -1; x <= 1; ++x)
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            if (projCoords.z - bias > pcfDepth)
                shadow += 1.0;
        }

    shadow /= 9.0;
    return shadow;
}



void main() {
    vec3 albedo = material.baseColor.rgb * vertexColor.rgb;
    float alpha = material.baseColor.a * vertexColor.a;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = 1.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    vec3 ambient = vec3(0.02) * albedo * ao;

    // === Point Lights ===
    for (int i = 0; i < 16; ++i) {
        vec3 L = normalize(lightPositions[i] - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 specular = (NDF * G * F) /
            max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // === Sun Light + Bounce ===
    if (sunDirection.y >= 0.0) {
        vec3 L = normalize(sunDirection);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float shadow = ShadowCalculation(FragPosLightSpace, N, L);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 specular = (NDF * G * F) /
            max(4.0 * max(dot(N, V), 0.0) * NdotL, 0.0001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 direct = (kD * albedo / PI + specular) * sunColor.rgb * NdotL * (1.0 - shadow);
        Lo += direct;

        // Bounce if NdotL is low (surface not facing the sun)
        if (NdotL < 0.3) {
            float bounceFactor = (0.3 - NdotL) / 0.3; // range 0 to 1
            vec3 bounceLight = (kD * albedo / PI) * sunColor.rgb * 0.15 * bounceFactor;
            ambient += bounceLight;
        }
    }

    // === Moon Light + Bounce ===
    {
        vec3 L = normalize(moonDirection);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float shadow = ShadowCalculation(FragPosLightSpace, N, L);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 specular = (NDF * G * F) /
            max(4.0 * max(dot(N, V), 0.0) * NdotL, 0.0001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 direct = (kD * albedo / PI + specular) * moonColor.rgb * NdotL * (1.0 - shadow);
        Lo += direct;

        // Bounce light if weakly lit
        if (NdotL < 0.3) {
            float bounceFactor = (0.3 - NdotL) / 0.3;
            vec3 bounceLight = (kD * albedo / PI) * moonColor.rgb * 0.05 * bounceFactor;
            ambient = (ambient*0.5f)+bounceLight;
        }
    }

    // === Final Color ===
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0)); // HDR tonemapping
    color = pow(color, vec3(1.0 / 2.2)); // gamma correction
    FragColor = vec4(color, alpha);
}


