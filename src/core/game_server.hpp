#pragma once
#include "core/ecs.hpp"
#include "core/net.hpp"
#include "core/events.hpp"
#include "core/serialize.hpp"
#include "core/jobs.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace TheSeed::Game {

struct Transform {
    float x, y, z;
    float qx, qy, qz, qw;
};

struct Velocity {
    float vx, vy, vz;
};

struct NetSync {
    uint32_t netId;
    bool dirty = true;
};

// Server: Authoritative Simulation
class GameServer {
public:
    GameServer();
    ~GameServer();

    bool Start(uint16_t port, uint32_t maxClients = 64);
    void Stop();

    void FixedTick(float dt);   // 60 Hz physics + replication
    void Tick(float dt);        // Event processing

    void SpawnCube(float x, float y, float z);
    size_t EntityCount() const;
    uint32_t ClientCount() const;

private:
    std::unique_ptr<Net::IServer> m_server;
    std::unique_ptr<ECS::World> m_world;
    std::unique_ptr<Events::EventBus> m_events;
    std::unique_ptr<Jobs::JobSystem> m_jobs;

    uint32_t m_nextNetId = 1;
    float m_accumulator = 0.0f;
    const float m_fixedDt = 1.0f / 60.0f;

    std::vector<uint8_t> m_lastState;
    std::vector<uint8_t> m_currentState;

    void OnClientConnect(Net::IConnection* conn);
    void OnClientDisconnect(Net::IConnection* conn);
    void ReplicateState();
    void SerializeWorld(std::vector<uint8_t>& out);
    void ApplyInput(uint32_t clientId, std::span<const uint8_t> data);
};

} // namespace TheSeed::Game
