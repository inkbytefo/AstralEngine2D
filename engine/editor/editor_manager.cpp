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
#include "core/scene_serializer.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include "renderer/renderer.h"
#include <glm/gtc/type_ptr.hpp>

void EditorManager::init(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat)
{
    m_window = window;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    // Multi-Viewport SDL_GPU ile uyumsuz olduğu için devre dışı
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    // Layered Depth Colors
    ImVec4 baseBg       = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    ImVec4 childBg      = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    ImVec4 popupBg      = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    ImVec4 accentColor  = ImVec4(0.29f, 0.62f, 1.00f, 1.00f); // #4A9EFF

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = baseBg;
    colors[ImGuiCol_ChildBg]                = childBg;
    colors[ImGuiCol_PopupBg]                = popupBg;
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.22f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]                = baseBg;
    colors[ImGuiCol_TitleBgActive]          = baseBg;
    colors[ImGuiCol_TitleBgCollapsed]       = baseBg;
    colors[ImGuiCol_MenuBarBg]              = baseBg;
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 0.80f);
    colors[ImGuiCol_CheckMark]              = accentColor;
    colors[ImGuiCol_SliderGrab]             = accentColor;
    colors[ImGuiCol_SliderGrabActive]       = accentColor;
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.29f, 0.62f, 1.00f, 0.40f);
    colors[ImGuiCol_ButtonActive]           = accentColor;
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.29f, 0.62f, 1.00f, 0.20f);
    colors[ImGuiCol_HeaderActive]           = accentColor;
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = accentColor;
    colors[ImGuiCol_SeparatorActive]        = accentColor;
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered]      = accentColor;
    colors[ImGuiCol_ResizeGripActive]       = accentColor;
    colors[ImGuiCol_Tab]                    = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered]             = accentColor;
    colors[ImGuiCol_TabActive]              = accentColor;
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = accentColor * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = accentColor * ImVec4(1.0f, 1.0f, 1.0f, 0.35f);
    colors[ImGuiCol_NavHighlight]           = accentColor;
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);
    
    // ImGui_ImplSDLGPU3_Init için InitInfo yapısı oluştur
    ImGui_ImplSDLGPU3_InitInfo initInfo;
    initInfo.Device = device;
    initInfo.ColorTargetFormat = swapchainFormat;
    initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    initInfo.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    initInfo.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
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

void EditorManager::setupDockspace(Astral::EntityManager& entityManager)
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
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);
        ImGui::DockBuilderDockWindow("Console", dock_bottom);
        ImGui::DockBuilderDockWindow("Content Browser", dock_bottom);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) { 
                entityManager.clear(); 
            }
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                ::SceneSerializer::deserialize("assets/scene.astral", entityManager);
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                ::SceneSerializer::serialize("assets/scene.astral", entityManager);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) { SDL_Event event; event.type = SDL_EVENT_QUIT; SDL_PushEvent(&event); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset Layout")) {
                m_dockspaceInitialized = false;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorManager::drawEditor(Astral::EntityManager& entityManager, SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer)
{
    setupDockspace(entityManager);

    // Setup hierarchy panel callback
    if (m_panels.find("Hierarchy") != m_panels.end())
    {
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

bool EditorManager::getEditorCameraMatrices(glm::mat4& view, glm::mat4& proj, glm::vec3& pos)
{
    if (m_panels.find("Viewport") != m_panels.end())
    {
        auto* viewportPanel = dynamic_cast<ViewportPanel*>(m_panels["Viewport"].get());
        if (viewportPanel)
        {
            auto& camera = viewportPanel->getEditorCamera();
            view = camera.getViewMatrix();
            
            float aspect = (float)viewportPanel->getViewportWidth() / (float)viewportPanel->getViewportHeight();
            proj = camera.getProjectionMatrix(aspect);
            pos = camera.position;
            return true;
        }
    }
    return false;
}
