#include "sdl3_renderer.h"
#include "graphics_device.h"

namespace Astral {

SDL3Renderer::SDL3Renderer(IGraphicsDevice* device) : m_device(device) {
    // Initialize viewport with default size
    resizeViewport(m_viewportWidth, m_viewportHeight);
}

SDL3Renderer::~SDL3Renderer() {
    if (m_depthTexture) {
        SDL_ReleaseGPUTexture(m_device->getInternalDevice(), m_depthTexture);
    }
    if (m_sceneColorTexture) {
        SDL_ReleaseGPUTexture(m_device->getInternalDevice(), m_sceneColorTexture);
    }
    if (m_sceneDepthTexture) {
        SDL_ReleaseGPUTexture(m_device->getInternalDevice(), m_sceneDepthTexture);
    }
}

bool SDL3Renderer::beginFrame() {
    m_currentCommandBuffer = m_device->acquireCommandBuffer();
    m_currentSwapchainTexture = nullptr;
    return m_currentCommandBuffer != nullptr;
}

void SDL3Renderer::endFrame() {
    if (m_currentCommandBuffer) {
        m_device->submitCommandBuffer(m_currentCommandBuffer);
        m_currentCommandBuffer = nullptr;
    }
}

void SDL3Renderer::beginRenderPass(SDL_Window* window, const SDL_FColor& clearColor) {
    if (!m_currentCommandBuffer) return;

    if (!m_currentSwapchainTexture) {
        m_currentSwapchainTexture = m_device->acquireSwapchainTexture(m_currentCommandBuffer, window, &m_currentSwapchainW, &m_currentSwapchainH);
    }
    
    if (!m_currentSwapchainTexture) return;

    updateDepthTexture(window, m_currentSwapchainW, m_currentSwapchainH);

    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = m_currentSwapchainTexture;
    colorTargetInfo.clear_color = clearColor;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
    depthTargetInfo.texture = m_depthTexture;
    depthTargetInfo.clear_depth = 1.0f;
    depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.cycle = false;

    m_currentRenderPass = SDL_BeginGPURenderPass(m_currentCommandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
}

void SDL3Renderer::beginUIRenderPass(SDL_Window* window) {
    if (!m_currentCommandBuffer) return;

    if (!m_currentSwapchainTexture) {
        m_currentSwapchainTexture = m_device->acquireSwapchainTexture(m_currentCommandBuffer, window, &m_currentSwapchainW, &m_currentSwapchainH);
    }

    if (!m_currentSwapchainTexture) return;

    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = m_currentSwapchainTexture;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD; // Temizleme yapma, önceki çizimin üzerine ekle
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // UI için derinlik tamponu (depth stencil attachment) NULL geçilir.
    m_currentRenderPass = SDL_BeginGPURenderPass(m_currentCommandBuffer, &colorTargetInfo, 1, nullptr);
}

void SDL3Renderer::endRenderPass() {
    if (m_currentRenderPass) {
        SDL_EndGPURenderPass(m_currentRenderPass);
        m_currentRenderPass = nullptr;
    }
}

void SDL3Renderer::resizeViewport(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == m_viewportWidth && height == m_viewportHeight && m_sceneColorTexture) return;

    m_viewportWidth = width;
    m_viewportHeight = height;

    SDL_GPUDevice* device = m_device->getInternalDevice();

    if (m_sceneColorTexture) SDL_ReleaseGPUTexture(device, m_sceneColorTexture);
    if (m_sceneDepthTexture) SDL_ReleaseGPUTexture(device, m_sceneDepthTexture);

    // Color Texture: SAMPLER | COLOR_TARGET
    SDL_GPUTextureCreateInfo colorInfo = {};
    colorInfo.type = SDL_GPU_TEXTURETYPE_2D;
    colorInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    colorInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    colorInfo.width = width;
    colorInfo.height = height;
    colorInfo.layer_count_or_depth = 1;
    colorInfo.num_levels = 1;
    colorInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    m_sceneColorTexture = SDL_CreateGPUTexture(device, &colorInfo);

    // Depth Texture: DEPTH_STENCIL_TARGET (No SAMPLER as requested)
    SDL_GPUTextureCreateInfo depthInfo = {};
    depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthInfo.width = width;
    depthInfo.height = height;
    depthInfo.layer_count_or_depth = 1;
    depthInfo.num_levels = 1;
    depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    m_sceneDepthTexture = SDL_CreateGPUTexture(device, &depthInfo);

    SDL_Log("Viewport resized to: %dx%d", width, height);
}

void SDL3Renderer::beginScenePass(const SDL_FColor& clearColor) {
    if (!m_currentCommandBuffer) return;
    if (!m_sceneColorTexture) resizeViewport(m_viewportWidth, m_viewportHeight);

    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = m_sceneColorTexture;
    colorTargetInfo.clear_color = clearColor;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
    depthTargetInfo.texture = m_sceneDepthTexture;
    depthTargetInfo.clear_depth = 1.0f;
    depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.cycle = false;

    m_currentRenderPass = SDL_BeginGPURenderPass(m_currentCommandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
}

void SDL3Renderer::endScenePass() {
    endRenderPass();
}

void SDL3Renderer::beginUIPass(SDL_Window* window) {
    if (!m_currentCommandBuffer) return;

    if (!m_currentSwapchainTexture) {
        m_currentSwapchainTexture = m_device->acquireSwapchainTexture(m_currentCommandBuffer, window, &m_currentSwapchainW, &m_currentSwapchainH);
    }

    if (!m_currentSwapchainTexture) return;

    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = m_currentSwapchainTexture;
    colorTargetInfo.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // UI pass explicitly has NO depth target to avoid Vulkan incompatibility
    m_currentRenderPass = SDL_BeginGPURenderPass(m_currentCommandBuffer, &colorTargetInfo, 1, nullptr);
}

void SDL3Renderer::endUIPass() {
    endRenderPass();
}

void SDL3Renderer::bindPipeline(SDL_GPUGraphicsPipeline* pipeline) {
    if (m_currentRenderPass) {
        SDL_BindGPUGraphicsPipeline(m_currentRenderPass, pipeline);
    }
}

void SDL3Renderer::bindVertexBuffers(uint32_t firstSlot, const SDL_GPUBufferBinding* bindings, uint32_t count) {
    if (m_currentRenderPass) {
        SDL_BindGPUVertexBuffers(m_currentRenderPass, firstSlot, bindings, count);
    }
}

void SDL3Renderer::bindIndexBuffer(const SDL_GPUBufferBinding* binding, SDL_GPUIndexElementSize indexSize) {
    if (m_currentRenderPass) {
        SDL_BindGPUIndexBuffer(m_currentRenderPass, binding, indexSize);
    }
}

void SDL3Renderer::pushVertexUniformData(uint32_t slot, const void* data, uint32_t size) {
    if (m_currentCommandBuffer) {
        SDL_PushGPUVertexUniformData(m_currentCommandBuffer, slot, data, size);
    }
}

void SDL3Renderer::pushFragmentUniformData(uint32_t slot, const void* data, uint32_t size) {
    if (m_currentCommandBuffer) {
        SDL_PushGPUFragmentUniformData(m_currentCommandBuffer, slot, data, size);
    }
}

void SDL3Renderer::bindFragmentSamplers(uint32_t firstSlot, const SDL_GPUTextureSamplerBinding* bindings, uint32_t count) {
    if (m_currentRenderPass) {
        SDL_BindGPUFragmentSamplers(m_currentRenderPass, firstSlot, bindings, count);
    }
}

void SDL3Renderer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    if (m_currentRenderPass) {
        SDL_DrawGPUIndexedPrimitives(m_currentRenderPass, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}

void SDL3Renderer::updateDepthTexture(SDL_Window* window, uint32_t width, uint32_t height) {
    if (width <= 0 || height <= 0) return;

    static uint32_t lastW = 0, lastH = 0;
    if (width == lastW && height == lastH && m_depthTexture) return;

    if (m_depthTexture) {
        SDL_ReleaseGPUTexture(m_device->getInternalDevice(), m_depthTexture);
    }

    SDL_GPUTextureCreateInfo depthInfo = {};
    depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET; // NO SAMPLER - only depth target
    depthInfo.width = width;
    depthInfo.height = height;
    depthInfo.layer_count_or_depth = 1;
    depthInfo.num_levels = 1;
    depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

    m_depthTexture = SDL_CreateGPUTexture(m_device->getInternalDevice(), &depthInfo);
    lastW = width;
    lastH = height;

    SDL_Log("Swapchain Depth Texture recreated: %dx%d", width, height);
}

} // namespace Astral
