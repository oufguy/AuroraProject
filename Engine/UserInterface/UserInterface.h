#pragma once

// ============================================================================
// Aurora Engine 2D User Interface Module Header
// ============================================================================
// Clean OOP hierarchy:
//   UI_Frame (Mother Base Class: Position, Size, Contains, Is_Hovered, virtual Render)
//    ├── UI_ImageBox (Textured 2D Quad, Tint, Native Size snap)
//    │    └── UI_ImageBtn (4-State Button: Normal, Hover, Pressed, Disabled)
//    ├── UI_Box (Solid Color Panel / HUD Backdrop)
//    └── UI_Text (TrueType Font Atlas Text, Drop-Shadow, Auto-Sizing)
// ============================================================================
#include <UI_Frame.h>
#include <UI_ImageBox.h>
#include <UI_ImageBtn.h>
#include <UI_Box.h>
#include <UI_Text.h>
#include <UI_FPSCounter.h>
#include <UI_FloatingText.h>

// ============================================================================
// Helper Factories & Direct Text Draw (namespace UI)
// ============================================================================
namespace UI {
    // Direct instant text rendering helpers (2000s style, call directly in Tick or Render!)
    inline void Draw_Text(StringView text, FLOAT x, FLOAT y, const COLOR &color = COLOR_WHITE, FLOAT scale = 1.0f) {
        M_Font::Draw_String(text, x, y, color, scale);
    }

    inline void Draw_Text_Shadowed(StringView text, FLOAT x, FLOAT y, const COLOR &color = COLOR_WHITE, const COLOR &shadowColor = COLOR_BLACK, FLOAT scale = 1.0f) {
        M_Font::Draw_String_Shadowed(text, x, y, color, shadowColor, scale);
    }

    // Direct instant solid 2D box / panel / healthbar drawing
    inline void Draw_Box(FLOAT x, FLOAT y, FLOAT w, FLOAT h, const COLOR &color = COLOR_WHITE, FLOAT transparency = 1.0f) {
        RHI_Manager::Draw_UI(0, { x, y }, { w, h }, color, transparency);
    }

    // On-screen diagnostic performance overlay (FPS, frame time, RHI, entity counts)
    inline void Draw_FPS(FLOAT x = 20.0f, FLOAT y = 20.0f) {
        UI_FPSCounter::Render(x, y);
    }

    inline void Toggle_FPS() {
        UI_FPSCounter::Toggle();
    }

    // 3D Floating Combat Damage Text helpers
    inline void Spawn_Damage(const Vector3D &worldPos, INT damage, BOOL bCritical = false) {
        UI_FloatingText::Spawn_Damage(worldPos, damage, bCritical);
    }

    inline void Spawn_Heal(const Vector3D &worldPos, INT healAmount) {
        UI_FloatingText::Spawn_Heal(worldPos, healAmount);
    }

    inline void Spawn_FloatingText(const Vector3D &worldPos, StringView text, const COLOR &color = COLOR_YELLOW, FLOAT scale = 1.0f, FLOAT duration = 1.2f) {
        UI_FloatingText::Spawn_Text(worldPos, text, color, scale, duration);
    }

    inline void Tick_FloatingText(FLOAT deltaTime) {
        UI_FloatingText::Tick(deltaTime);
    }

    inline void Render_FloatingText(const C_Camera &camera) {
        UI_FloatingText::Render(camera);
    }

    [[nodiscard]] inline Vector2D Measure_Text(StringView text, FLOAT scale = 1.0f) {
        return M_Font::Measure_String(text, scale);
    }

    [[nodiscard]] inline UI_ImageBox Create_Image(const FilePath &path = "", FLOAT x = 0, FLOAT y = 0, FLOAT w = 0, FLOAT h = 0) {
        return UI_ImageBox(path, x, y, w, h);
    }

    [[nodiscard]] inline UI_ImageBtn Create_Button(const FilePath &normal = "",
                                                   const FilePath &hover = "",
                                                   const FilePath &pressed = "",
                                                   const FilePath &disabled = "",
                                                   FLOAT x = 0, FLOAT y = 0, FLOAT w = 0, FLOAT h = 0) {
        return UI_ImageBtn(normal, hover, pressed, disabled, x, y, w, h);
    }

    [[nodiscard]] inline UI_Box Create_Box(FLOAT x = 0, FLOAT y = 0, FLOAT w = 0, FLOAT h = 0, const COLOR &color = COLOR_WHITE) {
        return UI_Box(x, y, w, h, color);
    }

    [[nodiscard]] inline UI_Text Create_Text(StringView text = "", FLOAT x = 0, FLOAT y = 0, const COLOR &color = COLOR_WHITE, BOOL bShadow = true, FLOAT scale = 1.0f) {
        return UI_Text(text, x, y, color, bShadow, scale);
    }
}
