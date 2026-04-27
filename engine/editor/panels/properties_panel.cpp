#include "properties_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

static void drawPropertyLabel(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
}

// Güvenli string kopyalama — null-terminator garanti edilir
static void safeCopyString(char* dest, const char* src, size_t destSize)
{
    if (destSize == 0) return;
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

void PropertiesPanel::draw()
{
    ImGui::Begin("Inspector");

    if (m_selectedEntity)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::TextDisabled("ID: %u", m_selectedEntity->id());
        
        char tagBuffer[256];
        safeCopyString(tagBuffer, m_selectedEntity->tag().c_str(), sizeof(tagBuffer));
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Tag", tagBuffer, sizeof(tagBuffer)))
        {
            m_selectedEntity->setTag(tagBuffer);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        ImGui::Separator();

        // Component çizim yardımcısı — CollapsingHeader + Table + Remove Component
        auto drawComponent = [&](const char* label, auto&& drawUI, auto&& removeFunc) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            bool open = ImGui::CollapsingHeader(label, treeNodeFlags);
            ImGui::PopStyleVar();

            // Remove Component butonu (header sağ tarafı)
            bool removeComponent = false;
            float lineHeight = ImGui::GetFrameHeight();
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - lineHeight * 0.5f);
            std::string popupId = std::string("ComponentSettings_") + label;
            if (ImGui::Button("+", ImVec2(lineHeight, lineHeight)))
            {
                ImGui::OpenPopup(popupId.c_str());
            }

            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::MenuItem("Remove Component"))
                    removeComponent = true;
                ImGui::EndPopup();
            }

            if (open)
            {
                if (ImGui::BeginTable("properties", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);
                    
                    drawUI();
                    
                    ImGui::EndTable();
                }
                ImGui::Spacing();
            }

            if (removeComponent)
            {
                removeFunc();
            }
        };

        // --- Transform Component ---
        if (m_selectedEntity->has<CTransform>())
        {
            drawComponent("Transform", [&]() {
                auto& transform = m_selectedEntity->get<CTransform>();
                
                drawPropertyLabel("Position");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Pos", glm::value_ptr(transform.pos), 0.1f);
                ImGui::PopItemWidth();

                drawPropertyLabel("Rotation");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Rot", glm::value_ptr(transform.rotation), 1.0f);
                ImGui::PopItemWidth();

                drawPropertyLabel("Scale");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Scale", glm::value_ptr(transform.scale), 0.1f);
                ImGui::PopItemWidth();
            }, [&]() {
                // Transform genellikle kaldırılmaz ama tutarlılık için bırakıyoruz
                m_selectedEntity->remove<CTransform>();
            });
        }

        // --- Camera Component ---
        if (m_selectedEntity->has<CCamera>())
        {
            drawComponent("Camera", [&]() {
                auto& camera = m_selectedEntity->get<CCamera>();
                
                drawPropertyLabel("Active");
                ImGui::Checkbox("##Active", &camera.isActive);

                drawPropertyLabel("FOV");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##FOV", &camera.fov, 0.5f, 10.0f, 179.0f, "%.1f°");
                ImGui::PopItemWidth();

                drawPropertyLabel("Near Plane");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Near", &camera.nearPlane, 0.01f, 0.001f, camera.farPlane - 0.01f, "%.3f");
                ImGui::PopItemWidth();

                drawPropertyLabel("Far Plane");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Far", &camera.farPlane, 1.0f, camera.nearPlane + 0.01f, 100000.0f, "%.1f");
                ImGui::PopItemWidth();

                drawPropertyLabel("Aspect Ratio");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Aspect", &camera.aspectRatio, 0.01f, 0.1f, 10.0f, "%.3f");
                ImGui::PopItemWidth();
            }, [&]() {
                m_selectedEntity->remove<CCamera>();
            });
        }

        // --- Light Component ---
        if (m_selectedEntity->has<CLight>())
        {
            drawComponent("Light", [&]() {
                auto& light = m_selectedEntity->get<CLight>();
                
                drawPropertyLabel("Type");
                const char* lightTypes[] = { "Directional", "Point", "Spot" };
                int currentType = (int)light.type;
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##LightType", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
                {
                    light.type = (LightType)currentType;
                }
                ImGui::PopItemWidth();

                drawPropertyLabel("Color");
                ImGui::ColorEdit3("##Color", glm::value_ptr(light.color), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

                drawPropertyLabel("Intensity");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Intensity", &light.intensity, 0.1f, 0.0f, 1000.0f);
                ImGui::PopItemWidth();

                // Direction — Directional ve Spot için
                if (light.type != LightType::Point)
                {
                    drawPropertyLabel("Direction");
                    ImGui::PushItemWidth(-1);
                    ImGui::DragFloat3("##Direction", glm::value_ptr(light.direction), 0.01f, -1.0f, 1.0f);
                    ImGui::PopItemWidth();
                }

                // Range — Point ve Spot için
                if (light.type != LightType::Directional)
                {
                    drawPropertyLabel("Range");
                    ImGui::PushItemWidth(-1);
                    ImGui::DragFloat("##Range", &light.range, 0.1f, 0.0f, 1000.0f);
                    ImGui::PopItemWidth();
                }

                // Cutoff — Sadece Spot için
                if (light.type == LightType::Spot)
                {
                    drawPropertyLabel("Inner Cutoff");
                    ImGui::PushItemWidth(-1);
                    ImGui::DragFloat("##Inner", &light.innerCutoff, 0.01f, 0.0f, 1.0f);
                    ImGui::PopItemWidth();

                    drawPropertyLabel("Outer Cutoff");
                    ImGui::PushItemWidth(-1);
                    ImGui::DragFloat("##Outer", &light.outerCutoff, 0.01f, 0.0f, 1.0f);
                    ImGui::PopItemWidth();
                }
            }, [&]() {
                m_selectedEntity->remove<CLight>();
            });
        }

        // --- Mesh Component ---
        if (m_selectedEntity->has<CMesh>())
        {
            drawComponent("Mesh", [&]() {
                auto& mesh = m_selectedEntity->get<CMesh>();
                
                drawPropertyLabel("Mesh Name");
                char meshBuffer[256];
                safeCopyString(meshBuffer, mesh.meshName.c_str(), sizeof(meshBuffer));
                ImGui::PushItemWidth(-1);
                if (ImGui::InputText("##MeshName", meshBuffer, sizeof(meshBuffer)))
                {
                    mesh.meshName = meshBuffer;
                }
                ImGui::PopItemWidth();

                drawPropertyLabel("Material");
                char matBuffer[256];
                safeCopyString(matBuffer, mesh.materialName.c_str(), sizeof(matBuffer));
                ImGui::PushItemWidth(-1);
                if (ImGui::InputText("##MaterialName", matBuffer, sizeof(matBuffer)))
                {
                    mesh.materialName = matBuffer;
                }
                ImGui::PopItemWidth();
            }, [&]() {
                m_selectedEntity->remove<CMesh>();
            });
        }
        
        // --- Free Look Component ---
        if (m_selectedEntity->has<CFreeLook>())
        {
            drawComponent("Free Look", [&]() {
                auto& fl = m_selectedEntity->get<CFreeLook>();
                drawPropertyLabel("Speed");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##FLSpeed", &fl.speed, 0.1f, 0.0f, 100.0f);
                ImGui::PopItemWidth();
                drawPropertyLabel("Sensitivity");
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##FLSens", &fl.sensitivity, 0.01f, 0.0f, 1.0f);
                ImGui::PopItemWidth();
            }, [&]() {
                m_selectedEntity->remove<CFreeLook>();
            });
        }
        
        // --- Add Component Button ---
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = ImGui::GetContentRegionAvail().x;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.29f, 0.62f, 1.0f, 0.3f));
        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 28)))
        {
            ImGui::OpenPopup("AddComponent");
        }
        ImGui::PopStyleColor(2);
        
        if (ImGui::BeginPopup("AddComponent"))
        {
            if (!m_selectedEntity->has<CTransform>() && ImGui::MenuItem("Transform")) { m_selectedEntity->add<CTransform>(); }
            if (!m_selectedEntity->has<CCamera>() && ImGui::MenuItem("Camera")) { m_selectedEntity->add<CCamera>(); }
            if (!m_selectedEntity->has<CLight>() && ImGui::MenuItem("Light")) { m_selectedEntity->add<CLight>(); }
            if (!m_selectedEntity->has<CMesh>() && ImGui::MenuItem("Mesh")) { m_selectedEntity->add<CMesh>("cube"); }
            if (!m_selectedEntity->has<CFreeLook>() && ImGui::MenuItem("Free Look")) { m_selectedEntity->add<CFreeLook>(); }
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::End();
}
