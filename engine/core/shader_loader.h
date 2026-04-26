#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace Astral {

// Shader yükleme helper fonksiyonları
class ShaderLoader {
public:
    // Dosyadan shader yükle
    static SDL_GPUShader* loadShaderFromFile(
        SDL_GPUDevice* device,
        const char* filepath,
        SDL_GPUShaderStage stage,
        const char* entrypoint = "main",
        uint32_t numSamplers = 0,
        uint32_t numStorageTextures = 0,
        uint32_t numStorageBuffers = 0,
        uint32_t numUniformBuffers = 1)
    {
        // Dosyayı oku
        SDL_IOStream* file = SDL_IOFromFile(filepath, "rb");
        if (!file) {
            SDL_Log("Shader dosyası açılamadı: %s - Hata: %s", filepath, SDL_GetError());
            return nullptr;
        }

        Sint64 fileSize = SDL_GetIOSize(file);
        if (fileSize <= 0) {
            SDL_Log("Shader dosyası boş: %s", filepath);
            SDL_CloseIO(file);
            return nullptr;
        }

        std::vector<uint8_t> shaderCode(fileSize);
        size_t bytesRead = SDL_ReadIO(file, shaderCode.data(), fileSize);
        SDL_CloseIO(file);

        if (bytesRead != static_cast<size_t>(fileSize)) {
            SDL_Log("Shader dosyası tam okunamadı: %s", filepath);
            return nullptr;
        }

        // Shader oluştur
        SDL_GPUShaderCreateInfo shaderInfo = {};
        shaderInfo.code_size = shaderCode.size();
        shaderInfo.code = shaderCode.data();
        shaderInfo.entrypoint = entrypoint;
        shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV; // SPIRV kullanıyoruz
        shaderInfo.stage = stage;
        shaderInfo.num_samplers = numSamplers;
        shaderInfo.num_storage_textures = numStorageTextures;
        shaderInfo.num_storage_buffers = numStorageBuffers;
        shaderInfo.num_uniform_buffers = numUniformBuffers;
        shaderInfo.props = 0;

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
        
        if (!shader) {
            SDL_Log("Shader oluşturulamadı: %s - Hata: %s", filepath, SDL_GetError());
            return nullptr;
        }

        SDL_Log("Shader yüklendi: %s", filepath);
        return shader;
    }
};

} // namespace Astral
