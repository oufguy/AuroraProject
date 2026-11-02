#pragma once

#include <UI_Frame.h>
#include <U_Colors.h>
#include <M_Texture.h>
#include <RHI_Manager.h>

// ============================================================================
// UI_Box: Solid Color 2D Panel / HUD Backdrop (Inherits from UI_Frame)
// ============================================================================
// Inherits Position, Size, and Bounds from UI_Frame.
// Renders an untextured solid quad tinted with Color.
// ============================================================================
class UI_Box : public UI_Frame {
public:
    COLOR Color = COLOR_WHITE;

    UI_Box() = default;
    UI_Box(FLOAT x, FLOAT y, FLOAT w = 0.0f, FLOAT h = 0.0f, const COLOR &color = COLOR_WHITE);

    inline void SetColor(const COLOR &color) { Color = color; }
    inline void SetColor(BYTE r, BYTE g, BYTE b, BYTE a = 255) { Color = COLOR(r, g, b, a); }

    void Render() const override;
};

// Aliases
using UI_Panel = UI_Box;
using F_Box    = UI_Box;
using FBox     = UI_Box;
