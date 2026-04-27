#pragma once
#include "system.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Astral {
class IAssetRegistry;
class IRenderer;
class EntityManager;
class Entity;
struct GPUMesh;
struct Material;

struct CameraFrameData {
    glm::mat4 view{ 1.0f };
    glm::mat4 projection{ 1.0f };
    glm::vec3 position{ 0.0f };
    bool isValid{ false };
};

// Uniform Data - Shader'a gönderilecek MVP matrisleri
struct UniformData {
    glm::mat4 model;      // 64 bytes
    glm::mat4 view;       // 64 bytes
    glm::mat4 projection; // 64 bytes
};

// Render batch for instanced rendering optimization
struct RenderBatch {
    SDL_GPUGraphicsPipeline* pipeline{ nullptr };
    GPUMesh* mesh{ nullptr };
    Material* material{ nullptr };
    std::vector<glm::mat4> transforms;
    std::vector<uint32_t> entityIds;

    // Batch key for grouping similar objects
    struct Key {
        SDL_GPUGraphicsPipeline* pipeline;
        GPUMesh* mesh;
        Material* material;

        bool operator==(const Key& other) const {
            return pipeline == other.pipeline &&
                   mesh == other.mesh &&
                   material == other.material;
        }

        bool operator<(const Key& other) const {
            if (pipeline != other.pipeline) return pipeline < other.pipeline;
            if (mesh != other.mesh) return mesh < other.mesh;
            return material < other.material;
        }
    };

    Key getKey() const { return {pipeline, mesh, material}; }
    bool isEmpty() const { return transforms.empty(); }
    size_t getInstanceCount() const { return transforms.size(); }

    void addInstance(const glm::mat4& transform, uint32_t entityId) {
        transforms.push_back(transform);
        entityIds.push_back(entityId);
    }

    void clear() {
        transforms.clear();
        entityIds.clear();
    }
};

class RenderSystem : public ISystem {
public:
    RenderSystem() = default;
    virtual ~RenderSystem() = default;

    // ISystem interface
    void init(Astral::EntityManager& entityManager) override {}
    void update(Astral::EntityManager& entityManager, float deltaTime) override;
    void shutdown() override {}

    // Rendering interface
    void render(IRenderer* renderer, Astral::EntityManager& entityManager);

    void setAssetManager(IAssetRegistry* assetRegistry) { m_assetManager = assetRegistry; }
    void setWindow(SDL_Window* window) { m_window = window; }
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    void setCameraData(const CameraFrameData& cameraData) { m_cameraData = cameraData; }
    void clearCameraData() { m_cameraData = CameraFrameData{}; }

    int32_t getPriority() const override { return 100; }
    const char* getName() const override { return "RenderSystem"; }

    int getDrawCallCount() const { return m_drawCallCount; }

private:
    void prepare(Astral::EntityManager& entityManager);
    void processBatches();
    void flushBatch(const RenderBatch& batch);
    CameraFrameData resolveSceneCamera(Astral::EntityManager& entityManager) const;

    std::vector<std::shared_ptr<Entity>> m_renderQueue;
    std::map<RenderBatch::Key, RenderBatch> m_batches; // Group similar objects for batching
    int m_drawCallCount{ 0 };
    IAssetRegistry* m_assetManager{ nullptr };
    SDL_Window* m_window{ nullptr };
    IRenderer* m_renderer{ nullptr };

    CameraFrameData m_cameraData;
};

} // namespace Astral
