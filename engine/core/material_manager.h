#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include <map>
#include <memory>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

// Material struct is now defined in asset_interfaces.h

namespace Astral {

// Forward declarations
class IPipelineManager;
class ITextureManager;
class ISamplerManager;

/**
 * @brief Concrete implementation of material management
 */
class MaterialManager : public IMaterialManager {
private:
    std::map<std::string, std::unique_ptr<Material>> m_materials;
    IPipelineManager* m_pipelineManager{ nullptr };
    ITextureManager* m_textureManager{ nullptr };
    ISamplerManager* m_samplerManager{ nullptr };

public:
    MaterialManager() = default;

    void setDependencies(IPipelineManager* pipelineMgr,
                        ITextureManager* textureMgr,
                        ISamplerManager* samplerMgr) {
        m_pipelineManager = pipelineMgr;
        m_textureManager = textureMgr;
        m_samplerManager = samplerMgr;
    }

    Material* createMaterial(const std::string& name,
                            const std::string& pipelineName,
                            const std::string& albedoName = "",
                            const std::string& normalName = "",
                            const std::string& mrName = "",
                            const glm::vec4& baseColor = glm::vec4(1.0f),
                            float metallic = 0.0f,
                            float roughness = 0.5f) override {
        auto mat = std::make_unique<Material>();
        mat->pipeline = m_pipelineManager ? m_pipelineManager->getPipeline(pipelineName) : nullptr;
        mat->baseColor = baseColor;
        mat->metallic = metallic;
        mat->roughness = roughness;

        if (!albedoName.empty() && m_textureManager) {
            mat->albedoTexture = m_textureManager->getTexture(albedoName);
            mat->hasAlbedoTexture = (mat->albedoTexture != nullptr);
        }
        if (!normalName.empty() && m_textureManager) {
            mat->normalTexture = m_textureManager->getTexture(normalName);
            mat->hasNormalTexture = (mat->normalTexture != nullptr);
        }
        if (!mrName.empty() && m_textureManager) {
            mat->metallicRoughnessTexture = m_textureManager->getTexture(mrName);
            mat->hasMetallicRoughnessTexture = (mat->metallicRoughnessTexture != nullptr);
        }

        // Get sampler from sampler manager
        if (m_samplerManager) {
            mat->sampler = m_samplerManager->createSampler();
        }

        Material* rawPtr = mat.get();
        m_materials[name] = std::move(mat);
        return rawPtr;
    }

    Material* getMaterial(const std::string& name) const override {
        auto it = m_materials.find(name);
        return (it != m_materials.end()) ? it->second.get() : nullptr;
    }

    void cleanup() override {
        m_materials.clear();
    }
};

} // namespace Astral