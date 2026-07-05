#include <catch2/catch_test_macros.hpp>
#include "core/net.hpp"
#include <thread>
#include <chrono>

using namespace TheSeed::Net;

TEST_CASE("Net: Server Start/Stop", "[net]") {
    auto server = CreateServer();
    REQUIRE(server->Start(7777, 64));
    server->Tick();
    server->Stop();
}

TEST_CASE("Net: Client Connect", "[net]") {
    auto server = CreateServer();
    REQUIRE(server->Start(7778, 64));

    auto client = CreateClient();
    REQUIRE(client->Connect("127.0.0.1", 7778));
    REQUIRE(client->IsConnected());

    // Kurze Zeit für Verbindungsaufbau
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->Tick();

    client->Disconnect();
    server->Stop();
}

TEST_CASE("Net: Send/Receive", "[net]") {
    auto server = CreateServer();
    REQUIRE(server->Start(7779, 64));

    auto client = CreateClient();
    REQUIRE(client->Connect("127.0.0.1", 7779));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server->Tick();

    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    client->Send(payload, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server->Tick();

    auto conns = server->GetConnections();
    REQUIRE(conns.size() >= 1);

    std::vector<uint8_t> received;
    REQUIRE(conns[0]->Receive(received));
    REQUIRE(received.size() == 4);
    REQUIRE(received[0] == 0x01);

    client->Disconnect();
    server->Stop();
}
