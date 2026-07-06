# TheSeed – Quickstart (VS 18 2026)

## 5-Minuten Setup

```powershell
# 1. Als Admin ausführen
# 2. PowerShell öffnen

# 3. Script ausführen
& "C:\Pfad\Zu\Setup-TheSeed.ps1"

# 4. vcpkg installieren (30-60 min, nur einmal)
.\vcpkg\vcpkg.exe install

# 5. CMake konfigurieren (VS 2026)
cmake -G "Visual Studio 18 2026" -A x64 -B build -S .

# 6. Build
cmake --build build --config Release --parallel

# 7. Tests
ctest --test-dir build --output-on-failure

# 8. Server starten
.\build\Release\TheSeedServer.exe
```

## Täglicher Workflow

```powershell
cd C:\dev\TheSeed

# Pull
git pull origin main

# Build
cmake --build build --config Release

# Test
ctest --test-dir build

# Run
.\build\Release\TheSeedServer.exe

# Commit
git add -A
git commit -m "feat: ..."
git push origin main
```

## Wichtige Pfade

| Pfad | Inhalt |
|------|--------|
| `src/core/` | ECS, Net, Modules, Events, Serialize |
| `src/editor/` | ImGui Editor |
| `src/server/` | Dedicated Server |
| `src/modules/` | Gameplay Module (DLLs) |
| `build/` | CMake Build Output |
| `vcpkg_installed/` | vcpkg Dependencies |

## VS 2026 Shortcuts

| Shortcut | Aktion |
|----------|--------|
| `Ctrl+Shift+B` | Build |
| `F5` | Debug |
| `Ctrl+F5` | Start ohne Debug |
| `Ctrl+Shift+P` | CMake: Configure |
| `Ctrl+K, Ctrl+O` | File → Open → CMake |

## Support

📧 hello@theseed.world | 🐦 @TheSeedEngine
