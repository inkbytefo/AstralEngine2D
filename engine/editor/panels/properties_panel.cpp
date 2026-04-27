#include "properties_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <glm/gtc/type_ptr.hpp>

static void drawPropertyLabel(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
}

void PropertiesPanel::draw()
{
    ImGui::Begin("Inspector");

    if (m_selectedEntity)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::TextDisabled("ID: %u", m_selectedEntity->id());
        
        char tagBuffer[256];
        strncpy(tagBuffer, m_selectedEntity->tag().c_str(), sizeof(tagBuffer));
        if (ImGui::InputText("##Tag", tagBuffer, sizeof(tagBuffer)))
        {
            m_selectedEntity->setTag(tagBuffer);
        }
        ImGui::PopStyleVar();

        ImGui::Separator();

        auto drawComponent = [&](const char* label, auto&& drawUI) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            bool open = ImGui::CollapsingHeader(label, treeNodeFlags);
            ImGui::PopStyleVar();

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
        };

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
            });
        }

        if (m_selectedEntity->has<CCamera>())
        {
            drawComponent("Camera", [&]() {
                auto& camera = m_selectedEntity->get<CCamera>();
                
                drawPropertyLabel("Active");
                ImGui::Checkbox("##Active", &camera.isActive);

                drawPropertyLabel("Aspect Ratio");
                ImGui::DragFloat("##Aspect", &camera.aspectRatio, 0.01f, 0.1f, 10.0f);
            });
        }

        if (m_selectedEntity->has<CLight>())
        {
            drawComponent("Light", [&]() {
                auto& light = m_selectedEntity->get<CLight>();
                
                drawPropertyLabel("Type");
                const char* lightTypes[] = { "Directional", "Point", "Spot" };
                int currentType = (int)light.type;
                if (ImGui::Combo("##LightType", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
                {
                    light.type = (LightType)currentType;
                }

                drawPropertyLabel("Color");
                ImGui::ColorEdit3("##Color", glm::value_ptr(light.color), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

                drawPropertyLabel("Intensity");
                ImGui::DragFloat("##Intensity", &light.intensity, 0.1f, 0.0f, 1000.0f);

                if (light.type != LightType::Directional)
                {
                    drawPropertyLabel("Range");
                    ImGui::DragFloat("##Range", &light.range, 0.1f, 0.0f, 1000.0f);
                }

                if (light.type == LightType::Spot)
                {
                    drawPropertyLabel("Inner Cut");
                    ImGui::DragFloat("##Inner", &light.innerCutoff, 0.01f, 0.0f, 1.0f);
                    drawPropertyLabel("Outer Cut");
                    ImGui::DragFloat("##Outer", &light.outerCutoff, 0.01f, 0.0f, 1.0f);
                }
            });
        }

        if (m_selectedEntity->has<CMesh>())
        {
            drawComponent("Mesh", [&]() {
                auto& mesh = m_selectedEntity->get<CMesh>();
                
                drawPropertyLabel("Mesh Name");
                char meshBuffer[256];
                strncpy(meshBuffer, mesh.meshName.c_str(), sizeof(meshBuffer));
                if (ImGui::InputText("##MeshName", meshBuffer, sizeof(meshBuffer)))
                {
                    mesh.meshName = meshBuffer;
                }

                drawPropertyLabel("Material");
                char matBuffer[256];
                strncpy(matBuffer, mesh.materialName.c_str(), sizeof(matBuffer));
                if (ImGui::InputText("##MaterialName", matBuffer, sizeof(matBuffer)))
                {
                    mesh.materialName = matBuffer;
                }
            });
        }
        
        if (m_selectedEntity->has<CFreeLook>())
        {
            drawComponent("Free Look", [&]() {
                auto& fl = m_selectedEntity->get<CFreeLook>();
                drawPropertyLabel("Speed");
                ImGui::DragFloat("##FLSpeed", &fl.speed, 0.1f, 0.0f, 100.0f);
                drawPropertyLabel("Sensitivity");
                ImGui::DragFloat("##FLSens", &fl.sensitivity, 0.01f, 0.0f, 1.0f);
            });
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add Component", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("AddComponent");
        }
        
        if (ImGui::BeginPopup("AddComponent"))
        {
            if (!m_selectedEntity->has<CCamera>() && ImGui::MenuItem("Camera")) { m_selectedEntity->add<CCamera>(); }
            if (!m_selectedEntity->has<CLight>() && ImGui::MenuItem("Light")) { m_selectedEntity->add<CLight>(); }
            if (!m_selectedEntity->has<CMesh>() && ImGui::MenuItem("Mesh")) { m_selectedEntity->add<CMesh>("cube"); }
            if (!m_selectedEntity->has<CFreeLook>() && ImGui::MenuItem("Free Look")) { m_selectedEntity->add<CFreeLook>(); }
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}
