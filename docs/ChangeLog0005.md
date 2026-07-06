# ChangeLog 0005 – Woche 5: Vulkan-Renderer

**Datum:** 2026-07-06  
**Version:** 0.1.4 → 0.1.5

## Hinzugefuegt
- Vulkan 1.3 Renderer (C-API, kein vulkan.hpp)
- SDL3 Windowing + Surface Creation
- Instance → Device → Swapchain → RenderPass → Pipeline
- Inline SPIR-V Shader (kein glslc Build-Schritt)
- Host-Visible Vertex/Index Buffers
- Triangle, Quad, Cube primitives

## Abhaengigkeiten
- `vulkan-headers` (vcpkg)
- `vulkan-loader` (vcpkg)
- `sdl3` (vcpkg)

## Gefixt
- SDL3 `SDL_Vulkan_GetInstanceExtensions` API-Change
- Plattformabhaengige Surface Extensions

## Tests
- Mesh struct layout
- Mesh creation
- Factory pattern
- Headless Vulkan Init + Triangle + Quad + Cube (GPU required)

## Dateien
- `src/core/renderer.hpp`
- `src/core/renderer.cpp`
- `tests/test_renderer.cpp`
