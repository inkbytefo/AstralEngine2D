#include "sdl3_graphics_device.h"
#include <SDL3/SDL.h>

namespace Astral {

bool SDL3GraphicsDevice::init(SDL_Window* window) {
    // SDL_GPU Cihazını oluştur (Vulkan ve SPIR-V'ye zorlayalım)
    m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (!m_device) {
        SDL_Log("SDL_GPU Cihazi olusturulamadi!");
        return false;
    }

    // Pencereyi GPU cihazına bağla
    if (window && !SDL_ClaimWindowForGPUDevice(m_device, window)) {
        SDL_Log("Pencere GPU cihazina baglanamadi!");
        return false;
    }

    return true;
}

void SDL3GraphicsDevice::shutdown() {
    if (m_device) {
        SDL_DestroyGPUDevice(m_device);
        m_device = nullptr;
    }
}

SDL_GPUCommandBuffer* SDL3GraphicsDevice::acquireCommandBuffer() {
    return SDL_AcquireGPUCommandBuffer(m_device);
}

void SDL3GraphicsDevice::submitCommandBuffer(SDL_GPUCommandBuffer* commandBuffer) {
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

SDL_GPUTexture* SDL3GraphicsDevice::acquireSwapchainTexture(SDL_GPUCommandBuffer* commandBuffer, SDL_Window* window, uint32_t* w, uint32_t* h) {
    SDL_GPUTexture* texture = nullptr;
    if (SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &texture, w, h)) {
        return texture;
    }
    return nullptr;
}

SDL_GPUTextureFormat SDL3GraphicsDevice::getSwapchainFormat(SDL_Window* window) const {
    if (!m_device || !window) return SDL_GPU_TEXTUREFORMAT_INVALID;
    return SDL_GetGPUSwapchainTextureFormat(m_device, window);
}

} // namespace Astral
