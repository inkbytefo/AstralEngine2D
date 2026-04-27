#pragma once
#include "system.h"
#include "../ecs/components.h"

namespace Astral {

class LifespanSystem : public ISystem {
public:
    void update(EntityManager& entityManager, float deltaTime) override {
        for (auto& entity : entityManager.view<CLifeSpan>()) {
            auto& lifespan = entity->get<CLifeSpan>();
            lifespan.remaining -= deltaTime;

            if (lifespan.remaining <= 0.0f) {
                entity->destroy();
            }
            else if (entity->has<CShape>()) {
                float ratio = lifespan.remaining / lifespan.total;
                entity->get<CShape>().a = static_cast<uint8_t>(255.0f * ratio);
            }
        }
    }

    int32_t getPriority() const override { return 30; }
    const char* getName() const override { return "LifespanSystem"; }
};

} // namespace Astral