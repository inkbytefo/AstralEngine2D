#pragma once

#include "editor_panel.h"
#include <functional>

namespace Astral {
class EntityManager;
}

/**
 * @brief Scene Hierarchy Panel - displays entity tree and handles selection.
 */
class SceneHierarchyPanel : public IEditorPanel {
public:
    SceneHierarchyPanel(Astral::EntityManager* entityManager);
    ~SceneHierarchyPanel() = default;

    void draw() override;
    const char* getName() const override { return "Hierarchy"; }

    std::shared_ptr<Astral::Entity> getSelectedEntity() const { return m_selectedEntity; }

    /**
     * @brief Set callback for when selection changes.
     */
    void setOnSelectionChanged(std::function<void(std::shared_ptr<Astral::Entity>)> callback) {
        m_onSelectionChanged = callback;
    }

private:
    void drawEntityNode(std::shared_ptr<Astral::Entity> entity);
    void drawContextMenu(std::shared_ptr<Astral::Entity> entity);

    Astral::EntityManager* m_entityManager{ nullptr };
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
    std::shared_ptr<Astral::Entity> m_contextMenuEntity{ nullptr };
    std::function<void(std::shared_ptr<Astral::Entity>)> m_onSelectionChanged;
};
