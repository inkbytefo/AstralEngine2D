#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include "renderer/renderer.h"
#include "editor/editor_manager.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "systems/render_system.h"
#include <SDL3/SDL.h>

// --- EditorCamera Implementation ---

glm::mat4 EditorCamera::getViewMatrix() const {
    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    front.z = sin(glm::radians(pitch));
    front = glm::normalize(front);
    
    return glm::lookAt(position, position + front, glm::vec3(0, 0, 1));
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void EditorCamera::update(float deltaTime, bool isActive) {
    if (!isActive) return;

    ImGuiIO& io = ImGui::GetIO();
    
    // Rotation (Mouse Delta)
    yaw += io.MouseDelta.x * mouseSensitivity;
    pitch -= io.MouseDelta.y * mouseSensitivity;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    // WASD Movement
    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    front.z = sin(glm::radians(pitch));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 0, 1)));
    
    float speed = movementSpeed * deltaTime;
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) speed *= 3.0f; // Turbo

    if (ImGui::IsKeyDown(ImGuiKey_W)) position += front * speed;
    if (ImGui::IsKeyDown(ImGuiKey_S)) position -= front * speed;
    if (ImGui::IsKeyDown(ImGuiKey_A)) position -= right * speed;
    if (ImGui::IsKeyDown(ImGuiKey_D)) position += right * speed;
    if (ImGui::IsKeyDown(ImGuiKey_E)) position.z += speed;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) position.z -= speed;
}

ViewportPanel::ViewportPanel(Astral::IRenderer* renderer, Astral::EntityManager* entityManager)
    : m_renderer(renderer), m_entityManager(entityManager), m_gizmoMode(7) // Default to TRANSLATE
{
}

void ViewportPanel::handleKeyboardShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_T, false) && !io.WantTextInput) m_gizmoMode = 7;
    if (ImGui::IsKeyPressed(ImGuiKey_R, false) && !io.WantTextInput) m_gizmoMode = 120;
    if (ImGui::IsKeyPressed(ImGuiKey_S, false) && !io.WantTextInput) m_gizmoMode = 896;
}

void ViewportPanel::draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    // 1. Toolbar
    if (ImGui::BeginChild("ViewportToolbar", ImVec2(0, 32), false, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::SetCursorPosY(4);
        ImGui::Indent(8);
        
        auto drawModeButton = [&](const char* label, int mode, bool active) {
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            if (ImGui::Button(label, ImVec2(32, 24))) m_gizmoMode = mode;
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine();
        };

        drawModeButton("T", 7, (m_gizmoMode & 7) != 0);
        drawModeButton("R", 120, (m_gizmoMode & 120) != 0);
        drawModeButton("S", 896, (m_gizmoMode & 896) != 0);

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        SceneState currentState = m_editorManager->getSceneState();
        if (currentState == SceneState::Edit)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("Play", ImVec2(60, 24))) m_editorManager->setSceneState(SceneState::Play);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(60, 24))) m_editorManager->setSceneState(SceneState::Edit);
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            if (currentState == SceneState::Play) {
                if (ImGui::Button("Pause", ImVec2(60, 24))) m_editorManager->setSceneState(SceneState::Pause);
            } else {
                if (ImGui::Button("Resume", ImVec2(60, 24))) m_editorManager->setSceneState(SceneState::Play);
            }
        }
    }
    ImGui::EndChild();

    handleKeyboardShortcuts();
    
    float aspectRatio = (m_viewportHeight > 0) ? (float)m_viewportWidth / (float)m_viewportHeight : 1.0f;
    glm::mat4 view = m_editorCamera.getViewMatrix();
    glm::mat4 projection = m_editorCamera.getProjectionMatrix(aspectRatio);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
    {
        m_isCameraActive = true;
        if (m_editorManager && m_editorManager->getWindow()) SDL_SetWindowRelativeMouseMode(m_editorManager->getWindow(), true);
    }

    if (m_isCameraActive && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        m_isCameraActive = false;
        if (m_editorManager && m_editorManager->getWindow()) SDL_SetWindowRelativeMouseMode(m_editorManager->getWindow(), false);
    }

    m_editorCamera.update(ImGui::GetIO().DeltaTime, m_isCameraActive);

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (viewportPanelSize.x < 64.0f) viewportPanelSize.x = 64.0f;
    if (viewportPanelSize.y < 64.0f) viewportPanelSize.y = 64.0f;

    if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        uint32_t w = (uint32_t)viewportPanelSize.x;
        uint32_t h = (uint32_t)viewportPanelSize.y;
        if (w != m_viewportWidth || h != m_viewportHeight)
        {
            m_viewportWidth = w; m_viewportHeight = h;
            m_renderer->resizeViewport(w, h);
            m_sceneTexture = m_renderer->getSceneTexture();
        }
    }

    ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    if (m_sceneTexture)
    {
        ImGui::Image((ImTextureID)(intptr_t)m_sceneTexture, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));
        
        // Selection Outline
        if (m_selectedEntity)
        {
            ImGui::GetWindowDrawList()->AddRect(viewportPos, ImVec2(viewportPos.x + viewportPanelSize.x, viewportPos.y + viewportPanelSize.y), IM_COL32(255, 165, 0, 150), 0.0f, 0, 2.0f);
        }

        // Stats Overlay
        ImGui::SetCursorScreenPos(viewportPos + ImVec2(10, 10));
        ImGui::BeginChild("StatsOverlay", ImVec2(180, 60), false, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "FPS: %.1f", m_fps);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Draw Calls: %d", m_drawCallCount);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "Res: %ux%u", m_viewportWidth, m_viewportHeight);
        ImGui::EndChild();
    }

    // Gizmos
    ImGuizmo::SetOrthographic(false);
    bool pushedFont = false;
    if (ImGui::GetIO().Fonts->IsBuilt()) {
        ImGui::GetWindowDrawList()->PushTextureID(ImGui::GetIO().Fonts->TexID);
        pushedFont = true;
    }

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportPanelSize.x, viewportPanelSize.y);

    if (m_selectedEntity && m_selectedEntity->has<CTransform>())
    {
        auto& transform = m_selectedEntity->get<CTransform>();
        glm::mat4 globalMatrix = transform.globalMatrix;

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), (ImGuizmo::OPERATION)m_gizmoMode, ImGuizmo::LOCAL, glm::value_ptr(globalMatrix)))
        {
            if (ImGuizmo::IsUsing())
            {
                glm::mat4 localMatrix = globalMatrix;
                if (!transform.parent.expired())
                {
                    auto parentEntity = transform.parent.lock();
                    localMatrix = glm::inverse(parentEntity->get<CTransform>().globalMatrix) * globalMatrix;
                }
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat rotation;
                glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);
                transform.pos = translation; transform.scale = scale; transform.rotation = glm::degrees(glm::eulerAngles(rotation));
            }
        }
    }

    if (pushedFont) ImGui::GetWindowDrawList()->PopTextureID();
    ImGui::End();
}
