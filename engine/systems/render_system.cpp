#include "render_system.h"
#include "../core/asset_manager.h"
#include "../renderer/renderer.h"
#include "../ecs/components.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {

void RenderSystem::update(Astral::EntityManager& entityManager, float deltaTime) {
    // 1. Hazırlık (Sorting vs.)
    prepare(entityManager);

    // 2. Eğer renderer ayarlanmışsa çizimi başlat
    if (m_renderer) {
        render(m_renderer, entityManager);
    }
}

void RenderSystem::prepare(Astral::EntityManager& entityManager) {
    // 1. Render edilebilir varlıkları filtrele ve sırala (Pipeline > Material > Mesh)
    m_renderQueue = entityManager.view<CMesh, CTransform>();

    std::sort(m_renderQueue.begin(), m_renderQueue.end(), [](const auto& a, const auto& b) {
        auto& ma = a->template get<CMesh>();
        auto& mb = b->template get<CMesh>();
        
        if (ma.materialName != mb.materialName) return ma.materialName < mb.materialName;
        return ma.meshName < mb.meshName;
    });
}

void RenderSystem::render(IRenderer* renderer, Astral::EntityManager& entityManager) {
    if (!renderer || !m_assetManager || !m_window) return;

    // 1. Aktif Kamerayı Bul
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projMatrix(1.0f);
    glm::vec3 camPos(0.0f);
    bool cameraFound = false;

    if (m_useCameraOverride) {
        viewMatrix = m_overrideView;
        projMatrix = m_overrideProj;
        camPos = m_overridePos;
        cameraFound = true;
    } else {
        for (auto& entity : entityManager.view<CCamera, CTransform>()) {
            auto& cam = entity->get<CCamera>();
            if (cam.isActive) {
                viewMatrix = cam.view;
                projMatrix = cam.projection;
                camPos = glm::vec3(entity->get<CTransform>().globalMatrix[3]);
                cameraFound = true;
                break;
            }
        }
    }
    
    if (!cameraFound) return;

    // 2. Işık verisini bul
    // ... (Light logic remains same but uses fragData below)

    // 3. State Tracking & Drawing
    AssetManager& assetMgr = *m_assetManager;
    SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
    std::string lastMaterialName = "";
    SDL_GPUBuffer* lastVertexBuffer = nullptr;
    SDL_GPUBuffer* lastIndexBuffer = nullptr;

    m_drawCallCount = 0;

    for (auto& entity : m_renderQueue) {
        auto& meshComp = entity->template get<CMesh>();
        auto& transform = entity->template get<CTransform>();
        
        Material* material = assetMgr.getMaterial(meshComp.materialName);
        GPUMesh* mesh = assetMgr.getMesh(meshComp.meshName);

        if (!mesh || !material || !material->pipeline) continue;

        // --- STATE MINIMIZATION ---
        
        // A. Pipeline Binding
        if (material->pipeline != lastPipeline) {
            renderer->bindPipeline(material->pipeline);
            lastPipeline = material->pipeline;
        }

        // B. Material Binding (Texture & Uniforms)
        if (meshComp.materialName != lastMaterialName) {
            SDL_GPUTexture* whiteTex = assetMgr.getWhiteTexture();
            SDL_GPUTexture* normalTex = assetMgr.getNormalTexture();

            SDL_GPUTexture* tex0 = material->hasAlbedoTexture ? material->albedoTexture : whiteTex;
            SDL_GPUTexture* tex1 = material->hasNormalTexture ? material->normalTexture : normalTex;
            SDL_GPUTexture* tex2 = material->hasMetallicRoughnessTexture ? material->metallicRoughnessTexture : whiteTex;

            SDL_GPUTextureSamplerBinding bindings[3] = {
                { tex0, material->sampler },
                { tex1, material->sampler },
                { tex2, material->sampler }
            };
            renderer->bindFragmentSamplers(0, bindings, 3);

            // Fragment Uniform (Set 3) - Multi-Light PBR
            struct GPULight {
                glm::vec4 posType;     // xyz = pos, w = type
                glm::vec4 colorInt;    // xyz = color, w = intensity
                glm::vec4 dirRange;    // xyz = dir, w = range
                glm::vec4 cutoff;      // x = inner, y = outer
            };

            struct PBRUniforms {
                glm::vec4 baseColor;         
                glm::vec4 camPosRoughness;   
                GPULight lights[8];
                int32_t lightCount;
                int32_t useAlbedoMap;
                int32_t useNormalMap;
                int32_t useMetallicRoughnessMap;
            } fragData = {};
            
            fragData.baseColor = material->baseColor;
            fragData.baseColor.a = material->metallic;
            fragData.camPosRoughness = glm::vec4(camPos, material->roughness);
            
            int lightIdx = 0;
            for (auto& e : entityManager.view<CLight>()) {
                if (lightIdx >= 8) break;
                auto& l = e->template get<CLight>();
                GPULight& gpuL = fragData.lights[lightIdx];
                glm::vec3 pos(0.0f);
                if (l.type != LightType::Directional && e->has<CTransform>()) {
                    pos = glm::vec3(e->template get<CTransform>().globalMatrix[3]);
                }
                gpuL.posType = glm::vec4(pos, (float)l.type);
                gpuL.colorInt = glm::vec4(l.color, l.intensity);
                gpuL.dirRange = glm::vec4(l.direction, l.range);
                gpuL.cutoff = glm::vec4(l.innerCutoff, l.outerCutoff, 0.0f, 0.0f);
                lightIdx++;
            }
            fragData.lightCount = lightIdx;
            fragData.useAlbedoMap = material->hasAlbedoTexture ? 1 : 0;
            fragData.useNormalMap = material->hasNormalTexture ? 1 : 0;
            fragData.useMetallicRoughnessMap = material->hasMetallicRoughnessTexture ? 1 : 0;

            renderer->pushFragmentUniformData(0, &fragData, sizeof(PBRUniforms));
            lastMaterialName = meshComp.materialName;
        }

        // C. Buffer Binding
        if (mesh->vertexBuffer != lastVertexBuffer) {
            SDL_GPUBufferBinding vBinding = { mesh->vertexBuffer, 0 };
            renderer->bindVertexBuffers(0, &vBinding, 1);
            lastVertexBuffer = mesh->vertexBuffer;
        }

        if (mesh->indexBuffer != lastIndexBuffer) {
            SDL_GPUBufferBinding iBinding = { mesh->indexBuffer, 0 };
            renderer->bindIndexBuffer(&iBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            lastIndexBuffer = mesh->indexBuffer;
        }

        // --- PER-OBJECT DATA ---
        UniformData uniforms = { transform.globalMatrix, viewMatrix, projMatrix };
        renderer->pushVertexUniformData(0, &uniforms, sizeof(UniformData));

        // --- DRAW ---
        uint32_t firstIndex = mesh->indexOffset / sizeof(uint32_t);
        int32_t vertexOffset = mesh->vertexOffset / sizeof(Vertex);
        
        renderer->drawIndexed(mesh->indexCount, 1, firstIndex, vertexOffset, 0);
        m_drawCallCount++;
    }
}

} // namespace Astral
