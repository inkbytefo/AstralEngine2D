#version 450

// Vertex Input (location'lar VertexLayout ile eşleşmeli)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

// Vertex Output -> Fragment Shader Input
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

// Uniform Buffer - MVP Matrisleri
// SDL_GPU: set=1, binding=0 (Vertex Uniform Slot 0)
layout(set = 1, binding = 0) uniform UniformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    // MVP transformation
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Pass-through
    fragColor = inColor;
    fragUV = inUV;
}
