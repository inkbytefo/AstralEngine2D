#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace Astral {
    class EntityManager;
    class Entity;
    class IRenderer;
}

class IEditorPanel;
class ViewportPanel;

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

private:
    void setupDockspace();

private:
    std::unordered_map<std::string, std::unique_ptr<IEditorPanel>> m_panels;
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    bool m_dockspaceInitialized{ false };
};
