#version 450

// Fragment Input (Vertex Shader'dan gelen)
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

// Fragment Output
layout(location = 0) out vec4 outColor;

void main() {
    // Basit vertex color rendering
    outColor = vec4(fragColor, 1.0);
}
