#pragma once
#include <gtest/gtest.h>

// Engine includes
#include "core/entity_manager.h"
#include "ecs/components.h"

// Test utilities
namespace Astral {
namespace Test {

// Test fixture for EntityManager tests
class EntityManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        entityManager = std::make_unique<Astral::EntityManager>();
    }

    void TearDown() override {
        entityManager->clear();
    }

    std::unique_ptr<Astral::EntityManager> entityManager;
};

// Test fixture for basic engine tests
class EngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Basic setup for engine tests
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

} // namespace Test
} // namespace Astral