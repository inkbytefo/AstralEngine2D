#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

layout(set = 1, binding = 0) uniform UniformData {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragTexCoord = inTexCoord;

    // Normal Matrix (inverse transpose of 3x3 model matrix)
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    
    fragTangent = normalize(normalMatrix * inTangent.xyz);
    // Bitangent with handedness
    fragBitangent = normalize(cross(fragNormal, fragTangent) * inTangent.w);

    gl_Position = ubo.projection * ubo.view * worldPos;
}
