#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS
#include "viewport_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "core/entity_manager.h"
#include "core/scene_serializer.h"
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
    : m_renderer(renderer), m_entityManager(entityManager)
{
}

void ViewportPanel::handleKeyboardShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return; // Text input aktifken shortcut'ları devre dışı bırak
    
    // Gizmo mode shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_T, false)) m_gizmo.operation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_gizmo.operation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) m_gizmo.operation = ImGuizmo::SCALE;
    
    // Snap toggle
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) m_gizmo.useSnap = !m_gizmo.useSnap;
    
    // Local/World space toggle
    if (ImGui::IsKeyPressed(ImGuiKey_G, false)) m_gizmo.mode = (m_gizmo.mode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
}

void ViewportPanel::drawToolbar()
{
    if (!ImGui::BeginChild("ViewportToolbar", ImVec2(0, 36), false, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::EndChild();
        return;
    }

    ImGui::SetCursorPosY(4);
    ImGui::Indent(8);

    // --- Gizmo Mode Buttons ---
    auto drawModeButton = [&](const char* label, const char* tooltip, int mode, bool active) {
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        if (ImGui::Button(label, ImVec2(32, 26)))
            m_gizmo.operation = mode;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        if (active)
            ImGui::PopStyleColor();
        ImGui::SameLine();
    };

    drawModeButton("T", "Translate (T)", ImGuizmo::TRANSLATE, m_gizmo.operation == ImGuizmo::TRANSLATE);
    drawModeButton("R", "Rotate (R)",    ImGuizmo::ROTATE,    m_gizmo.operation == ImGuizmo::ROTATE);
    drawModeButton("S", "Scale (Y)",     ImGuizmo::SCALE,     m_gizmo.operation == ImGuizmo::SCALE);

    // --- Local/World Toggle ---
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    {
        bool isLocal = (m_gizmo.mode == ImGuizmo::LOCAL);
        const char* spaceLabel = isLocal ? "Local" : "World";
        if (ImGui::Button(spaceLabel, ImVec2(48, 26)))
            m_gizmo.mode = isLocal ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World Space (G)");
    }

    // --- Snap Toggle ---
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &m_gizmo.useSnap);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Snap (X)");

    if (m_gizmo.useSnap)
    {
        ImGui::SameLine();
        ImGui::PushItemWidth(50);
        if (m_gizmo.operation == ImGuizmo::TRANSLATE)
            ImGui::DragFloat("##SnapVal", &m_gizmo.translateSnap, 0.1f, 0.1f, 100.0f, "%.1f");
        else if (m_gizmo.operation == ImGuizmo::ROTATE)
            ImGui::DragFloat("##SnapVal", &m_gizmo.rotateSnap, 1.0f, 1.0f, 180.0f, "%.0f");
        else if (m_gizmo.operation == ImGuizmo::SCALE)
            ImGui::DragFloat("##SnapVal", &m_gizmo.scaleSnap, 0.01f, 0.01f, 10.0f, "%.2f");
        ImGui::PopItemWidth();
    }

    // --- Play/Pause/Stop Controls ---
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (m_editorManager)
    {
        SceneState currentState = m_editorManager->getSceneState();

        if (currentState == SceneState::Edit)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            if (ImGui::Button("Play", ImVec2(60, 26)))
            {
                // Sahne snapshot'ını kaydet — Stop'ta geri yüklenir
                if (m_entityManager)
                {
                    std::string snapshot = SceneSerializer::createSnapshot(*m_entityManager);
                    m_editorManager->setSceneSnapshot(snapshot);
                }
                m_editorManager->setSceneState(SceneState::Play);
            }
            ImGui::PopStyleColor(2);
        }
        else
        {
            // Stop Button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(60, 26)))
            {
                // Snapshot'tan sahneyi geri yükle
                if (m_entityManager)
                {
                    const std::string& snapshot = m_editorManager->getSceneSnapshot();
                    if (!snapshot.empty())
                    {
                        SceneSerializer::restoreFromSnapshot(snapshot, *m_entityManager);
                    }
                }
                m_editorManager->setSceneState(SceneState::Edit);
                m_editorManager->clearSelection();
            }
            ImGui::PopStyleColor(2);

            // Pause/Resume Button
            ImGui::SameLine();
            if (currentState == SceneState::Play)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
                if (ImGui::Button("Pause", ImVec2(60, 26)))
                    m_editorManager->setSceneState(SceneState::Pause);
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                if (ImGui::Button("Resume", ImVec2(60, 26)))
                    m_editorManager->setSceneState(SceneState::Play);
                ImGui::PopStyleColor();
            }
        }
    }

    ImGui::EndChild();
}

void ViewportPanel::drawGizmos(const glm::mat4& view, const glm::mat4& projection, glm::vec2 viewportPos, glm::vec2 viewportSize)
{
    if (!m_selectedEntity || !m_selectedEntity->has<CTransform>()) return;

    ImGuizmo::SetID(0);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
    ImGuizmo::AllowAxisFlip(false); // Axis flip can be confusing in Z-up systems

    auto& transform = m_selectedEntity->get<CTransform>();
    glm::mat4 globalMatrix = transform.globalMatrix;

    // Snap değerlerini hazırla
    float snapValues[3] = { 0.0f, 0.0f, 0.0f };
    float* snapPtr = nullptr;

    if (m_gizmo.useSnap)
    {
        float snapVal = m_gizmo.translateSnap;
        if (m_gizmo.operation == ImGuizmo::ROTATE)
            snapVal = m_gizmo.rotateSnap;
        else if (m_gizmo.operation == ImGuizmo::SCALE)
            snapVal = m_gizmo.scaleSnap;

        snapValues[0] = snapVal;
        snapValues[1] = snapVal;
        snapValues[2] = snapVal;
        snapPtr = snapValues;
    }

    // Gizmo çizimi ve manipülasyonu
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            (ImGuizmo::OPERATION)m_gizmo.operation,
            (ImGuizmo::MODE)m_gizmo.mode,
            glm::value_ptr(globalMatrix),
            nullptr,     // deltaMatrix
            snapPtr))    // snap values
    {
        if (ImGuizmo::IsUsing())
        {
            // Global → Local dönüşümü (parent varsa)
            glm::mat4 localMatrix = globalMatrix;
            if (!transform.parent.expired())
            {
                auto parentEntity = transform.parent.lock();
                if (parentEntity && parentEntity->has<CTransform>())
                {
                    localMatrix = glm::inverse(parentEntity->get<CTransform>().globalMatrix) * globalMatrix;
                }
            }

            // ImGuizmo'nun kendi decomposition fonksiyonunu kullanmak daha güvenlidir
            float t[3], r[3], s[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMatrix), t, r, s);

            // Sadece ilgili operasyonun değerlerini güncelle
            if (m_gizmo.operation == ImGuizmo::TRANSLATE)
            {
                transform.pos = glm::vec3(t[0], t[1], t[2]);
            }
            else if (m_gizmo.operation == ImGuizmo::ROTATE)
            {
                transform.rotation = glm::vec3(r[0], r[1], r[2]);
            }
            else if (m_gizmo.operation == ImGuizmo::SCALE)
            {
                transform.scale = glm::vec3(s[0], s[1], s[2]);
            }
        }
    }
}

void ViewportPanel::draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    // 1. Toolbar (Play/Stop, Gizmo modes, Snap)
    drawToolbar();
    handleKeyboardShortcuts();
    
    // 2. Editor Camera Update
    float aspectRatio = (m_viewportHeight > 0) ? (float)m_viewportWidth / (float)m_viewportHeight : 1.0f;
    glm::mat4 view = m_editorCamera.getViewMatrix();
    glm::mat4 projection = m_editorCamera.getProjectionMatrix(aspectRatio);

    // Camera activation via right-click
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

    // 3. Viewport Sizing
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

    // 4. Scene Texture Rendering
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    if (m_sceneTexture)
    {
        ImGui::Image((ImTextureID)(intptr_t)m_sceneTexture, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        // Stats Overlay (semi-transparent, viewport üzerine)
        ImGui::SetCursorScreenPos(viewportPos + ImVec2(10, 10));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.4f));
        ImGui::BeginChild("StatsOverlay", ImVec2(190, 64), false, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "  FPS: %.1f", m_fps);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "  Draw Calls: %d", m_drawCallCount);
        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "  Res: %ux%u", m_viewportWidth, m_viewportHeight);
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // 5. Gizmo Rendering (Edit modunda, kamera aktif değilken)
    if (m_editorManager && m_editorManager->getSceneState() == SceneState::Edit && !m_isCameraActive)
    {
        drawGizmos(view, projection, glm::vec2(viewportPos.x, viewportPos.y), glm::vec2(viewportPanelSize.x, viewportPanelSize.y));
    }

    ImGui::End();
}
