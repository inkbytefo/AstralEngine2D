#pragma once

#include "core/system_manager.h"

namespace Astral {

class TraitSystem : public ISystem {
public:
    void update(EntityManager& em, float dt) override;
    const char* getName() const override { return "TraitSystem"; }
};

} // namespace Astral
