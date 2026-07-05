#include "core/modules.hpp"
#include "core/events.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace TheSeed;

int main() {
    spdlog::info("=== TheSeed Editor v0.1.0 ===");
    spdlog::info("Editor bereit. ESC + Enter zum Beenden.");

    Modules::ModuleLoader loader;
    Events::EventBus eventBus;

    // Versuche HelloWorld-Modul zu laden (falls verfügbar)
    Modules::ModuleConfig cfg;
    cfg.name = "HelloWorld";
    cfg.parameters["greeting"] = "Editor-Modus";

    auto modPath = "modules/HelloWorldModule.dll";
    auto result = loader.Load(modPath, cfg);
    if (result) {
        spdlog::info("Editor: Modul {} geladen.", result.value()->GetName());
    } else {
        spdlog::warn("Editor: Modul nicht geladen: {}", result.error());
    }

    // Simple Loop
    auto lastTime = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        loader.TickAll(dt);
        eventBus.Tick();

        if (std::cin.peek() == 27) { // ESC
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (result) {
        loader.Unload(result.value());
    }

    spdlog::info("Editor beendet.");
    return 0;
}
