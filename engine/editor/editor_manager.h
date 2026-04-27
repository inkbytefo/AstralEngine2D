#pragma once

#include <SDL3/SDL.h>
#include <memory>

namespace Astral {
    class EntityManager;
    class Entity;
    class IRenderer;
}

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

private:
    /**
     * @brief Draws the Scene Hierarchy panel.
     */
    void drawHierarchyPanel(Astral::EntityManager& entityManager);

    /**
     * @brief Draws the Inspector panel for the selected entity.
     */
    void drawInspectorPanel();

    /**
     * @brief Draws the Viewport panel displaying the offscreen scene texture.
     */
    void drawViewportPanel(SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer, Astral::EntityManager& entityManager);

private:
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    uint32_t m_viewportWidth{ 1280 };  // Başlangıç boyutu 1280x720
    uint32_t m_viewportHeight{ 720 };
};
