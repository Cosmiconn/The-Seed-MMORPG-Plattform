#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <functional>
#include <string>
#include <memory>
#include <queue>
#include <mutex>

namespace TheSeed::Net {

// --- Protokoll-Header (16 Bytes, aligned) ---
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t sequence;      // Sequenznummer (für Reliable)
    uint32_t ack;           // Letztes empfangenes ACK
    uint32_t ackBits;       // 32-Bit ACK-History
    uint16_t payloadSize;   // Größe des Payloads
    uint8_t  channel;       // 0=Unreliable, 1=Reliable, 2=Control
    uint8_t  flags;         // Komprimierung, Fragmentierung
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 16, "PacketHeader muss 16 Bytes sein");

// --- Verbindungs-Interface ---
class IConnection {
public:
    virtual ~IConnection() = default;

    virtual void Send(std::span<const uint8_t> data, bool reliable = false) = 0;
    virtual bool Receive(std::vector<uint8_t>& outData) = 0;
    virtual uint32_t GetId() const = 0;
    virtual float GetLatency() const = 0;
    virtual float GetPacketLoss() const = 0;
    virtual void Disconnect() = 0;
    virtual void OnData(std::function<void(std::span<const uint8_t>)> cb) = 0;
};

// --- Server-Interface ---
class IServer {
public:
    virtual ~IServer() = default;

    virtual bool Start(uint16_t port, uint32_t maxConnections = 64) = 0;
    virtual void Stop() = 0;
    virtual void Tick() = 0;
    virtual std::vector<IConnection*> GetConnections() const = 0;
    virtual IConnection* GetConnection(uint32_t id) const = 0;
    virtual void OnConnect(std::function<void(IConnection*)> cb) = 0;
    virtual void OnDisconnect(std::function<void(IConnection*)> cb) = 0;
    virtual void Broadcast(std::span<const uint8_t> data, bool reliable = false) = 0;
};

// --- Client-Interface ---
class IClient {
public:
    virtual ~IClient() = default;

    virtual bool Connect(const std::string& host, uint16_t port) = 0;
    virtual void Disconnect() = 0;
    virtual void Tick() = 0;
    virtual bool IsConnected() const = 0;
    virtual void Send(std::span<const uint8_t> data, bool reliable = false) = 0;
    virtual bool Receive(std::vector<uint8_t>& outData) = 0;
    virtual float GetLatency() const = 0;
    virtual uint32_t GetLocalId() const = 0;
};

// Factory-Funktionen
std::unique_ptr<IServer> CreateServer();
std::unique_ptr<IClient> CreateClient();

} // namespace TheSeed::Net
