#pragma once
#include "../core/entity_manager.h"
#include "../ecs/components.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {

/**
 * @brief Free-Look Kamera Sistemi.
 * WASD ile hareket, Sağ Tık + Mouse ile bakış sağlar.
 */
class CameraSystem {
public:
    static void update(EntityManager& entityManager, float deltaTime) {
        for (auto& entity : entityManager.getEntities()) {
            if (!entity->has<CCamera>() || !entity->has<CFreeLook>() || !entity->has<CTransform>()) continue;

            auto& camera = entity->get<CCamera>();
            auto& look = entity->get<CFreeLook>();
            auto& transform = entity->get<CTransform>();

            if (!camera.isActive) continue;

            // 1. ROTASYON HESAPLAMA (Z-Up Standartı)
            // Yaw: Z ekseni etrafında dönme, Pitch: Y ekseni etrafında (yukarı/aşağı) bakma
            glm::vec3 front;
            front.x = cos(glm::radians(look.pitch)) * cos(glm::radians(look.yaw));
            front.y = cos(glm::radians(look.pitch)) * sin(glm::radians(look.yaw));
            front.z = sin(glm::radians(look.pitch));
            front = glm::normalize(front);

            glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 right = glm::normalize(glm::cross(worldUp, front)); // Sağ vektör hesabı düzeltildi
            glm::vec3 up = glm::normalize(glm::cross(right, front));

            // 2. HAREKET HESAPLAMA (WASD)
            if (entity->has<CInput>()) {
                auto& input = entity->get<CInput>();
                float velocity = look.speed * deltaTime;

                if (input.up)    transform.pos += front * velocity;
                if (input.down)  transform.pos -= front * velocity;
                if (input.left)  transform.pos -= right * velocity;
                if (input.right) transform.pos += right * velocity;
            }

            // 3. VIEW MATRIX GÜNCELLEME
            camera.view = glm::lookAt(transform.pos, transform.pos + front, up);
        }
    }
};

} // namespace Astral
