#include "core/modules.hpp"
#include "core/serialize.hpp"
#include "core/events.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace TheSeed::Modules {

class HelloWorldModule : public IModule {
    bool m_initialized = false;

public:
    const char* GetName() const override { return "HelloWorld"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetAuthor() const override { return "Cosmiconn"; }

    bool Initialize(const ModuleConfig& config) override {
        m_initialized = true;
        spdlog::info(fmt::format("[{}] Initialized! Config: name={}", GetName(), config.GetString("greeting", "Hello")));
        return true;
    }

    void PostInitialize() override {
        spdlog::info(fmt::format("[{}] All modules loaded. Ready.", GetName()));
    }

    void Shutdown() override {
        spdlog::info(fmt::format("[{}] Shutdown", GetName()));
        m_initialized = false;
    }

    void Tick(float deltaTime) override {
        static float accumulator = 0.0f;
        accumulator += deltaTime;
        if (accumulator >= 60.0f) {
            spdlog::debug(fmt::format("[{}] Heartbeat ({}s)", GetName(), accumulator));
            accumulator = 0.0f;
        }
    }

    void FixedTick(float fixedDelta) override {
        // 60 Hz Server-Logik (leer fuer Demo)
        (void)fixedDelta;
    }

    void Serialize(ISerializer& ser) override {
        ser.WriteString("HelloWorldState");
    }

    void Deserialize(IDeserializer& de) override {
        auto state = de.ReadString();
        spdlog::info(fmt::format("[{}] Deserialized: {}", GetName(), state));
    }

    void OnHotReload() override {
        spdlog::info(fmt::format("[{}] Hot Reload triggered!", GetName()));
    }

    void OnEvent(const GameEvent& evt) override {
        if (evt.type == EventType::PlayerJoined) {
            spdlog::info(fmt::format("[{}] Welcome, Player {}!", GetName(), evt.senderId));
        }
    }
};

THESEED_MODULE(HelloWorldModule, "HelloWorld", "1.0.0")

} // namespace TheSeed::Modules
