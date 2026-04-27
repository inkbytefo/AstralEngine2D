#include "scene_hierarchy_panel.h"
#include "imgui.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <cstdint>

SceneHierarchyPanel::SceneHierarchyPanel(Astral::EntityManager* entityManager)
    : m_entityManager(entityManager)
{
}

void SceneHierarchyPanel::drawEntityNode(std::shared_ptr<Astral::Entity> entity)
{
    if (!entity) return;

    // Check if entity has children
    bool hasChildren = false;
    if (entity->has<CTransform>())
    {
        auto& transform = entity->get<CTransform>();
        hasChildren = !transform.children.empty();
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (m_selectedEntity == entity)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity->id(), flags, "%s", entity->tag().c_str());

    // Handle selection
    if (ImGui::IsItemClicked())
    {
        m_selectedEntity = entity;
        if (m_onSelectionChanged)
        {
            m_onSelectionChanged(m_selectedEntity);
        }
    }

    // Handle right-click context menu
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_contextMenuEntity = entity;
    }

    // Draw children
    if (opened)
    {
        if (entity->has<CTransform>())
        {
            auto& transform = entity->get<CTransform>();
            for (auto& child : transform.children)
            {
                drawEntityNode(child);
            }
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::drawContextMenu(std::shared_ptr<Astral::Entity> entity)
{
    if (!entity) return;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete"))
        {
            // TODO: Implement entity deletion
            m_contextMenuEntity = nullptr;
        }

        if (ImGui::MenuItem("Duplicate"))
        {
            // TODO: Implement entity duplication
        }

        if (ImGui::MenuItem("Create Empty Child"))
        {
            // TODO: Implement creating empty child entity
        }

        ImGui::EndPopup();
    }
}

void SceneHierarchyPanel::draw()
{
    ImGui::Begin("Hierarchy");

    if (m_entityManager)
    {
        // Draw root entities (entities without parent)
        for (auto& entity : m_entityManager->getEntities())
        {
            if (entity->has<CTransform>())
            {
                auto& transform = entity->get<CTransform>();
                if (transform.parent.expired())
                {
                    drawEntityNode(entity);
                }
            }
            else
            {
                drawEntityNode(entity);
            }
        }

        // Handle context menu
        if (m_contextMenuEntity)
        {
            drawContextMenu(m_contextMenuEntity);
        }

        // Clear selection on empty space click
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            m_selectedEntity = nullptr;
            if (m_onSelectionChanged)
            {
                m_onSelectionChanged(nullptr);
            }
        }
    }

    ImGui::End();
}
