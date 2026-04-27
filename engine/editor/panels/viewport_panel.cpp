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
    : m_renderer(renderer), m_entityManager(entityManager), m_gizmoMode(7) // Default to TRANSLATE (7 = X|Y|Z)
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

void ViewportPanel::drawPlayControls()
{
    if (!m_editorManager || !m_entityManager) return;
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Simulation:");
    ImGui::SameLine();
    
    SceneState currentState = m_editorManager->getSceneState();
    
    // Play button
    if (currentState == SceneState::Edit)
    {
        if (ImGui::Button("Play", ImVec2(60, 0)))
        {
            // Snapshot al
            std::string snapshot = SceneSerializer::createSnapshot(*m_entityManager);
            m_editorManager->setSceneSnapshot(snapshot);
            m_editorManager->setSceneState(SceneState::Play);
            SDL_Log("Simulation started - Play mode");
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button("Play", ImVec2(60, 0));
        ImGui::EndDisabled();
    }
    
    ImGui::SameLine();
    
    // Pause button
    if (currentState == SceneState::Play)
    {
        if (ImGui::Button("Pause", ImVec2(60, 0)))
        {
            m_editorManager->setSceneState(SceneState::Pause);
            SDL_Log("Simulation paused");
        }
    }
    else if (currentState == SceneState::Pause)
    {
        if (ImGui::Button("Resume", ImVec2(60, 0)))
        {
            m_editorManager->setSceneState(SceneState::Play);
            SDL_Log("Simulation resumed");
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button("Pause", ImVec2(60, 0));
        ImGui::EndDisabled();
    }
    
    ImGui::SameLine();
    
    // Stop button
    if (currentState != SceneState::Edit)
    {
        if (ImGui::Button("Stop", ImVec2(60, 0)))
        {
            // Snapshot'tan restore et
            const std::string& snapshot = m_editorManager->getSceneSnapshot();
            if (!snapshot.empty())
            {
                SceneSerializer::restoreFromSnapshot(snapshot, *m_entityManager);
                SDL_Log("Simulation stopped - Scene restored");
            }
            m_editorManager->setSceneState(SceneState::Edit);
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button("Stop", ImVec2(60, 0));
        ImGui::EndDisabled();
    }
    
    // Display current state
    ImGui::SameLine();
    ImGui::Text("| State: %s", 
        currentState == SceneState::Edit ? "Edit" :
        currentState == SceneState::Play ? "Play" : "Pause");
    
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
    
    // Draw toolbar at the top of the viewport
    ImGui::PopStyleVar();
    
    if (ImGui::BeginChild("ViewportToolbar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::SetCursorPosY(5);
        ImGui::Indent(5);
        
        // Gizmo Modes
        bool isTranslate = (m_gizmoMode & 7) != 0 && (m_gizmoMode & 112) == 0;
        bool isRotate = (m_gizmoMode & 112) != 0;
        bool isScale = (m_gizmoMode & 896) != 0;

        if (ImGui::RadioButton("Translate", isTranslate)) m_gizmoMode = 7; // TRANSLATE
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", isRotate)) m_gizmoMode = 120; // ROTATE
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", isScale)) m_gizmoMode = 896; // SCALE
        
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        
        // Play Controls
        SceneState currentState = m_editorManager->getSceneState();
        if (currentState == SceneState::Edit)
        {
            if (ImGui::Button("Play")) {
                m_editorManager->deselectEntity();
                std::string snapshot = SceneSerializer::createSnapshot(*m_entityManager);
                m_editorManager->setSceneSnapshot(snapshot);
                m_editorManager->setSceneState(SceneState::Play);
            }
        }
        else
        {
            if (ImGui::Button("Stop")) {
                m_editorManager->deselectEntity();
                const std::string& snapshot = m_editorManager->getSceneSnapshot();
                if (!snapshot.empty()) SceneSerializer::restoreFromSnapshot(snapshot, *m_entityManager);
                m_editorManager->setSceneState(SceneState::Edit);
            }
            ImGui::SameLine();
            if (currentState == SceneState::Play) {
                if (ImGui::Button("Pause")) m_editorManager->setSceneState(SceneState::Pause);
            } else {
                if (ImGui::Button("Resume")) m_editorManager->setSceneState(SceneState::Play);
            }
        }
        
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        
        // Stats
        ImGui::TextDisabled("FPS: %.1f | Draw Calls: %d", m_fps, m_drawCallCount);
    }
    ImGui::EndChild();

    handleKeyboardShortcuts();
    
    // Update Editor Camera State Machine
    bool isViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    
    // --- GIZMO SYNC FIX ---
    // We capture the matrices BEFORE updating the camera for the next frame.
    // This ensures ImGuizmo uses the same matrices that were used to render the scene texture.
    float aspectRatio = (m_viewportHeight > 0) ? (float)m_viewportWidth / (float)m_viewportHeight : 1.0f;
    glm::mat4 view = m_editorCamera.getViewMatrix();
    glm::mat4 projection = m_editorCamera.getProjectionMatrix(aspectRatio);

    // Start camera rotation if right-clicked while hovered and NOT using gizmos
    if (isViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
    {
        m_isCameraActive = true;
        if (m_editorManager && m_editorManager->getWindow()) {
            SDL_SetWindowRelativeMouseMode(m_editorManager->getWindow(), true);
        }
    }

    // Stop camera rotation when right-click is released
    if (m_isCameraActive && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        m_isCameraActive = false;
        if (m_editorManager && m_editorManager->getWindow()) {
            SDL_SetWindowRelativeMouseMode(m_editorManager->getWindow(), false);
        }
    }

    // Now we can update the camera for the NEXT frame
    m_editorCamera.update(ImGui::GetIO().DeltaTime, m_isCameraActive);

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
            m_sceneTexture = m_renderer->getSceneTexture(); // Update local pointer immediately
            
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
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    
    if (m_sceneTexture != nullptr && m_viewportWidth >= 64 && m_viewportHeight >= 64 && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        // Texture'ı ImGui'ye göster
        ImGui::Image(
            (ImTextureID)(intptr_t)m_sceneTexture,
            viewportPanelSize,
            ImVec2(0, 1),      // UV min (sol-alt)
            ImVec2(1, 0)       // UV max (sağ-üst)
        );
        
        // Eğer bir nesne seçiliyse viewport etrafına ince bir çizgi çek (Unreal stili)
        if (m_selectedEntity)
        {
            ImGui::GetWindowDrawList()->AddRect(
                viewportPos, 
                ImVec2(viewportPos.x + viewportPanelSize.x, viewportPos.y + viewportPanelSize.y), 
                IM_COL32(255, 165, 0, 150), // Turuncu
                0.0f, 0, 2.0f
            );
        }
    }
    else
    {
        ImGui::Dummy(viewportPanelSize);
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(10, 10));
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Scene texture initializing...");
    }

    // 3. ImGuizmo Setup
    ImGuizmo::SetOrthographic(false);
    
    // Vulkan'da null texture crash'ini önlemek için bir doku (font atlas) push'luyoruz
    // Eğer font texture henüz hazır değilse (NULL), push yapmıyoruz.
    bool pushedFont = false;
    if (ImGui::GetIO().Fonts->IsBuilt()) {
        ImGui::GetWindowDrawList()->PushTextureID(ImGui::GetIO().Fonts->TexID);
        pushedFont = true;
    }

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportPanelSize.x, viewportPanelSize.y);

    // 4. Matrix ready for ImGuizmo (Captured above)
    bool cameraReady = true;

    // 5. Orientation Gizmo
    if (cameraReady)
    {
        float viewManipSize = 128.f;
        float padding = 10.f;
        ImVec2 viewManipPos = ImVec2(viewportPos.x + viewportPanelSize.x - viewManipSize - padding, viewportPos.y + padding);
        ImGuizmo::ViewManipulate(glm::value_ptr(view), 8.0f, viewManipPos, ImVec2(viewManipSize, viewManipSize), 0x00000000);
    }

    // 6. Gizmo Manipulation & Selection Highlight
    if (cameraReady && m_selectedEntity && m_selectedEntity->has<CTransform>())
    {
        auto& transform = m_selectedEntity->get<CTransform>();
        
        // --- SELECTION HIGHLIGHT (Bounding Box) ---
        glm::mat4 highlightMatrix = transform.globalMatrix;
        float cubeSize = 1.0f;
        if (m_selectedEntity->has<CMesh>()) cubeSize = 1.1f;
        
        ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(highlightMatrix), 1);
        
        // --- GIZMO MANIPULATION ---
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

    if (pushedFont) {
        ImGui::GetWindowDrawList()->PopTextureID();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
