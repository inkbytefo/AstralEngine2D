#pragma once

#include "editor_panel.h"

/**
 * @brief Properties/Inspector Panel - displays and edits selected entity properties.
 */
class PropertiesPanel : public IEditorPanel {
public:
    PropertiesPanel() = default;
    ~PropertiesPanel() = default;

    void draw() override;
    const char* getName() const override { return "Inspector"; }

    void onEntitySelected(std::shared_ptr<Astral::Entity> entity) override {
        m_selectedEntity = entity;
    }

    void onEntityDeselected() override {
        m_selectedEntity = nullptr;
    }

private:
    std::shared_ptr<Astral::Entity> m_selectedEntity{ nullptr };
};
