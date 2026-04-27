#version 450

// Vertex Input - Vertex struct'ımızla eşleşmeli (position, color, uv)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// Vertex Output - Fragment shader'a gönderilecek
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragNormal;

// Uniform Buffer - MVP Matrisleri (Push Constants)
layout(push_constant) uniform UniformData {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    // World space pozisyon
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    // Clip space pozisyon (GPU'nun kullanacağı final pozisyon)
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Normal hesaplama (basit versiyon - non-uniform scale için düzeltme gerekebilir)
    // Gerçek projede: mat3(transpose(inverse(ubo.model))) * normal
    fragNormal = mat3(ubo.model) * normalize(inPosition);
    
    // Renk ve texture coordinate'leri fragment shader'a aktar
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
