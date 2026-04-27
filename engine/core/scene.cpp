#include "scene.h"
#include "app.h"
#include "../systems/input_system.h"

void Scene::registerAction(SDL_Keycode key, const std::string& actionName) {
    if (m_app) {
        if (auto* is = m_app->getSystemManager().getSystem<Astral::InputSystem>()) {
            is->registerAction(key, actionName);
        }
    }
    m_actionMap[key] = actionName;
}
