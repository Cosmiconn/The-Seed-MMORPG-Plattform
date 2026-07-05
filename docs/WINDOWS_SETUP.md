# TheSeed – Windows 11 Vorbereitung & Setup
**Visual Studio 18 2026 / C++23 / vcpkg / D:\dev**

> **Dieses Dokument ist dein absoluter Pflicht-Checklist.** Wenn du jeden Schritt befolgst, baut TheSeed auf Anhieb.

---

## 1. Systemvoraussetzungen

| Komponente | Minimum | Empfohlen | Pruefbefehl |
|------------|---------|-----------|------------|
| **Windows** | 11 22H2 | 11 24H2 | `winver` |
| **RAM** | 16 GB | 32 GB | Task-Manager |
| **Festplatte** | 50 GB frei (SSD) | 100 GB frei (NVMe) | Explorer |
| **Internet** | 50 Mbit/s | 100+ Mbit/s | `speedtest.net` |

> **Wichtig:** vcpkg kompiliert alle Dependencies von Source. Das braucht Platz und Zeit.

---

## 2. Visual Studio 2026 installieren

1. [visualstudio.microsoft.com](https://visualstudio.microsoft.com) → "Visual Studio 2026" downloaden
2. Installer starten → **"Desktop development with C++"** Workload auswaehlen
3. Zusaetzlich ankreuzen:
   - ✅ MSVC v145 - VS 2026 C++ x64/x86 build tools
   - ✅ Windows 11 SDK (10.0.22621.0 oder neuer)
   - ✅ C++ CMake tools for Windows
4. Installieren → Neustart

**Pruefung:**
```powershell
# Oeffne "Developer PowerShell for VS 2026" (Startmenue)
cl.exe
# Ausgabe: "Microsoft (R) C/C++ Optimizing Compiler Version 19.xx"
```

---

## 3. Tools installieren

### 3.1 CMake (mindestens 3.28, empfohlen 4.2+)

```powershell
cmake --version
# Wenn < 3.28 oder nicht gefunden:
winget install Kitware.CMake
# PowerShell NEU STARTEN nach Installation!
```

### 3.2 Git

```powershell
git --version
# Falls nicht vorhanden:
winget install Git.Git
```

**Git-Config (einmalig):**
```powershell
git config --global user.name "Dein Name"
git config --global user.email "dein@email.com"
git config --global core.autocrlf true
```

### 3.3 Python (fuer vcpkg Portfiles)

```powershell
python --version
# Falls nicht vorhanden oder < 3.10:
winget install Python.Python.3.11
# PowerShell NEU STARTEN!
```

---

## 4. vcpkg installieren (einmalig, 30-60 Minuten)

```powershell
# Verzeichnis erstellen
mkdir D:\dev
cd D:\dev

# vcpkg klonen
git clone https://github.com/Microsoft/vcpkg.git

# Bootstrap (kompiliert vcpkg selbst)
cd vcpkg
.\bootstrap-vcpkg.bat

# Integration in VS (empfohlen)
.\vcpkg.exe integrate install

# Umgebungsvariable setzen (User-Level, PERSISTENT!)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "D:\dev\vcpkg", "User")
$env:VCPKG_ROOT = "D:\dev\vcpkg"
```

> **Wichtig:** `$env:VCPKG_ROOT` ist nur fuer DIESE Session. `[Environment]::SetEnvironmentVariable` ist permanent. Nach einem Windows-Neustart ist die Variable aktiv.

**Pruefung:**
```powershell
$env:VCPKG_ROOT
# Ausgabe: D:\dev\vcpkg
```

---

## 5. TheSeed Projekt einrichten

### 5.1 ZIP entpacken

Entpacke `TheSeed_V0.1.0_Fixed.zip` nach `D:\dev\TheSeed`:

```
D:\dev\TheSeed\
├── CMakeLists.txt
├── vcpkg.json
├── Setup-Build.ps1
├── src\
├── tests\
└── ...
```

### 5.2 AUTOMATISCHER Build (empfohlen)

```powershell
cd D:\dev\TheSeed
# PowerShell als Admin ausfuehren!
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\Setup-Build.ps1
```

Das Script macht alles automatisch:
- Prueft VS 2026, CMake, vcpkg
- Setzt VCPKG_ROOT
- Fuehrt `vcpkg install` aus
- Konfiguriert CMake
- Baut das Projekt
- Fuehrt Tests aus

### 5.3 MANUELLER Build (Falls das Script nicht funktioniert)

```powershell
cd D:\dev\TheSeed

# 1. vcpkg install (nur wenn noetig)
D:\dev\vcpkg\vcpkg.exe install

# 2. CMake konfigurieren (TOOLCHAIN explizit angeben!)
cmake -G "Visual Studio 18 2026" -A x64 -B build -S . `
    -DCMAKE_TOOLCHAIN_FILE="D:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"

# 3. Build
cmake --build build --config Release --parallel

# 4. Tests
ctest --test-dir build --output-on-failure
```

> **Der Schluessel ist `-DCMAKE_TOOLCHAIN_FILE="D:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"`** — ohne das findet CMake die vcpkg-Packages nicht.

---

## 6. Erste Ausfuehrung

```powershell
# Server
cd D:\dev\TheSeed
.\build\Release\TheSeedServer.exe

# Editor
.\build\Release\TheSeedEditor.exe
```

**Erwartete Ausgabe:**
```
[info] === TheSeed Server v0.1.0 ===
[info] [HelloWorld] Initialized! Config: name=Willkommen auf TheSeed!
[info] Server laeuft auf Port 7777...
```

---

## 7. Taeglicher Workflow

```powershell
cd D:\dev\TheSeed

# Build (inkrementell)
cmake --build build --config Release

# Tests
ctest --test-dir build --output-on-failure

# Server starten
.\build\Release\TheSeedServer.exe
```

---

## 8. Troubleshooting

### "Could not find a package configuration file provided by spdlog"

**Ursache:** CMake kennt vcpkg nicht.

**Loesung:**
```powershell
# Explizit den Toolchain-Pfad angeben:
cmake -B build -S . `
    -DCMAKE_TOOLCHAIN_FILE="D:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"

# ODER ueber Umgebungsvariable:
$env:VCPKG_ROOT = "D:\dev\vcpkg"
cmake -B build -S .
```

### "No CMAKE_C_COMPILER could be found"

**Ursache:** Nicht in Developer PowerShell.

**Loesung:**
```powershell
# Starte "Developer PowerShell for VS 2026"
# ODER:
& "${env:ProgramFiles}\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### "CMake Error: Could not create named generator Visual Studio 18 2026"

**Ursache:** CMake zu alt.

**Loesung:**
```powershell
cmake --version  # Muss >= 4.2.0 sein
winget upgrade Kitware.CMake
```

### vcpkg Packages fail zu kompilieren

```powershell
cd D:\dev\vcpkg
git pull
.\bootstrap-vcpkg.bat
cd D:\dev\TheSeed
Remove-Item -Recurse -Force build, vcpkg_installed
D:\dev\vcpkg\vcpkg.exe install
```

---

## 9. Checkliste

- [ ] Windows 11 22H2+
- [ ] Visual Studio 2026 mit C++ Workload
- [ ] CMake 3.28+ (4.2+ fuer VS 2026)
- [ ] Git installiert & konfiguriert
- [ ] Python 3.10+ installiert
- [ ] vcpkg in `D:\dev\vcpkg` geklont & gebootstrapt
- [ ] `VCPKG_ROOT` Umgebungsvariable gesetzt
- [ ] TheSeed-Dateien in `D:\dev\TheSeed` entpackt
- [ ] `vcpkg install` erfolgreich
- [ ] CMake mit `-DCMAKE_TOOLCHAIN_FILE=...` konfiguriert
- [ ] Build erfolgreich
- [ ] Tests gruen
- [ ] Server startet

---

**Letzte Aktualisierung:** 2026-07-05
