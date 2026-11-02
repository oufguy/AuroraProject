cmake_minimum_required(VERSION 3.25)
project(AuroraGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bake C++ runtime DLLs directly into .exe as Static (/MT and /MTd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Locate Aurora Engine SDK (Defaults to sibling ../AuroraProject or custom cache path)
if(NOT DEFINED AURORA_ENGINE_ROOT)
    if(DEFINED ENV{AURORA_ENGINE_ROOT})
        set(AURORA_ENGINE_ROOT "$ENV{AURORA_ENGINE_ROOT}")
    else()
        get_filename_component(AURORA_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../AuroraProject" ABSOLUTE)
    endif()
endif()

message(STATUS "[AuroraGame] Linking to Aurora Engine SDK at: ${AURORA_ENGINE_ROOT}")

if(NOT EXISTS "${AURORA_ENGINE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "[AuroraGame] Aurora Engine SDK not found at ${AURORA_ENGINE_ROOT}! Please set AURORA_ENGINE_ROOT.")
endif()

# Include Aurora Engine SDK
set(AURORA_BUILD_EDITOR OFF CACHE BOOL "Do not build editor in game project" FORCE)
add_subdirectory("${AURORA_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/AuroraEngine")

# ---------- Game Client Executable ----------
file(GLOB_RECURSE CLIENT_SOURCES
    "Source/Client/*.cpp"
    "Source/Client/*.h"
    "Source/Shared/*.cpp"
    "Source/Shared/*.h"
)

add_executable(GameClient ${CLIENT_SOURCES})

target_include_directories(GameClient
  PRIVATE
    Source/Client
    Source/Shared
)

target_link_libraries(GameClient
  PRIVATE
    EngineSource
)

# Output executable directly to Build/Binaries/
set_target_properties(GameClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/Binaries"
)
