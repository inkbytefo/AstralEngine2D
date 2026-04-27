#include "render_system.h"
#include "../core/asset_manager.h"
#include "../renderer/renderer.h"
#include "../ecs/components.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {

void RenderSystem::update(Astral::EntityManager& entityManager, float deltaTime) {
    if (!m_cameraData.isValid) {
        m_cameraData = resolveSceneCamera(entityManager);
    }

    // 1. Hazırlık (Sorting vs.)
    prepare(entityManager);

    // 2. Eğer renderer ayarlanmışsa çizimi başlat
    if (m_renderer) {
        render(m_renderer, entityManager);
    }
}

void RenderSystem::prepare(Astral::EntityManager& entityManager) {
    // 1. Batch entities by pipeline, mesh, and material for instanced rendering
    m_renderQueue.clear();
    m_batches.clear();

    entityManager.each<CMesh, CTransform>([&](const auto& entity) {
        m_renderQueue.push_back(entity);

        auto& meshComp = entity->template get<CMesh>();
        auto& transform = entity->template get<CTransform>();

        // Get assets
        Material* material = m_assetManager->getMaterialManager().getMaterial(meshComp.materialName);
        GPUMesh* mesh = m_assetManager->getMeshManager().getMesh(meshComp.meshName);

        if (!material || !mesh || !material->pipeline) return;

        // Create batch key
        RenderBatch::Key key = {material->pipeline, mesh, material};

        // Add to batch or create new batch
        auto& batch = m_batches[key];
        if (batch.isEmpty()) {
            batch.pipeline = material->pipeline;
            batch.mesh = mesh;
            batch.material = material;
        }

        glm::mat4 model = transform.globalMatrix;
        if (model == glm::mat4(1.0f) &&
            (transform.pos != glm::vec3(0.0f) ||
             transform.scale != glm::vec3(1.0f) ||
             transform.rotation != glm::vec3(0.0f))) {
            model = glm::translate(glm::mat4(1.0f), transform.pos);
            if (transform.scale != glm::vec3(1.0f)) {
                model = glm::scale(model, transform.scale);
            }
            if (transform.rotation != glm::vec3(0.0f)) {
                model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
                model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
                model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
            }
        }

        batch.addInstance(model, entity->id());
    });
}

void RenderSystem::render(IRenderer* renderer, Astral::EntityManager& entityManager) {
    if (!renderer || !m_assetManager || !m_window) return;

    // Prepare render batches (group by pipeline/mesh/material for optimization)
    prepare(entityManager);

    // Process batches for optimized rendering with minimized state changes
    processBatches();
}

void RenderSystem::processBatches() {
    if (!m_renderer) return;

    m_drawCallCount = 0;

    // Process each batch for instanced rendering
    for (const auto& [key, batch] : m_batches) {
        if (batch.isEmpty()) continue;
        flushBatch(batch);
    }

    // Clear batches for next frame
    m_batches.clear();
}

void RenderSystem::flushBatch(const RenderBatch& batch) {
    // Bind pipeline (state minimization)
    m_renderer->bindPipeline(batch.pipeline);

    // Bind vertex buffers
    SDL_GPUBufferBinding vertexBinding = { batch.mesh->vertexBuffer, batch.mesh->vertexOffset };
    m_renderer->bindVertexBuffers(0, &vertexBinding, 1);

    // Bind index buffer
    SDL_GPUBufferBinding indexBinding = { batch.mesh->indexBuffer, batch.mesh->indexOffset };
    m_renderer->bindIndexBuffer(&indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    // For instanced rendering, we would need:
    // 1. Instance buffer with transform matrices
    // 2. Modified shader that reads per-instance data
    // For now, render each instance separately but batched by state

    // --- MATERIAL BINDING (STATE MINIMIZATION) ---
    if (batch.material) {
        auto& textureMgr = m_assetManager->getTextureManager();
        SDL_GPUTexture* whiteTex = textureMgr.getWhiteTexture();
        SDL_GPUTexture* normalTex = textureMgr.getNormalTexture();

        SDL_GPUTexture* tex0 = batch.material->hasAlbedoTexture ? batch.material->albedoTexture : whiteTex;
        SDL_GPUTexture* tex1 = batch.material->hasNormalTexture ? batch.material->normalTexture : normalTex;
        SDL_GPUTexture* tex2 = batch.material->hasMetallicRoughnessTexture ? batch.material->metallicRoughnessTexture : whiteTex;

        SDL_GPUTextureSamplerBinding bindings[3] = {
            { tex0, batch.material->sampler },
            { tex1, batch.material->sampler },
            { tex2, batch.material->sampler }
        };
        m_renderer->bindFragmentSamplers(0, bindings, 3);

        // PBR Uniform Data
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
            int32_t padding; // Alignment
        } fragData = {};
        
        fragData.baseColor = batch.material->baseColor;
        fragData.camPosRoughness = glm::vec4(m_cameraData.position, batch.material->roughness);
        fragData.baseColor.a = batch.material->metallic; // Pack metallic in alpha? Or use separate field if struct matches
        
        // Note: For now we don't have light logic here, we'd need to pass light data to RenderSystem
        fragData.lightCount = 0; 
        fragData.useAlbedoMap = batch.material->hasAlbedoTexture ? 1 : 0;
        fragData.useNormalMap = batch.material->hasNormalTexture ? 1 : 0;
        fragData.useMetallicRoughnessMap = batch.material->hasMetallicRoughnessTexture ? 1 : 0;

        m_renderer->pushFragmentUniformData(0, &fragData, sizeof(PBRUniforms));
    }

    // TODO: Implement full instanced rendering
    // For now, render each instance individually but with minimized state changes
    for (size_t i = 0; i < batch.transforms.size(); ++i) {
        // Set model matrix uniform
        UniformData uniforms;
        uniforms.model = batch.transforms[i];
        uniforms.view = m_cameraData.view;
        uniforms.projection = m_cameraData.projection;

        m_renderer->pushVertexUniformData(0, &uniforms, sizeof(UniformData));

        // Draw indexed
        m_renderer->drawIndexed(batch.mesh->indexCount, 1, 0, 0, 0);
        m_drawCallCount++;
    }
}

CameraFrameData RenderSystem::resolveSceneCamera(Astral::EntityManager& entityManager) const {
    CameraFrameData resolved;

    entityManager.each<CCamera, CTransform>([&](const auto& entity) {
        if (resolved.isValid) {
            return;
        }

        auto& camera = entity->get<CCamera>();
        auto& transform = entity->get<CTransform>();
        if (!camera.isActive) {
            return;
        }

        resolved.view = camera.view;
        resolved.projection = camera.projection;
        resolved.position = transform.pos;
        resolved.isValid = true;
    });

    return resolved;
}

} // namespace Astral
