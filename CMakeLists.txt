cmake_minimum_required(VERSION 3.25)

project(AuroraProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bake C++ runtime DLLs (vcruntime140 / msvcp140) directly into .exe as Static (/MT and /MTd)
# So Debug, Development, and Release .exe files run on any PC or Linux Wine with zero missing DLLs!
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Guard against accidental 32-bit (x86) developer shells when linking 64-bit engine
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(DEFINED ENV{VSCMD_ARG_TGT_ARCH} AND NOT "$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "x64")
        message(FATAL_ERROR "\n[Aurora Engine Error] You opened a 32-bit (x86) Developer Terminal ($ENV{VSCMD_ARG_TGT_ARCH})!\n"
            "Aurora is a 64-bit engine and requires the 64-bit MSVC environment.\n"
            "-> In Start Menu, open: 'x64 Native Tools Command Prompt for VS 2026'\n"
            "-> Or in PowerShell run: & \"D:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1\" -Arch amd64 -SkipAutomaticLocation\n"
            "-> Or build directly in Visual Studio (Ctrl+Shift+B)\n")
    endif()
endif()

# ---------- ThirdParty: SDL3 (Static — Zero DLLs!) ----------
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# ---------- Engine ----------
# Auto-export compile commands for IDE IntelliSense (Visual Studio, VS Code, Clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Engine Folders (Explicit, non-recursive — the ONE place to register a new folder)
# Each folder is added as an include path AND its .h/.cpp files are compiled.
# So `#include <M_Mesh.h>` works without writing the folder path.
set(ENGINE_FOLDERS
    "Engine"                              # Aurora.h (Master umbrella header)

    # Core Module (Platform, RHI, OS abstractions)
    "Engine/Core"                         # Core.h (BeginPlay / Tick / EndPlay) + pch.h
    "Engine/Core/Utilities"               # U_*  Types, Math, Colors
    "Engine/Core/Input"                   # I_*  Keyboard, Mouse
    "Engine/Core/Managers"                # M_*  Log, File, Time, Window, Threads, Crash, Input, Asset
    "Engine/Core/Managers/Loaders"        # M_*  Config, Mesh, Texture (depend on M_File)
    "Engine/Core/Plugins"                 # I_Plugin, M_PluginManager
    "Engine/Core/Internal/RHI"            # RHI_Manager, R_* Vulkan / DirectX backends
    "Engine/Core/Internal/RHI/Shaders"    # S_*  Precompiled shader bytecode

    # World Module (3D Scene, Entities, Camera)
    "Engine/World"                        # World.h (Module header)
    "Engine/World/Base"                   # B_Game (Base 3D game lifecycle)
    "Engine/World/Managers"               # M_World (3D World, culling, ticking)
    "Engine/World/Entities"               # C_*  Entities (Camera, StaticMesh, DynamicMesh)

    # UI Module (2D Screen-space HUD & Controls)
    "Engine/UserInterface"                # UI_Frame, UI_ImageBox, UI_ImageBtn, UI_Box, UI_Text
)

set(ENGINE_SOURCES "")
set(ENGINE_INCLUDE_DIRS "")
foreach(folder ${ENGINE_FOLDERS})
    file(GLOB folder_files CONFIGURE_DEPENDS "${folder}/*.h" "${folder}/*.cpp")
    list(APPEND ENGINE_SOURCES ${folder_files})
    list(APPEND ENGINE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${folder}")
endforeach()

add_library(EngineSource STATIC)

target_sources(EngineSource
  PRIVATE
    ${ENGINE_SOURCES}
)

target_include_directories(EngineSource
    PUBLIC
        ${ENGINE_INCLUDE_DIRS}
)

target_precompile_headers(EngineSource
  PUBLIC
    Engine/Core/pch.h
)

# Required on Linux (GCC/Clang) for std::thread and std::mutex (automatic no-op on Windows)
find_package(Threads REQUIRED)

# ---------- ThirdParty: Vulkan SDK ----------
find_package(Vulkan REQUIRED)

target_link_libraries(EngineSource
  PUBLIC
    Threads::Threads
    SDL3::SDL3-static
    Vulkan::Vulkan
)

# Pass configuration macros (_DEBUG, _DEVELOPMENT, _RELEASE)
target_compile_definitions(EngineSource
  PUBLIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:_DEVELOPMENT>
    $<$<CONFIG:Release>:_RELEASE>
)

# High-Performance Compiler Optimizations (/O2 MaxSpeed, /Ob2 InlineAny, /Oi Intrinsics, /Ot FastCode, /fp:fast SIMD FMA, /GS- ZeroCookie)
if(MSVC)
    target_compile_options(EngineSource
      PUBLIC
        $<$<NOT:$<CONFIG:Debug>>:/O2 /Ob2 /Oi /Ot /fp:fast /GS->
    )
endif()
