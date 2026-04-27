#include "test_framework.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Astral {
namespace Test {

// EntityManager tests
TEST_F(EntityManagerTest, AddEntity) {
    auto entity = entityManager->addEntity("test_entity");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->tag(), "test_entity");
    EXPECT_TRUE(entity->isActive());
}

TEST_F(EntityManagerTest, GetEntitiesByTag) {
    auto entity1 = entityManager->addEntity("player");
    auto entity2 = entityManager->addEntity("player");
    auto entity3 = entityManager->addEntity("enemy");

    entityManager->update();

    auto players = entityManager->getEntities("player");
    auto enemies = entityManager->getEntities("enemy");

    EXPECT_EQ(players.size(), 2);
    EXPECT_EQ(enemies.size(), 1);
    EXPECT_EQ(players[0], entity1);
    EXPECT_EQ(players[1], entity2);
    EXPECT_EQ(enemies[0], entity3);
}

TEST_F(EntityManagerTest, ViewWithComponents) {
    auto entity1 = entityManager->addEntity("entity1");
    entity1->add<CTransform>(glm::vec3(1.0f, 2.0f, 3.0f));

    auto entity2 = entityManager->addEntity("entity2");
    entity2->add<CTransform>(glm::vec3(4.0f, 5.0f, 6.0f));
    entity2->add<CMesh>("cube", "material");

    entityManager->update();

    // View entities with CTransform
    auto transformEntities = entityManager->view<CTransform>();
    EXPECT_EQ(transformEntities.size(), 2);

    // View entities with both CTransform and CMesh
    auto meshEntities = entityManager->view<CTransform, CMesh>();
    EXPECT_EQ(meshEntities.size(), 1);
    EXPECT_EQ(meshEntities[0], entity2);
}

TEST_F(EntityManagerTest, UpdateAddsPendingEntities) {
    auto entity1 = entityManager->addEntity("entity1");
    auto entity2 = entityManager->addEntity("entity2");

    // Newly added entities should be in pending state
    auto allEntities = entityManager->getEntities();
    EXPECT_EQ(allEntities.size(), 0); // Not yet updated

    // After update, entities should be added
    entityManager->update();
    allEntities = entityManager->getEntities();
    EXPECT_EQ(allEntities.size(), 2);
}

TEST_F(EntityManagerTest, PendingEntitiesVisibleSeparately) {
    auto entity1 = entityManager->addEntity("pending_a");
    auto entity2 = entityManager->addEntity("pending_b");

    const auto& pending = entityManager->getPendingEntities();
    EXPECT_EQ(pending.size(), 2);
    EXPECT_EQ(pending[0], entity1);
    EXPECT_EQ(pending[1], entity2);
    EXPECT_TRUE(entityManager->getEntities().empty());
}

TEST_F(EntityManagerTest, TryGetEntitiesDoesNotInsertMissingTag) {
    entityManager->addEntity("player");
    entityManager->update();

    const auto* missing = entityManager->tryGetEntities("missing_tag");
    EXPECT_EQ(missing, nullptr);

    const auto* players = entityManager->tryGetEntities("player");
    ASSERT_NE(players, nullptr);
    EXPECT_EQ(players->size(), 1);
}

TEST_F(EntityManagerTest, EachFiltersActiveEntitiesByComponents) {
    auto entity1 = entityManager->addEntity("entity1");
    entity1->add<CTransform>(glm::vec3(1.0f));

    auto entity2 = entityManager->addEntity("entity2");
    entity2->add<CTransform>(glm::vec3(2.0f));
    entity2->add<CMesh>("cube", "material");

    auto entity3 = entityManager->addEntity("entity3");
    entity3->add<CTransform>(glm::vec3(3.0f));
    entity3->add<CMesh>("cube", "material");

    entityManager->update();
    entity3->destroy();

    int transformCount = 0;
    int meshCount = 0;

    entityManager->each<CTransform>([&](const auto&) {
        transformCount++;
    });

    entityManager->each<CTransform, CMesh>([&](const auto&) {
        meshCount++;
    });

    EXPECT_EQ(transformCount, 2);
    EXPECT_EQ(meshCount, 1);
}

// Basic engine tests
TEST_F(EngineTest, GLM_VectorOperations) {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);

    glm::vec3 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    glm::vec3 normalized = glm::normalize(a);
    EXPECT_NEAR(glm::length(normalized), 1.0f, 0.001f);
}

TEST_F(EngineTest, GLM_MatrixOperations) {
    glm::mat4 identity = glm::mat4(1.0f);
    glm::mat4 translation = glm::translate(identity, glm::vec3(1, 2, 3));

    glm::vec4 position(0, 0, 0, 1);
    glm::vec4 result = translation * position;

    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

} // namespace Test
} // namespace Astral
