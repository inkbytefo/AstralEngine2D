#pragma once
#include <string>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "../math/vertex.h"

namespace Astral {

// GPU Mesh Handle - VRAM'de yaşayan mesh verisi
struct GPUMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t vertexOffset = 0; // Bayt cinsinden
    uint32_t indexOffset = 0;  // Bayt cinsinden
};

// Material - Pipeline + Texture + Properties (DOD uyumlu)
struct Material {
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUTexture* albedoTexture = nullptr;
    SDL_GPUTexture* normalTexture = nullptr;
    SDL_GPUTexture* metallicRoughnessTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    // PBR özellikleri
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic{0.0f};
    float roughness{0.5f};

    bool hasAlbedoTexture{false};
    bool hasNormalTexture{false};
    bool hasMetallicRoughnessTexture{false};
};

/**
 * @brief Interface for mesh management
 */
class IMeshManager {
public:
    virtual ~IMeshManager() = default;
    virtual GPUMesh* uploadMesh(const std::string& name,
                               const std::vector<Vertex>& vertices,
                               const std::vector<uint32_t>& indices,
                               bool isDynamic = false) = 0;
    virtual GPUMesh* getMesh(const std::string& name) const = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for texture management
 */
class ITextureManager {
public:
    virtual ~ITextureManager() = default;
    virtual SDL_GPUTexture* uploadTexture(const std::string& name, const std::string& filepath) = 0;
    virtual SDL_GPUTexture* getTexture(const std::string& name) const = 0;
    virtual SDL_GPUTexture* getWhiteTexture() const = 0;
    virtual SDL_GPUTexture* getBlackTexture() const = 0;
    virtual SDL_GPUTexture* getNormalTexture() const = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for shader management
 */
class IShaderManager {
public:
    virtual ~IShaderManager() = default;
    virtual SDL_GPUShader* loadShader(const std::string& path, SDL_GPUShaderStage stage) = 0;
    virtual void addShader(SDL_GPUShader* shader) = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for pipeline management
 */
class IPipelineManager {
public:
    virtual ~IPipelineManager() = default;
    virtual SDL_GPUGraphicsPipeline* createPipeline(const std::string& name,
                                                   SDL_GPUShader* vertexShader,
                                                   SDL_GPUShader* fragmentShader,
                                                   SDL_GPUTextureFormat renderTargetFormat,
                                                   bool enableDepth = true,
                                                   SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_BACK) = 0;
    virtual SDL_GPUGraphicsPipeline* getPipeline(const std::string& name) const = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for material management
 */
class IMaterialManager {
public:
    virtual ~IMaterialManager() = default;
    virtual Material* createMaterial(const std::string& name,
                                    const std::string& pipelineName,
                                    const std::string& albedoName = "",
                                    const std::string& normalName = "",
                                    const std::string& mrName = "",
                                    const glm::vec4& baseColor = glm::vec4(1.0f),
                                    float metallic = 0.0f,
                                    float roughness = 0.5f) = 0;
    virtual Material* getMaterial(const std::string& name) const = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for sampler management
 */
class ISamplerManager {
public:
    virtual ~ISamplerManager() = default;
    virtual SDL_GPUSampler* createSampler(SDL_GPUFilter filter = SDL_GPU_FILTER_LINEAR) = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Interface for font management
 */
class IFontManager {
public:
    virtual ~IFontManager() = default;
    virtual bool loadFont(const std::string& name, const std::string& path, float size) = 0;
    virtual TTF_Font* getFont(const std::string& name) const = 0;
    virtual void cleanup() = 0;
};

/**
 * @brief Main asset registry interface - coordinates all asset managers
 */
class IAssetRegistry {
public:
    virtual ~IAssetRegistry() = default;

    // Manager accessors
    virtual IMeshManager& getMeshManager() = 0;
    virtual ITextureManager& getTextureManager() = 0;
    virtual IShaderManager& getShaderManager() = 0;
    virtual IPipelineManager& getPipelineManager() = 0;
    virtual IMaterialManager& getMaterialManager() = 0;
    virtual ISamplerManager& getSamplerManager() = 0;
    virtual IFontManager& getFontManager() = 0;

    // GPU device management
    virtual void setGPUDevice(SDL_GPUDevice* device) = 0;
    virtual SDL_GPUDevice* getGPUDevice() const = 0;

    // Global cleanup
    virtual void cleanup() = 0;
};

} // namespace Astral
