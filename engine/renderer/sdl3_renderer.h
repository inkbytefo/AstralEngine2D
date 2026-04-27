#pragma once
#include "renderer.h"
#include <memory>

namespace Astral {

class IGraphicsDevice;

class SDL3Renderer : public IRenderer {
public:
    SDL3Renderer(IGraphicsDevice* device);
    ~SDL3Renderer() override;

    bool beginFrame() override;
    void endFrame() override;

    void beginRenderPass(SDL_Window* window, const SDL_FColor& clearColor) override;
    void beginUIRenderPass(SDL_Window* window) override;
    void endRenderPass() override;

    void bindPipeline(SDL_GPUGraphicsPipeline* pipeline) override;
    void bindVertexBuffers(uint32_t firstSlot, const SDL_GPUBufferBinding* bindings, uint32_t count) override;
    void bindIndexBuffer(const SDL_GPUBufferBinding* binding, SDL_GPUIndexElementSize indexSize) override;
    
    void pushVertexUniformData(uint32_t slot, const void* data, uint32_t size) override;
    void pushFragmentUniformData(uint32_t slot, const void* data, uint32_t size) override;
    void bindFragmentSamplers(uint32_t firstSlot, const SDL_GPUTextureSamplerBinding* bindings, uint32_t count) override;

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

    SDL_GPUCommandBuffer* getCurrentCommandBuffer() const override { return m_currentCommandBuffer; }
    SDL_GPURenderPass* getCurrentRenderPass() const override { return m_currentRenderPass; }

private:
    IGraphicsDevice* m_device{ nullptr };
    SDL_GPUCommandBuffer* m_currentCommandBuffer{ nullptr };
    SDL_GPURenderPass* m_currentRenderPass{ nullptr };
    SDL_GPUTexture* m_depthTexture{ nullptr };
    SDL_GPUTexture* m_currentSwapchainTexture{ nullptr }; // Cache for the frame
    uint32_t m_currentSwapchainW{ 0 };
    uint32_t m_currentSwapchainH{ 0 };
    
    void updateDepthTexture(SDL_Window* window, uint32_t width, uint32_t height);
};

} // namespace Astral
