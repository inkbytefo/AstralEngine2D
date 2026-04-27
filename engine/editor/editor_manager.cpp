#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS
#include "editor_manager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include "renderer/renderer.h"
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>

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
    ImGuizmo::BeginFrame();
}

void EditorManager::drawEditor(Astral::EntityManager& entityManager, SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer)
{
    // Create a fullscreen dockspace
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
    static bool first_time = true;
    if (first_time)
    {
        first_time = false;
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

    drawHierarchyPanel(entityManager);
    drawInspectorPanel();
    drawViewportPanel(sceneTexture, renderer, entityManager);

    ImGui::End();
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
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorManager::drawHierarchyPanel(Astral::EntityManager& entityManager)
{
    ImGui::Begin("Hierarchy");

    for (auto& entity : entityManager.getEntities())
    {
        ImGuiTreeNodeFlags flags = ((m_selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        
        bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity->id(), flags, "%s", entity->tag().c_str());
        
        if (ImGui::IsItemClicked())
        {
            m_selectedEntity = entity;
        }

        if (opened)
        {
            // TODO: Children
            ImGui::TreePop();
        }
    }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        m_selectedEntity = nullptr;

    ImGui::End();
}

void EditorManager::drawInspectorPanel()
{
    ImGui::Begin("Inspector");

    if (m_selectedEntity)
    {
        ImGui::Text("ID: %u", m_selectedEntity->id());
        ImGui::Text("Tag: %s", m_selectedEntity->tag().c_str());

        if (m_selectedEntity->has<CTransform>())
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& transform = m_selectedEntity->get<CTransform>();
                ImGui::DragFloat3("Position", glm::value_ptr(transform.pos), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(transform.rotation), 1.0f);
                ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
            }
        }
    }
    else
    {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}

void EditorManager::drawViewportPanel(SDL_GPUTexture* sceneTexture, Astral::IRenderer* renderer, Astral::EntityManager& entityManager)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    
    // 1. Resizing check
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    
    // Minimum boyut kontrolü - çok küçük viewport'ları engelle
    if (viewportPanelSize.x < 64.0f) viewportPanelSize.x = 64.0f;
    if (viewportPanelSize.y < 64.0f) viewportPanelSize.y = 64.0f;
    
    if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        uint32_t w = (uint32_t)viewportPanelSize.x;
        uint32_t h = (uint32_t)viewportPanelSize.y;
        
        if (w != m_viewportWidth || h != m_viewportHeight)
        {
             m_viewportWidth = w;
             m_viewportHeight = h;
             renderer->resizeViewport(w, h);
             
             // Update camera projection to match new aspect ratio
             for (auto& entity : entityManager.getEntities())
             {
                 if (entity->has<CCamera>())
                 {
                     auto& cam = entity->get<CCamera>();
                     if (cam.isActive)
                     {
                         cam.projection = glm::perspective(glm::radians(60.0f), (float)w / (float)h, 0.1f, 1000.0f);
                     }
                 }
             }
        }
    }

    // 2. Render Scene Texture to ImGui
    if (sceneTexture && m_viewportWidth >= 64 && m_viewportHeight >= 64)
    {
        ImGui::Image((ImTextureID)(intptr_t)sceneTexture, viewportPanelSize);
    }
    else
    {
        // Texture henüz hazır değilse placeholder göster
        ImGui::Text("Viewport initializing...");
    }

    // 3. ImGuizmo Setup
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    
    float windowWidth = (float)ImGui::GetWindowWidth();
    float windowHeight = (float)ImGui::GetWindowHeight();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

    // 4. Find Active Camera
    glm::mat4 view(1.0f);
    glm::mat4 projection(1.0f);
    bool cameraFound = false;

    for (auto& entity : entityManager.getEntities())
    {
        if (entity->has<CCamera>() && entity->has<CTransform>())
        {
            auto& cam = entity->get<CCamera>();
            if (cam.isActive)
            {
                view = cam.view;
                projection = cam.projection;
                cameraFound = true;
                break;
            }
        }
    }

    // 5. Gizmo Manipulation
    if (cameraFound && m_selectedEntity && m_selectedEntity->has<CTransform>())
    {
        auto& transform = m_selectedEntity->get<CTransform>();
        glm::mat4 globalMatrix = transform.globalMatrix;

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                                 ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE, 
                                 ImGuizmo::LOCAL, glm::value_ptr(globalMatrix)))
        {
            if (ImGuizmo::IsUsing())
            {
                glm::mat4 localMatrix;
                if (transform.parent.expired())
                {
                    localMatrix = globalMatrix;
                }
                else
                {
                    auto parentEntity = transform.parent.lock();
                    glm::mat4 parentGlobalMatrix = parentEntity->get<CTransform>().globalMatrix;
                    localMatrix = glm::inverse(parentGlobalMatrix) * globalMatrix;
                }

                // Decompose matrix to extract transform components
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);
                
                transform.pos = translation;
                transform.scale = scale;
                transform.rotation = glm::degrees(glm::eulerAngles(rotation));
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
