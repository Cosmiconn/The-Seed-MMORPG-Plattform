# TheSeed – Technische Architektur (C++23)

## 1. ECS-Design (Archetype-basiert)

```cpp
// src/core/ecs.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <span>
#include <type_traits>

namespace TheSeed::ECS {

// Packed Entity: 32-Bit ID + 32-Bit Version
using Entity = uint64_t;

inline constexpr uint32_t EntityId(Entity e)     { return static_cast<uint32_t>(e); }
inline constexpr uint32_t EntityVersion(Entity e)  { return static_cast<uint32_t>(e >> 32); }
inline constexpr Entity MakeEntity(uint32_t id, uint32_t ver) { return (static_cast<uint64_t>(ver) << 32) | id; }

// Component-Constraint (C++20 Concepts)
template<typename T>
concept Component = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// Archetype = eindeutige Kombination aus Komponenten-Typen
class Archetype {
public:
    explicit Archetype(std::vector<std::type_index> types);

    template<Component T>
    T* GetComponent(Entity e);

    template<Component T, typename... Args>
    T& AddComponent(Entity e, Args&&... args);

    size_t EntityCount() const;
    size_t ChunkSize() const;

private:
    std::vector<std::type_index> m_types;
    std::vector<uint8_t> m_storage; // SoA-Layout
    std::unordered_map<uint32_t, size_t> m_entityToIndex;
};

// Query-Filter
struct QueryFilter {
    std::vector<std::type_index> all;      // Muss alle haben
    std::vector<std::type_index> any;    // Mindestens einer
    std::vector<std::type_index> none;   // Darf keinen haben
};

class World {
public:
    World();
    ~World();

    Entity CreateEntity();
    void DestroyEntity(Entity e);

    template<Component T, typename... Args>
    T& AddComponent(Entity e, Args&&... args);

    template<Component T>
    T* GetComponent(Entity e);

    template<Component T>
    void RemoveComponent(Entity e);

    template<Component... Cs>
    std::vector<Entity> Query();

    // System-Registrierung
    using SystemFunc = void(*)(World&, float);
    void RegisterSystem(SystemFunc sys, const char* name, int priority = 0);
    void Tick(float deltaTime);

    size_t EntityCount() const;

private:
    std::vector<std::unique_ptr<Archetype>> m_archetypes;
    std::vector<Entity> m_freeEntities;
    std::vector<SystemFunc> m_systems;
    uint32_t m_nextId = 1;
};

} // namespace TheSeed::ECS
```

## 2. Netzwerk-Protokoll (Custom UDP)

```cpp
// src/core/net.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <functional>
#include <string>
#include <memory>

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

    // Senden (reliable = true → garantierte Zustellung)
    virtual void Send(std::span<const uint8_t> data, bool reliable = false) = 0;

    // Empfangen (nicht-blockierend)
    virtual bool Receive(std::vector<uint8_t>& outData) = 0;

    // Metadaten
    virtual uint32_t GetId() const = 0;
    virtual float GetLatency() const = 0;      // RTT in ms
    virtual float GetPacketLoss() const = 0; // 0.0–1.0
    virtual void Disconnect() = 0;

    // Callbacks
    virtual void OnData(std::function<void(std::span<const uint8_t>)> cb) = 0;
};

// --- Server-Interface ---
class IServer {
public:
    virtual ~IServer() = default;

    virtual bool Start(uint16_t port, uint32_t maxConnections = 64) = 0;
    virtual void Stop() = 0;
    virtual void Tick() = 0; // Muss 60×/s aufgerufen werden

    virtual std::vector<IConnection*> GetConnections() const = 0;
    virtual IConnection* GetConnection(uint32_t id) const = 0;

    // Events
    virtual void OnConnect(std::function<void(IConnection*)> cb) = 0;
    virtual void OnDisconnect(std::function<void(IConnection*)> cb) = 0;

    // Broadcast an alle verbundenen Clients
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
    virtual uint32_t GetLocalId() const = 0; // Vom Server zugewiesen
};

// Factory-Funktionen (Implementierung in .cpp)
std::unique_ptr<IServer> CreateServer();
std::unique_ptr<IClient> CreateClient();

} // namespace TheSeed::Net
```

## 3. Modul-Schnittstelle (Data-Driven, Hot-Reload)

```cpp
// src/core/modules.hpp
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <expected>
#include <filesystem>
#include <unordered_map>

namespace TheSeed::Modules {

// --- Konfiguration (YAML/JSON) ---
struct ModuleConfig {
    std::string name;
    std::string version;
    std::filesystem::path assetPath;
    std::unordered_map<std::string, std::string> parameters;

    bool GetBool(const std::string& key, bool defaultVal = false) const;
    int GetInt(const std::string& key, int defaultVal = 0) const;
    float GetFloat(const std::string& key, float defaultVal = 0.0f) const;
    std::string GetString(const std::string& key, const std::string& defaultVal = "") const;
};

// --- Modul-Interface (pure virtual) ---
class IModule {
public:
    virtual ~IModule() = default;

    // Metadaten
    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
    virtual const char* GetAuthor() const = 0;

    // Lebenszyklus
    virtual bool Initialize(const ModuleConfig& config) = 0;
    virtual void PostInitialize() = 0; // Nachdem alle Module geladen sind
    virtual void Shutdown() = 0;

    // Game-Loop
    virtual void Tick(float deltaTime) = 0;
    virtual void FixedTick(float fixedDelta) = 0; // 60 Hz Server

    // Persistenz
    virtual void Serialize(class ISerializer& ser) = 0;
    virtual void Deserialize(class IDeserializer& de) = 0;

    // Hot-Reload
    virtual void OnHotReload() = 0;

    // Event-Subscription
    virtual void OnEvent(const struct GameEvent& evt) = 0;
};

// --- Export-Table (für DLLs) ---
using CreateModuleFunc  = IModule* (*)();
using DestroyModuleFunc = void (*)(IModule*);
using GetModuleInfoFunc = const char* (*)(); // JSON-String mit Metadaten

struct ModuleExportTable {
    const char*         apiVersion;   // "TheSeed.ModuleAPI.v1"
    CreateModuleFunc    create;
    DestroyModuleFunc   destroy;
    GetModuleInfoFunc   getInfo;
};

// --- Makro für DLL-Export ---
#ifdef _WIN32
    #define THESEED_EXPORT extern "C" __declspec(dllexport)
#else
    #define THESEED_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define THESEED_MODULE(className, modName, modVersion)     THESEED_EXPORT const char* GetModuleInfo() {         return "{\"name\":\"" modName "\",\"version\":\"" modVersion "\",\"api\":\"TheSeed.ModuleAPI.v1\"}";     }     THESEED_EXPORT TheSeed::Modules::IModule* CreateModule() {         return new className();     }     THESEED_EXPORT void DestroyModule(TheSeed::Modules::IModule* m) {         delete m;     }

// --- Modul-Loader ---
class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();

    // Lädt eine Modul-DLL/SO
    std::expected<IModule*, std::string> Load(const std::filesystem::path& path, const ModuleConfig& config);

    // Entlädt ein Modul (graceful)
    bool Unload(IModule* module);

    // Hot-Reload: DLL ersetzen, Zustand serialisieren/deserialisieren
    std::expected<IModule*, std::string> HotReload(IModule* module);

    // Zugriff
    IModule* GetModule(const std::string& name) const;
    std::vector<IModule*> GetAllModules() const;

    // Broadcast-Tick
    void TickAll(float deltaTime);
    void FixedTickAll(float fixedDelta);

private:
    struct ModuleHandle {
        std::filesystem::path path;
        IModule* instance;
        void* osHandle; // HMODULE / dlopen handle
        ModuleConfig config;
    };
    std::vector<std::unique_ptr<ModuleHandle>> m_modules;
    std::unordered_map<std::string, size_t> m_nameIndex;
};

} // namespace TheSeed::Modules
```

## 4. Serialisierung (Delta-Kompression)

```cpp
// src/core/serialize.hpp
#pragma once
#include <vector>
#include <cstdint>
#include <span>
#include <type_traits>

namespace TheSeed::Serialize {

class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual void Write(std::span<const uint8_t> data) = 0;
    virtual void WriteU32(uint32_t v) = 0;
    virtual void WriteF32(float v) = 0;
    virtual void WriteString(const std::string& s) = 0;
    virtual std::vector<uint8_t> Finalize() = 0;
};

class IDeserializer {
public:
    virtual ~IDeserializer() = default;
    virtual bool Read(std::span<uint8_t> out) = 0;
    virtual uint32_t ReadU32() = 0;
    virtual float ReadF32() = 0;
    virtual std::string ReadString() = 0;
};

// Delta-Kompression: Nur geänderte Bits senden
std::vector<uint8_t> ComputeDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> newState);
std::vector<uint8_t> ApplyDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> delta);

} // namespace TheSeed::Serialize
```

## 5. Event-Bus (Inter-Modular)

```cpp
// src/core/events.hpp
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <any>
#include <typeindex>

namespace TheSeed::Events {

enum class EventType : uint32_t {
    EntitySpawned,
    EntityDestroyed,
    ComponentChanged,
    QuestAccepted,
    QuestCompleted,
    CombatHit,
    InventoryChanged,
    ChatMessage,
    PlayerJoined,
    PlayerLeft,
    Custom = 0x10000
};

struct GameEvent {
    EventType type;
    uint32_t senderId;   // Entity-ID oder 0
    double timestamp;    // Server-Zeit
    std::any payload;    // Typ-sicher via std::any
};

class EventBus {
public:
    using Handler = std::function<void(const GameEvent&)>;

    // Subscribe mit Filter
    void Subscribe(EventType type, Handler handler);
    void Subscribe(std::vector<EventType> types, Handler handler);

    // Publish (synchron, sofortige Zustellung)
    void Publish(const GameEvent& evt);

    // Publish (asynchron, nächster Tick)
    void PublishAsync(const GameEvent& evt);

    // Verarbeitet async Queue
    void Tick();

private:
    std::unordered_map<EventType, std::vector<Handler>> m_handlers;
    std::vector<GameEvent> m_asyncQueue;
};

} // namespace TheSeed::Events
```
