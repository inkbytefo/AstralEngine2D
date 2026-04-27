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
                ImGui::DragFloat3("Position", glm::value_ptr(transform.pos), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(transform.rotation), 1.0f);
                ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
            }
        }

        if (m_selectedEntity->has<CCamera>())
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& camera = m_selectedEntity->get<CCamera>();
                ImGui::Checkbox("Active##Camera", &camera.isActive);
                ImGui::DragFloat("Aspect Ratio", &camera.aspectRatio, 0.01f, 0.1f, 10.0f);
            }
        }

        if (m_selectedEntity->has<CLight>())
        {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& light = m_selectedEntity->get<CLight>();
                ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 10.0f);
                ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
            }
        }
    }
    else
    {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}
