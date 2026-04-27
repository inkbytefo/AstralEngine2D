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
            entity->destroy();
            if (m_selectedEntity == entity) m_selectedEntity = nullptr;
        }

        if (ImGui::MenuItem("Duplicate"))
        {
            // Simple duplication logic
            auto newEntity = m_entityManager->addEntity(entity->tag() + "_Copy");
            if (entity->has<CTransform>()) {
                auto& t = entity->get<CTransform>();
                auto& nt = newEntity->add<CTransform>(t.pos, t.velocity);
                nt.rotation = t.rotation;
                nt.scale = t.scale;
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Create Empty Child"))
        {
            auto child = m_entityManager->addEntity("Empty");
            auto& ct = child->add<CTransform>();
            ct.parent = entity;
            if (entity->has<CTransform>()) {
                entity->get<CTransform>().children.push_back(child);
            }
        }

        ImGui::EndPopup();
    }
}

void SceneHierarchyPanel::draw()
{
    ImGui::Begin("Hierarchy");

    if (m_entityManager)
    {
        // Add Entity button
        if (ImGui::Button("Add Entity", ImVec2(-1, 0)))
        {
            m_entityManager->addEntity("New Entity")->add<CTransform>();
        }
        ImGui::Separator();

        // Draw root entities (entities without parent)
        for (auto& entity : m_entityManager->getEntities())
        {
            if (!entity->isActive()) continue;

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
        
        // Right-click on empty space
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                m_entityManager->addEntity("Empty")->add<CTransform>();
            }
            ImGui::EndPopup();
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
