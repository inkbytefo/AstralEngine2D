#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../core/entity_manager.h"
#include "../core/asset_manager.h"
#include "../ecs/components.h"

namespace Astral {

// Uniform Data - Shader'a gönderilecek MVP matrisleri
// GLSL std140 layout kurallarına uygun (mat4 = 64 bytes)
struct UniformData {
    glm::mat4 model;      // 64 bytes
    glm::mat4 view;       // 64 bytes
    glm::mat4 projection; // 64 bytes
    // Toplam: 192 bytes (SDL_GPU için uygun boyut)
};

// Modern 3D Rendering System - SDL_GPU ile
class RenderSystem
{
public:
    // Render Pass başlatma ve temel setup
    static void beginRenderPass(
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPUTexture* renderTarget,
        SDL_GPUTexture* depthTarget = nullptr,
        const glm::vec4& clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f))
    {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = renderTarget;
        colorTarget.mip_level = 0;
        colorTarget.layer_or_depth_plane = 0;
        colorTarget.clear_color.r = clearColor.r;
        colorTarget.clear_color.g = clearColor.g;
        colorTarget.clear_color.b = clearColor.b;
        colorTarget.clear_color.a = clearColor.a;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        colorTarget.resolve_texture = nullptr;
        colorTarget.resolve_mip_level = 0;
        colorTarget.resolve_layer = 0;
        colorTarget.cycle = true;
        colorTarget.cycle_resolve_texture = false;

        SDL_GPUDepthStencilTargetInfo depthStencilTarget = {};
        SDL_GPUDepthStencilTargetInfo* depthPtr = nullptr;

        if (depthTarget) {
            depthStencilTarget.texture = depthTarget;
            depthStencilTarget.clear_depth = 1.0f;
            depthStencilTarget.load_op = SDL_GPU_LOADOP_CLEAR;
            depthStencilTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
            depthStencilTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
            depthStencilTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
            depthStencilTarget.cycle = true;
            depthStencilTarget.clear_stencil = 0;
            depthStencilTarget.mip_level = 0;
            depthStencilTarget.layer = 0;
            depthPtr = &depthStencilTarget;
        }

        s_currentRenderPass = SDL_BeginGPURenderPass(
            commandBuffer,
            &colorTarget,
            1,
            depthPtr
        );
    }

    // Saf ECS Yaklaşımı: Kamerayı entity'lerden bul!
    static void update(
        EntityManager& entityManager,
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPURenderPass* renderPass = nullptr)
    {
        // Eğer dışarıdan render pass verilmişse onu kullan
        if (renderPass) {
            s_currentRenderPass = renderPass;
        }
        
        if (!s_currentRenderPass) {
            SDL_Log("RenderSystem: Render pass başlatılmamış!");
            return;
        }

        // 1. Sahnedeki aktif kamerayı bul
        glm::mat4 viewMatrix(1.0f);
        glm::mat4 projMatrix(1.0f);
        bool cameraFound = false;

        for (auto& entity : entityManager.getEntities()) {
            // has<T>() template fonksiyonunu kullan
            if (entity->has<CCamera>()) {
                auto& cam = entity->get<CCamera>();
                if (cam.isActive) {
                    viewMatrix = cam.view;
                    projMatrix = cam.projection;
                    cameraFound = true;
                    break;
                }
            }
        }

        if (!cameraFound) {
            SDL_LogWarn(0, "RenderSystem: Sahnede aktif bir CCamera bulunamadı!");
            // Default matrislerle devam et
            return; // Kamera yoksa çizme
        }

        Astral::AssetManager& assetMgr = Astral::AssetManager::getInstance();

        // 2. State tracking - gereksiz bind'ları önle
        SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
        std::string lastMeshName = "";

        // 3. Tüm mesh'li entity'leri çiz
        for (auto& entity : entityManager.getEntities()) {
            if (!entity->has<CMesh>() || !entity->has<CTransform>()) continue;

            // Mesh ve Pipeline al
            auto& meshComp = entity->get<CMesh>();
            Astral::GPUMesh* mesh = assetMgr.getMesh(meshComp.meshName);
            SDL_GPUGraphicsPipeline* pipeline = assetMgr.getPipeline(meshComp.materialName);

            if (!mesh || !pipeline) continue;

            // ============================================================
            // STATE CHANGE MINIMIZATION - Sadece değişenleri bind et
            // ============================================================

            // Pipeline değiştiyse bind et
            if (pipeline != lastPipeline) {
                SDL_BindGPUGraphicsPipeline(s_currentRenderPass, pipeline);
                lastPipeline = pipeline;
            }

            // Mesh değiştiyse vertex/index buffer'ları bind et
            if (meshComp.meshName != lastMeshName) {
                // Vertex buffer bind
                SDL_GPUBufferBinding vertexBinding = {};
                vertexBinding.buffer = mesh->vertexBuffer;
                vertexBinding.offset = 0;
                
                SDL_BindGPUVertexBuffers(
                    s_currentRenderPass,
                    0, // first slot
                    &vertexBinding,
                    1  // num bindings
                );

                // Index buffer bind
                SDL_GPUBufferBinding indexBinding = {};
                indexBinding.buffer = mesh->indexBuffer;
                indexBinding.offset = 0;
                
                SDL_BindGPUIndexBuffer(
                    s_currentRenderPass,
                    &indexBinding,
                    SDL_GPU_INDEXELEMENTSIZE_32BIT
                );

                lastMeshName = meshComp.meshName;
            }

            // ============================================================
            // UNIFORM DATA - MVP Matrisleri gönder (Push Constants)
            // ============================================================

            // Model matrix hesapla (Transform'dan)
            auto& transform = entity->get<CTransform>();
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, transform.pos);
            model = glm::rotate(model, transform.rotation.x, glm::vec3(1, 0, 0));
            model = glm::rotate(model, transform.rotation.y, glm::vec3(0, 1, 0));
            model = glm::rotate(model, transform.rotation.z, glm::vec3(0, 0, 1));
            model = glm::scale(model, transform.scale);

            UniformData uniforms = {};
            uniforms.model = model;
            uniforms.view = viewMatrix;
            uniforms.projection = projMatrix;

            // Vertex shader'a uniform data push et
            // SDL_GPU bunu command buffer'a direkt yazar (push constant benzeri)
            SDL_PushGPUVertexUniformData(
                commandBuffer,
                0, // slot index
                &uniforms,
                sizeof(UniformData)
            );

            // ============================================================
            // DRAW CALL - Indexed draw
            // ============================================================

            SDL_DrawGPUIndexedPrimitives(
                s_currentRenderPass,
                mesh->indexCount,  // num_indices
                1,                 // num_instances
                0,                 // first_index
                0,                 // vertex_offset
                0                  // first_instance
            );
        }
    }

    // Render pass'i bitir
    static void endRenderPass() {
        if (s_currentRenderPass) {
            SDL_EndGPURenderPass(s_currentRenderPass);
            s_currentRenderPass = nullptr;
        }
    }

    // Viewport ayarla
    static void setViewport(float x, float y, float width, float height) {
        if (s_currentRenderPass) {
            SDL_GPUViewport viewport = {};
            viewport.x = x;
            viewport.y = y;
            viewport.w = width;
            viewport.h = height;
            viewport.min_depth = 0.0f;
            viewport.max_depth = 1.0f;
            SDL_SetGPUViewport(s_currentRenderPass, &viewport);
        }
    }

    // Scissor rect ayarla (opsiyonel culling)
    static void setScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
        if (s_currentRenderPass) {
            SDL_Rect scissor = {};
            scissor.x = x;
            scissor.y = y;
            scissor.w = width;
            scissor.h = height;
            SDL_SetGPUScissor(s_currentRenderPass, &scissor);
        }
    }

private:
    static inline SDL_GPURenderPass* s_currentRenderPass = nullptr;
};

} // namespace Astral