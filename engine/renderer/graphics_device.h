#pragma once
#include <SDL3/SDL.h>
#include <cstdint>

namespace Astral {

class IGraphicsDevice {
public:
    virtual ~IGraphicsDevice() = default;

    virtual bool init(SDL_Window* window) = 0;
    virtual void shutdown() = 0;

    // Command Buffer yönetimi
    virtual SDL_GPUCommandBuffer* acquireCommandBuffer() = 0;
    virtual void submitCommandBuffer(SDL_GPUCommandBuffer* commandBuffer) = 0;

    // Swapchain yönetimi
    virtual SDL_GPUTexture* acquireSwapchainTexture(SDL_GPUCommandBuffer* commandBuffer, SDL_Window* window, uint32_t* w, uint32_t* h) = 0;
    
    // Yardımcı metotlar
    virtual SDL_GPUDevice* getInternalDevice() const = 0;
    virtual SDL_GPUTextureFormat getSwapchainFormat(SDL_Window* window) const = 0;
};

} // namespace Astral
