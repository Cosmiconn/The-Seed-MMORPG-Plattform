#include "core/modules.hpp"
#include "core/net.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

using namespace TheSeed;

int main() {
    spdlog::info("=== TheSeed Server v0.1.0 ===");

    // Modul-Loader initialisieren
    Modules::ModuleLoader loader;

    // HelloWorld-Modul laden
    Modules::ModuleConfig cfg;
    cfg.name = "HelloWorld";
    cfg.parameters["greeting"] = "Willkommen auf TheSeed!";

    auto result = loader.Load("modules/HelloWorldModule.dll", cfg);
    if (!result) {
        spdlog::error("Modul-Ladefehler: {}", result.error());
        // Server läuft trotzdem (graceful degradation)
    } else {
        spdlog::info("Modul erfolgreich geladen: {}", result.value()->GetName());
    }

    // Netzwerk starten
    auto server = Net::CreateServer();
    if (!server->Start(7777, 64)) {
        spdlog::error("Server konnte nicht starten!");
        return 1;
    }

    server->OnConnect([](Net::IConnection* conn) {
        spdlog::info("Client verbunden: ID={}", conn->GetId());
    });

    server->OnDisconnect([](Net::IConnection* conn) {
        spdlog::info("Client getrennt: ID={}", conn->GetId());
    });

    // Game Loop (60 Hz)
    spdlog::info("Server läuft auf Port 7777...");
    auto lastTime = std::chrono::steady_clock::now();
    const float fixedDt = 1.0f / 60.0f;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        server->Tick();
        loader.TickAll(dt);
        loader.FixedTickAll(fixedDt);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (result) {
        loader.Unload(result.value());
    }
    return 0;
}
