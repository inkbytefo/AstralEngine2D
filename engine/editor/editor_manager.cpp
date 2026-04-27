#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS
#include "editor_manager.h"
#include "panels/editor_panel.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/properties_panel.h"
#include "panels/viewport_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include "renderer/renderer.h"
#include <glm/gtc/type_ptr.hpp>

void EditorManager::init(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    // Multi-Viewport SDL_GPU ile uyumsuz olduğu için devre dışı
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);
    
    // ImGui_ImplSDLGPU3_Init için InitInfo yapısı oluştur
    ImGui_ImplSDLGPU3_InitInfo initInfo = {};
    initInfo.Device = device;
    initInfo.ColorTargetFormat = swapchainFormat;
    initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&initInfo);
}

void EditorManager::processEvent(const SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void EditorManager::newFrame()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorManager::registerPanel(std::unique_ptr<IEditorPanel> panel)
{
    if (panel)
    {
        m_panels[panel->getName()] = std::move(panel);
    }
}

void EditorManager::selectEntity(std::shared_ptr<Astral::Entity> entity)
{
    m_selectedEntity = entity;
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        if (it->second)
        {
            it->second->onEntitySelected(entity);
        }
    }
}

void EditorManager::deselectEntity()
{
    m_selectedEntity = nullptr;
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        if (it->second)
        {
            it->second->onEntityDeselected();
        }
    }
}

void EditorManager::setupDockspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("Astral Editor", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("AstralDockSpace");
    
    // İlk açılışta default layout'u kur
    if (!m_dockspaceInitialized)
    {
        m_dockspaceInitialized = true;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        // Layout: Sol panel (Hierarchy), Merkez (Viewport), Sağ panel (Inspector)
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) { /* TODO */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorManager::drawEditor(Astral::EntityManager& entityManager, SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer)
{
    setupDockspace();

    // Setup hierarchy panel callback if not already done
    static bool callbackSetup = false;
    if (!callbackSetup && m_panels.find("Hierarchy") != m_panels.end())
    {
        callbackSetup = true;
        auto* hierarchyPanel = dynamic_cast<SceneHierarchyPanel*>(m_panels["Hierarchy"].get());
        if (hierarchyPanel)
        {
            hierarchyPanel->setOnSelectionChanged([this](std::shared_ptr<Astral::Entity> entity) {
                this->selectEntity(entity);
            });
        }
    }

    // Set scene texture and debug info for viewport panel
    if (m_panels.find("Viewport") != m_panels.end())
    {
        auto* viewportPanel = dynamic_cast<ViewportPanel*>(m_panels["Viewport"].get());
        if (viewportPanel)
        {
            viewportPanel->setSceneTexture(sceneTexture);
            
            // Get draw call count from renderer
            int drawCalls = 0;
            if (renderer)
            {
                // Note: We'll need to add a method to get draw call count from renderer
                // For now, we'll pass 0
                drawCalls = 0;
            }
            
            // Debug info will be set from App
        }
    }

    // Draw all registered panels
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        if (it->second)
        {
            it->second->draw();
        }
    }
}

void EditorManager::prepare(SDL_GPUCommandBuffer* cmd)
{
    ImGui::Render();
    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), cmd);
}

void EditorManager::render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* renderPass)
{
    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), cmd, renderPass);
}

void EditorManager::shutdown()
{
    for (auto it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        it->second.reset();
    }
    m_panels.clear();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorManager::setDebugInfo(float fps, int drawCalls)
{
    if (m_panels.find("Viewport") != m_panels.end())
    {
        auto* viewportPanel = dynamic_cast<ViewportPanel*>(m_panels["Viewport"].get());
        if (viewportPanel)
        {
            viewportPanel->setDebugInfo(fps, drawCalls);
        }
    }
}
