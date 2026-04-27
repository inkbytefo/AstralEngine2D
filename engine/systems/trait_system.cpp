#include "systems/trait_system.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"

namespace Astral {

void TraitSystem::update(EntityManager& em, float dt) {
    for (auto& e : em.getEntities()) {
        if (e->isActive() && e->has<CTrait>()) {
            auto& ct = e->get<CTrait>();
            for (auto& trait : ct.traits) {
                trait->onUpdate(*e, dt);
            }
        }
    }
}

} // namespace Astral
