#include "core/net.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601  // Windows 7+
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_TYPE SOCKET
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET_TYPE int
    #define INVALID_SOCKET_VAL (-1)
    #define CLOSE_SOCKET close
#endif

namespace TheSeed::Net {

struct SocketAddr {
    sockaddr_in addr;
    bool operator==(const SocketAddr& other) const {
        return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr &&
               addr.sin_port == other.addr.sin_port;
    }
};

struct SocketAddrHash {
    size_t operator()(const SocketAddr& s) const {
        return std::hash<uint32_t>{}(s.addr.sin_addr.s_addr) ^
               (std::hash<uint16_t>{}(s.addr.sin_port) << 1);
    }
};

class UDPConnection : public IConnection {
public:
    UDPConnection(uint32_t id, SOCKET_TYPE socket, const SocketAddr& addr)
        : m_id(id), m_socket(socket), m_remoteAddr(addr), m_connected(true) {}

    void Send(std::span<const uint8_t> data, bool reliable) override {
        if (!m_connected) return;
        std::lock_guard<std::mutex> lock(m_sendMutex);

        PacketHeader header{};
        header.sequence = m_sendSequence++;
        header.payloadSize = static_cast<uint16_t>(data.size());
        header.channel = reliable ? 1 : 0;

        std::vector<uint8_t> packet(sizeof(PacketHeader) + data.size());
        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        std::memcpy(packet.data() + sizeof(PacketHeader), data.data(), data.size());

        sendto(m_socket, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
               reinterpret_cast<const sockaddr*>(&m_remoteAddr.addr), sizeof(m_remoteAddr.addr));
    }

    bool Receive(std::vector<uint8_t>& outData) override {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        if (m_recvQueue.empty()) return false;
        outData = m_recvQueue.front();
        m_recvQueue.pop();
        return true;
    }

    uint32_t GetId() const override { return m_id; }
    float GetLatency() const override { return m_rtt; }
    float GetPacketLoss() const override { return 0.0f; }

    void Disconnect() override {
        m_connected = false;
    }

    void OnData(std::function<void(std::span<const uint8_t>)> cb) override {
        m_dataCallback = cb;
    }

    void OnRawPacket(const std::vector<uint8_t>& payload) {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        m_recvQueue.push(payload);
        if (m_dataCallback) {
            m_dataCallback(payload);
        }
    }

    const SocketAddr& GetRemoteAddr() const { return m_remoteAddr; }
    bool IsConnected() const { return m_connected; }

private:
    uint32_t m_id;
    SOCKET_TYPE m_socket;
    SocketAddr m_remoteAddr;
    bool m_connected;
    uint32_t m_sendSequence = 1;
    float m_rtt = 0.0f;

    std::queue<std::vector<uint8_t>> m_recvQueue;
    std::mutex m_recvMutex;
    std::mutex m_sendMutex;
    std::function<void(std::span<const uint8_t>)> m_dataCallback;
};

class UDPServer : public IServer {
public:
    UDPServer() : m_socket(INVALID_SOCKET_VAL), m_nextId(1) {}

    bool Start(uint16_t port, uint32_t maxConnections) override {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            spdlog::error("WSAStartup failed");
            return false;
        }
#endif

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket == INVALID_SOCKET_VAL) {
            spdlog::error("Socket creation failed");
            return false;
        }

#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);
#else
        int flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            spdlog::error("Bind failed on port {}", port);
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET_VAL;
            return false;
        }

        m_running = true;
        spdlog::info("Server gestartet auf Port {} (max {} Verbindungen)", port, maxConnections);
        return true;
    }

    void Stop() override {
        m_running = false;
        if (m_socket != INVALID_SOCKET_VAL) {
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET_VAL;
        }
#ifdef _WIN32
        WSACleanup();
#endif
        spdlog::info("Server gestoppt");
    }

    void Tick() override {
        if (!m_running || m_socket == INVALID_SOCKET_VAL) return;

        char buffer[4096];
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);

        while (true) {
            int received = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                    reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
            if (received <= 0) break;

            SocketAddr saddr{fromAddr};

            std::shared_ptr<UDPConnection> conn;
            {
                std::lock_guard<std::mutex> lock(m_connMutex);
                auto it = m_addrToConn.find(saddr);
                if (it != m_addrToConn.end()) {
                    conn = it->second;
                } else {
                    uint32_t id = m_nextId++;
                    auto newConn = std::make_shared<UDPConnection>(id, m_socket, saddr);
                    conn = newConn;
                    m_connections.push_back(newConn);
                    m_addrToConn[saddr] = newConn;
                    m_idToConn[id] = newConn;

                    if (m_onConnect) {
                        m_onConnect(newConn.get());
                    }
                    char addrStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &fromAddr.sin_addr, addrStr, INET_ADDRSTRLEN);
                    spdlog::info("Client verbunden: ID={} von {}:{}", id,
                                 addrStr, ntohs(fromAddr.sin_port));
                }
            }

            if (received > static_cast<int>(sizeof(PacketHeader))) {
                size_t payloadSize = received - sizeof(PacketHeader);
                std::vector<uint8_t> payload(payloadSize);
                std::memcpy(payload.data(), buffer + sizeof(PacketHeader), payloadSize);
                conn->OnRawPacket(payload);
            }
        }
    }

    std::vector<IConnection*> GetConnections() const override {
        std::vector<IConnection*> result;
        std::lock_guard<std::mutex> lock(m_connMutex);
        for (auto& c : m_connections) {
            if (c->IsConnected()) result.push_back(c.get());
        }
        return result;
    }

    IConnection* GetConnection(uint32_t id) const override {
        std::lock_guard<std::mutex> lock(m_connMutex);
        auto it = m_idToConn.find(id);
        if (it != m_idToConn.end() && it->second->IsConnected()) return it->second.get();
        return nullptr;
    }

    void OnConnect(std::function<void(IConnection*)> cb) override { m_onConnect = cb; }
    void OnDisconnect(std::function<void(IConnection*)> cb) override { m_onDisconnect = cb; }

    void Broadcast(std::span<const uint8_t> data, bool reliable) override {
        std::lock_guard<std::mutex> lock(m_connMutex);
        for (auto& c : m_connections) {
            if (c->IsConnected()) {
                c->Send(data, reliable);
            }
        }
    }

private:
    SOCKET_TYPE m_socket;
    bool m_running = false;
    uint32_t m_nextId;

    mutable std::mutex m_connMutex;
    std::vector<std::shared_ptr<UDPConnection>> m_connections;
    std::unordered_map<SocketAddr, std::shared_ptr<UDPConnection>, SocketAddrHash> m_addrToConn;
    std::unordered_map<uint32_t, std::shared_ptr<UDPConnection>> m_idToConn;

    std::function<void(IConnection*)> m_onConnect;
    std::function<void(IConnection*)> m_onDisconnect;
};

class UDPClient : public IClient {
public:
    UDPClient() : m_socket(INVALID_SOCKET_VAL), m_localId(0), m_connected(false) {}

    bool Connect(const std::string& host, uint16_t port) override {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket == INVALID_SOCKET_VAL) return false;

        m_serverAddr.sin_family = AF_INET;
        m_serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &m_serverAddr.sin_addr);

#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);
#else
        int flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

        m_connected = true;
        spdlog::info("Client versucht Verbindung zu {}:{}", host, port);
        return true;
    }

    void Disconnect() override {
        m_connected = false;
        if (m_socket != INVALID_SOCKET_VAL) {
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET_VAL;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Tick() override {
        if (!m_connected || m_socket == INVALID_SOCKET_VAL) return;

        char buffer[4096];
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);

        while (true) {
            int received = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                    reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
            if (received <= 0) break;

            if (received > static_cast<int>(sizeof(PacketHeader))) {
                size_t payloadSize = received - sizeof(PacketHeader);
                std::vector<uint8_t> payload(payloadSize);
                std::memcpy(payload.data(), buffer + sizeof(PacketHeader), payloadSize);
                std::lock_guard<std::mutex> lock(m_recvMutex);
                m_recvQueue.push(payload);
            }
        }
    }

    bool IsConnected() const override { return m_connected; }

    void Send(std::span<const uint8_t> data, bool reliable) override {
        if (!m_connected) return;

        PacketHeader header{};
        header.sequence = m_sendSequence++;
        header.payloadSize = static_cast<uint16_t>(data.size());
        header.channel = reliable ? 1 : 0;

        std::vector<uint8_t> packet(sizeof(PacketHeader) + data.size());
        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        std::memcpy(packet.data() + sizeof(PacketHeader), data.data(), data.size());

        sendto(m_socket, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
               reinterpret_cast<const sockaddr*>(&m_serverAddr), sizeof(m_serverAddr));
    }

    bool Receive(std::vector<uint8_t>& outData) override {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        if (m_recvQueue.empty()) return false;
        outData = m_recvQueue.front();
        m_recvQueue.pop();
        return true;
    }

    float GetLatency() const override { return 0.0f; }
    uint32_t GetLocalId() const override { return m_localId; }

private:
    SOCKET_TYPE m_socket;
    sockaddr_in m_serverAddr{};
    uint32_t m_localId;
    bool m_connected;
    uint32_t m_sendSequence = 1;

    std::queue<std::vector<uint8_t>> m_recvQueue;
    std::mutex m_recvMutex;
};

std::unique_ptr<IServer> CreateServer() {
    return std::make_unique<UDPServer>();
}

std::unique_ptr<IClient> CreateClient() {
    return std::make_unique<UDPClient>();
}

} // namespace TheSeed::Net
