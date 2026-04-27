#include "editor_manager.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"

void EditorManager::init(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat swapchainFormat)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);
    
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = device;
    init_info.ColorTargetFormat = swapchainFormat;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    
    ImGui_ImplSDLGPU3_Init(&init_info);
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

void EditorManager::drawEditor(Astral::EntityManager& entityManager)
{
    // Create a fullscreen dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    static bool p_open = true;
    ImGui::Begin("DockSpace Demo", &p_open, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) { /* Handle exit */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    drawHierarchyPanel(entityManager);
    drawInspectorPanel();

    ImGui::End();
}

void EditorManager::prepare(SDL_GPUCommandBuffer* cmd)
{
    ImGui::Render();
    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), cmd);
}

void EditorManager::render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* renderPass)
{
    if (renderPass)
    {
        ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), cmd, renderPass);
    }
}

void EditorManager::shutdown()
{
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorManager::drawHierarchyPanel(Astral::EntityManager& entityManager)
{
    ImGui::Begin("Hierarchy");

    for (auto& entity : entityManager.getEntities())
    {
        if (!entity->isActive()) continue;

        std::string label = entity->tag() + " (" + std::to_string(entity->id()) + ")";
        
        bool isSelected = (m_selectedEntity == entity);
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            m_selectedEntity = entity;
        }

        if (isSelected)
        {
            ImGui::SetItemDefaultFocus();
        }
    }

    ImGui::End();
}

void EditorManager::drawInspectorPanel()
{
    ImGui::Begin("Inspector");

    if (m_selectedEntity && m_selectedEntity->isActive())
    {
        ImGui::Text("ID: %u", m_selectedEntity->id());
        ImGui::Text("Tag: %s", m_selectedEntity->tag().c_str());
        
        ImGui::Separator();

        if (m_selectedEntity->has<CTransform>())
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& transform = m_selectedEntity->get<CTransform>();
                
                ImGui::DragFloat3("Position", &transform.pos[0], 0.1f);
                
                // Rotation handling (degrees to euler is direct in CTransform as per comments)
                ImGui::DragFloat3("Rotation", &transform.rotation[0], 1.0f);
                
                ImGui::DragFloat3("Scale", &transform.scale[0], 0.1f);
            }
        }
    }
    else
    {
        ImGui::Text("No entity selected");
        m_selectedEntity = nullptr;
    }

    ImGui::End();
}
