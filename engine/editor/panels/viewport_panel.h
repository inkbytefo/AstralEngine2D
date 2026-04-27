#pragma once

#include "editor_panel.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace Astral {
class EntityManager;
class IRenderer;
}

// Forward declarations
class EditorManager;

namespace Astral {
    class Entity;
}

// Forward declaration for ImGuizmo
namespace ImGuizmo {
    enum OPERATION : int;
    enum MODE : int;
}

/**
 * @brief Editor Camera class for viewport manipulation.
 */
class EditorCamera {
public:
    glm::vec3 position = { 0.0f, -10.0f, 5.0f };
    float yaw = 90.0f;   // Looking along +Y
    float pitch = 0.0f;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float movementSpeed = 10.0f;
    float mouseSensitivity = 0.1f;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    void update(float deltaTime, bool isHovered);
};

/**
 * @brief Gizmo ayarları — snap, space mode ve operasyon bilgisi
 */
struct GizmoSettings {
    // ImGuizmo::OPERATION bitmask values:
    //   TRANSLATE = 7, ROTATE = 120, SCALE = 896
    int operation{ 7 };         // Varsayılan: TRANSLATE
    int mode{ 0 };              // 0 = LOCAL, 1 = WORLD (ImGuizmo::MODE)
    bool useSnap{ false };
    float translateSnap{ 0.5f };  // Çeviri snap birimi (metre)
    float rotateSnap{ 15.0f };    // Rotasyon snap birimi (derece)
    float scaleSnap{ 0.1f };     // Ölçek snap birimi
};

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

    /**
     * @brief Set editor manager reference for Play/Stop functionality
     */
    void setEditorManager(EditorManager* editorManager) { m_editorManager = editorManager; }

    /**
     * @brief Get Editor Camera reference
     */
    EditorCamera& getEditorCamera() { return m_editorCamera; }

private:
    void drawToolbar();
    void drawGizmos(const glm::mat4& view, const glm::mat4& projection, glm::vec2 viewportPos, glm::vec2 viewportSize);
    void handleKeyboardShortcuts();

private:
    Astral::IRenderer* m_renderer{ nullptr };
    Astral::EntityManager* m_entityManager{ nullptr };
    EditorManager* m_editorManager{ nullptr };
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    SDL_GPUTexture* m_sceneTexture{ nullptr };
    uint32_t m_viewportWidth{ 1280 };
    uint32_t m_viewportHeight{ 720 };
    
    // Gizmo settings (typed struct replaces raw int)
    GizmoSettings m_gizmo;
    
    // Debug info
    float m_fps{ 0.0f };
    int m_drawCallCount{ 0 };

    // Editor Camera
    EditorCamera m_editorCamera;
    bool m_isCameraActive{ false };
};

