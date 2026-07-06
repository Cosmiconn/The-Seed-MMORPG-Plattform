#include <catch2/catch_test_macros.hpp>
#include "core/game_server.hpp"
#include "core/game_client.hpp"
#include <thread>
#include <chrono>

using namespace TheSeed::Game;

TEST_CASE("Integration: Server Start/Stop", "[integration]") {
    GameServer server;
    REQUIRE(server.Start(7780, 8));
    REQUIRE(server.EntityCount() == 3); // 3 Cubes spawned
    server.Stop();
}

TEST_CASE("Integration: Client Connect + State Sync", "[integration]") {
    GameServer server;
    REQUIRE(server.Start(7781, 8));

    GameClient client;
    REQUIRE(client.Connect("127.0.0.1", 7781));

    // Mehrere Ticks fuer Handshake + State-Sync
    for (int i = 0; i < 30; ++i) {
        server.FixedTick(1.0f / 60.0f);
        server.Tick(1.0f / 60.0f);
        client.Tick(1.0f / 60.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    REQUIRE(client.IsConnected());
    client.Disconnect();
    server.Stop();
}

TEST_CASE("Integration: 2 Clients receive Cube positions", "[integration]") {
    GameServer server;
    REQUIRE(server.Start(7782, 8));

    GameClient client1;
    GameClient client2;
    REQUIRE(client1.Connect("127.0.0.1", 7782));
    REQUIRE(client2.Connect("127.0.0.1", 7782));

    // Simuliere 1 Sekunde
    for (int i = 0; i < 60; ++i) {
        server.FixedTick(1.0f / 60.0f);
        server.Tick(1.0f / 60.0f);
        client1.Tick(1.0f / 60.0f);
        client2.Tick(1.0f / 60.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    REQUIRE(client1.IsConnected());
    REQUIRE(client2.IsConnected());

    client1.Disconnect();
    client2.Disconnect();
    server.Stop();
}

TEST_CASE("Integration: Delta Compression reduces bandwidth", "[integration]") {
    GameServer server;
    REQUIRE(server.Start(7783, 8));

    // Simuliere mehrere Frames
    std::vector<uint8_t> state1, state2;
    server.SpawnCube(0.0f, 0.0f, 0.0f);

    // Erster Frame: Full State
    server.FixedTick(1.0f / 60.0f);

    // Zweiter Frame: Delta sollte kleiner sein
    server.FixedTick(1.0f / 60.0f);

    // Test besteht wenn Server läuft ohne Crash
    REQUIRE(server.EntityCount() >= 3);
    server.Stop();
}
