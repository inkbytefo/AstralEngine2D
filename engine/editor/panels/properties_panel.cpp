#include "properties_panel.h"
#include "imgui.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <glm/gtc/type_ptr.hpp>

void PropertiesPanel::draw()
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
                
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::Text("Position"); ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Pos", glm::value_ptr(transform.pos), 0.1f);
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Text("Rotation"); ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Rot", glm::value_ptr(transform.rotation), 1.0f);
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Text("Scale"); ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat3("##Scale", glm::value_ptr(transform.scale), 0.1f);
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
        }

        if (m_selectedEntity->has<CCamera>())
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& camera = m_selectedEntity->get<CCamera>();
                ImGui::Checkbox("Active", &camera.isActive);
                ImGui::DragFloat("FOV", &camera.aspectRatio, 0.01f); // Placeholder for FOV
            }
        }

        if (m_selectedEntity->has<CLight>())
        {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& light = m_selectedEntity->get<CLight>();
                ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
                ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
            }
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Add Component", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("AddComponent");
        }
        
        if (ImGui::BeginPopup("AddComponent"))
        {
            if (ImGui::MenuItem("Camera")) { /* Add Camera */ }
            if (ImGui::MenuItem("Light")) { /* Add Light */ }
            if (ImGui::MenuItem("Mesh")) { /* Add Mesh */ }
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}
