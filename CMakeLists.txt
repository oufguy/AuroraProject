cmake_minimum_required(VERSION 3.25)

project(AuroraProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bake C++ runtime DLLs (vcruntime140 / msvcp140) directly into .exe as Static (/MT and /MTd)
# So Debug, Development, and Release .exe files run on any PC or Linux Wine with zero missing DLLs!
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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

# ---------- Aurora Studio Editor (Single Unified Tool) ----------
option(AURORA_BUILD_EDITOR "Build Aurora Studio Editor" ON)

if(AURORA_BUILD_EDITOR)
    set(AURORA_EDITOR_SOURCES
        "Editor/Source/Main.cpp"
        "Editor/Source/Editor.h"
        "Editor/Source/E_EditorApp.cpp"
        "Editor/Source/E_EditorApp.h"
        "Editor/Source/E_SceneEditor.cpp"
        "Editor/Source/E_SceneEditor.h"
        "Editor/Source/E_ProjectManager.cpp"
        "Editor/Source/E_ProjectManager.h"
        "Editor/Source/E_AssetPacker.cpp"
        "Editor/Source/E_AssetPacker.h"
        "Editor/Source/E_XORGenerator.cpp"
        "Editor/Source/E_XORGenerator.h"
        "Editor/Source/E_UIDesigner.cpp"
        "Editor/Source/E_UIDesigner.h"

        # Built-in Default Plugins (Off by default)
        "Plugins/Blueprint/BlueprintPlugin.cpp"
        "Plugins/Blueprint/BlueprintPlugin.h"
        "Plugins/CodeEditor/CodeEditorPlugin.cpp"
        "Plugins/CodeEditor/CodeEditorPlugin.h"
    )

    add_executable(AuroraEditor ${AURORA_EDITOR_SOURCES})

    target_include_directories(AuroraEditor
      PRIVATE
        Editor/Source
        Plugins/Blueprint
        Plugins/CodeEditor
    )

    target_link_libraries(AuroraEditor
      PRIVATE
        EngineSource
    )

    # Output directly to AuroraProject root (D:/AuroraProject/AuroraEditor.exe)
    set_target_properties(AuroraEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()