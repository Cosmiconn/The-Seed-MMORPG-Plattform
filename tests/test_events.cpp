#include <catch2/catch_test_macros.hpp>
#include "core/events.hpp"

using namespace TheSeed::Events;

TEST_CASE("Events: Subscribe and Publish", "[events]") {
    EventBus bus;
    bool received = false;
    uint32_t receivedId = 0;

    bus.Subscribe(EventType::PlayerJoined, [&](const GameEvent& evt) {
        received = true;
        receivedId = evt.senderId;
    });

    GameEvent evt;
    evt.type = EventType::PlayerJoined;
    evt.senderId = 42;
    evt.timestamp = 0.0;

    bus.Publish(evt);

    REQUIRE(received);
    REQUIRE(receivedId == 42);
}

TEST_CASE("Events: Async Publish", "[events]") {
    EventBus bus;
    bool received = false;

    bus.Subscribe(EventType::ChatMessage, [&](const GameEvent& evt) {
        received = true;
    });

    GameEvent evt;
    evt.type = EventType::ChatMessage;
    evt.senderId = 1;
    evt.timestamp = 0.0;

    bus.PublishAsync(evt);
    REQUIRE(!received); // Noch nicht verarbeitet

    bus.Tick();
    REQUIRE(received);
}

TEST_CASE("Events: Multiple Handlers", "[events]") {
    EventBus bus;
    int count = 0;

    bus.Subscribe(EventType::CombatHit, [&](const GameEvent&) { count++; });
    bus.Subscribe(EventType::CombatHit, [&](const GameEvent&) { count++; });

    GameEvent evt{EventType::CombatHit, 0, 0.0, {}};
    bus.Publish(evt);

    REQUIRE(count == 2);
}
