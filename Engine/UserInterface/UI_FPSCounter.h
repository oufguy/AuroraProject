#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>

// ============================================================================
// Aurora Engine — On-Screen Performance & FPS Overlay (UI_FPSCounter)
// ============================================================================
// Single Responsibility:
//   Displays real-time diagnostic performance metrics on-screen:
//   - Current FPS and Frame Time (ms)
//   - Active Graphics Backend (DirectX 11 / Vulkan) & GPU device name
//   - 3D World visibility metrics (Total entities, Frustum visible, Culled)
//   - Toggleable via UI_FPSCounter::Toggle() or F2 hotkey
// ============================================================================

class UI_FPSCounter {
private:
    static BOOL s_bVisible;
    static BOOL s_bDetailed;

public:
    // Toggles visibility of the on-screen FPS counter overlay (Default hotkey: F2)
    static void Toggle();

    // Sets visibility directly
    static void Set_Visible(BOOL bVisible);

    [[nodiscard]] static inline BOOL Is_Visible() {
        return s_bVisible;
    }

    // Toggles between compact (FPS only) and detailed (FPS + ms + Backend + Entities)
    static void Toggle_Detailed();

    // Renders the performance HUD at the specified screen coordinates (default top-right or top-left)
    static void Render(FLOAT x = 20.0f, FLOAT y = 20.0f);
};
