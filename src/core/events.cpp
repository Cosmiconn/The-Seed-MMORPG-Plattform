#include "core/events.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace TheSeed::Events {

void EventBus::Subscribe(EventType type, Handler handler) {
    m_handlers[type].push_back(handler);
}

void EventBus::Subscribe(std::vector<EventType> types, Handler handler) {
    for (auto t : types) {
        Subscribe(t, handler);
    }
}

void EventBus::Publish(const GameEvent& evt) {
    auto it = m_handlers.find(evt.type);
    if (it != m_handlers.end()) {
        for (auto& handler : it->second) {
            try {
                handler(evt);
            } catch (const std::exception& ex) {
                spdlog::error("Event handler exception: {}", ex.what());
            }
        }
    }
}

void EventBus::PublishAsync(const GameEvent& evt) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_asyncQueue.push_back(evt);
}

void EventBus::Tick() {
    std::vector<GameEvent> toProcess;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        toProcess = std::move(m_asyncQueue);
        m_asyncQueue.clear();
    }
    for (auto& evt : toProcess) {
        Publish(evt);
    }
}

} // namespace TheSeed::Events
