#pragma once

#include <memory>

namespace Astral {
class Entity;
}

/**
 * @brief Base interface for all editor panels.
 * Provides a common contract for panel implementations.
 */
class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;

    /**
     * @brief Draw the panel UI.
     */
    virtual void draw() = 0;

    /**
     * @brief Get the panel's display name (used for docking).
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Called when an entity is selected in the editor.
     */
    virtual void onEntitySelected(std::shared_ptr<Astral::Entity> entity) {}

    /**
     * @brief Called when the selection is cleared.
     */
    virtual void onEntityDeselected() {}
};
