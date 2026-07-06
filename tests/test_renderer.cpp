#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

using namespace TheSeed::Render;

TEST_CASE("Render: Mesh struct layout", "[render]") {
    Vertex v{1.0f, 2.0f, 3.0f, 0.5f, 0.5f, 0.5f};
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.r == 0.5f);
    REQUIRE(v.g == 0.5f);
    REQUIRE(v.b == 0.5f);

    // Vertex size = 6 floats = 24 bytes
    REQUIRE(sizeof(Vertex) == 24);
}

TEST_CASE("Render: Mesh creation", "[render]") {
    Mesh tri;
    tri.vertices = {
        {0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f},
    };
    tri.indices = {0, 1, 2};

    REQUIRE(tri.vertices.size() == 3);
    REQUIRE(tri.indices.size() == 3);
}

TEST_CASE("Render: Factory", "[render]") {
    auto renderer = CreateRenderer();
    REQUIRE(renderer != nullptr);
    REQUIRE(!renderer->IsInitialized());
}

// --- Headless-Test: Vulkan Initialisierung (nur wenn GPU verfuegbar) ---
TEST_CASE("Render: Vulkan Init + Triangle + Quad + Cube", "[render][integration]") {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SKIP("SDL_Init fehlgeschlagen: " + std::string(SDL_GetError()));
    }

    // Headless-Window (hidden)
    SDL_Window* window = SDL_CreateWindow("TheSeed Test", 800, 600, SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
    if (!window) {
        SDL_Quit();
        SKIP("SDL_CreateWindow fehlgeschlagen: " + std::string(SDL_GetError()));
    }

    auto renderer = CreateRenderer();
    REQUIRE(renderer != nullptr);

    bool initOk = renderer->Initialize(window, 800, 600);
    if (!initOk) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        SKIP("Vulkan nicht verfuegbar (keine GPU / Treiber)");
    }

    REQUIRE(renderer->IsInitialized());

    // Frame 1: Triangle (rot/gruen/blau)
    renderer->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    if (renderer->BeginFrame()) {
        renderer->DrawTriangle();
        renderer->EndFrame();
    }

    // Frame 2: Quad (bunt)
    if (renderer->BeginFrame()) {
        renderer->DrawQuad();
        renderer->EndFrame();
    }

    // Frame 3: Cube (rot/gruen)
    if (renderer->BeginFrame()) {
        renderer->DrawCube();
        renderer->EndFrame();
    }

    renderer->Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    REQUIRE(!renderer->IsInitialized());
}
