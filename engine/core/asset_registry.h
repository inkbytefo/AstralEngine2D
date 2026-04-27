#pragma once
#include "asset_interfaces.h"
#include "mesh_manager.h"
#include "texture_manager.h"
#include "shader_manager.h"
#include "pipeline_manager.h"
#include "material_manager.h"
#include "sampler_manager.h"
#include "font_manager.h"
#include <memory>
#include <SDL3/SDL.h>

namespace Astral {

/**
 * @brief Main asset registry that coordinates all asset managers
 * Follows the Registry pattern to provide centralized access to all asset types
 */
class AssetRegistry : public IAssetRegistry {
private:
    SDL_GPUDevice* m_device{ nullptr };

    // Manager instances - owned by registry
    std::unique_ptr<MeshManager> m_meshManager;
    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<PipelineManager> m_pipelineManager;
    std::unique_ptr<MaterialManager> m_materialManager;
    std::unique_ptr<SamplerManager> m_samplerManager;
    std::unique_ptr<FontManager> m_fontManager;

    void initializeManagers() {
        m_meshManager = std::make_unique<MeshManager>();
        m_textureManager = std::make_unique<TextureManager>();
        m_shaderManager = std::make_unique<ShaderManager>();
        m_pipelineManager = std::make_unique<PipelineManager>();
        m_materialManager = std::make_unique<MaterialManager>();
        m_samplerManager = std::make_unique<SamplerManager>();
        m_fontManager = std::make_unique<FontManager>();

        // Set GPU device for GPU-dependent managers
        if (m_device) {
            m_meshManager->setGPUDevice(m_device);
            m_textureManager->setGPUDevice(m_device);
            m_shaderManager->setGPUDevice(m_device);
            m_pipelineManager->setGPUDevice(m_device);
            m_samplerManager->setGPUDevice(m_device);
        }

        // Set dependencies for material manager
        m_materialManager->setDependencies(m_pipelineManager.get(),
                                         m_textureManager.get(),
                                         m_samplerManager.get());
    }

public:
    AssetRegistry() {
        initializeManagers();
    }

    void setGPUDevice(SDL_GPUDevice* device) override {
        m_device = device;

        // Update device in all GPU-dependent managers
        m_meshManager->setGPUDevice(device);
        m_textureManager->setGPUDevice(device);
        m_shaderManager->setGPUDevice(device);
        m_pipelineManager->setGPUDevice(device);
        m_samplerManager->setGPUDevice(device);
    }

    SDL_GPUDevice* getGPUDevice() const override {
        return m_device;
    }

    // Manager accessors
    IMeshManager& getMeshManager() override { return *m_meshManager; }
    ITextureManager& getTextureManager() override { return *m_textureManager; }
    IShaderManager& getShaderManager() override { return *m_shaderManager; }
    IPipelineManager& getPipelineManager() override { return *m_pipelineManager; }
    IMaterialManager& getMaterialManager() override { return *m_materialManager; }
    ISamplerManager& getSamplerManager() override { return *m_samplerManager; }
    IFontManager& getFontManager() override { return *m_fontManager; }

    void cleanup() override {
        m_meshManager->cleanup();
        m_textureManager->cleanup();
        m_shaderManager->cleanup();
        m_pipelineManager->cleanup();
        m_materialManager->cleanup();
        m_samplerManager->cleanup();
        m_fontManager->cleanup();
    }
};

} // namespace Astral