#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include "error_handling.h"
#include <map>
#include <memory>
#include <SDL3/SDL.h>

// GPUMesh struct is now defined in asset_interfaces.h

namespace Astral {

/**
 * @brief Concrete implementation of mesh management
 */
class MeshManager : public IMeshManager {
private:
    SDL_GPUDevice* m_device{ nullptr };
    std::map<std::string, std::unique_ptr<GPUMesh>> m_meshes;

    // Mega-buffers for bindless architecture
    GpuBuffer m_megaVertexBuffer;
    GpuBuffer m_megaIndexBuffer;
    uint32_t m_vertexOffset{ 0 };
    uint32_t m_indexOffset{ 0 };

    static constexpr uint32_t MEGA_VERTEX_CAPACITY{ 50 * 1024 * 1024 }; // 50 MB
    static constexpr uint32_t MEGA_INDEX_CAPACITY{ 20 * 1024 * 1024 };  // 20 MB

    void initMegaBuffers() {
        if (!m_device || m_megaVertexBuffer) return;

        SDL_GPUBufferCreateInfo vInfo = {};
        vInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vInfo.size = MEGA_VERTEX_CAPACITY;
        SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(m_device, &vInfo);
        m_megaVertexBuffer.reset(vertexBuffer, m_device);

        SDL_GPUBufferCreateInfo iInfo = {};
        iInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        iInfo.size = MEGA_INDEX_CAPACITY;
        SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(m_device, &iInfo);
        m_megaIndexBuffer.reset(indexBuffer, m_device);

        SDL_Log("MeshManager: Mega-Buffers initialized (Vertex: 50MB, Index: 20MB)");
    }

public:
    MeshManager() = default;

    void setGPUDevice(SDL_GPUDevice* device) {
        m_device = device;
        initMegaBuffers();
    }

    GPUMesh* uploadMesh(const std::string& name,
                       const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices,
                       bool isDynamic = false) override {
        if (!m_device) {
            ENGINE_ERROR(GPU, "GPU device not set for mesh upload");
        }

        if (vertices.empty() || indices.empty()) {
            ENGINE_ERROR_CTX(Asset, "Cannot upload empty mesh", name);
        }

        uint32_t vSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        uint32_t iSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

        if (m_vertexOffset + vSize > MEGA_VERTEX_CAPACITY ||
            m_indexOffset + iSize > MEGA_INDEX_CAPACITY) {
            ENGINE_ERROR_CTX(Memory, "Mega-buffer capacity exceeded for mesh", name);
        }

        // Create transfer buffer
        size_t totalSize = vSize + iSize;
        SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
        transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferBufferInfo.size = static_cast<uint32_t>(totalSize);
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_device, &transferBufferInfo);

        // Upload data to transfer buffer
        void* transferData = SDL_MapGPUTransferBuffer(m_device, transferBuffer, false);
        if (transferData) {
            memcpy(transferData, vertices.data(), vSize);
            memcpy(static_cast<char*>(transferData) + vSize, indices.data(), iSize);
            SDL_UnmapGPUTransferBuffer(m_device, transferBuffer);
        }

        // Copy to GPU mega-buffers
        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        // Vertex buffer upload
        SDL_GPUTransferBufferLocation vertexTransferLoc = { transferBuffer, 0 };
        SDL_GPUBufferRegion vertexBufferRegion = { m_megaVertexBuffer.get(), m_vertexOffset, vSize };
        SDL_UploadToGPUBuffer(copyPass, &vertexTransferLoc, &vertexBufferRegion, false);

        // Index buffer upload
        SDL_GPUTransferBufferLocation indexTransferLoc = { transferBuffer, vSize };
        SDL_GPUBufferRegion indexBufferRegion = { m_megaIndexBuffer.get(), m_indexOffset, iSize };
        SDL_UploadToGPUBuffer(copyPass, &indexTransferLoc, &indexBufferRegion, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);
        SDL_ReleaseGPUTransferBuffer(m_device, transferBuffer);

        // Create mesh handle
        auto mesh = std::make_unique<GPUMesh>();
        mesh->vertexBuffer = m_megaVertexBuffer.get();
        mesh->indexBuffer = m_megaIndexBuffer.get();
        mesh->vertexCount = static_cast<uint32_t>(vertices.size());
        mesh->indexCount = static_cast<uint32_t>(indices.size());
        mesh->vertexOffset = m_vertexOffset;
        mesh->indexOffset = m_indexOffset;

        // Update offsets
        m_vertexOffset += vSize;
        m_indexOffset += iSize;

        GPUMesh* rawPtr = mesh.get();
        m_meshes[name] = std::move(mesh);

        Astral::ErrorHandler::logInfo("Mesh uploaded: " + name +
            " (V Offset: " + std::to_string(rawPtr->vertexOffset) +
            ", I Offset: " + std::to_string(rawPtr->indexOffset) + ")");

        return rawPtr;
    }

    GPUMesh* getMesh(const std::string& name) const override {
        auto it = m_meshes.find(name);
        return (it != m_meshes.end()) ? it->second.get() : nullptr;
    }

    void cleanup() override {
        m_meshes.clear();
        m_megaVertexBuffer.reset();
        m_megaIndexBuffer.reset();
        m_vertexOffset = 0;
        m_indexOffset = 0;
    }
};

} // namespace Astral