#pragma once
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

namespace Astral {
    /**
     * @brief GPU dostu Vertex yapısı.
     * 3D Rendering için optimize edilmiş bellek dizilimi.
     */
    struct Vertex {
        glm::vec3 pos;      // Location 0
        glm::vec3 normal;   // Location 1
        glm::vec2 uv;       // Location 2
        glm::vec4 tangent;  // Location 3 (xyz = dir, w = handedness)
        glm::vec4 color;    // Location 4 (RGBA)

        Vertex() = default;
        Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& u, const glm::vec4& t, const glm::vec4& c)
            : pos(p), normal(n), uv(u), tangent(t), color(c) {}

        // Toplam: 64 bytes (Cache line alignment için mükemmel)

        static void getAttributeDescriptions(SDL_GPUVertexAttribute* attrs) {
            attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[0].offset = offsetof(Vertex, pos);
            attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[1].offset = offsetof(Vertex, normal);
            attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; attrs[2].offset = offsetof(Vertex, uv);
            attrs[3].location = 3; attrs[3].buffer_slot = 0; attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; attrs[3].offset = offsetof(Vertex, tangent);
            attrs[4].location = 4; attrs[4].buffer_slot = 0; attrs[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; attrs[4].offset = offsetof(Vertex, color);
        }
        
        static SDL_GPUVertexBufferDescription getBindingDescription() {
            SDL_GPUVertexBufferDescription desc;
            desc.slot = 0;
            desc.pitch = sizeof(Vertex);
            desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            desc.instance_step_rate = 0;
            return desc;
        }
    };
}
