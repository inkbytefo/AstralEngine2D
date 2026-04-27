#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;

// Texture Samplers (Set 2 for samplers in SDL_GPU)
layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D metallicRoughnessMap;

// Fragment Uniforms (Set 3 for uniform buffers in SDL_GPU)
layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 baseColor;         // 0-15
    vec4 lightDirInt;       // 16-31 (xyz = dir, w = intensity)
    vec4 lightColorMet;     // 32-47 (xyz = color, w = metallic)
    vec4 camPosRoughness;   // 48-63 (xyz = camPos, w = roughness)
    
    int useAlbedoMap;       // 64-67
    int useNormalMap;       // 68-71
    int useMetallicRoughnessMap; // 72-75
    int _padding;           // 76-79
} ubo;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); 
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Unpack uniforms safely
    vec3 lightDir = ubo.lightDirInt.xyz;
    float lightIntensity = ubo.lightDirInt.w;
    vec3 lightColor = ubo.lightColorMet.xyz;
    float metallic = ubo.lightColorMet.w;
    vec3 camPos = ubo.camPosRoughness.xyz;
    float roughness = ubo.camPosRoughness.w;

    vec3 albedo = ubo.baseColor.rgb;
    if (ubo.useAlbedoMap > 0) {
        albedo *= pow(texture(albedoMap, fragTexCoord).rgb, vec3(2.2)); 
    }

    if (ubo.useMetallicRoughnessMap > 0) {
        vec4 mr = texture(metallicRoughnessMap, fragTexCoord);
        roughness *= mr.g;
        metallic *= mr.b;
    }

    vec3 N = normalize(fragNormal);
    if (ubo.useNormalMap > 0) {
        vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
        mat3 TBN = mat3(normalize(fragTangent), normalize(fragBitangent), N);
        N = normalize(TBN * tangentNormal);
    }

    vec3 V = normalize(camPos - fragWorldPos);
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    vec3 radiance = lightColor * lightIntensity;

    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular     = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);        
    
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color, 1.0);
}
