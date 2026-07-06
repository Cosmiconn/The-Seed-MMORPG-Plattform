# ChangeLog 0001 – Woche 1: Build-System

**Datum:** 2026-07-01  
**Version:** 0.1.0 → 0.1.1

## Hinzugefuegt
- CMake 3.28+ Build-System mit vcpkg Integration
- Auto-Discovery fuer vcpkg toolchain
- Visual Studio 2026 Presets (Debug/Release)
- Compiler-Flags: /W4, /permissive-, /Zc:__cplusplus
- HelloWorldModule.dll Post-Build Kopie in `modules/`

## Gefixt
- vcpkg.json ohne SHA-Hashes (vcpkg baseline stattdessen)
- yaml-cpp::yaml-cpp target (deprecated warning)
- Dynamic CRT Linkage (/MD, /MDd) fuer vcpkg-Kompatibilitaet

## Dateien
- `CMakeLists.txt`
- `CMakePresets.json`
- `vcpkg.json`
- `vcpkg-configuration.json`
