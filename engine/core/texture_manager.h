#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include "error_handling.h"
#include <map>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <filesystem>

namespace Astral {

/**
 * @brief Concrete implementation of texture management
 */
class TextureManager : public ITextureManager {
private:
    SDL_GPUDevice* m_device{ nullptr };
    std::map<std::string, GpuTexture> m_textures;

    // Default textures (fallback)
    GpuTexture m_whiteTexture;
    GpuTexture m_blackTexture;
    GpuTexture m_defaultNormalTexture;

    GpuTexture create1x1Texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_device, &texInfo);

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = 4;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_device, &transferInfo);

        uint8_t* data = (uint8_t*)SDL_MapGPUTransferBuffer(m_device, transferBuffer, false);
        if (data) {
            data[0] = r; data[1] = g; data[2] = b; data[3] = a;
            SDL_UnmapGPUTransferBuffer(m_device, transferBuffer);
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer = transferBuffer;
        src.offset = 0;

        SDL_GPUTextureRegion dst = {};
        dst.texture = texture;
        dst.w = 1;
        dst.h = 1;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);

        SDL_WaitForGPUIdle(m_device);
        SDL_ReleaseGPUTransferBuffer(m_device, transferBuffer);

        return GpuTexture(texture, m_device);
    }

    void createDefaultTextures() {
        if (!m_device || m_whiteTexture) return;

        m_whiteTexture = create1x1Texture(255, 255, 255, 255);
        m_blackTexture = create1x1Texture(0, 0, 0, 255);
        m_defaultNormalTexture = create1x1Texture(128, 128, 255, 255); // Flat normal (0.5, 0.5, 1.0)

        SDL_Log("TextureManager: Default (fallback) textures created");
    }

public:
    TextureManager() = default;

    void setGPUDevice(SDL_GPUDevice* device) {
        m_device = device;
        createDefaultTextures();
    }

    SDL_GPUTexture* uploadTexture(const std::string& name, const std::string& filepath) override {
        // Check if already loaded
        if (m_textures.find(name) != m_textures.end()) {
            return m_textures[name].get();
        }

        if (!m_device) {
            ENGINE_ERROR(GPU, "GPU device not initialized");
        }

        // Check if file exists
        if (!std::filesystem::exists(filepath)) {
            ENGINE_ERROR_CTX(FileSystem, "Texture file not found", filepath);
        }

        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            ENGINE_ERROR_CTX(Asset, "Failed to load texture file", filepath);
        }

        SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);

        if (!rgbaSurface) {
            ENGINE_ERROR_CTX(Asset, "Failed to convert texture to RGBA format", filepath);
        }

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.width = rgbaSurface->w;
        texInfo.height = rgbaSurface->h;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_device, &texInfo);
        if (!texture) {
            SDL_DestroySurface(rgbaSurface);
            ENGINE_ERROR_CTX(GPU, "Failed to create GPU texture", name);
        }

        uint32_t dataSize = rgbaSurface->w * rgbaSurface->h * 4;
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = dataSize;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_device, &transferInfo);
        if (!transferBuffer) {
            SDL_DestroySurface(rgbaSurface);
            ENGINE_ERROR_CTX(GPU, "Failed to create GPU transfer buffer", name);
        }

        void* transferData = SDL_MapGPUTransferBuffer(m_device, transferBuffer, false);
        if (transferData) {
            memcpy(transferData, rgbaSurface->pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(m_device, transferBuffer);
        }
        SDL_DestroySurface(rgbaSurface);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        SDL_GPUTextureTransferInfo transferLoc = {};
        transferLoc.transfer_buffer = transferBuffer;

        SDL_GPUTextureRegion texRegion = {};
        texRegion.texture = texture;
        texRegion.w = texInfo.width;
        texRegion.h = texInfo.height;
        texRegion.d = 1;

        SDL_UploadToGPUTexture(copyPass, &transferLoc, &texRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        SDL_WaitForGPUIdle(m_device);
        SDL_ReleaseGPUTransferBuffer(m_device, transferBuffer);

        m_textures[name] = GpuTexture(texture, m_device);
        Astral::ErrorHandler::logInfo("Texture loaded: " + name + " (" +
            std::to_string(texInfo.width) + "x" + std::to_string(texInfo.height) + ")");
        return texture;
    }

    SDL_GPUTexture* getTexture(const std::string& name) const override {
        auto it = m_textures.find(name);
        return (it != m_textures.end()) ? it->second.get() : nullptr;
    }

    SDL_GPUTexture* getWhiteTexture() const override { return m_whiteTexture.get(); }
    SDL_GPUTexture* getBlackTexture() const override { return m_blackTexture.get(); }
    SDL_GPUTexture* getNormalTexture() const override { return m_defaultNormalTexture.get(); }

    void cleanup() override {
        m_textures.clear();
        m_whiteTexture.reset();
        m_blackTexture.reset();
        m_defaultNormalTexture.reset();
    }
};

} // namespace Astral