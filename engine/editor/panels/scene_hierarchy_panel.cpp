#include "scene_hierarchy_panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "core/entity_manager.h"
#include "core/scene_serializer.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <cstdint>
#include <string>
#include <algorithm>

SceneHierarchyPanel::SceneHierarchyPanel(Astral::EntityManager* entityManager)
    : m_entityManager(entityManager)
{
}

void SceneHierarchyPanel::detachFromParent(std::shared_ptr<Astral::Entity> entity)
{
    if (!entity || !entity->has<CTransform>()) return;
    auto& transform = entity->get<CTransform>();
    
    if (!transform.parent.expired())
    {
        auto parent = transform.parent.lock();
        if (parent && parent->has<CTransform>())
        {
            auto& parentTransform = parent->get<CTransform>();
            auto& children = parentTransform.children;
            children.erase(
                std::remove_if(children.begin(), children.end(),
                    [&entity](const std::shared_ptr<Astral::Entity>& child) {
                        return child->id() == entity->id();
                    }),
                children.end());
        }
        transform.parent.reset();
    }
}

void SceneHierarchyPanel::handleDragDrop(std::shared_ptr<Astral::Entity> entity)
{
    // Drag Source — bu entity'yi sürüklenebilir yap
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        uint32_t entityId = entity->id();
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entityId, sizeof(uint32_t));
        ImGui::Text("%s", entity->tag().c_str());
        ImGui::EndDragDropSource();
    }

    // Drop Target — başka bir entity'yi buraya bırakılabilir yap
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
        {
            uint32_t draggedId = *(const uint32_t*)payload->Data;
            
            // Kendine parent olamaz
            if (draggedId != entity->id())
            {
                // Sürüklenen entity'yi bul
                std::shared_ptr<Astral::Entity> draggedEntity = nullptr;
                for (auto& e : m_entityManager->getEntities())
                {
                    if (e->id() == draggedId && e->isActive())
                    {
                        draggedEntity = e;
                        break;
                    }
                }

                if (draggedEntity && draggedEntity->has<CTransform>() && entity->has<CTransform>())
                {
                    // Döngüsel parent kontrolü — entity, draggedEntity'nin child'ı mı?
                    bool isCyclic = false;
                    auto& targetTransform = entity->get<CTransform>();
                    auto checkParent = targetTransform.parent;
                    while (!checkParent.expired())
                    {
                        auto p = checkParent.lock();
                        if (p && p->id() == draggedId) { isCyclic = true; break; }
                        if (p && p->has<CTransform>()) checkParent = p->get<CTransform>().parent;
                        else break;
                    }

                    if (!isCyclic)
                    {
                        // Eski parent'tan ayır
                        detachFromParent(draggedEntity);

                        // Yeni parent'a bağla
                        auto& draggedTransform = draggedEntity->get<CTransform>();
                        draggedTransform.parent = entity;
                        entity->get<CTransform>().children.push_back(draggedEntity);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void SceneHierarchyPanel::drawEntityNode(std::shared_ptr<Astral::Entity> entity)
{
    if (!entity) return;

    // Filter check
    if (m_filterBuffer[0] != '\0')
    {
        std::string tag = entity->tag();
        std::string filter = m_filterBuffer;
        std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
        if (tag.find(filter) == std::string::npos) return;
    }

    // Determine Icon/Prefix
    const char* icon = "  ";
    ImVec4 iconColor = ImVec4(1, 1, 1, 1);
    
    if (entity->has<CCamera>()) { icon = "[C]"; iconColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); }
    else if (entity->has<CLight>()) { icon = "[L]"; iconColor = ImVec4(1.0f, 0.9f, 0.3f, 1.0f); }
    else if (entity->has<CMesh>()) { icon = "[M]"; iconColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); }

    // Check if entity has children
    bool hasChildren = false;
    if (entity->has<CTransform>())
    {
        auto& transform = entity->get<CTransform>();
        hasChildren = !transform.children.empty();
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_selectedEntity == entity) flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
    bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity->id(), flags, "%s %s", icon, entity->tag().c_str());
    ImGui::PopStyleColor();

    // Handle selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        m_selectedEntity = entity;
        if (m_onSelectionChanged) m_onSelectionChanged(m_selectedEntity);
    }

    // Drag-and-Drop reparenting
    handleDragDrop(entity);

    // Context Menu
    if (ImGui::BeginPopupContextItem())
    {
        m_contextMenuEntity = entity;
        
        if (ImGui::MenuItem("Rename"))
        {
            m_renameEntity = entity;
            strncpy(m_renameBuffer, entity->tag().c_str(), sizeof(m_renameBuffer) - 1);
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }

        if (ImGui::MenuItem("Duplicate"))
        {
            auto newEntity = m_entityManager->addEntity(entity->tag() + "_Copy");
            
            // Copy Components
            if (entity->has<CTransform>()) {
                auto& src = entity->get<CTransform>();
                auto& dst = newEntity->add<CTransform>(src.pos);
                dst.rotation = src.rotation; dst.scale = src.scale;
            }
            if (entity->has<CCamera>()) { newEntity->add<CCamera>(entity->get<CCamera>()); }
            if (entity->has<CLight>()) { newEntity->add<CLight>(entity->get<CLight>()); }
            if (entity->has<CMesh>()) { newEntity->add<CMesh>(entity->get<CMesh>()); }
            if (entity->has<CFreeLook>()) { newEntity->add<CFreeLook>(entity->get<CFreeLook>()); }
        }

        if (ImGui::MenuItem("Save as Prefab"))
        {
            std::string prefabPath = "assets/" + entity->tag() + ".prefab";
            SceneSerializer::serializeEntityToPrefab(prefabPath, entity);
            SDL_Log("Prefab saved: %s", prefabPath.c_str());
        }

        ImGui::Separator();

        // Unparent — parent'tan ayır (root'a taşı)
        if (entity->has<CTransform>() && !entity->get<CTransform>().parent.expired())
        {
            if (ImGui::MenuItem("Unparent"))
            {
                detachFromParent(entity);
            }
        }

        if (ImGui::MenuItem("Delete", "Del"))
        {
            entity->destroy();
            if (m_selectedEntity == entity)
            {
                m_selectedEntity = nullptr;
                if (m_onSelectionChanged) m_onSelectionChanged(nullptr);
            }
        }

        ImGui::EndPopup();
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

void SceneHierarchyPanel::draw()
{
    ImGui::Begin("Hierarchy");

    // Search Box
    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##Filter", "Search...", m_filterBuffer, sizeof(m_filterBuffer))) {}
    ImGui::PopItemWidth();
    
    ImGui::Spacing();

    if (m_entityManager)
    {
        // Draw root entities
        ImGui::BeginChild("HierarchyScroll");

        // Root seviyeye drop target — entity'leri root'a bırakabilmek için
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
            {
                uint32_t draggedId = *(const uint32_t*)payload->Data;
                for (auto& e : m_entityManager->getEntities())
                {
                    if (e->id() == draggedId && e->isActive())
                    {
                        detachFromParent(e);
                        break;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        for (auto& entity : m_entityManager->getEntities())
        {
            if (!entity->isActive()) continue;

            if (entity->has<CTransform>())
            {
                auto& transform = entity->get<CTransform>();
                if (transform.parent.expired()) drawEntityNode(entity);
            }
            else
            {
                drawEntityNode(entity);
            }
        }
        
        // Right-click on empty space — Entity oluşturma presets
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                m_entityManager->addEntity("Empty")->add<CTransform>();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Create Camera"))
            {
                auto e = m_entityManager->addEntity("Camera");
                e->add<CTransform>();
                e->add<CCamera>();
            }

            if (ImGui::MenuItem("Create Directional Light"))
            {
                auto e = m_entityManager->addEntity("Dir Light");
                e->add<CTransform>();
                e->add<CLight>(LightType::Directional, glm::vec3(1.0f), 1.0f);
            }

            if (ImGui::MenuItem("Create Point Light"))
            {
                auto e = m_entityManager->addEntity("Point Light");
                e->add<CTransform>();
                e->add<CLight>(LightType::Point, glm::vec3(1.0f), 1.0f);
            }

            if (ImGui::MenuItem("Create Mesh (Cube)"))
            {
                auto e = m_entityManager->addEntity("Cube");
                e->add<CTransform>();
                e->add<CMesh>("cube");
            }

            ImGui::EndPopup();
        }

        // Clear selection on empty space click
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        {
            m_selectedEntity = nullptr;
            if (m_onSelectionChanged) m_onSelectionChanged(nullptr);
        }
        ImGui::EndChild();
    }

    // Rename Popup
    if (m_renameEntity)
    {
        ImGui::OpenPopup("Rename Entity");
        if (ImGui::BeginPopupModal("Rename Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("New Name", m_renameBuffer, sizeof(m_renameBuffer));
            if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
            {
                m_renameEntity->setTag(m_renameBuffer);
                m_renameEntity = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_renameEntity = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}
