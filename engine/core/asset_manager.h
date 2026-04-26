#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <string>
#include <vector>
#include "../math/vertex.h"

namespace Astral {

// GPU Mesh Handle - VRAM'de yaşayan mesh verisi
struct GPUMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
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

        // Mesh handle'ı cache'e ekle
        GPUMesh* mesh = new GPUMesh();
        mesh->vertexBuffer = vertexBuffer;
        mesh->indexBuffer = indexBuffer;
        mesh->vertexCount = static_cast<uint32_t>(vertices.size());
        mesh->indexCount = static_cast<uint32_t>(indices.size());

        m_meshes[name] = mesh;
        SDL_Log("AssetManager: Mesh '%s' yüklendi (V:%u, I:%u)", 
                name.c_str(), mesh->vertexCount, mesh->indexCount);
        
        return mesh;
    }

    GPUMesh* getMesh(const std::string& name) {
        auto it = m_meshes.find(name);
        return (it != m_meshes.end()) ? it->second : nullptr;
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

        // Shader'ları sakla (cleanup için)
        m_shaders.push_back(vertexShader);
        m_shaders.push_back(fragmentShader);

        // Vertex Input State - Vertex layout'u tanımla
        SDL_GPUVertexAttribute attributes[3];
        VertexLayout::GetAttributes(attributes);

        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.slot = VertexLayout::BINDING_INDEX;
        vertexBufferDesc.pitch = VertexLayout::STRIDE;
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexBufferDesc.instance_step_rate = 0;

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
        multisampleState.sample_mask = 0; // SDL_GPU assertion: sample_mask must be 0!
        multisampleState.enable_mask = false;

        // Depth Stencil State
        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        depthStencilState.compare_mask = 0;
        depthStencilState.write_mask = 0;
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
    // TEXTURE MANAGEMENT
    // ============================================================================

    bool loadTexture(const std::string& name, const std::string& path) {
        // TODO: SDL_GPUTexture yukleme mantigi eklenecek
        return true;
    }

    SDL_GPUTexture* getTexture(const std::string& name) {
        if (m_textures.find(name) == m_textures.end()) return nullptr;
        return m_textures[name];
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
        // Shaders (önce shader'ları temizle)
        for (auto shader : m_shaders) {
            if (shader) SDL_ReleaseGPUShader(m_gpuDevice, shader);
        }
        m_shaders.clear();

        // Pipelines
        for (auto& [name, pipeline] : m_pipelines) {
            SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, pipeline);
        }
        m_pipelines.clear();

        // Meshes
        for (auto& [name, mesh] : m_meshes) {
            if (mesh->vertexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, mesh->vertexBuffer);
            if (mesh->indexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, mesh->indexBuffer);
            delete mesh;
        }
        m_meshes.clear();

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
    std::map<std::string, GPUMesh*> m_meshes;
    std::map<std::string, SDL_GPUGraphicsPipeline*> m_pipelines;
    std::map<std::string, SDL_GPUTexture*> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
    std::vector<SDL_GPUShader*> m_shaders; // Shader cleanup için
};

} // namespace Astral