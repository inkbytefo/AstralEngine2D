#pragma once

#include <string>
#include <memory>
#include "core/json.hpp"

namespace Astral {

class Entity;

/**
 * @brief Base class for all script-like behaviors in the engine.
 * Traits are added to entities via the CTrait component.
 */
class ITrait {
public:
    virtual ~ITrait() = default;

    /**
     * @brief Called when the trait is first initialized or when simulation starts.
     */
    virtual void onInit(Entity& entity) {}

    /**
     * @brief Called every frame during Play mode.
     * @param entity The entity this trait belongs to.
     * @param dt Delta time in seconds.
     */
    virtual void onUpdate(Entity& entity, float dt) {}

    /**
     * @brief Called when the entity collides with another entity.
     */
    virtual void onCollision(Entity& entity, Entity& other) {}

    /**
     * @brief Returns the unique name of the trait type (used for serialization).
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Serialize trait properties to JSON.
     */
    virtual void serialize(nlohmann::json& j) const {}

    /**
     * @brief Deserialize trait properties from JSON.
     */
    virtual void deserialize(const nlohmann::json& j) {}
};

} // namespace Astral
