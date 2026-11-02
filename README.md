# Aurora Engine & Aurora Studio

A lightweight, high-performance C++23 game engine and single-binary level editor built from scratch. Designed with strict zero-bloat architecture, native multi-threading, custom RHI (DirectX 11 & Vulkan 1.3/1.4), and cross-platform compatibility for Windows and Linux (via Wine/Proton & native backends).

---

## Repository Structure

The repository follows a clean, minimalist layout separating the engine core, editor toolchain, and build artifacts:

```text
AuroraProject/
├── Build/                              # Intermediate build files, debug symbols (.pdb, .ilk), and dependencies
├── Editor/
│   ├── Plugins/                        # In-engine plugins (Blueprint Visual Scripting, Code Editor)
│   ├── Resources/                      # Application icons (.ico / .png), Windows resource scripts (.rc), .reg
│   ├── Source/                         # Aurora Studio implementation (Hub, Scene Editor, UI Designer, etc.)
│   └── Templates/                      # Project creation templates (Blank, GameExample Town Demo)
├── Engine/                             # Core engine framework (Application-agnostic runtime)
│   ├── Core/                           # Foundation: Window, Input, Threads, File, Crash, Log, RHI
│   ├── World/                          # 3D World: Entities, Camera, Mesh loaders, Spatial Hash Grid
│   ├── Online/                         # Low-latency networking & packet replication
│   └── UserInterface/                  # 2D Screen-space UI components (UI_Frame, UI_ImageBox, UI_Button)
├── AuroraEditor.exe                    # Unified Editor & Project Hub executable (Outputs to root)
├── CMakeLists.txt                      # Master CMake configuration
├── CMakePresets.json                   # Presets (Debug, Development, Release)
├── .gitignore / .gitattributes         # Git tracking configuration
└── README.md / CHANGELOG.md / LICENSE  # Documentation & License
```

---

## Key Features & Editor Toolchain

### 1. Project Hub & Workspace Isolation
* **Standalone Workspaces**: Game projects live outside the engine repository (e.g. `D:/MyGame/Project.aurora`).
* **Recent Projects**: Stored privately in `%APPDATA%/Aurora/Engine/RecentProjects.ini` so project paths are never accidentally committed or exposed.
* **Template Generation**: One-click creation of clean starter projects or full MMORPG demos from `Editor/Templates/`.
* **OS File Association**: Double-clicking any `.aurora` project file launches directly into Aurora Studio with custom multi-resolution application icons baked in.

### 2. Modern 3D Viewport & Navigation
* **Virtual Center-Warp Flycam**: Holding **RMB** locks the cursor to a virtual center anchor, rotating the camera smoothly without cursor drifting or breaking screen boundaries. Releasing RMB restores the cursor to its exact original desktop position.
* **Freecam Flight**: **W/A/S/D** for flight, **E** for upward elevation, **Q** for downward descent, and **Space** for turbo boost.

### 3. Direct 3D Gizmo & Raycast Object Translation
* **Ground Plane Raycast Dragging**: Left-click and drag any entity to slide it across the 3D ground plane in real time.
* **Axis Constraints**: Hold **X** (lock to Red X axis), **Y** (lock to Green elevation axis), or **Z** (lock to Blue depth axis).
* **Visual Helpers**: Interactive ground drop lines, target crosshairs, and cyan AABB wireframe bounding boxes highlight selected objects during editing.

### 4. Dual RHI Backends (DirectX 11 & Vulkan)
* **DirectX 11 Backend**: Zero-overhead modern DirectX 11 pipeline using dynamic symbol resolution without static OS SDK lock-in.
* **Vulkan Core Backend**: High-throughput Vulkan 1.3/1.4 rendering with explicit memory allocations, custom descriptor pools, and pipeline state caching.
* **Safe Configuration Persistence**: Switching backends is configured safely in `Project.aurora`, initializing the target GPU device cleanly before window creation to avoid device loss or invalid swapchains.

### 5. Built-in Studio Tool Suite
* **UI Designer**: Visual drag-and-drop designer for 2D game interfaces, HUDs, buttons, and image boxes.
* **Asset Packer & XOR Generator**: Proprietary asset encryption, XOR key generation, and `.pak` archive compression.
* **Plugin System**: Modular plugins located in `Editor/Plugins/` (Blueprint visual scripting, syntax-highlighted code editor).

---

## Building from Source

### Windows (64-bit)

#### Prerequisites
* **Windows 10 / 11 (64-bit)**
* **Visual Studio 2022 or 2026** (MSVC v143+ with C++23 support)
* **CMake 3.25+** and **Ninja**
* **Vulkan SDK** (Vulkan 1.3+)

#### Method 1: Inside Visual Studio (Recommended)
1. Open the repository root `AuroraProject/` in Visual Studio (`File -> Open -> Folder`).
2. Select your preset from the configuration dropdown (`Debug`, `Development`, or `Release`).
3. Press **`Ctrl + Shift + B`** (or click **Build $\rightarrow$ Build All**). Visual Studio automatically configures the 64-bit environment and builds `AuroraEditor.exe`.

#### Method 2: Command Line (CLI)
> **Important**: Aurora Engine is a 64-bit engine and requires the 64-bit MSVC environment. Use the **"x64 Native Tools Command Prompt for VS 2026"** from the Start Menu, or initialize 64-bit in PowerShell:

```powershell
# In PowerShell (run once per terminal session to load 64-bit tools):
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -SkipAutomaticLocation

# Build Debug configuration (full logs and debug info):
cmake --build --preset Debug

# Build Development configuration (optimized with debug symbols):
cmake --build --preset Development

# Build Release configuration (maximum optimization, stripped binary ~3.9 MB):
cmake --build --preset Release
```

*The binary `AuroraEditor.exe` outputs directly into the repository root `AuroraProject/`, while debug symbols (`.pdb`, `.ilk`) and intermediate files stay neatly inside `Build/`.*

---

### Linux (Native Compilation)

Aurora Engine is built from the ground up with standard C++23 and zero Windows OS headers (`<windows.h>` strictly avoided). It compiles natively on Linux using GCC or Clang:

#### Prerequisites
* **Ubuntu / Debian / Linux Mint**:
  ```bash
  sudo apt update
  sudo apt install build-essential cmake ninja-build libvulkan-dev
  ```
* **Arch Linux / Manjaro**:
  ```bash
  sudo pacman -S base-devel cmake ninja vulkan-devel
  ```
* **Fedora / RHEL**:
  ```bash
  sudo dnf install gcc-c++ cmake ninja-build vulkan-loader-devel
  ```

#### Build & Run Commands
```bash
# 1. Clone repository:
git clone https://github.com/oufguy/AuroraProject.git
cd AuroraProject

# 2. Configure & Build:
cmake --preset Release
cmake --build --preset Release

# 3. Launch native Linux editor:
./AuroraEditor
```

---

### Linux (Running Windows Binary via Wine / Proton)

If you already have `AuroraEditor.exe` compiled from Windows or downloaded from a release bundle, you can run it on Linux without compiling anything:

```bash
# 1. Run Aurora Studio & Project Hub:
wine AuroraEditor.exe

# 2. Open a project workspace directly:
wine AuroraEditor.exe --project /home/user/AuroraWorldOnline

# 3. Or launch via Proton (Steam / Steam Deck):
proton run AuroraEditor.exe
```

#### Why it runs with native performance under Wine:
* **Vulkan Native Pass-Through**: When configured with `RHIBackend = Vulkan`, Wine / Proton forwards Vulkan commands directly to the host Linux GPU driver (`RADV`, `AMDVLK`, or `NVIDIA`) with **100% native performance**.
* **DirectX 11 via DXVK**: When running with DirectX 11, Wine automatically uses **DXVK** (the same layer used by Steam Proton) to translate D3D11 calls to Vulkan with near-native efficiency.
* **Static SDL3 & Zero DLLs**: SDL3 is statically compiled inside the binary, cleanly routing windowing to Wayland / X11, audio to PipeWire / PulseAudio, and input to Linux evdev without missing DLL dependencies.

---

## Sharing & Distribution: How It Works

### Scenario A: Sending to a Friend (Artist, Level Designer, Player)
Your friend **does not need** the source code, Visual Studio, or CMake. `AuroraEditor.exe` is completely self-contained because SDL3 is compiled statically into the binary with zero external DLL requirements.

To package a standalone build for your friend, zip only:
```text
AuroraStudio/
├── AuroraEditor.exe                    # Standalone engine & editor executable
└── Editor/
    └── Templates/                      # Starter project templates for "+ New Project"
        ├── Blank/
        └── GameExample/
```
*(You can also include `Editor/Resources/Register_Aurora_Extension.reg` so your friend can double-click it to associate `.aurora` project files with the editor).*

### Scenario B: Sharing with a Developer
Developers simply clone the repository from GitHub and run `cmake --build --preset Debug`. Everything (including SDL3) is fetched and configured automatically via CMake `FetchContent`.

---

## Compilation Model: Engine C++ vs Project Content

### Will changing the Engine require recompiling the Editor?
* **Yes, for Engine C++ changes**: The engine (`EngineSource`) is compiled as a high-performance static library and baked directly into `AuroraEditor.exe`. If you modify core engine C++ files (e.g. `Engine/Core/`, `Engine/RHI/`, `Engine/World/`), you run `cmake --build --preset Debug`.
  * *Thanks to Ninja and precompiled headers (`pch.h`), incremental compilation only recompiles modified files, finishing in ~2 to 5 seconds.*
  * *This is the exact same architectural model used by Unreal Engine (`UnrealEditor.exe`) and Godot Engine (`godot.exe`), guaranteeing zero DLL version conflicts and maximum inlining optimizations.*

### Do I recompile when working on a Game?
* **No**: Level editing, scene objects, 3D models (`.obj`), audio (`.wav`), `.scene` files, and `.aurora` configuration files load **dynamically at runtime**. You can create, move, texture, and save objects inside your game workspace without ever recompiling the editor.

---

## Engine Architecture Quick Reference

### 1. Thread Manager (`M_Threads.h`)
Worker thread pool powered by C++23 standard threads for background decoding and parallel updates:
```cpp
// Fire-and-forget background worker:
Threads::Run([]() {
    // Heavy texture decompression or disk operations
});

// Multi-core parallel for loop:
Threads::Parallel_For(0, 2500, [](size_t i) {
    // Process entity physics batches across CPU cores
});
```

### 2. File Manager (`M_File.h`)
Cross-platform thread-safe file handling:
```cpp
// Check, read, write, or clean files safely:
if (M_File::Exists("Config/Packs.ini")) {
    String content = M_File::Open("Config/Packs.ini");
    M_File::Close(content);
}
M_File::Write("Logs/Output.log", "Initialized\n", /*bOverWrite=*/true);
```

### 3. Crash Guard (`M_Crash.h`)
Hardware exception interceptor creating detailed crash reports (`Aurora StudioCrash.log`) with call stacks and source locations:
```cpp
Crash::Check(entityPtr != nullptr, "Entity pointer was null during spawn!");
```

---

## License

This project is licensed under the MIT License — see the [`LICENSE.txt`](LICENSE.txt) file for details.
