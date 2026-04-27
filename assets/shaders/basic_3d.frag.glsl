#version 450

// Fragment Input - Vertex shader'dan gelen interpolated data
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragNormal;

// Fragment Output - Final renk (swapchain'e yazılacak)
layout(location = 0) out vec4 outColor;

void main() {
    // Basit directional light (yukarıdan gelen ışık)
    vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3));
    vec3 normal = normalize(fragNormal);
    
    // Diffuse lighting (Lambertian)
    float diffuse = max(dot(normal, -lightDir), 0.0);
    
    // Ambient + Diffuse
    float ambient = 0.3;
    float lighting = ambient + diffuse * 0.7;
    
    // Vertex color ile lighting'i birleştir
    vec3 finalColor = fragColor * lighting;
    
    // Final output (alpha = 1.0 opaque)
    outColor = vec4(finalColor, 1.0);
}
