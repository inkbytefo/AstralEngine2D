#pragma once
#include "system.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Astral {
class AssetManager;
class IRenderer;


// Uniform Data - Shader'a gönderilecek MVP matrisleri
struct UniformData {
    glm::mat4 model;      // 64 bytes
    glm::mat4 view;       // 64 bytes
    glm::mat4 projection; // 64 bytes
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

    void setAssetManager(AssetManager* assetManager) { m_assetManager = assetManager; }
    void setWindow(SDL_Window* window) { m_window = window; }
    void setRenderer(IRenderer* renderer) { m_renderer = renderer; }

    // Camera override for Editor
    void setCameraOverride(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position) {
        m_overrideView = view;
        m_overrideProj = projection;
        m_overridePos = position;
        m_useCameraOverride = true;
    }
    void clearCameraOverride() { m_useCameraOverride = false; }

    int32_t getPriority() const override { return 100; }
    const char* getName() const override { return "RenderSystem"; }

    int getDrawCallCount() const { return m_drawCallCount; }

private:
    void prepare(Astral::EntityManager& entityManager);

    std::vector<std::shared_ptr<Entity>> m_renderQueue;
    int m_drawCallCount{ 0 };
    AssetManager* m_assetManager{ nullptr };
    SDL_Window* m_window{ nullptr };
    IRenderer* m_renderer{ nullptr };

    // Camera override
    bool m_useCameraOverride{ false };
    glm::mat4 m_overrideView{ 1.0f };
    glm::mat4 m_overrideProj{ 1.0f };
    glm::vec3 m_overridePos{ 0.0f };
};

} // namespace Astral