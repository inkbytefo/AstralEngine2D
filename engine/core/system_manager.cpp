#include "system_manager.h"

namespace Astral {

void SystemManager::addSystem(std::unique_ptr<ISystem> system) {
    systems.push_back(std::move(system));
}

void SystemManager::init(EntityManager& em) {
    // Priority'ye göre sırala (düşük priority önce çalışsın)
    std::sort(systems.begin(), systems.end(),
        [](const auto& a, const auto& b) {
            return a->getPriority() < b->getPriority();
        });

    for (auto& sys : systems) {
        sys->init(em);
    }
}

void SystemManager::update(EntityManager& em, float dt) {
    for (auto& sys : systems) {
        sys->update(em, dt);
    }
}

void SystemManager::handleEvent(const SDL_Event& event) {
    for (auto& sys : systems) {
        sys->onEvent(event);
    }
}

void SystemManager::shutdown() {
    for (auto& sys : systems) {
        sys->shutdown();
    }
}

} // namespace Astral
