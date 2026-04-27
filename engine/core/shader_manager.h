#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include "shader_loader.h"
#include <vector>
#include <SDL3/SDL.h>

namespace Astral {

/**
 * @brief Concrete implementation of shader management
 */
class ShaderManager : public IShaderManager {
private:
    SDL_GPUDevice* m_device{ nullptr };
    std::vector<GpuShader> m_shaders;

public:
    ShaderManager() = default;

    void setGPUDevice(SDL_GPUDevice* device) {
        m_device = device;
    }

    SDL_GPUShader* loadShader(const std::string& path, SDL_GPUShaderStage stage) override {
        if (!m_device) return nullptr;

        SDL_GPUShader* shader = ShaderLoader::loadShaderFromFile(m_device, path.c_str(), stage);
        if (shader) {
            m_shaders.push_back(GpuShader(shader, m_device));
            SDL_Log("ShaderManager: Shader loaded: %s", path.c_str());
            return shader;
        }
        return nullptr;
    }

    void addShader(SDL_GPUShader* shader) override {
        if (shader) {
            m_shaders.push_back(GpuShader(shader, m_device));
        }
    }

    void cleanup() override {
        m_shaders.clear();
    }
};

} // namespace Astral