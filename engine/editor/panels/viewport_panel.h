#pragma once

#include "editor_panel.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace Astral {
class EntityManager;
class IRenderer;
}

// Forward declaration for ImGuizmo
namespace ImGuizmo {
    enum OPERATION : int;
}

/**
 * @brief Viewport Panel - displays 3D scene and handles gizmo manipulation.
 */
class ViewportPanel : public IEditorPanel {
public:
    ViewportPanel(Astral::IRenderer* renderer, Astral::EntityManager* entityManager);
    ~ViewportPanel() = default;

    void draw() override;
    const char* getName() const override { return "Viewport"; }

    void onEntitySelected(std::shared_ptr<Astral::Entity> entity) override {
        m_selectedEntity = entity;
    }

    void onEntityDeselected() override {
        m_selectedEntity = nullptr;
    }

    uint32_t getViewportWidth() const { return m_viewportWidth; }
    uint32_t getViewportHeight() const { return m_viewportHeight; }

    /**
     * @brief Set the scene texture to display.
     */
    void setSceneTexture(SDL_GPUTexture* texture) { m_sceneTexture = texture; }

    /**
     * @brief Set FPS and draw call count for display.
     */
    void setDebugInfo(float fps, int drawCalls) { m_fps = fps; m_drawCallCount = drawCalls; }

private:
    void drawGizmoModeButtons();
    void drawViewportInfo();
    void handleKeyboardShortcuts();

private:
    Astral::IRenderer* m_renderer{ nullptr };
    Astral::EntityManager* m_entityManager{ nullptr };
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    SDL_GPUTexture* m_sceneTexture{ nullptr };
    uint32_t m_viewportWidth{ 1280 };
    uint32_t m_viewportHeight{ 720 };
    
    // Gizmo mode (using int to avoid ImGuizmo dependency in header)
    int m_gizmoMode{ 0 }; // 0 = TRANSLATE, 1 = ROTATE, 2 = SCALE
    
    // Debug info
    float m_fps{ 0.0f };
    int m_drawCallCount{ 0 };
};
