# TheSeed – Projekt-Setup (Windows 11)

## Voraussetzungen

- Windows 11 (22H2+)
- Visual Studio 2022 (C++23 Workload)
- Git
- CMake 3.28+
- vcpkg (in `C:\devcpkg` empfohlen)
- Vulkan SDK (LunarG)

## 1. Verzeichnisstruktur

```
TheSeed/
├── .github/
│   └── workflows/
│       └── build.yml
├── cmake/
│   └── modules/
│       ├── FindJoltPhysics.cmake
│       └── CompilerWarnings.cmake
├── docs/
├── src/
│   ├── core/
│   │   ├── ecs.hpp
│   │   ├── net.hpp
│   │   ├── modules.hpp
│   │   ├── events.hpp
│   │   ├── serialize.hpp
│   │   ├── ecs.cpp
│   │   ├── net.cpp
│   │   └── modules.cpp
│   ├── editor/
│   │   └── main.cpp
│   ├── server/
│   │   └── main.cpp
│   └── modules/
│       └── hello_world/
│           └── hello_world.cpp
├── tests/
│   └── test_ecs.cpp
├── third_party/          # (optional, falls vcpkg nicht reicht)
├── CMakeLists.txt
├── vcpkg.json
├── vcpkg-configuration.json
└── README.md
```

## 2. `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28)
project(TheSeed VERSION 0.1.0 LANGUAGES CXX)

# --- C++23 Standard ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- vcpkg Toolchain ---
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "vcpkg toolchain")
endif()

# --- Compiler-Flags (Windows) ---
if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus /MP)
    add_compile_options(/wd4267 /wd4244) # Size-Cast Warnings (vorerst)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Find Packages ---
find_package(Vulkan REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# --- Core Library ---
add_library(TheSeedCore STATIC)
target_sources(TheSeedCore
    PRIVATE
        src/core/ecs.cpp
        src/core/net.cpp
        src/core/modules.cpp
        src/core/events.cpp
        src/core/serialize.cpp
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS src
        FILES
            src/core/ecs.ixx      # C++23 Modules (optional, sonst .hpp)
            src/core/net.ixx
            src/core/modules.ixx
)
target_include_directories(TheSeedCore PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(TheSeedCore PUBLIC 
    Vulkan::Vulkan 
    spdlog::spdlog 
    yaml-cpp 
    glm::glm
)
target_compile_features(TheSeedCore PUBLIC cxx_std_23)

# --- Editor Executable ---
add_executable(TheSeedEditor src/editor/main.cpp)
target_link_libraries(TheSeedEditor PRIVATE TheSeedCore glfw imgui::imgui)
set_target_properties(TheSeedEditor PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# --- Server Executable ---
add_executable(TheSeedServer src/server/main.cpp)
target_link_libraries(TheSeedServer PRIVATE TheSeedCore)

# --- Hello World Module (DLL) ---
add_library(HelloWorldModule SHARED src/modules/hello_world/hello_world.cpp)
target_link_libraries(HelloWorldModule PRIVATE TheSeedCore)
set_target_properties(HelloWorldModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
)

# --- Tests ---
enable_testing()
add_executable(TheSeedTests 
    tests/test_ecs.cpp
    tests/test_modules.cpp
)
target_link_libraries(TheSeedTests PRIVATE TheSeedCore Catch2::Catch2)
include(Catch)
catch_discover_tests(TheSeedTests)

# --- Install ---
install(TARGETS TheSeedCore TheSeedEditor TheSeedServer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
```

## 3. `vcpkg.json`

```json
{
  "name": "theseed",
  "version": "0.1.0",
  "description": "TheSeed MMORPG Engine & Platform",
  "license": "MIT",
  "dependencies": [
    "vulkan",
    "spdlog",
    "fmt",
    "yaml-cpp",
    "catch2",
    "glm",
    "glfw3",
    "imgui",
    "enet",
    "joltphysics",
    "stb",
    "tinygltf"
  ],
  "builtin-baseline": "2024.06.15"
}
```

## 4. `vcpkg-configuration.json`

```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/Microsoft/vcpkg",
    "baseline": "2024.06.15"
  }
}
```

## 5. `.github/workflows/build.yml`

```yaml
name: Build & Test

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build-windows:
    runs-on: windows-latest
    env:
      VCPKG_ROOT: ${{ github.workspace }}/vcpkg
    steps:
      - uses: actions/checkout@v4

      - name: Setup vcpkg
        uses: lukka/run-vcpkg@v11
        with:
          vcpkgGitCommitId: '2024.06.15'

      - name: Configure CMake
        run: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Test
        working-directory: build
        run: ctest -C Release --output-on-failure

      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: theseed-windows
          path: |
            build/Release/TheSeedEditor.exe
            build/Release/TheSeedServer.exe
            build/modules/
```

## 6. `src/modules/hello_world/hello_world.cpp`

```cpp
#include "core/modules.hpp"
#include <spdlog/spdlog.h>

namespace TheSeed::Modules {

class HelloWorldModule : public IModule {
    bool m_initialized = false;

public:
    const char* GetName() const override { return "HelloWorld"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetAuthor() const override { return "Cosmiconn"; }

    bool Initialize(const ModuleConfig& config) override {
        m_initialized = true;
        spdlog::info("[{}] Initialized! Config: name={}", GetName(), config.GetString("greeting", "Hello"));
        return true;
    }

    void PostInitialize() override {
        spdlog::info("[{}] All modules loaded. Ready.", GetName());
    }

    void Shutdown() override {
        spdlog::info("[{}] Shutdown", GetName());
        m_initialized = false;
    }

    void Tick(float deltaTime) override {
        // Nur jede 60 Sekunden loggen
        static float accumulator = 0.0f;
        accumulator += deltaTime;
        if (accumulator >= 60.0f) {
            spdlog::debug("[{}] Heartbeat ({}s)", GetName(), accumulator);
            accumulator = 0.0f;
        }
    }

    void FixedTick(float fixedDelta) override {
        // 60 Hz Server-Logik (leer für Demo)
    }

    void Serialize(ISerializer& ser) override {
        ser.WriteString("HelloWorldState");
    }

    void Deserialize(IDeserializer& de) override {
        auto state = de.ReadString();
        spdlog::info("[{}] Deserialized: {}", GetName(), state);
    }

    void OnHotReload() override {
        spdlog::info("[{}] Hot Reload triggered!", GetName());
    }

    void OnEvent(const GameEvent& evt) override {
        if (evt.type == EventType::PlayerJoined) {
            spdlog::info("[{}] Welcome, Player {}!", GetName(), evt.senderId);
        }
    }
};

// DLL-Export
THESEED_MODULE(HelloWorldModule, "HelloWorld", "1.0.0")

} // namespace TheSeed::Modules
```

## 7. `src/server/main.cpp` (Minimal-Server)

```cpp
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
        return 1;
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

    loader.Unload(result.value());
    return 0;
}
```

## 8. Build-Anleitung (PowerShell)

```powershell
# 1. vcpkg Setup (einmalig)
git clone https://github.com/Microsoft/vcpkg.git C:\devcpkg
C:\devcpkgootstrap-vcpkg.bat
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\devcpkg", "User")

# 2. Projekt bauen
cd C:\dev\TheSeed
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

# 3. Server starten
.uild\Release\TheSeedServer.exe

# 4. Tests
ctest --test-dir build --output-on-failure
```

## 9. Nächste Schritte

1. `test_ecs.cpp` schreiben (100k Entities Benchmark)
2. Vulkan-Renderer POC (Dreieck → Cube → Mesh)
3. Netzwerk-Client bauen (verbindet sich mit Server)
4. Erstes echtes Modul: `InventoryModule`
