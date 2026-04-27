#version 450

// Vertex shader'dan gelen veriler
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragNormal;

// Çıktı
layout(location = 0) out vec4 outColor;

// SDL_GPU Fragment Shader Binding:
// Sampler binding - slot 0
layout(set = 2, binding = 0) uniform sampler2D albedoTex;

// Fragment Uniforms (SDL_GPU set = 3, binding = 0)
layout(set = 3, binding = 0) uniform FragmentPushConstants {
    vec4 baseColor;
    int hasTexture;
    int _padding[3];
} fragUniforms;

void main() {
    vec4 texColor = vec4(1.0);
    
    // Texture varsa oku
    if (fragUniforms.hasTexture > 0) {
        texColor = texture(albedoTex, fragTexCoord);
    }
    
    // Sadece texture'ı göster (Vertex rengini şimdilik devre dışı bırakalım ki texture net görünsün)
    vec3 finalColor = texColor.rgb * fragUniforms.baseColor.rgb;
    
    outColor = vec4(finalColor, texColor.a * fragUniforms.baseColor.a);
}
