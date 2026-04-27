#pragma once
#include <glm/glm.hpp>
#include <SDL3/SDL_gpu.h>

namespace Astral {
    /**
     * @brief GPU dostu Vertex yapısı.
     * 3D Rendering için optimize edilmiş bellek dizilimi.
     */
    struct Vertex {
        glm::vec3 pos;      // 12 bytes
        glm::vec3 color;    // 12 bytes
        glm::vec2 uv;       // 8 bytes

        Vertex() = default;
        Vertex(const glm::vec3& p, const glm::vec3& c, const glm::vec2& u)
            : pos(p), color(c), uv(u) {}

        // Toplam: 32 bytes (GPU alignment için uygun)

        static void getAttributeDescriptions(SDL_GPUVertexAttribute* attrs) {
            // Position: Location 0
            attrs[0].location = 0;
            attrs[0].buffer_slot = 0;
            attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attrs[0].offset = offsetof(Vertex, pos);

            // Color: Location 1
            attrs[1].location = 1;
            attrs[1].buffer_slot = 0;
            attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attrs[1].offset = offsetof(Vertex, color);

            // UV: Location 2
            attrs[2].location = 2;
            attrs[2].buffer_slot = 0;
            attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            attrs[2].offset = offsetof(Vertex, uv);
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
