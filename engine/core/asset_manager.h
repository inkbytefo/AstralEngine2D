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
private:
    SDL_GPUDevice* m_gpuDevice{ nullptr };
    
    // Kaynak Haritaları (Resource Maps)
    std::map<std::string, std::unique_ptr<GPUMesh>> m_meshes;
    std::map<std::string, std::unique_ptr<Material>> m_materials;
    std::map<std::string, SDL_GPUGraphicsPipeline*> m_pipelines;
    std::map<std::string, SDL_GPUTexture*> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
    
    // Temizlik Listeleri (Cleanup Lists)
    std::vector<SDL_GPUShader*> m_shaders;
    std::vector<SDL_GPUSampler*> m_samplers;
    
    // MEGA-BUFFER (Bindless Architecture) Değişkenleri
    SDL_GPUBuffer* m_megaVertexBuffer{ nullptr };
    SDL_GPUBuffer* m_megaIndexBuffer{ nullptr };
    uint32_t m_megaVertexOffset{ 0 }; // Bayt cinsinden
    uint32_t m_megaIndexOffset{ 0 };  // Bayt cinsinden
    const uint32_t MEGA_VERTEX_CAPACITY{ 50 * 1024 * 1024 }; // 50 MB
    const uint32_t MEGA_INDEX_CAPACITY{ 20 * 1024 * 1024 };  // 20 MB

    // Varsayılan dokular (Fallback textures)
    SDL_GPUTexture* m_whiteTexture{ nullptr };
    SDL_GPUTexture* m_blackTexture{ nullptr };
    SDL_GPUTexture* m_defaultNormalTexture{ nullptr };

    AssetManager() = default;
    ~AssetManager() { cleanup(); }

public:
    static AssetManager& getInstance() {
        static AssetManager instance;
        return instance;
    }

    // GPU Device'ı set et (App başlangıcında çağrılmalı)
    void setGPUDevice(SDL_GPUDevice* device) {
        m_gpuDevice = device;
        initMegaBuffers(); // Device geldiğinde mega-buffer'ları oluştur
        createDefaultTextures(); // Varsayılan dokuları oluştur
    }

private:
    void initMegaBuffers() {
        if (!m_gpuDevice || m_megaVertexBuffer) return;

        SDL_GPUBufferCreateInfo vInfo = {};
        vInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vInfo.size = MEGA_VERTEX_CAPACITY;
        m_megaVertexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &vInfo);

        SDL_GPUBufferCreateInfo iInfo = {};
        iInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        iInfo.size = MEGA_INDEX_CAPACITY;
        m_megaIndexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &iInfo);

        SDL_Log("Mega-Buffers oluşturuldu! (Vertex: 50MB, Index: 20MB)");
    }

    void createDefaultTextures() {
        if (!m_gpuDevice || m_whiteTexture) return;

        m_whiteTexture = create1x1Texture(255, 255, 255, 255);
        m_blackTexture = create1x1Texture(0, 0, 0, 255);
        m_defaultNormalTexture = create1x1Texture(128, 128, 255, 255); // Flat normal (0.5, 0.5, 1.0)
        
        SDL_Log("Varsayılan (Fallback) dokular oluşturuldu.");
    }

    SDL_GPUTexture* create1x1Texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_gpuDevice, &texInfo);

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = 4;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        uint8_t* data = (uint8_t*)SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        if (data) {
            data[0] = r; data[1] = g; data[2] = b; data[3] = a;
            SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
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
        
        SDL_WaitForGPUIdle(m_gpuDevice);
        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        return texture;
    }
public:
    SDL_GPUTexture* getWhiteTexture() const { return m_whiteTexture; }
    SDL_GPUTexture* getBlackTexture() const { return m_blackTexture; }
    SDL_GPUTexture* getNormalTexture() const { return m_defaultNormalTexture; }
    // ============================================================================
    // MESH UPLOAD - Staging Buffer ile Mega-Buffer'a Append
    // ============================================================================
    
    GPUMesh* uploadMesh(const std::string& name, 
                        const std::vector<Vertex>& vertices,
                        const std::vector<uint32_t>& indices,
                        bool isDynamic = false) {
        
        if (!m_gpuDevice) {
            SDL_Log("AssetManager: GPU Device set edilmemiş!");
            return nullptr;
        }

        uint32_t vSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        uint32_t iSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

        if (m_megaVertexOffset + vSize > MEGA_VERTEX_CAPACITY || 
            m_megaIndexOffset + iSize > MEGA_INDEX_CAPACITY) {
            SDL_Log("HATA: Mega-Buffer kapasitesi aşıldı! (%s)", name.c_str());
            return nullptr;
        }

        // Transfer Buffer oluştur (Staging - CPU)
        size_t totalSize = vSize + iSize;
        SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
        transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferBufferInfo.size = static_cast<uint32_t>(totalSize);
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferBufferInfo);

        // Veriyi transfer buffer'a yaz
        void* transferData = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        if (transferData) {
            memcpy(transferData, vertices.data(), vSize);
            memcpy(static_cast<char*>(transferData) + vSize, indices.data(), iSize);
            SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);
        }

        // Copy Pass (VRAM'deki Mega-Buffer'ın sonuna ekle)
        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        // Vertex buffer upload (Offsetli)
        SDL_GPUTransferBufferLocation vertexTransferLoc = { transferBuffer, 0 };
        SDL_GPUBufferRegion vertexBufferRegion = { m_megaVertexBuffer, m_megaVertexOffset, vSize };
        SDL_UploadToGPUBuffer(copyPass, &vertexTransferLoc, &vertexBufferRegion, false);

        // Index buffer upload (Offsetli)
        SDL_GPUTransferBufferLocation indexTransferLoc = { transferBuffer, vSize };
        SDL_GPUBufferRegion indexBufferRegion = { m_megaIndexBuffer, m_megaIndexOffset, iSize };
        SDL_UploadToGPUBuffer(copyPass, &indexTransferLoc, &indexBufferRegion, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        // Mesh handle'ı cache'e ekle
        auto mesh = std::make_unique<GPUMesh>();
        mesh->vertexBuffer = m_megaVertexBuffer; 
        mesh->indexBuffer = m_megaIndexBuffer;
        mesh->vertexCount = static_cast<uint32_t>(vertices.size());
        mesh->indexCount = static_cast<uint32_t>(indices.size());
        mesh->vertexOffset = m_megaVertexOffset;
        mesh->indexOffset = m_megaIndexOffset;

        // Global offsetleri ilerlet
        m_megaVertexOffset += vSize;
        m_megaIndexOffset += iSize;

        GPUMesh* rawPtr = mesh.get();
        
        SDL_Log("AssetManager: Mesh '%s' Mega-Buffer'a eklendi (V Offset: %u, I Offset: %u)", 
                name.c_str(), rawPtr->vertexOffset, rawPtr->indexOffset);

        m_meshes[name] = std::move(mesh);
                
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
    // PIPELINE MANAGEMENT
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

        SDL_GPUVertexAttribute attributes[5];
        Vertex::getAttributeDescriptions(attributes);

        SDL_GPUVertexBufferDescription vertexBufferDesc = Vertex::getBindingDescription();

        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = attributes;
        vertexInputState.num_vertex_attributes = 5;

        SDL_GPURasterizerState rasterizerState = {};
        rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
        rasterizerState.cull_mode = cullMode;
        rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizerState.enable_depth_clip = true;

        SDL_GPUMultisampleState multisampleState = {};
        multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;

        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        depthStencilState.enable_depth_test = enableDepth;
        depthStencilState.enable_depth_write = enableDepth;

        SDL_GPUColorTargetBlendState blendState = {};
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask = 0xF;
        blendState.enable_blend = true;

        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = renderTargetFormat;
        colorTargetDesc.blend_state = blendState;

        SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
        targetInfo.color_target_descriptions = &colorTargetDesc;
        targetInfo.num_color_targets = 1;
        targetInfo.depth_stencil_format = enableDepth ? SDL_GPU_TEXTUREFORMAT_D16_UNORM : SDL_GPU_TEXTUREFORMAT_INVALID;
        targetInfo.has_depth_stencil_target = enableDepth;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.vertex_shader = vertexShader;
        pipelineCreateInfo.fragment_shader = fragmentShader;
        pipelineCreateInfo.vertex_input_state = vertexInputState;
        pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.rasterizer_state = rasterizerState;
        pipelineCreateInfo.multisample_state = multisampleState;
        pipelineCreateInfo.depth_stencil_state = depthStencilState;
        pipelineCreateInfo.target_info = targetInfo;

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
    // TEXTURE MANAGEMENT
    // ============================================================================

    SDL_GPUTexture* uploadTexture(const std::string& name, const std::string& filepath) {
        // Eğer aynı isimle zaten yüklenmişse, tekrar yükleme
        if (m_textures.find(name) != m_textures.end()) {
            return m_textures[name];
        }

        if (!m_gpuDevice) return nullptr;

        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            SDL_Log("HATA: Texture yuklenemedi: %s", filepath.c_str());
            return nullptr;
        }

        SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        
        if (!rgbaSurface) return nullptr;

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

        uint32_t dataSize = rgbaSurface->w * rgbaSurface->h * 4;
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = dataSize;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        void* transferData = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        if (transferData) {
            memcpy(transferData, rgbaSurface->pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);
        }
        SDL_DestroySurface(rgbaSurface);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
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

        SDL_WaitForGPUIdle(m_gpuDevice);
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
                             const std::string& albedoName = "", const std::string& normalName = "", const std::string& mrName = "",
                             const glm::vec4& baseColor = glm::vec4(1.0f), float metallic = 0.0f, float roughness = 0.5f) 
    {
        auto mat = std::make_unique<Material>();
        mat->pipeline = getPipeline(pipelineName);
        mat->baseColor = baseColor;
        mat->metallic = metallic;
        mat->roughness = roughness;

        if (!albedoName.empty()) {
            mat->albedoTexture = getTexture(albedoName);
            mat->hasAlbedoTexture = (mat->albedoTexture != nullptr);
        }
        if (!normalName.empty()) {
            mat->normalTexture = getTexture(normalName);
            mat->hasNormalTexture = (mat->normalTexture != nullptr);
        }
        if (!mrName.empty()) {
            mat->metallicRoughnessTexture = getTexture(mrName);
            mat->hasMetallicRoughnessTexture = (mat->metallicRoughnessTexture != nullptr);
        }

        if (m_samplers.empty()) createSampler(); 
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
        if (!font) return false;
        m_fonts[name] = font;
        return true;
    }

    TTF_Font* getFont(const std::string& name) {
        auto it = m_fonts.find(name);
        return (it != m_fonts.end()) ? it->second : nullptr;
    }

    // ============================================================================
    // CLEANUP
    // ============================================================================

    void cleanup() {
        if (!m_gpuDevice) return;

        m_materials.clear();

        for (auto sampler : m_samplers) {
            if (sampler) SDL_ReleaseGPUSampler(m_gpuDevice, sampler);
        }
        m_samplers.clear();

        for (auto& [name, pipeline] : m_pipelines) {
            SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, pipeline);
        }
        m_pipelines.clear();

        for (auto shader : m_shaders) {
            if (shader) SDL_ReleaseGPUShader(m_gpuDevice, shader);
        }
        m_shaders.clear();

        m_meshes.clear();

        if (m_megaVertexBuffer) {
            SDL_ReleaseGPUBuffer(m_gpuDevice, m_megaVertexBuffer);
            m_megaVertexBuffer = nullptr;
        }
        if (m_megaIndexBuffer) {
            SDL_ReleaseGPUBuffer(m_gpuDevice, m_megaIndexBuffer);
            m_megaIndexBuffer = nullptr;
        }

        for (auto& [name, tex] : m_textures) {
            SDL_ReleaseGPUTexture(m_gpuDevice, tex);
        }
        m_textures.clear();

        for (auto& [name, font] : m_fonts) {
            TTF_CloseFont(font);
        }
        m_fonts.clear();

        // Varsayılan dokuları temizle
        if (m_whiteTexture) {
            SDL_ReleaseGPUTexture(m_gpuDevice, m_whiteTexture);
            m_whiteTexture = nullptr;
        }
        if (m_blackTexture) {
            SDL_ReleaseGPUTexture(m_gpuDevice, m_blackTexture);
            m_blackTexture = nullptr;
        }
        if (m_defaultNormalTexture) {
            SDL_ReleaseGPUTexture(m_gpuDevice, m_defaultNormalTexture);
            m_defaultNormalTexture = nullptr;
        }
    }
};

} // namespace Astral