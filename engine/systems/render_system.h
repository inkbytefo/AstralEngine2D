#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
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

    // Modern Render Loop - AAA Stratejisi: Sorting & Minimization
    static void update(
        EntityManager& entityManager,
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPUDevice* gpuDevice,
        SDL_Window* window,
        SDL_GPURenderPass* renderPass = nullptr)
    {
        if (renderPass) s_currentRenderPass = renderPass;
        if (!s_currentRenderPass) return;

        // 1. Aktif Kamerayı Bul
        glm::mat4 viewMatrix(1.0f);
        glm::mat4 projMatrix(1.0f);
        bool cameraFound = false;

        for (auto& entity : entityManager.getEntities()) {
            if (entity->has<CCamera>() && entity->get<CCamera>().isActive) {
                auto& cam = entity->get<CCamera>();
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                if (w > 0 && h > 0) {
                    cam.projection = glm::perspective(glm::radians(60.0f), (float)w / h, 0.1f, 1000.0f);
                }
                viewMatrix = cam.view;
                projMatrix = cam.projection;
                cameraFound = true;
                break;
            }
        }
        if (!cameraFound) return;

        // 2. AAA Sorting: Entities listesini kopyala ve Pipeline > Material > Mesh hiyerarşisine göre sırala
        // Not: Gerçek bir motorda bu her kare yapılmaz, sadece liste değiştiğinde veya her kare "render proxy"ler üzerinde yapılır.
        auto renderEntities = entityManager.getEntities();
        std::sort(renderEntities.begin(), renderEntities.end(), [](const auto& a, const auto& b) {
            if (!a->template has<CMesh>() || !b->template has<CMesh>()) return false;
            auto& ma = a->template get<CMesh>();
            auto& mb = b->template get<CMesh>();
            
            // 1. Önce Material ismine göre (dolayısıyla Pipeline'a göre)
            if (ma.materialName != mb.materialName) return ma.materialName < mb.materialName;
            // 2. Sonra Mesh ismine göre
            return ma.meshName < mb.meshName;
        });

        // 3. State Tracking & Drawing
        AssetManager& assetMgr = AssetManager::getInstance();
        SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
        std::string lastMaterialName = "";
        std::string lastMeshName = "";

        s_drawCalls = 0;

        for (auto& entity : renderEntities) {
            if (!entity->has<CMesh>() || !entity->has<CTransform>()) continue;

            auto& meshComp = entity->get<CMesh>();
            Material* material = assetMgr.getMaterial(meshComp.materialName);
            GPUMesh* mesh = assetMgr.getMesh(meshComp.meshName);

            if (!mesh || !material || !material->pipeline) continue;

            // --- STATE MINIMIZATION ---
            
            // A. Pipeline Binding (Context Switch - En Pahalı)
            if (material->pipeline != lastPipeline) {
                SDL_BindGPUGraphicsPipeline(s_currentRenderPass, material->pipeline);
                lastPipeline = material->pipeline;
            }

            // B. Material Binding (Texture & Uniforms - Orta)
            if (meshComp.materialName != lastMaterialName) {
                // Texture Bind (Set 2)
                if (material->hasAlbedoTexture) {
                    SDL_GPUTextureSamplerBinding texBinding = { material->albedoTexture, material->sampler };
                    SDL_BindGPUFragmentSamplers(s_currentRenderPass, 0, &texBinding, 1);
                }

                // Fragment Uniform (Set 3)
                struct FragmentUniform {
                    glm::vec4 baseColor;
                    int hasTexture;
                    int _padding[3];
                } fragData = { material->baseColor, material->hasAlbedoTexture ? 1 : 0 };
                
                // Debug log (sadece değişimde)
                SDL_Log("RenderSystem: Material bind ediliyor: %s (Texture: %d)", 
                        meshComp.materialName.c_str(), fragData.hasTexture);

                SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragData, sizeof(FragmentUniform));
                lastMaterialName = meshComp.materialName;
            }

            // C. Mesh Binding (Buffers - Ucuz)
            if (meshComp.meshName != lastMeshName) {
                SDL_GPUBufferBinding vBinding = { mesh->vertexBuffer, 0 };
                SDL_BindGPUVertexBuffers(s_currentRenderPass, 0, &vBinding, 1);

                SDL_GPUBufferBinding iBinding = { mesh->indexBuffer, 0 };
                SDL_BindGPUIndexBuffer(s_currentRenderPass, &iBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                lastMeshName = meshComp.meshName;
            }

            // --- PER-OBJECT DATA (Push Constants) ---
            auto& transform = entity->get<CTransform>();
            glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.pos);
            model = glm::rotate(model, transform.rotation.x, {1,0,0});
            model = glm::rotate(model, transform.rotation.y, {0,1,0});
            model = glm::rotate(model, transform.rotation.z, {0,0,1});
            model = glm::scale(model, transform.scale);

            UniformData uniforms = { model, viewMatrix, projMatrix };
            SDL_PushGPUVertexUniformData(commandBuffer, 0, &uniforms, sizeof(UniformData));

            // --- DRAW ---
            SDL_DrawGPUIndexedPrimitives(s_currentRenderPass, mesh->indexCount, 1, 0, 0, 0);
            s_drawCalls++;
        }
    }

    static int getDrawCalls() { return s_drawCalls; }

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
    static inline int s_drawCalls = 0;
};

} // namespace Astral