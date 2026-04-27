#pragma once
#include "system.h"
#include "../ecs/components.h"

namespace Astral {

class PhysicsSystem : public ISystem {
public:
    void update(EntityManager& entityManager, float deltaTime) override {
        entityManager.each<CTransform>([&](const auto& entity) {
            auto& transform = entity->get<CTransform>();
            transform.pos += transform.velocity * deltaTime;
        });
    }

    int32_t getPriority() const override { return -10; }
    const char* getName() const override { return "PhysicsSystem"; }
};

} // namespace Astral
