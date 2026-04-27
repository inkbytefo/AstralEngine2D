#pragma once
#include "system.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {

class TransformSystem : public ISystem
{
public:
    void update(EntityManager& entityManager, float deltaTime) override
    {
        // 1. Önce "Kök" (Root) olan nesneleri bul
        // Kök nesne = parent'ı olmayan nesnelerdir.
        for (auto& entity : entityManager.view<CTransform>()) {
            auto& transform = entity->get<CTransform>();
            if (transform.parent.expired()) { // Kök mü?
                updateTransform(entity, glm::mat4(1.0f));
            }
        }
    }

    int32_t getPriority() const override { return 0; }
    const char* getName() const override { return "TransformSystem"; }

private:
    void updateTransform(std::shared_ptr<Astral::Entity> entity, const glm::mat4& parentGlobalMatrix)

    {
        auto& transform = entity->get<CTransform>();

        // 2. Bu nesnenin Yerel (Local) matrisini hesapla
        glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.pos);
        localMatrix = glm::rotate(localMatrix, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        localMatrix = glm::rotate(localMatrix, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        localMatrix = glm::rotate(localMatrix, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        localMatrix = glm::scale(localMatrix, transform.scale);

        // 3. Global matris = Ebeveynin Globali * Benim Yerelim
        transform.globalMatrix = parentGlobalMatrix * localMatrix;

        // 4. Tüm çocukları özyinelemeli (recursive) olarak güncelle
        for (auto& child : transform.children) {
            updateTransform(child, transform.globalMatrix);
        }
    }
};

} // namespace Astral
