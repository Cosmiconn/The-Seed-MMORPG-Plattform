#include "core/game_client.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace TheSeed::Game {

GameClient::GameClient() : m_client(Net::CreateClient()) {}

GameClient::~GameClient() {
    Disconnect();
}

bool GameClient::Connect(const std::string& host, uint16_t port) {
    if (!m_client->Connect(host, port)) {
        spdlog::error("GameClient: Verbindung zu {}:{} fehlgeschlagen", host, port);
        return false;
    }

    spdlog::info("GameClient: Verbinde zu {}:{}...", host, port);
    return true;
}

void GameClient::Disconnect() {
    m_client->Disconnect();
    m_entities.clear();
}

void GameClient::Tick(float dt) {
    if (!m_client->IsConnected()) return;

    m_client->Tick();

    // Empfange State-Updates
    std::vector<uint8_t> packet;
    while (m_client->Receive(packet)) {
        ProcessPacket(packet);
    }

    // Interpolation
    Interpolate(dt);
}

void GameClient::ProcessPacket(std::span<const uint8_t> data) {
    if (data.size() < 8) {
        // Delta-komprimiert: versuche zu dekodieren
        auto decoded = Serialize::ApplyDelta(m_lastState, data);
        if (decoded.empty()) return;
        data = std::span<const uint8_t>(decoded.data(), decoded.size());
        m_lastState = std::move(decoded);
    } else {
        // Full state
        m_lastState.assign(data.begin(), data.end());
    }

    // Parse Binary
    try {
        Serialize::BinaryDeserializer de(std::span<const uint8_t>(m_lastState.data(), m_lastState.size()));
        uint32_t count = de.ReadU32();

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t netId = de.ReadU32();
            float x = de.ReadF32();
            float y = de.ReadF32();
            float z = de.ReadF32();

            auto it = m_entities.find(netId);
            if (it == m_entities.end()) {
                RemoteEntity re;
                re.netId = netId;
                re.x = x; re.y = y; re.z = z;
                re.interpX = x; re.interpY = y; re.interpZ = z;
                m_entities[netId] = re;
            } else {
                it->second.x = x;
                it->second.y = y;
                it->second.z = z;
            }
        }
    } catch (const std::exception& e) {
        spdlog::warn("GameClient: State-Parse fehlgeschlagen: {}", e.what());
    }
}

void GameClient::Interpolate(float dt) {
    const float speed = 10.0f; // Interpolation-Geschwindigkeit
    for (auto& [id, ent] : m_entities) {
        ent.interpX += (ent.x - ent.interpX) * speed * dt;
        ent.interpY += (ent.y - ent.interpY) * speed * dt;
        ent.interpZ += (ent.z - ent.interpZ) * speed * dt;
    }
}

void GameClient::RenderFrame(Render::VulkanRenderer& renderer) {
    for (auto& [id, ent] : m_entities) {
        Render::Mesh cube;
        float s = 0.15f; // Cube size
        cube.vertices = {
            { ent.interpX - s, ent.interpY - s, ent.interpZ + s,  1.0f, 0.2f, 0.2f },
            { ent.interpX + s, ent.interpY - s, ent.interpZ + s,  0.2f, 1.0f, 0.2f },
            { ent.interpX + s, ent.interpY + s, ent.interpZ + s,  0.2f, 0.2f, 1.0f },
            { ent.interpX - s, ent.interpY + s, ent.interpZ + s,  1.0f, 1.0f, 0.2f },
            { ent.interpX - s, ent.interpY - s, ent.interpZ - s,  1.0f, 0.2f, 1.0f },
            { ent.interpX + s, ent.interpY - s, ent.interpZ - s,  0.2f, 1.0f, 1.0f },
            { ent.interpX + s, ent.interpY + s, ent.interpZ - s,  1.0f, 1.0f, 1.0f },
            { ent.interpX - s, ent.interpY + s, ent.interpZ - s,  0.5f, 0.5f, 0.5f },
        };
        cube.indices = {
            0,1,2, 0,2,3, 4,6,5, 4,7,6,
            4,0,3, 4,3,7, 1,5,6, 1,6,2,
            3,2,6, 3,6,7, 4,5,1, 4,1,0,
        };
        renderer.SubmitMesh(cube);
    }
}

bool GameClient::IsConnected() const {
    return m_client->IsConnected();
}

uint32_t GameClient::GetLocalId() const {
    return m_client->GetLocalId();
}

} // namespace TheSeed::Game
