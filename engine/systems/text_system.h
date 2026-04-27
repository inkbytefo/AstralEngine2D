#pragma once
#include "system.h"
#include "../ecs/components.h"

namespace Astral {

/*
class TextSystem : public ISystem {
public:
    void update(EntityManager& entityManager, float deltaTime) override {
        // Text rendering usually happens in UI pass or RenderSystem
        // This system might handle text updates or caching
        for (auto& entity : entityManager.view<CText, CTransform>()) {
            auto& text = entity->get<CText>();
            // Logic for text updates if needed
        }
    }

    int32_t getPriority() const override { return 90; }
    const char* getName() const override { return "TextSystem"; }
};
*/

} // namespace Astral