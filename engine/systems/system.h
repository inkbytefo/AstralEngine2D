#pragma once
#include <cstdint>
#include "../core/entity_manager.h"

namespace Astral {

class ISystem {
public:
    virtual ~ISystem() = default;

    // Sistem ilk kez başlatıldığında (bir kere çağrılır)
    virtual void init(Astral::EntityManager& entityManager) {}

    // Her frame çağrılacak ana güncelleme
    virtual void update(Astral::EntityManager& entityManager, float deltaTime) = 0;

    // SDL Event'lerini yakalamak için (Opsiyonel)
    virtual void onEvent(const SDL_Event& event) {}

    // Sistem kapatılırken (opsiyonel)
    virtual void shutdown() {}

    // Sistemlerin çalıştırılma sırasını belirler (daha düşük = daha erken)
    virtual int32_t getPriority() const { return 0; }

    // Sistem adı (debug / logging için)
    virtual const char* getName() const = 0;
};

} // namespace Astral
