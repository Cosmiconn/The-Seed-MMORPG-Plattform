# TheSeed – GitHub + Lokal Setup Guide
**Visual Studio 18 2026 / C++23 / Windows 11**

---

## Voraussetzungen

| Tool | Version | Download |
|------|---------|----------|
| **Visual Studio** | 2026 (18.x) | [visualstudio.microsoft.com](https://visualstudio.microsoft.com) |
| **CMake** | >= 4.2 | [cmake.org/download](https://cmake.org/download/) |
| **Git** | >= 2.40 | [git-scm.com](https://git-scm.com/download/win) |
| **Python** | >= 3.10 | [python.org](https://python.org) (für vcpkg) |
| **GitHub CLI** | optional | [cli.github.com](https://cli.github.com) |

> **Wichtig:** Visual Studio 2026 (Version 18.x) benötigt **CMake 4.2+** für den Generator `"Visual Studio 18 2026"`. Ältere CMake-Versionen erkennen VS 2026 nicht.

---

## Option A: Automatisches Setup (PowerShell Script)

### 1. PowerShell als Admin öffnen

```powershell
# Win + X → Windows PowerShell (Admin) oder Terminal (Admin)
```

### 2. Execution Policy setzen (einmalig)

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### 3. Script herunterladen & ausführen

```powershell
# Script herunterladen
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/cosmiconn/theseed/main/Setup-TheSeed.ps1" -OutFile "$env:TEMP\Setup-TheSeed.ps1"

# Ausführen
& "$env:TEMP\Setup-TheSeed.ps1" -ProjectPath "C:\dev\TheSeed" -GitHubUser "cosmiconn" -RepoName "theseed"
```

### Script-Parameter

| Parameter | Default | Beschreibung |
|-----------|---------|--------------|
| `-ProjectPath` | `C:\dev\TheSeed` | Wo das Projekt erstellt wird |
| `-GitHubUser` | `cosmiconn` | Dein GitHub Username |
| `-RepoName` | `theseed` | Repository-Name |
| `-SkipGitHub` | `false` | Kein GitHub-Repo erstellen |
| `-SkipVcpkg` | `false` | Kein vcpkg installieren |
| `-SkipVSCheck` | `false` | Kein VS 2026 Check |

---

## Option B: Manuelles Setup

### Schritt 1: Verzeichnis erstellen

```powershell
mkdir C:\dev\TheSeed
cd C:\dev\TheSeed
```

### Schritt 2: Git initialisieren

```powershell
git init
git branch -M main

# Git Config (falls nicht gesetzt)
git config --global user.name "Dein Name"
git config --global user.email "dein@email.com"
```

### Schritt 3: GitHub Repository erstellen

**Option A – GitHub CLI:**
```powershell
gh auth login
gh repo create theseed --private --description "TheSeed MMORPG Engine" --confirm
git remote add origin https://github.com/deinuser/theseed.git
```

**Option B – Manuell:**
1. Öffne [github.com/new](https://github.com/new)
2. Name: `theseed`
3. Private → Create Repository
4. Kopiere die HTTPS URL

```powershell
git remote add origin https://github.com/deinuser/theseed.git
```

### Schritt 4: vcpkg installieren

```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat

# Umgebungsvariable (User)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\dev\vcpkg", "User")
$env:VCPKG_ROOT = "C:\dev\vcpkg"

# Integration
C:\dev\vcpkg\vcpkg.exe integrate install
```

### Schritt 5: Projekt-Dateien erstellen

Kopiere alle Dateien aus diesem Repository in `C:\dev\TheSeed`:

```
TheSeed/
├── .github/
│   └── workflows/
│       └── build.yml
├── src/
│   ├── core/
│   │   ├── ecs.hpp
│   │   ├── net.hpp
│   │   ├── modules.hpp
│   │   ├── events.hpp
│   │   ├── serialize.hpp
│   │   ├── ecs.cpp
│   │   ├── net.cpp
│   │   ├── modules.cpp
│   │   ├── events.cpp
│   │   └── serialize.cpp
│   ├── editor/
│   │   └── main.cpp
│   ├── server/
│   │   └── main.cpp
│   └── modules/
│       └── hello_world/
│           └── hello_world.cpp
├── tests/
│   ├── test_ecs.cpp
│   └── test_modules.cpp
├── cmake/
│   └── modules/
├── docs/
├── CMakeLists.txt
├── vcpkg.json
├── vcpkg-configuration.json
├── .gitignore
└── README.md
```

### Schritt 6: Dependencies installieren

```powershell
cd C:\dev\TheSeed
C:\dev\vcpkg\vcpkg.exe install
```
> **Dauer:** 30–60 Minuten (kompiliert alle Dependencies von Source)

### Schritt 7: CMake konfigurieren (VS 18 2026)

```powershell
# Visual Studio 18 2026 Generator (CMake 4.2+ erforderlich!)
cmake -G "Visual Studio 18 2026" -A x64 -B build -S .
```

**Alternative Generatoren:**
```powershell
# Ninja (schneller, aber kein VS-Projekt)
cmake -G "Ninja Multi-Config" -B build -S .

# VS 2022 (falls VS 2026 nicht verfügbar)
cmake -G "Visual Studio 17 2022" -A x64 -B build -S .
```

### Schritt 8: Build

```powershell
# Release Build
cmake --build build --config Release --parallel

# Oder öffne build\TheSeed.sln in Visual Studio 2026
```

### Schritt 9: Tests

```powershell
ctest --test-dir build --output-on-failure
```

### Schritt 10: Server starten

```powershell
.\build\Release\TheSeedServer.exe
```

---

## VS 18 2026 Spezifika

### CMake Generator

```cmake
# CMakeLists.txt
# Der Generator wird automatisch erkannt, wenn du cmake -G angibst
# VS 2026 nutzt Toolset v145 (default)
```

### CMakeSettings.json (Visual Studio IDE)

Falls du CMake direkt in VS 2026 öffnest:

```json
{
  "configurations": [
    {
      "name": "x64-Release",
      "generator": "Visual Studio 18 2026",
      "configurationType": "Release",
      "buildRoot": "${projectDir}\build",
      "installRoot": "${projectDir}\install",
      "cmakeCommandArgs": "",
      "buildCommandArgs": "-m -v:minimal",
      "ctestCommandArgs": "",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "variables": []
    }
  ]
}
```

### v145 Toolset

VS 2026 kommt mit **MSVC v145**. In `CMakeLists.txt`:

```cmake
if(MSVC)
    # v145 ist default, aber explizit setzbar:
    # set(CMAKE_GENERATOR_TOOLSET "v145")

    # Host=x64 für schnelleres Compilen
    set(CMAKE_GENERATOR_TOOLSET "v145,host=x64")
endif()
```

---

## GitHub Actions CI/CD

Die `.github/workflows/build.yml` ist bereits konfiguriert für:

- **Runner:** `windows-latest`
- **Generator:** `Visual Studio 18 2026`
- **Platform:** `x64`
- **vcpkg:** Caching via `lukka/run-vcpkg`
- **Artifacts:** Editor, Server, Module-DLLs

### Manueller Trigger

```powershell
# Push zu main
git add -A
git commit -m "feat: initial engine setup"
git push origin main
```

---

## Troubleshooting

### "No CMAKE_C_COMPILER could be found"

```powershell
# Visual Studio Developer PowerShell öffnen:
# Start → "Developer PowerShell for VS 2026"
# ODER:
& "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### "CMake Error: Could not create named generator Visual Studio 18 2026"

```powershell
# CMake zu alt! Mindestens 4.2 erforderlich.
cmake --version
# Download: https://cmake.org/download/
```

### vcpkg Packages fail zu kompilieren

```powershell
# vcpkg aktualisieren
cd C:\dev\vcpkg
git pull
.\bootstrap-vcpkg.bat

# Clean build
cd C:\dev\TheSeed
Remove-Item -Recurse -Force build, vcpkg_installed
C:\dev\vcpkg\vcpkg.exe install
```

### "LNK2019: unresolved external symbol"

```powershell
# vcpkg integrate nicht korrekt?
C:\dev\vcpkg\vcpkg.exe integrate install

# Oder CMake Cache löschen
Remove-Item -Recurse -Force build
cmake -G "Visual Studio 18 2026" -A x64 -B build -S .
```

---

## Nächste Schritte

| Priorität | Task | Zeit |
|-----------|------|------|
| 1 | vcpkg `install` durchlaufen lassen | 30-60 min |
| 2 | Erster Build erfolgreich | 5 min |
| 3 | Tests laufen grün | 1 min |
| 4 | Server startet | sofort |
| 5 | ECS Benchmark: 100k Entities | 1 Tag |
| 6 | Netzwerk: 2 Clients bewegen Cubes | 1 Woche |
| 7 | Erstes Modul: Inventory | 1 Woche |

---

**Kontakt:** hello@theseed.world | 🐦 @TheSeedEngine
