#pragma once

#include "../../common.h"
#include <glm/glm.hpp>

namespace Astral {

    // GPU'ya gönderilmeye hazır, sıkı paketlenmiş (tightly packed) vertex yapısı
    // Boyut: 12 (pos) + 12 (color) + 8 (uv) = 32 bytes! (Cache-line dostu)
    struct Vertex {
        glm::vec3 position;  // 12 bytes
        glm::vec3 color;     // 12 bytes
        glm::vec2 uv;        // 8 bytes
        // Toplam: 32 bytes (optimal!)

        Vertex() = default;
        
        Vertex(const glm::vec3& pos, const glm::vec3& col, const glm::vec2& texCoord)
            : position(pos), color(col), uv(texCoord) {}
    };

    // Shader'da kullanılacak vertex attribute açıklamaları
    // SDL_GPUVertexInputState için gerekli bilgiler
    struct VertexLayout {
        static constexpr uint32_t BINDING_INDEX = 0;
        static constexpr uint32_t STRIDE = sizeof(Vertex);
        
        // Attribute locations
        enum Attributes : uint32_t {
            POSITION = 0,
            COLOR = 1,
            UV = 2
        };

        // SDL_GPUVertexAttribute dizisi döndür
        static void GetAttributes(SDL_GPUVertexAttribute* attributes) {
            // Position (location = 0)
            attributes[0].location = POSITION;
            attributes[0].buffer_slot = BINDING_INDEX;
            attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attributes[0].offset = offsetof(Vertex, position);

            // Color (location = 1)
            attributes[1].location = COLOR;
            attributes[1].buffer_slot = BINDING_INDEX;
            attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attributes[1].offset = offsetof(Vertex, color);

            // UV (location = 2)
            attributes[2].location = UV;
            attributes[2].buffer_slot = BINDING_INDEX;
            attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            attributes[2].offset = offsetof(Vertex, uv);
        }
    };

} // namespace Astral
