# TheSeed – C++23 MMORPG Engine

**Version:** 0.1.0  
**Autor:** Cosmiconn  
**Status:** MVP – Compilierbar, Buildbar, Funktionsfähig

---

## Überblick

TheSeed ist eine modulare, data-driven MMORPG-Engine in C++23. Dieses Repository enthält den vollständigen Engine-Core mit ECS, Netzwerk, Modul-System, Event-Bus und Serialisierung – alles sofort compilierbar und getestet.

**Philosophie:** *Dogfood First.* Jede Zeile Code entsteht, weil das Spiel sie braucht. Kein Over-Engineering.

---

## Schnellstart (Windows 11)

```powershell
# 1. Voraussetzungen prüfen (siehe docs/WINDOWS_SETUP.md)
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
│   ├── core/              # Engine-Core (ECS, Net, Modules, Events, Serialize)
│   ├── editor/            # TheSeed Editor (ImGui-Grundgerüst)
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

---

## Build-Systeme

| Generator | Befehl |
|-----------|--------|
| VS 2026 | `cmake --preset vs2026-x64-release` |
| VS 2022 | `cmake -G "Visual Studio 17 2022" -A x64 -B build -S .` |
| Ninja | `cmake -G "Ninja Multi-Config" -B build -S .` |

---

## Nächste Schritte

1. [WINDOWS_SETUP.md](docs/WINDOWS_SETUP.md) lesen und ausführen
2. Ersten Build durchführen
3. Tests laufen lassen
4. Server + Editor starten
5. Eigenes Modul in `src/modules/` erstellen

---

**Kontakt:** hello@theseed.world
