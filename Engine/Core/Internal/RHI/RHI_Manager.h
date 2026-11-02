#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <M_Log.h>
#include <M_Window.h>
#include <M_Mesh.h>

// ============================================================================
// Supported Graphics Backend APIs
// ============================================================================
enum class API_Type : BYTE {
    None      = 0,
    Vulkan    = 1, // Vulkan 1.3 / 1.4 Core (Default, Native Cross-Platform)
    DirectX11 = 2, // DirectX 11 (Windows DXGI / DXVK)
    DirectX9  = 3  // DirectX 9  (Windows D3D9 / DXVK)
};

// ============================================================================
// Device & Graphics Compatibility Information (Pure Runtime Detection)
// ============================================================================
struct DeviceCompatibility {
    String Platform;                 // "Windows", "Linux", "macOS", etc. (via SDL_GetPlatform)
    String GPU;                      // e.g. "NVIDIA GeForce RTX 4080", "AMD Radeon RX 7800 XT"
    BOOL   bIsWineProton     = false;// Running on Linux/Steam Deck via Wine/Proton
    BOOL   bHasDirectX11     = false;// d3d11.dll or DXVK is present and callable
    BOOL   bHasDirectX9      = false;// d3d9.dll or DXVK is present and callable
    BOOL   bHasVulkan        = true; // Vulkan is natively supported everywhere
    String CompatibilityNote;        // Pre-formatted notice for in-game UI / Settings HUD
};

// ============================================================================
// Master RHI Manager (PoE 2 Style Dynamic Graphics Backend Dispatcher)
// ============================================================================
// Features:
//   - Encapsulates and hides low-level Vulkan / DirectX swapchains and devices
//   - Hot-swaps backends on the live window via RHI_Manager::Swap_API(API_Type)
//   - Pure runtime hardware & platform compatibility detection (Zero #ifdef)
//   - Safe crash-free fallback and in-game UI warning notifications
//   - Headless awareness: zero window / graphics overhead on dedicated servers
// ============================================================================
class RHI_Manager {
private:
    static API_Type s_ActiveAPI;
    static BOOL     s_VSync;
    static BOOL     s_IsInitialized;
    static String   s_ActiveWarning;

    static BOOL InitBackend(API_Type api, SDL_Window *window);
    static void ShutdownBackend(API_Type api);

public:
    // ========================================================================
    // Hardware & Platform Runtime Detection (Zero #ifdef, 100% Cross-Platform)
    // ========================================================================

    // Probes the running environment dynamically without crashing or compile-time OS locks
    static DeviceCompatibility Detect_Device();

    // Checks if the requested API is supported on the user's current environment
    static BOOL Is_APISupported(API_Type api);

    // ========================================================================
    // In-Game UI / HUD Query Helpers
    // ========================================================================

    // Returns active warning for in-game UI overlay/dialog (empty if everything is normal)
    static inline StringView Get_ActiveWarning() {
        return s_ActiveWarning;
    }

    // Clears the active warning once dismissed by the player in the in-game GUI
    static inline void Clear_ActiveWarning() {
        s_ActiveWarning.clear();
    }

    // Helper for Settings Menu: provides readable status text for each API option
    static String Get_APIStatusText(API_Type api);

    // ========================================================================
    // Init & Shutdown (Called automatically by Core::BeginPlay / EndPlay)
    // ========================================================================

    // Initializes the active Graphics API (Default: Vulkan) on the live window.
    // Graceful fallback: If requested backend is missing, logs warning and defaults to Vulkan/None.
    // Headless processes (e.g. FieldServer) automatically bypass RHI with 0 graphics overhead.
    static BOOL Init();

    // Shuts down the active graphics backend cleanly
    static void Shutdown();

    // Sets or hot-swaps graphics backends on the fly (PoE 2 style, no game restart required!)
    // Works both BEFORE Core::BeginPlay (selects initial API) and AFTER (live hot-swap).
    // Example: RHI_Manager::Swap_API(API_Type::DirectX11);
    static BOOL Swap_API(API_Type newAPI);

    // ========================================================================
    // Frame Rendering
    // ========================================================================

    // Prepares the screen for drawing. Clears screen using Dynamic Rendering.
    // Returns TRUE if frame is ready to render; FALSE if minimized or resizing.
    static BOOL Begin_Frame();

    // Draws a MeshData directly with position, rotation, size, color tint, transparency (0.0 to 1.0), and optional TextureID.
    // Example: RHI_Manager::Draw_Mesh(cubeMesh, { -1.2f, 0.0f, 0.0f }, { 0.0f, 45.0f, 0.0f });
    static void Draw_Mesh(
        const MeshData &mesh,
        const Vector3D &pos          = { 0.0f, 0.0f, 0.0f },
        const Vector3D &rot          = { 0.0f, 0.0f, 0.0f },
        const Vector3D &size         = { 1.0f, 1.0f, 1.0f },
        const COLOR    &color        = COLOR_WHITE,
        FLOAT           transparency = 1.0f,
        UINT            textureID    = 0);

    // Draws a 2D screen-space UI element (Quad) with texture (or 0 for untextured box), pixel position, pixel size, color tint and transparency.
    // Coordinates are in screen pixels: (0, 0) is top-left, (WindowWidth, WindowHeight) is bottom-right.
    static void Draw_UI(
        UINT            textureID,
        const Vector2D &pos,
        const Vector2D &size,
        const COLOR    &color        = COLOR_WHITE,
        FLOAT           transparency = 1.0f);

    // Draws a 2D screen-space UI element with a custom UV sub-rectangle (sprite atlas / font glyph).
    static void Draw_UI_SubRect(
        UINT            textureID,
        const Vector2D &pos,
        const Vector2D &size,
        const Vector2D &uvMin,
        const Vector2D &uvMax,
        const COLOR    &color        = COLOR_WHITE,
        FLOAT           transparency = 1.0f);

    // Batched 2D UI Quad rendering: draws an array of screen quads in a single GPU draw call!
    static void Draw_UI_Quads(
        UINT          textureID,
        const UIQuad *quads,
        UINT          count);

    // Finishes rendering and presents the frame to screen
    static void End_Frame();

    // ========================================================================
    // Settings & Queries
    // ========================================================================

    // Turns VSync ON or OFF dynamically across all backends (works before OR after Core::BeginPlay).
    static void Set_VSync(BOOL enabled);

    static BOOL Get_VSync();

    static void Set_ClearColor(const COLOR &color);

    static COLOR Get_ClearColor();

    static StringView Get_GPUName();

    static inline API_Type Get_ActiveAPI() {
        return s_ActiveAPI;
    }

    static inline const Char *Get_APIName(API_Type api) {
        switch (api) {
            case API_Type::Vulkan:    return "Vulkan";
            case API_Type::DirectX11: return "DirectX 11";
            case API_Type::DirectX9:  return "DirectX 9";
            default:                  return "None";
        }
    }

    // Converts string ("Vulkan", "DirectX11", "DirectX") into API_Type
    static inline API_Type Parse_API(StringView name) {
        if (name == "DirectX11" || name == "DirectX" || name == "DirectX 11" || name == "DX11") return API_Type::DirectX11;
        if (name == "DirectX9" || name == "DirectX 9" || name == "DX9") return API_Type::DirectX9;
        return API_Type::Vulkan;
    }

    static inline BOOL Is_Initialized() {
        return s_IsInitialized;
    }
};
