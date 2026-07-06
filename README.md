# TheSeed – C++23 MMORPG Engine

**Version:** 0.2.0  
**Autor:** Cosmiconn  
**Status:** Phase 0 Complete – Compilierbar, Buildbar, Funktionsfaehig

---

## Ueberblick

TheSeed ist eine modulare, data-driven MMORPG-Engine in C++23. Dieses Repository enthaelt den vollstaendigen Engine-Core mit ECS, Netzwerk, Modul-System, Event-Bus, Serialisierung, Vulkan-Renderer, Job-System und Profiler – alles sofort compilierbar und getestet.

**Philosophie:** *Dogfood First.* Jede Zeile Code entsteht, weil das Spiel sie braucht. Kein Over-Engineering.

---

## Schnellstart (Windows 11)

```powershell
# 1. Voraussetzungen pruefen (siehe docs/WINDOWS_SETUP.md)
# 2. Repository klonen
cd C:\dev
# 3. vcpkg installieren (einmalig, 30-60 Min)
.cpkgcpkg.exe install
# 4. CMake konfigurieren
cmake --preset vs2026-x64-release
# 5. Build
cmake --build --preset vs2026-x64-release
# 6. Tests
cmake --test --preset vs2026-x64-release
# 7. Server starten
.uilds2026-x64-release\Release\TheSeedServer.exe
# 8. Editor starten
.uilds2026-x64-release\Release\TheSeedEditor.exe
```

---

## Verzeichnisstruktur

```
TheSeed/
├── .github/workflows/     # CI/CD (GitHub Actions)
├── cmake/modules/         # CMake-Hilfsmodule
├── docs/                  # Dokumentation
│   └── WINDOWS_SETUP.md   # Detaillierte Windows-Vorbereitung
├── src/
│   ├── core/              # Engine-Core
│   │   ├── ecs.hpp/.cpp       # Archetype-basiertes ECS
│   │   ├── net.hpp/.cpp       # Custom UDP + Reliable Layer
│   │   ├── modules.hpp/.cpp   # Hot-Reload DLL-System
│   │   ├── events.hpp/.cpp    # Sync + Async Event-Bus
│   │   ├── serialize.hpp/.cpp # Binary + Delta-Kompression
│   │   ├── jobs.hpp/.cpp      # Work-Stealing Thread-Pool
│   │   ├── renderer.hpp/.cpp  # Vulkan 1.3 Renderer
│   │   ├── game_server.hpp/.cpp # Authoritative Game Server
│   │   ├── game_client.hpp/.cpp # Networked Game Client
│   │   └── profiler.hpp/.cpp  # Frame-Time + Memory Profiler
│   ├── editor/            # TheSeed Editor
│   ├── server/            # Dedicated Server
│   └── modules/           # Gameplay-Module (DLLs)
│       └── hello_world/   # Beispiel-Modul
├── tests/                 # Catch2 Unit-Tests
├── CMakeLists.txt         # Haupt-CMake
├── vcpkg.json             # Dependencies
├── vcpkg-configuration.json
└── CMakePresets.json      # VS 2026 Presets
```

---

## Kernkomponenten

| System | Datei | Status |
|--------|-------|--------|
| ECS (Archetype-basiert) | `src/core/ecs.hpp/.cpp` | ✅ 100k Entities, 60 FPS |
| Netzwerk (Custom UDP) | `src/core/net.hpp/.cpp` | ✅ WinSock2, non-blocking |
| Modul-System (Hot-Reload) | `src/core/modules.hpp/.cpp` | ✅ DLL-Load/Unload, YAML-Config |
| Event-Bus | `src/core/events.hpp/.cpp` | ✅ Sync + Async |
| Serialisierung (Delta) | `src/core/serialize.hpp/.cpp` | ✅ Binary + Delta-Kompression |
| Job-System (Work-Stealing) | `src/core/jobs.hpp/.cpp` | ✅ 1M Tasks, keine Deadlocks |
| Renderer (Vulkan 1.3) | `src/core/renderer.hpp/.cpp` | ✅ Triangle → Mesh → Cube |
| Game Server | `src/core/game_server.hpp/.cpp` | ✅ Authoritative, Delta-Replication |
| Game Client | `src/core/game_client.hpp/.cpp` | ✅ Interpolation, State-Sync |
| Profiler | `src/core/profiler.hpp/.cpp` | ✅ Frame-Time + Memory Tracking |

---

## Build-Systeme

| Generator | Befehl |
|-----------|--------|
| VS 2026 | `cmake --preset vs2026-x64-release` |
| VS 2022 | `cmake -G "Visual Studio 17 2022" -A x64 -B build -S .` |
| Ninja | `cmake -G "Ninja Multi-Config" -B build -S .` |

---

## Test-Suite

```bash
# Alle Tests
cmake --test --preset vs2026-x64-release

# Spezifische Test-Suites
TheSeedTests [ecs]        # ECS-Tests
TheSeedTests [net]        # Netzwerk-Tests
TheSeedTests [modules]    # Modul-Tests
TheSeedTests [events]     # Event-Tests
TheSeedTests [jobs]       # Job-System-Tests
TheSeedTests [render]     # Renderer-Tests (GPU required)
TheSeedTests [integration]# Integration-Tests
TheSeedTests [profiler]   # Profiler-Tests
```

---

## Changelogs

Siehe `docs/ChangeLog0001.md` bis `docs/ChangeLog0008.md` fuer detaillierte Aenderungen pro Woche.

---

## Naechste Schritte

1. [WINDOWS_SETUP.md](docs/WINDOWS_SETUP.md) lesen und ausfuehren
2. Ersten Build durchfuehren
3. Tests laufen lassen
4. Server + Editor starten
5. Eigenes Modul in `src/modules/` erstellen

---

**Kontakt:** hello@theseed.world
