#pragma once
#include "graphics_device.h"

namespace Astral {

class SDL3GraphicsDevice : public IGraphicsDevice {
public:
    SDL3GraphicsDevice() = default;
    ~SDL3GraphicsDevice() override { shutdown(); }

    bool init(SDL_Window* window) override;
    void shutdown() override;

    SDL_GPUCommandBuffer* acquireCommandBuffer() override;
    void submitCommandBuffer(SDL_GPUCommandBuffer* commandBuffer) override;

    SDL_GPUTexture* acquireSwapchainTexture(SDL_GPUCommandBuffer* commandBuffer, SDL_Window* window, uint32_t* w, uint32_t* h) override;
    
    SDL_GPUDevice* getInternalDevice() const override { return m_device; }
    SDL_GPUTextureFormat getSwapchainFormat(SDL_Window* window) const override;

private:
    SDL_GPUDevice* m_device{ nullptr };
};

} // namespace Astral
