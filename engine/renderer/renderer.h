#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace Astral {

class IGraphicsDevice;

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Kare döngüsü yönetimi
    virtual bool beginFrame() = 0;
    virtual void endFrame() = 0;

    // Render Pass yönetimi
    virtual void beginRenderPass(SDL_Window* window, const SDL_FColor& clearColor) = 0;
    virtual void beginUIRenderPass(SDL_Window* window) = 0;
    virtual void endRenderPass() = 0;

    // Phase 1 Part B: Viewport & Offscreen Rendering
    virtual void resizeViewport(uint32_t width, uint32_t height) = 0;
    virtual void beginScenePass(const SDL_FColor& clearColor) = 0;
    virtual void endScenePass() = 0;
    virtual void beginUIPass(SDL_Window* window) = 0;
    virtual void endUIPass() = 0;
    virtual SDL_GPUTexture* getSceneTexture() const = 0;

    // Pipeline ve Bindings
    virtual void bindPipeline(SDL_GPUGraphicsPipeline* pipeline) = 0;
    virtual void bindVertexBuffers(uint32_t firstSlot, const SDL_GPUBufferBinding* bindings, uint32_t count) = 0;
    virtual void bindIndexBuffer(const SDL_GPUBufferBinding* binding, SDL_GPUIndexElementSize indexSize) = 0;
    
    // Uniform ve Resource Binding
    virtual void pushVertexUniformData(uint32_t slot, const void* data, uint32_t size) = 0;
    virtual void pushFragmentUniformData(uint32_t slot, const void* data, uint32_t size) = 0;
    virtual void bindFragmentSamplers(uint32_t firstSlot, const SDL_GPUTextureSamplerBinding* bindings, uint32_t count) = 0;

    // Çizim Komutları
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

    // State sorgulama
    virtual SDL_GPUCommandBuffer* getCurrentCommandBuffer() const = 0;
    virtual SDL_GPURenderPass* getCurrentRenderPass() const = 0;
};

} // namespace Astral
