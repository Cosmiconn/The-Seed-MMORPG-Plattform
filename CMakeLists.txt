cmake_minimum_required(VERSION 3.28)
project(TheSeed VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# vcpkg Toolchain Auto-Discovery
# =============================================================================
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    set(VCPKG_CANDIDATES
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "E:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake"
        "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    )

    foreach(candidate IN LISTS VCPKG_CANDIDATES)
        if(EXISTS "${candidate}")
            set(CMAKE_TOOLCHAIN_FILE "${candidate}" CACHE STRING "vcpkg toolchain" FORCE)
            message(STATUS "vcpkg toolchain gefunden: ${candidate}")
            break()
        endif()
    endforeach()
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR
        "\n============================================================\n"
        "vcpkg toolchain file NICHT GEFUNDEN!\n"
        "------------------------------------------------------------\n"
        "Loesung 1 (empfohlen):\n"
        "  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake\n"
        "\n"
        "Loesung 2:\n"
        "  [Environment]::SetEnvironmentVariable('VCPKG_ROOT','D:/dev/vcpkg','User')\n"
        "  # Dann PowerShell neu starten\n"
        "\n"
        "Loesung 3:\n"
        "  .\Setup-Build.ps1   (automatisch)\n"
        "============================================================\n"
    )
endif()

# =============================================================================
# Compiler-Flags
# =============================================================================
if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus /MP)
    add_compile_options(/wd4267 /wd4244)
    # yaml-cpp Deprecation Warning unterdruecken
    add_compile_options(/wd4996)
    # FIX: Dynamic runtime to match vcpkg default builds (MD/MDd)
    # vcpkg builds packages with dynamic runtime by default
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
endif()

# =============================================================================
# Dependencies
# =============================================================================
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

# =============================================================================
# TheSeed Core Library
# =============================================================================
add_library(TheSeedCore STATIC)
target_sources(TheSeedCore
    PRIVATE
        src/core/ecs.cpp
        src/core/net.cpp
        src/core/modules.cpp
        src/core/events.cpp
        src/core/serialize.cpp
        src/core/jobs.cpp
        src/core/renderer.cpp
        src/core/game_server.cpp
        src/core/game_client.cpp
        src/core/profiler.cpp
    PUBLIC
        src/core/ecs.hpp
        src/core/net.hpp
        src/core/modules.hpp
        src/core/events.hpp
        src/core/serialize.hpp
        src/core/jobs.hpp
        src/core/renderer.hpp
        src/core/game_server.hpp
        src/core/game_client.hpp
        src/core/profiler.hpp
)
target_include_directories(TheSeedCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# FIX: yaml-cpp::yaml-cpp statt yaml-cpp (deprecated target)
target_link_libraries(TheSeedCore PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    Vulkan::Vulkan
    SDL3::SDL3
)

target_compile_features(TheSeedCore PUBLIC cxx_std_23)

if(WIN32)
    target_link_libraries(TheSeedCore PUBLIC ws2_32)
else()
    target_link_libraries(TheSeedCore PUBLIC pthread)
endif()

# =============================================================================
# Executables
# =============================================================================
add_executable(TheSeedEditor src/editor/main.cpp)
target_link_libraries(TheSeedEditor PRIVATE TheSeedCore)
set_target_properties(TheSeedEditor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(TheSeedServer src/server/main.cpp)
target_link_libraries(TheSeedServer PRIVATE TheSeedCore)
set_target_properties(TheSeedServer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# =============================================================================
# HelloWorld Module (DLL)
# =============================================================================
add_library(HelloWorldModule SHARED src/modules/hello_world/hello_world.cpp)
target_link_libraries(HelloWorldModule PRIVATE TheSeedCore)
set_target_properties(HelloWorldModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
)

# =============================================================================
# Tests
# =============================================================================
enable_testing()
add_executable(TheSeedTests
    tests/test_ecs.cpp
    tests/test_net.cpp
    tests/test_modules.cpp
    tests/test_events.cpp
    tests/test_jobs.cpp
    tests/test_renderer.cpp
    tests/test_integration.cpp
    tests/test_profiler.cpp
)
# FIX: Link against dynamic CRT to match vcpkg-built Catch2
# Also add /NODEFAULTLIB to avoid libcmt.lib conflicts if any remain
target_link_libraries(TheSeedTests PRIVATE TheSeedCore Catch2::Catch2WithMain)
if(MSVC)
    # Ensure tests also use dynamic runtime
    set_target_properties(TheSeedTests PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()
include(Catch)
catch_discover_tests(TheSeedTests)

# =============================================================================
# Install
# =============================================================================
install(TARGETS TheSeedCore TheSeedEditor TheSeedServer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY src/core/ DESTINATION include/TheSeed/core FILES_MATCHING PATTERN "*.hpp")
