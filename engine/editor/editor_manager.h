#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <glm/glm.hpp>

namespace Astral {
    class EntityManager;
    class Entity;
    class IRenderer;
}

class IEditorPanel;
class ViewportPanel;

/**
 * @brief Scene simulation state
 */
enum class SceneState {
    Edit,   // Normal editör modu - sadece render ve transform sistemleri çalışır
    Play,   // Simülasyon çalışıyor - tüm sistemler çalışır
    Pause   // Simülasyon duraklatılmış - sistemler çalışmaz ama state korunur
};

class EditorManager {
public:
    EditorManager() = default;
    ~EditorManager() = default;

    /**
     * @brief Initializes ImGui context, SDL3, and SDL_GPU backends.
     * Sets up docking, dark theme, and keyboard navigation.
     */
    void init(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat);

    /**
     * @brief Forwards SDL events to ImGui.
     */
    void processEvent(const SDL_Event* event);

    /**
     * @brief Starts a new ImGui frame.
     */
    void newFrame();

    /**
     * @brief Main entry point for drawing the Editor UI (Dockspace, Panels, etc.).
     */
    void drawEditor(Astral::EntityManager& entityManager, SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer);
    
    /**
     * @brief Prepares the ImGui draw data (uploads to GPU). Must be called BEFORE beginRenderPass.
     */
    void prepare(SDL_GPUCommandBuffer* cmd);

    /**
     * @brief Renders the ImGui draw data. Must be called INSIDE beginRenderPass.
     */
    void render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* renderPass);

    /**
     * @brief Cleans up ImGui resources.
     */
    void shutdown();

    /**
     * @brief Register a panel with the editor.
     */
    void registerPanel(std::unique_ptr<IEditorPanel> panel);

    /**
     * @brief Select an entity and notify all panels.
     */
    void selectEntity(std::shared_ptr<Astral::Entity> entity);

    /**
     * @brief Deselect the current entity and notify all panels.
     */
    void deselectEntity();

    /**
     * @brief Get the currently selected entity.
     */
    std::shared_ptr<Astral::Entity> getSelectedEntity() const { return m_selectedEntity; }

    /**
     * @brief Set debug info (FPS, draw calls) for viewport display.
     */
    void setDebugInfo(float fps, int drawCalls);

    /**
     * @brief Get current scene state (Edit, Play, Pause)
     */
    SceneState getSceneState() const { return m_sceneState; }

    /**
     * @brief Set scene state
     */
    void setSceneState(SceneState state) { m_sceneState = state; }

    /**
     * @brief Get scene snapshot (for Play/Stop functionality)
     */
    const std::string& getSceneSnapshot() const { return m_sceneSnapshot; }

    /**
     * @brief Set scene snapshot
     */
    void setSceneSnapshot(const std::string& snapshot) { m_sceneSnapshot = snapshot; }

    /**
     * @brief Get Editor Camera matrices if possible
     */
    bool getEditorCameraMatrices(glm::mat4& view, glm::mat4& proj, glm::vec3& pos);

    /**
     * @brief Clear all selection state.
     */
    void clearSelection() { deselectEntity(); }

    /**
     * @brief Get the SDL window associated with the editor.
     */
    SDL_Window* getWindow() const { return m_window; }

private:
    void setupDockspace(Astral::EntityManager& entityManager);

private:
    std::unordered_map<std::string, std::unique_ptr<IEditorPanel>> m_panels;
    SDL_Window* m_window{ nullptr };
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    bool m_dockspaceInitialized{ false };
    SceneState m_sceneState{ SceneState::Edit };
    std::string m_sceneSnapshot;  // JSON snapshot for Play/Stop
};
