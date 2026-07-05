#include <catch2/catch_test_macros.hpp>
#include "core/ecs.hpp"

using namespace TheSeed::ECS;

struct Transform {
    float x, y, z;
};

struct Health {
    float current, max;
};

TEST_CASE("ECS: Entity Create/Destroy", "[ecs]") {
    World world;
    auto e = world.CreateEntity();
    REQUIRE(e != 0);
    REQUIRE(world.IsAlive(e));
    REQUIRE(world.EntityCount() == 1);

    world.DestroyEntity(e);
    REQUIRE(!world.IsAlive(e));
    REQUIRE(world.EntityCount() == 0);
}

TEST_CASE("ECS: Component Add/Get/Remove", "[ecs]") {
    World world;
    auto e = world.CreateEntity();

    auto& t = world.AddComponent<Transform>(e, 1.0f, 2.0f, 3.0f);
    REQUIRE(t.x == 1.0f);
    REQUIRE(t.y == 2.0f);
    REQUIRE(t.z == 3.0f);

    auto* t2 = world.GetComponent<Transform>(e);
    REQUIRE(t2 != nullptr);
    REQUIRE(t2->x == 1.0f);

    auto& h = world.AddComponent<Health>(e, 100.0f, 100.0f);
    REQUIRE(h.current == 100.0f);

    REQUIRE(world.HasComponent<Transform>(e));
    REQUIRE(world.HasComponent<Health>(e));

    world.RemoveComponent<Transform>(e);
    REQUIRE(!world.HasComponent<Transform>(e));
    REQUIRE(world.HasComponent<Health>(e));
}

TEST_CASE("ECS: Query", "[ecs]") {
    World world;
    auto e1 = world.CreateEntity();
    world.AddComponent<Transform>(e1, 0.0f, 0.0f, 0.0f);
    world.AddComponent<Health>(e1, 100.0f, 100.0f);

    auto e2 = world.CreateEntity();
    world.AddComponent<Transform>(e2, 1.0f, 1.0f, 1.0f);

    auto results = world.Query<Transform, Health>();
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == e1);
}

TEST_CASE("ECS: Entity Version Recycling", "[ecs]") {
    World world;
    auto e1 = world.CreateEntity();
    uint32_t id = EntityId(e1);
    world.DestroyEntity(e1);

    auto e2 = world.CreateEntity();
    REQUIRE(EntityId(e2) == id);
    REQUIRE(EntityVersion(e2) == EntityVersion(e1) + 1);
    REQUIRE(!world.IsAlive(e1));
    REQUIRE(world.IsAlive(e2));
}
