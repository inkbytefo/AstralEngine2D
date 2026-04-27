#pragma once
#include "../systems/system.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace Astral {

class SystemManager {
public:
    void addSystem(std::unique_ptr<ISystem> system);

    void init(EntityManager& entityManager);
    void update(EntityManager& entityManager, float deltaTime);
    void handleEvent(const SDL_Event& event);
    void shutdown();

    // Belirli bir sistem tipi al (örnek: getSystem<RenderSystem>())
    template<typename T>
    T* getSystem() {
        for (auto& sys : systems) {
            if (T* casted = dynamic_cast<T*>(sys.get())) {
                return casted;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<ISystem>> systems;
};

} // namespace Astral
