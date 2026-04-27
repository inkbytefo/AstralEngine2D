#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport_panel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include "renderer/renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL3/SDL.h>

ViewportPanel::ViewportPanel(Astral::IRenderer* renderer, Astral::EntityManager* entityManager)
    : m_renderer(renderer), m_entityManager(entityManager)
{
}

void ViewportPanel::drawGizmoModeButtons()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Gizmo Mode:");
    ImGui::SameLine();
    
    // Translate button
    if (ImGui::Button("T: Translate", ImVec2(100, 0)))
    {
        m_gizmoMode = (int)ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    
    // Rotate button
    if (ImGui::Button("R: Rotate", ImVec2(100, 0)))
    {
        m_gizmoMode = (int)ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    
    // Scale button
    if (ImGui::Button("S: Scale", ImVec2(100, 0)))
    {
        m_gizmoMode = (int)ImGuizmo::SCALE;
    }
    
    ImGui::Separator();
}

void ViewportPanel::drawViewportInfo()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Viewport Info:");
    
    // Viewport dimensions
    ImGui::Text("Resolution: %ux%u", m_viewportWidth, m_viewportHeight);
    
    // Aspect ratio
    float aspectRatio = m_viewportHeight > 0 ? (float)m_viewportWidth / (float)m_viewportHeight : 0.0f;
    ImGui::Text("Aspect Ratio: %.2f:1", aspectRatio);
    
    // FPS and draw calls
    ImGui::Text("FPS: %.1f", m_fps);
    ImGui::Text("Draw Calls: %d", m_drawCallCount);
    
    ImGui::Separator();
}

void ViewportPanel::handleKeyboardShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // T = Translate
    if (ImGui::IsKeyPressed(ImGuiKey_T, false) && !io.WantTextInput)
    {
        m_gizmoMode = (int)ImGuizmo::TRANSLATE;
    }
    
    // R = Rotate
    if (ImGui::IsKeyPressed(ImGuiKey_R, false) && !io.WantTextInput)
    {
        m_gizmoMode = (int)ImGuizmo::ROTATE;
    }
    
    // S = Scale
    if (ImGui::IsKeyPressed(ImGuiKey_S, false) && !io.WantTextInput)
    {
        m_gizmoMode = (int)ImGuizmo::SCALE;
    }
}

void ViewportPanel::draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    
    // Draw gizmo mode buttons at the top
    ImGui::PopStyleVar();
    drawGizmoModeButtons();
    drawViewportInfo();
    handleKeyboardShortcuts();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
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
            m_renderer->resizeViewport(w, h);
            
            // Update camera projection to match new aspect ratio
            if (m_entityManager)
            {
                for (auto& entity : m_entityManager->getEntities())
                {
                    if (entity->has<CCamera>())
                    {
                        auto& cam = entity->get<CCamera>();
                        if (cam.isActive)
                        {
                            cam.aspectRatio = (float)w / (float)h;
                            cam.projection = glm::perspective(glm::radians(60.0f), cam.aspectRatio, 0.1f, 1000.0f);
                        }
                    }
                }
            }
        }
    }

    // 2. Render Scene Texture to ImGui
    if (m_sceneTexture && m_viewportWidth >= 64 && m_viewportHeight >= 64 && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        ImGui::Image((ImTextureID)(intptr_t)m_sceneTexture, viewportPanelSize);
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

    if (m_entityManager)
    {
        for (auto& entity : m_entityManager->getEntities())
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
    }

    // 5. Gizmo Manipulation
    if (cameraFound && m_selectedEntity && m_selectedEntity->has<CTransform>())
    {
        auto& transform = m_selectedEntity->get<CTransform>();
        glm::mat4 globalMatrix = transform.globalMatrix;

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
                                 (ImGuizmo::OPERATION)m_gizmoMode, 
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
