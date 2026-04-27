#include <benchmark/benchmark.h>
#include <glm/gtc/matrix_transform.hpp>
#include "core/entity_manager.h"
#include "ecs/components.h"
#include "systems/render_system.h"

// Performance benchmarks for engine components
namespace Astral {
namespace Benchmark {

// EntityManager benchmarks
static void BM_EntityManager_AddEntities(benchmark::State& state) {
    EntityManager em;

    for (auto _ : state) {
        benchmark::DoNotOptimize(em.addEntity("bench_entity"));
    }

    em.clear();
}
BENCHMARK(BM_EntityManager_AddEntities);

static void BM_EntityManager_ViewComponents(benchmark::State& state) {
    EntityManager em;

    // Create test entities
    for (int i = 0; i < state.range(0); ++i) {
        auto entity = em.addEntity("test");
        entity->add<CTransform>(glm::vec3(i, i, i));
        if (i % 2 == 0) {
            entity->add<CMesh>("cube", "material");
        }
    }
    em.update(); // Add entities

    for (auto _ : state) {
        auto entities = em.view<CTransform>();
        benchmark::DoNotOptimize(entities);
    }
}
BENCHMARK(BM_EntityManager_ViewComponents)->Range(100, 10000);

static void BM_EntityManager_ViewMultipleComponents(benchmark::State& state) {
    EntityManager em;

    // Create test entities
    for (int i = 0; i < state.range(0); ++i) {
        auto entity = em.addEntity("test");
        entity->add<CTransform>(glm::vec3(i, i, i));
        entity->add<CMesh>("cube", "material");
    }
    em.update();

    for (auto _ : state) {
        auto entities = em.view<CTransform, CMesh>();
        benchmark::DoNotOptimize(entities);
    }
}
BENCHMARK(BM_EntityManager_ViewMultipleComponents)->Range(100, 10000);

// GLM benchmarks (using GLM SIMD)
static void BM_GLM_MatrixMultiply(benchmark::State& state) {
    glm::mat4 a = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
    glm::mat4 b = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 3));

    for (auto _ : state) {
        glm::mat4 result = a * b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_GLM_MatrixMultiply);

static void BM_GLM_VectorOperations(benchmark::State& state) {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);

    for (auto _ : state) {
        glm::vec3 result = a + b;
        result = glm::normalize(result);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_GLM_VectorOperations);

} // namespace Benchmark
} // namespace Astral
