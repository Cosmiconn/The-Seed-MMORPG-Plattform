#pragma once
#include <string>
#include <vector>
#include <functional>
#include <any>
#include <typeindex>
#include <unordered_map>
#include <cstdint>
#include <mutex>  // <-- FIX: std::mutex

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
    uint32_t senderId;
    double timestamp;
    std::any payload;
};

class EventBus {
public:
    using Handler = std::function<void(const GameEvent&)>;

    void Subscribe(EventType type, Handler handler);
    void Subscribe(std::vector<EventType> types, Handler handler);

    void Publish(const GameEvent& evt);
    void PublishAsync(const GameEvent& evt);

    void Tick();

private:
    std::unordered_map<EventType, std::vector<Handler>> m_handlers;
    std::vector<GameEvent> m_asyncQueue;
    std::mutex m_mutex;  // <-- FIX: jetzt mit #include <mutex>
};

} // namespace TheSeed::Events
