#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "../math/vertex.h"
#include "shader_loader.h"

namespace Astral {

// GPU Mesh Handle - VRAM'de yaşayan mesh verisi
struct GPUMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

// Material - Pipeline + Texture + Properties (DOD uyumlu)
struct Material {
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUTexture* albedoTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    bool hasAlbedoTexture{false};
};

// Pipeline Cache Key - Pipeline'ları önbelleklemek için
struct PipelineKey {
    std::string vertexShader;
    std::string fragmentShader;
    SDL_GPUTextureFormat renderTargetFormat;
    bool hasDepth;

    bool operator<(const PipelineKey& other) const {
        if (vertexShader != other.vertexShader) return vertexShader < other.vertexShader;
        if (fragmentShader != other.fragmentShader) return fragmentShader < other.fragmentShader;
        if (renderTargetFormat != other.renderTargetFormat) return renderTargetFormat < other.renderTargetFormat;
        return hasDepth < other.hasDepth;
    }
};

class AssetManager {
public:
    static AssetManager& getInstance() {
        static AssetManager instance;
        return instance;
    }

    // GPU Device'ı set et (App başlangıcında çağrılmalı)
    void setGPUDevice(SDL_GPUDevice* device) {
        m_gpuDevice = device;
    }

    // ============================================================================
    // MESH UPLOAD - Staging Buffer ile VRAM'e yükleme
    // ============================================================================
    
    // Mesh'i GPU'ya yükle (vertices + indices)
    // Cycling: true = her frame yeni mesh yüklenebilir, false = static mesh
    GPUMesh* uploadMesh(const std::string& name, 
                        const std::vector<Vertex>& vertices,
                        const std::vector<uint32_t>& indices,
                        bool isDynamic = false) {
        
        if (!m_gpuDevice) {
            SDL_Log("AssetManager: GPU Device set edilmemiş!");
            return nullptr;
        }

        // Vertex Buffer oluştur (VRAM)
        SDL_GPUBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexBufferInfo.size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        
        SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &vertexBufferInfo);

        // Index Buffer oluştur (VRAM)
        SDL_GPUBufferCreateInfo indexBufferInfo = {};
        indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexBufferInfo.size = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        
        SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &indexBufferInfo);

        if (!vertexBuffer || !indexBuffer) {
            SDL_Log("AssetManager: Buffer oluşturulamadı!");
            return nullptr;
        }

        // Transfer Buffer oluştur (Staging - CPU tarafında)
        size_t totalSize = (vertices.size() * sizeof(Vertex)) + (indices.size() * sizeof(uint32_t));
        
        SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
        transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferBufferInfo.size = static_cast<uint32_t>(totalSize);
        
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferBufferInfo);

        // Transfer buffer'a veriyi yaz (CPU tarafında)
        void* transferData = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        if (transferData) {
            // Vertex data
            memcpy(transferData, vertices.data(), vertices.size() * sizeof(Vertex));
            // Index data (vertex data'dan sonra)
            memcpy(static_cast<char*>(transferData) + vertices.size() * sizeof(Vertex),
                   indices.data(), indices.size() * sizeof(uint32_t));
            SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);
        }

        // Copy Pass ile GPU'ya transfer et
        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        // Vertex buffer upload
        SDL_GPUTransferBufferLocation vertexTransferLoc = {};
        vertexTransferLoc.transfer_buffer = transferBuffer;
        vertexTransferLoc.offset = 0;
        
        SDL_GPUBufferRegion vertexBufferRegion = {};
        vertexBufferRegion.buffer = vertexBuffer;
        vertexBufferRegion.offset = 0;
        vertexBufferRegion.size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        
        SDL_UploadToGPUBuffer(copyPass, &vertexTransferLoc, &vertexBufferRegion, false);

        // Index buffer upload
        SDL_GPUTransferBufferLocation indexTransferLoc = {};
        indexTransferLoc.transfer_buffer = transferBuffer;
        indexTransferLoc.offset = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        
        SDL_GPUBufferRegion indexBufferRegion = {};
        indexBufferRegion.buffer = indexBuffer;
        indexBufferRegion.offset = 0;
        indexBufferRegion.size = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        
        SDL_UploadToGPUBuffer(copyPass, &indexTransferLoc, &indexBufferRegion, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        // Transfer buffer'ı temizle
        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        // Mesh handle'ı cache'e ekle (Modern C++20 - Smart Pointer)
        auto mesh = std::make_unique<GPUMesh>();
        mesh->vertexBuffer = vertexBuffer;
        mesh->indexBuffer = indexBuffer;
        mesh->vertexCount = static_cast<uint32_t>(vertices.size());
        mesh->indexCount = static_cast<uint32_t>(indices.size());

        GPUMesh* rawPtr = mesh.get(); // Raw pointer döndür (kullanım için)
        m_meshes[name] = std::move(mesh);
        
        SDL_Log("AssetManager: Mesh '%s' yüklendi (V:%u, I:%u)", 
                name.c_str(), rawPtr->vertexCount, rawPtr->indexCount);
        
        return rawPtr;
    }

    // ============================================================================
    // SHADER MANAGEMENT
    // ============================================================================
    
    SDL_GPUShader* loadShader(const std::string& path, SDL_GPUShaderStage stage) {
        if (!m_gpuDevice) return nullptr;
        
        SDL_GPUShader* shader = ShaderLoader::loadShaderFromFile(m_gpuDevice, path.c_str(), stage);
        if (shader) {
            m_shaders.push_back(shader);
        }
        return shader;
    }

    void addShader(SDL_GPUShader* shader) {
        if (shader) m_shaders.push_back(shader);
    }

    GPUMesh* getMesh(const std::string& name) {
        auto it = m_meshes.find(name);
        return (it != m_meshes.end()) ? it->second.get() : nullptr;
    }

    // ============================================================================
    // PIPELINE MANAGEMENT - Pipeline önbellekleme
    // ============================================================================
    
    SDL_GPUGraphicsPipeline* createPipeline(
        const std::string& name,
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader,
        SDL_GPUTextureFormat renderTargetFormat,
        bool enableDepth = true,
        SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_BACK)
    {
        if (!m_gpuDevice) return nullptr;

        // Vertex Input State - Vertex layout'u tanımla
        SDL_GPUVertexAttribute attributes[3];
        Vertex::getAttributeDescriptions(attributes);

        SDL_GPUVertexBufferDescription vertexBufferDesc = Vertex::getBindingDescription();

        // Vertex Input State
        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = attributes;
        vertexInputState.num_vertex_attributes = 3;

        // Rasterizer State
        SDL_GPURasterizerState rasterizerState = {};
        rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
        rasterizerState.cull_mode = cullMode;
        rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizerState.depth_bias_constant_factor = 0.0f;
        rasterizerState.depth_bias_clamp = 0.0f;
        rasterizerState.depth_bias_slope_factor = 0.0f;
        rasterizerState.enable_depth_bias = false;
        rasterizerState.enable_depth_clip = true;

        // Multisample State
        SDL_GPUMultisampleState multisampleState = {};
        multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;
        multisampleState.sample_mask = 0;
        multisampleState.enable_mask = false;

        // Depth Stencil State
        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        depthStencilState.compare_mask = 0xFF;
        depthStencilState.write_mask = 0xFF;
        depthStencilState.enable_depth_test = enableDepth;
        depthStencilState.enable_depth_write = enableDepth;
        depthStencilState.enable_stencil_test = false;

        // Blend State
        SDL_GPUColorTargetBlendState blendState = {};
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask = 0xF;
        blendState.enable_blend = true;
        blendState.enable_color_write_mask = false;

        // Color Target Description
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = renderTargetFormat;
        colorTargetDesc.blend_state = blendState;

        // Target Info
        SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
        targetInfo.color_target_descriptions = &colorTargetDesc;
        targetInfo.num_color_targets = 1;
        targetInfo.depth_stencil_format = enableDepth ? SDL_GPU_TEXTUREFORMAT_D16_UNORM : SDL_GPU_TEXTUREFORMAT_INVALID;
        targetInfo.has_depth_stencil_target = enableDepth;

        // Pipeline Create Info
        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;
        pipelineCreateInfo.vertex_input_state = vertexInputState;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.rasterizer_state = rasterizerState;
        pipelineCreateInfo.multisample_state = multisampleState;
        pipelineCreateInfo.depth_stencil_state = depthStencilState;
        pipelineCreateInfo.target_info = targetInfo;
        pipelineCreateInfo.props = 0;

        // Pipeline oluştur
        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(m_gpuDevice, &pipelineCreateInfo);

        if (pipeline) {
            m_pipelines[name] = pipeline;
            SDL_Log("AssetManager: Pipeline '%s' oluşturuldu", name.c_str());
        }

        return pipeline;
    }

    SDL_GPUGraphicsPipeline* getPipeline(const std::string& name) {
        auto it = m_pipelines.find(name);
        return (it != m_pipelines.end()) ? it->second : nullptr;
    }

    // ============================================================================
    // TEXTURE MANAGEMENT - Staging Buffer ile VRAM'e yükleme
    // ============================================================================

    SDL_GPUTexture* uploadTexture(const std::string& name, const std::string& filepath) {
        if (!m_gpuDevice) return nullptr;

        // 1. Resmi CPU'ya yükle
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            SDL_Log("HATA: Texture yuklenemedi: %s", filepath.c_str());
            return nullptr;
        }

        // 2. Formatı GPU dostu (RGBA32) formata çevir
        SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface); // Eskisini sil
        
        if (!rgbaSurface) return nullptr;

        // 3. VRAM Texture oluştur
        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.width = rgbaSurface->w;
        texInfo.height = rgbaSurface->h;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        
        SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_gpuDevice, &texInfo);

        // 4. Staging Buffer oluştur
        uint32_t dataSize = rgbaSurface->w * rgbaSurface->h * 4;
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = dataSize;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        // 5. Veriyi Staging Buffer'a kopyala
        void* transferData = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        if (transferData) {
            memcpy(transferData, rgbaSurface->pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);
        }
        SDL_DestroySurface(rgbaSurface);

        // 6. Copy Pass (VRAM'e aktarım)
        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        SDL_GPUTextureTransferInfo transferLoc = {};
        transferLoc.transfer_buffer = transferBuffer;
        transferLoc.offset = 0;

        SDL_GPUTextureRegion texRegion = {};
        texRegion.texture = texture;
        texRegion.w = texInfo.width;
        texRegion.h = texInfo.height;
        texRegion.d = 1;

        SDL_UploadToGPUTexture(copyPass, &transferLoc, &texRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        // KRITIK: GPU'nun texture upload'ı bitirmesini bekle!
        SDL_WaitForGPUIdle(m_gpuDevice);

        // 7. Temizlik
        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        m_textures[name] = texture;
        SDL_Log("Texture yüklendi: %s (%dx%d)", name.c_str(), texInfo.width, texInfo.height);
        return texture;
    }

    SDL_GPUTexture* getTexture(const std::string& name) {
        auto it = m_textures.find(name);
        return (it != m_textures.end()) ? it->second : nullptr;
    }

    // ============================================================================
    // SAMPLER MANAGEMENT
    // ============================================================================

    SDL_GPUSampler* createSampler(SDL_GPUFilter filter = SDL_GPU_FILTER_LINEAR) {
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = filter;
        samplerInfo.mag_filter = filter;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        
        SDL_GPUSampler* sampler = SDL_CreateGPUSampler(m_gpuDevice, &samplerInfo);
        if (sampler) m_samplers.push_back(sampler);
        return sampler;
    }

    // ============================================================================
    // MATERIAL MANAGEMENT
    // ============================================================================

    Material* createMaterial(const std::string& name, const std::string& pipelineName, 
                             const std::string& textureName = "", const glm::vec4& baseColor = glm::vec4(1.0f)) 
    {
        auto mat = std::make_unique<Material>();
        mat->pipeline = getPipeline(pipelineName);
        mat->baseColor = baseColor;

        if (!textureName.empty()) {
            mat->albedoTexture = getTexture(textureName);
            mat->hasAlbedoTexture = (mat->albedoTexture != nullptr);
        }

        if (m_samplers.empty()) {
            createSampler(); // Default sampler oluştur
        }
        mat->sampler = m_samplers[0];

        Material* rawPtr = mat.get();
        m_materials[name] = std::move(mat);
        return rawPtr;
    }

    Material* getMaterial(const std::string& name) {
        auto it = m_materials.find(name);
        return (it != m_materials.end()) ? it->second.get() : nullptr;
    }

    // ============================================================================
    // FONT MANAGEMENT
    // ============================================================================

    bool loadFont(const std::string& name, const std::string& path, float size) {
        TTF_Font* font = TTF_OpenFont(path.c_str(), size);
        if (!font) {
            SDL_Log("Font yuklenemedi: %s - Hata: %s", path.c_str(), SDL_GetError());
            return false;
        }
        m_fonts[name] = font;
        return true;
    }

    TTF_Font* getFont(const std::string& name) {
        if (m_fonts.find(name) == m_fonts.end()) return nullptr;
        return m_fonts[name];
    }

    // ============================================================================
    // CLEANUP
    // ============================================================================

    void cleanup() {
        // Materials - Smart pointer'lar otomatik temizlenir
        m_materials.clear();

        // Samplers
        for (auto sampler : m_samplers) {
            if (sampler) SDL_ReleaseGPUSampler(m_gpuDevice, sampler);
        }
        m_samplers.clear();

        // Önce pipeline'ları temizle (shader'lara bağımlı)
        for (auto& [name, pipeline] : m_pipelines) {
            SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, pipeline);
        }
        m_pipelines.clear();

        // Sonra shader'ları temizle
        for (auto shader : m_shaders) {
            if (shader) SDL_ReleaseGPUShader(m_gpuDevice, shader);
        }
        m_shaders.clear();

        // Meshes - Smart pointer'lar otomatik temizlenir, sadece GPU resource'ları serbest bırak
        for (auto& [name, mesh] : m_meshes) {
            if (mesh->vertexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, mesh->vertexBuffer);
            if (mesh->indexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, mesh->indexBuffer);
        }
        m_meshes.clear(); // unique_ptr'lar otomatik delete edilir

        // Textures
        for (auto& [name, tex] : m_textures) {
            SDL_ReleaseGPUTexture(m_gpuDevice, tex);
        }
        m_textures.clear();

        // Fonts
        for (auto& [name, font] : m_fonts) {
            TTF_CloseFont(font);
        }
        m_fonts.clear();
    }

private:
    AssetManager() = default;
    ~AssetManager() = default;

    SDL_GPUDevice* m_gpuDevice = nullptr;
    std::map<std::string, std::unique_ptr<GPUMesh>> m_meshes; // Modern C++20 - Smart Pointers
    std::map<std::string, SDL_GPUGraphicsPipeline*> m_pipelines;
    std::map<std::string, SDL_GPUTexture*> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
    std::vector<SDL_GPUShader*> m_shaders; // Shader cleanup için
    std::vector<SDL_GPUSampler*> m_samplers; // Sampler cleanup için
    std::map<std::string, std::unique_ptr<Material>> m_materials; // Material management
};

} // namespace Astral