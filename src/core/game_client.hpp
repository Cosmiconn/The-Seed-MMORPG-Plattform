#pragma once
#include "core/net.hpp"
#include "core/renderer.hpp"
#include "core/serialize.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace TheSeed::Game {

// Client: Empfängt State, rendert Cubes
class GameClient {
public:
    GameClient();
    ~GameClient();

    bool Connect(const std::string& host, uint16_t port);
    void Disconnect();

    void Tick(float dt);
    bool IsConnected() const;

    // Rendering (called after BeginFrame)
    void RenderFrame(Render::VulkanRenderer& renderer);

    uint32_t GetLocalId() const;

private:
    std::unique_ptr<Net::IClient> m_client;

    struct RemoteEntity {
        uint32_t netId;
        float x, y, z;
        float interpX, interpY, interpZ;
    };
    std::unordered_map<uint32_t, RemoteEntity> m_entities;
    std::vector<uint8_t> m_lastState;

    void ProcessPacket(std::span<const uint8_t> data);
    void Interpolate(float dt);
};

} // namespace TheSeed::Game
