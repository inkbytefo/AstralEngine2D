#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include <vector>
#include <SDL3/SDL.h>

namespace Astral {

/**
 * @brief Concrete implementation of sampler management
 */
class SamplerManager : public ISamplerManager {
private:
    SDL_GPUDevice* m_device{ nullptr };
    std::vector<GpuSampler> m_samplers;

public:
    SamplerManager() = default;

    void setGPUDevice(SDL_GPUDevice* device) {
        m_device = device;
    }

    SDL_GPUSampler* createSampler(SDL_GPUFilter filter = SDL_GPU_FILTER_LINEAR) override {
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = filter;
        samplerInfo.mag_filter = filter;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

        SDL_GPUSampler* sampler = SDL_CreateGPUSampler(m_device, &samplerInfo);
        if (sampler) {
            m_samplers.push_back(GpuSampler(sampler, m_device));
            return sampler;
        }
        return nullptr;
    }

    void cleanup() override {
        m_samplers.clear();
    }
};

} // namespace Astral