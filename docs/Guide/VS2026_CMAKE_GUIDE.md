# Visual Studio 18 2026 – CMake Integration Guide
**Für TheSeed Engine (C++23)**

---

## Fakten zu VS 2026

| Eigenschaft | Wert |
|-------------|------|
| **Produktname** | Visual Studio 2026 |
| **Interne Version** | 18.x (aktuell: 18.7.3) |
| **Release** | 11. November 2025 |
| **CMake Generator** | `Visual Studio 18 2026` |
| **Min. CMake** | 4.2.0 |
| **Default Toolset** | v145 |
| **C++ Standard** | C++23 (vollständig) |
| **Build Tools** | MSVC 14.51 |

---

## CMake Generator Syntax

### Kommandozeile

```powershell
# Standard (x64, v145 toolset)
cmake -G "Visual Studio 18 2026" -A x64 -B build -S .

# Mit explizitem Toolset
cmake -G "Visual Studio 18 2026" -A x64 -T v145 -B build -S .

# Mit x64 Host Tools (schnelleres Compilen)
cmake -G "Visual Studio 18 2026" -A x64 -T v145,host=x64 -B build -S .

# ARM64
ncmake -G "Visual Studio 18 2026" -A ARM64 -B build -S .
```

### CMakeLists.txt Erkennung

```cmake
# VS 2026 erkennen
if(CMAKE_GENERATOR MATCHES "Visual Studio 18 2026")
    message(STATUS "Building with Visual Studio 2026")
endif()

# Toolset Version
if(MSVC_TOOLSET_VERSION EQUAL 145)
    message(STATUS "Using v145 toolset")
endif()

# Compiler Version
if(MSVC_VERSION EQUAL 1950)
    message(STATUS "MSVC 14.51 (VS 2026)")
endif()
```

---

## CMakePresets.json (Empfohlen)

Erstelle `CMakePresets.json` im Projekt-Root:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "vs2026-base",
      "hidden": true,
      "generator": "Visual Studio 18 2026",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "installDir": "${sourceDir}/install/${presetName}",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "23",
        "CMAKE_CXX_STANDARD_REQUIRED": "ON",
        "CMAKE_CXX_EXTENSIONS": "OFF"
      }
    },
    {
      "name": "vs2026-x64-debug",
      "inherits": "vs2026-base",
      "architecture": {
        "value": "x64",
        "strategy": "set"
      },
      "toolset": {
        "value": "v145,host=x64",
        "strategy": "set"
      },
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "vs2026-x64-release",
      "inherits": "vs2026-base",
      "architecture": {
        "value": "x64",
        "strategy": "set"
      },
      "toolset": {
        "value": "v145,host=x64",
        "strategy": "set"
      },
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "vs2026-x64-release",
      "configurePreset": "vs2026-x64-release",
      "configuration": "Release"
    }
  ],
  "testPresets": [
    {
      "name": "vs2026-x64-release",
      "configurePreset": "vs2026-x64-release",
      "configuration": "Release",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

**Nutzung:**
```powershell
# Preset anwenden
cmake --preset vs2026-x64-release

# Build
cmake --build --preset vs2026-x64-release

# Test
cmake --test --preset vs2026-x64-release
```

---

## Visual Studio 2026 IDE Integration

### CMake-Projekt öffnen

1. **File → Open → CMake...**
2. Wähle `CMakeLists.txt` aus `C:\dev\TheSeed`
3. VS 2026 erkennt automatisch `CMakePresets.json`

### Konfiguration auswählen

```
Toolbar → Configuration: vs2026-x64-release
```

### Build & Debug

```
Ctrl+Shift+B  → Build
F5            → Debug (TheSeedEditor oder TheSeedServer)
```

---

## vcpkg + VS 2026

### vcpkg.json für VS 2026

```json
{
  "name": "theseed",
  "version": "0.1.0",
  "description": "TheSeed MMORPG Engine",
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
  "builtin-baseline": "2024.06.15",
  "overrides": [
    {
      "name": "imgui",
      "version": "1.91.0"
    }
  ]
}
```

### vcpkg mit VS 2026 kompilieren

```powershell
# vcpkg muss mit VS 2026 kompiliert werden
$env:VCPKG_ROOT = "C:\dev\vcpkg"

# vcpkg mit VS 2026 Toolchain
C:\dev\vcpkg\vcpkg.exe install `
    --triplet x64-windows `
    --cmake-toolchain-file="C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

---

## Compiler-Flags für VS 2026 / v145

### CMakeLists.txt

```cmake
if(MSVC)
    # VS 2026 / v145 spezifisch
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus /MP)

    # C++23 Features erzwingen
    add_compile_options(/std:c++23)

    # Performance
    add_compile_options(/O2 /Oi /Ot /GL)
    add_link_options(/LTCG)

    # Security
    add_compile_options(/GS /sdl)

    # Runtime Library (static linking)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Modules (C++20/23)
    if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
        add_compile_options(/experimental:module)
    endif()
endif()
```

---

## SLNX Format (VS 2026 Neuheit)

VS 2026 führt das **SLNX** Format ein (ersetzt .sln):

```xml
<!-- TheSeed.slnx -->
<Solution>
  <Project Path="src\core\TheSeedCore.vcxproj" />
  <Project Path="src\editor\TheSeedEditor.vcxproj" />
  <Project Path="src\server\TheSeedServer.vcxproj" />
  <Configurations>
    <Configuration Name="Debug" />
    <Configuration Name="Release" />
  </Configurations>
  <Platforms>
    <Platform Name="x64" />
  </Platforms>
</Solution>
```

> **Hinweis:** CMake generiert automatisch SLNX wenn VS 2026 Generator verwendet wird.

---

## Häufige Fehler & Lösungen

### Fehler: "CMake Error: Could not create named generator Visual Studio 18 2026"

**Ursache:** CMake < 4.2

```powershell
cmake --version
# Muss >= 4.2.0 sein

# Lösung: CMake aktualisieren
winget install Kitware.CMake
# ODER: https://cmake.org/download/
```

### Fehler: "The C++ compiler is not able to compile a simple test program"

**Ursache:** VS 2026 C++ Workload nicht installiert

```powershell
# Lösung: Visual Studio Installer öffnen
# → "Desktop development with C++" workload installieren
# → "C++ CMake tools for Windows" installieren
```

### Fehler: "vcpkg cannot find Visual Studio"

**Ursache:** vcpkg erkennt VS 2026 nicht

```powershell
# Lösung: vcpkg aktualisieren
cd C:\dev\vcpkg
git pull
.\bootstrap-vcpkg.bat

# ODER: Explicit Visual Studio Path
$env:VCPKG_VISUAL_STUDIO_PATH = "C:\Program Files\Microsoft Visual Studio\2026\Community"
```

### Fehler: "Module interface unit not found"

**Ursache:** C++23 Modules noch experimentell in v145

```cmake
# CMakeLists.txt
if(MSVC)
    add_compile_options(/experimental:module /std:c++23)
endif()
```

---

## Performance-Tipps für VS 2026

| Tipp | Befehl/Setting |
|------|---------------|
| **Faster Linking** | `/LINKTIME:PGINSTRUMENT` → `/LTCG:PGOPTIMIZE` |
| **PCH** | `target_precompile_headers(TheSeedCore PRIVATE src/core/pch.hpp)` |
| **Unity Build** | `set(CMAKE_UNITY_BUILD ON)` |
| **Ninja statt VS** | `cmake -G "Ninja Multi-Config"` (2x schneller) |
| **IncrediBuild** | In VS 2026 integriert, `Build → Parallelize` |

---

## Ressourcen

- [VS 2026 Release Notes](https://learn.microsoft.com/en-us/visualstudio/releases/2026/release-notes)
- [CMake VS 18 2026 Generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2018%202026.html)
- [vcpkg Documentation](https://learn.microsoft.com/en-us/vcpkg/)
- [C++23 in VS 2026](https://devblogs.microsoft.com/cppblog/whats-new-for-c-developers-in-visual-studio-2026-18-1-18-6/)
