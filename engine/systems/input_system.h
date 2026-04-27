#pragma once
#include "system.h"
#include "../ecs/components.h"
#include <map>
#include <string>
#include <functional>

namespace Astral {

class InputSystem : public ISystem {
public:
    using ActionCallback = std::function<void(const std::string&, bool)>;

    void setApp(class App* app) { m_app = app; }
    void setActionCallback(ActionCallback callback) { m_actionCallback = callback; }

    void init(EntityManager& entityManager) override {
        m_entityManager = &entityManager;
    }

    void update(EntityManager& entityManager, float deltaTime) override {
        m_entityManager = &entityManager;
    }

    void onEvent(const SDL_Event& event) override {
        if (!m_entityManager) return;

        handleMouse(event);
        handleKeyboard(event);
    }

    void registerAction(SDL_Keycode key, const std::string& action) {
        m_actionMap[key] = action;
    }

    int32_t getPriority() const override { return -100; }
    const char* getName() const override { return "InputSystem"; }

private:
    void handleMouse(const SDL_Event& event) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            m_entityManager->each<CFreeLook>([&](const auto& entity) {
                auto& look = entity->get<CFreeLook>();
                if (look.isRightMouseDown) {
                    look.yaw += event.motion.xrel * look.sensitivity;
                    look.pitch += event.motion.yrel * look.sensitivity;
                    if (look.pitch > 89.0f) look.pitch = 89.0f;
                    if (look.pitch < -89.0f) look.pitch = -89.0f;
                }
            });
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (event.button.button == SDL_BUTTON_RIGHT) {
                bool pressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                m_entityManager->each<CFreeLook>([&](const auto& entity) {
                    entity->get<CFreeLook>().isRightMouseDown = pressed;
                });
                
                // Mouse'u kilitle/serbest bırak
                if (m_app) {
                    SDL_SetWindowRelativeMouseMode(SDL_GetWindowFromID(event.button.windowID), pressed);
                }
            }
        }
    }

    void handleKeyboard(const SDL_Event& event) {
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            SDL_Keycode key = event.key.key;
            if (m_actionMap.count(key)) {
                const std::string& action = m_actionMap[key];
                bool started = (event.type == SDL_EVENT_KEY_DOWN);

                // 1. CInput bileşenlerini güncelle
                m_entityManager->each<CInput>([&](const auto& entity) {
                    auto& input = entity->get<CInput>();
                    if (action == "UP")    input.up = started;
                    if (action == "DOWN")  input.down = started;
                    if (action == "LEFT")  input.left = started;
                    if (action == "RIGHT") input.right = started;
                });

                // 2. Scene-specific action? (Daha sonra bir callback sistemi eklenebilir)
                if (m_actionCallback) m_actionCallback(action, started);
            }
        }
    }

    EntityManager* m_entityManager{ nullptr };
    class App* m_app{ nullptr };
    std::map<SDL_Keycode, std::string> m_actionMap;
    ActionCallback m_actionCallback;
};

} // namespace Astral
