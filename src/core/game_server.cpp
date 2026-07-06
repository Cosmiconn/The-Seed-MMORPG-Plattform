#include "core/game_server.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cmath>

namespace TheSeed::Game {

// Register ECS components as trivial types
static_assert(std::is_trivially_copyable_v<Transform>);
static_assert(std::is_trivially_copyable_v<Velocity>);
static_assert(std::is_trivially_copyable_v<NetSync>);

GameServer::GameServer()
    : m_server(Net::CreateServer())
    , m_world(std::make_unique<ECS::World>())
    , m_events(std::make_unique<Events::EventBus>())
    , m_jobs(std::make_unique<Jobs::JobSystem>())
{
    m_jobs->Initialize();
}

GameServer::~GameServer() {
    Stop();
}

bool GameServer::Start(uint16_t port, uint32_t maxClients) {
    if (!m_server->Start(port, maxClients)) {
        spdlog::error("GameServer: Konnte nicht auf Port {} starten", port);
        return false;
    }

    m_server->OnConnect([this](Net::IConnection* conn) { OnClientConnect(conn); });
    m_server->OnDisconnect([this](Net::IConnection* conn) { OnClientDisconnect(conn); });

    // Spawn 3 Cubes als Demo
    SpawnCube(-1.0f, 0.0f, 0.0f);
    SpawnCube(0.0f, 0.0f, 0.0f);
    SpawnCube(1.0f, 0.0f, 0.0f);

    spdlog::info("GameServer gestartet auf Port {} mit {} Cubes", port, EntityCount());
    return true;
}

void GameServer::Stop() {
    m_server->Stop();
    m_jobs->Shutdown();
    spdlog::info("GameServer gestoppt");
}

void GameServer::SpawnCube(float x, float y, float z) {
    auto e = m_world->CreateEntity();

    // Transform
    auto& t = m_world->AddComponent<Transform>(e);
    t.x = x; t.y = y; t.z = z;
    t.qx = 0.0f; t.qy = 0.0f; t.qz = 0.0f; t.qw = 1.0f;

    // Velocity (orbital movement)
    auto& v = m_world->AddComponent<Velocity>(e);
    v.vx = 0.0f; v.vy = 1.0f; v.vz = 0.0f;

    // NetSync
    auto& ns = m_world->AddComponent<NetSync>(e);
    ns.netId = m_nextNetId++;
    ns.dirty = true;

    spdlog::debug("GameServer: Cube {} spawned bei ({}, {}, {})", ns.netId, x, y, z);
}

void GameServer::FixedTick(float dt) {
    m_accumulator += dt;

    while (m_accumulator >= m_fixedDt) {
        m_accumulator -= m_fixedDt;

        // 1. Netzwerk-Tick (empfange Inputs)
        m_server->Tick();

        // 2. Physics: Move cubes in circle
        auto entities = m_world->Query<Transform, Velocity, NetSync>();
        for (auto e : entities) {
            auto* t = m_world->GetComponent<Transform>(e);
            auto* v = m_world->GetComponent<Velocity>(e);
            auto* ns = m_world->GetComponent<NetSync>(e);
            if (!t || !v || !ns) continue;

            // Orbital movement
            float angle = m_fixedDt * 2.0f; // 2 rad/s
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            float newX = t->x * cosA - t->z * sinA;
            float newZ = t->x * sinA + t->z * cosA;
            t->x = newX;
            t->z = newZ;

            ns->dirty = true;
        }

        // 3. Replicate state to all clients
        ReplicateState();
    }
}

void GameServer::Tick(float dt) {
    (void)dt;
    m_events->Tick();
}

void GameServer::OnClientConnect(Net::IConnection* conn) {
    spdlog::info("GameServer: Client {} verbunden", conn->GetId());

    // Sende initialen World-State
    std::vector<uint8_t> state;
    SerializeWorld(state);
    conn->Send(state, true);
}

void GameServer::OnClientDisconnect(Net::IConnection* conn) {
    spdlog::info("GameServer: Client {} getrennt", conn->GetId());
}

void GameServer::ReplicateState() {
    SerializeWorld(m_currentState);

    // Delta-Kompression
    auto delta = Serialize::ComputeDelta(m_lastState, m_currentState);
    m_lastState = m_currentState;

    if (delta.size() > 8) { // Nur senden wenn sich was geändert hat
        m_server->Broadcast(delta, false); // Unreliable für Position-Updates
    }
}

void GameServer::SerializeWorld(std::vector<uint8_t>& out) {
    Serialize::BinarySerializer ser;

    auto entities = m_world->Query<Transform, NetSync>();
    ser.WriteU32(static_cast<uint32_t>(entities.size()));

    for (auto e : entities) {
        auto* t = m_world->GetComponent<Transform>(e);
        auto* ns = m_world->GetComponent<NetSync>(e);
        if (!t || !ns) continue;

        ser.WriteU32(ns->netId);
        ser.WriteF32(t->x);
        ser.WriteF32(t->y);
        ser.WriteF32(t->z);
    }

    out = ser.Finalize();
}

void GameServer::ApplyInput(uint32_t clientId, std::span<const uint8_t> data) {
    (void)clientId;
    (void)data;
    // TODO: Player input handling (WASD, etc.)
}

size_t GameServer::EntityCount() const {
    return m_world->EntityCount();
}

uint32_t GameServer::ClientCount() const {
    return static_cast<uint32_t>(m_server->GetConnections().size());
}

} // namespace TheSeed::Game
