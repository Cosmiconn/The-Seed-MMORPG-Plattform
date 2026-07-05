#include <catch2/catch_test_macros.hpp>
#include "core/modules.hpp"
#include <filesystem>

using namespace TheSeed::Modules;

TEST_CASE("Modules: Config Defaults", "[modules]") {
    ModuleConfig cfg;
    cfg.parameters["test_int"] = "42";
    cfg.parameters["test_bool"] = "true";
    cfg.parameters["test_float"] = "3.14";
    cfg.parameters["test_str"] = "hello";

    REQUIRE(cfg.GetInt("test_int", 0) == 42);
    REQUIRE(cfg.GetBool("test_bool", false) == true);
    REQUIRE(cfg.GetFloat("test_float", 0.0f) == 3.14f);
    REQUIRE(cfg.GetString("test_str", "") == "hello");

    REQUIRE(cfg.GetInt("missing", 99) == 99);
    REQUIRE(cfg.GetBool("missing", true) == true);
}

TEST_CASE("Modules: Load/Unload HelloWorld", "[modules]") {
    ModuleLoader loader;

    ModuleConfig cfg;
    cfg.name = "HelloWorld";
    cfg.parameters["greeting"] = "Test";

    // Hinweis: Dieser Test funktioniert nur, wenn HelloWorldModule.dll existiert
    // Im CI-Build wird sie vor den Tests erstellt
    auto modPath = std::filesystem::path("modules") / "HelloWorldModule.dll";
    if (!std::filesystem::exists(modPath)) {
        // Linux-Fallback
        modPath = std::filesystem::path("modules") / "HelloWorldModule.so";
    }

    if (std::filesystem::exists(modPath)) {
        auto result = loader.Load(modPath, cfg);
        REQUIRE(result.has_value());
        REQUIRE(result.value() != nullptr);
        REQUIRE(std::string(result.value()->GetName()) == "HelloWorld");

        auto* mod = loader.GetModule("HelloWorld");
        REQUIRE(mod != nullptr);

        REQUIRE(loader.Unload(result.value()));
        REQUIRE(loader.GetModule("HelloWorld") == nullptr);
    }
}
