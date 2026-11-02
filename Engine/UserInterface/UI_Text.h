#pragma once

#include <UI_Frame.h>
#include <U_Colors.h>
#include <M_Font.h>

// ============================================================================
// UI_Text: 2D Screen-Space Text Element (Inherits from UI_Frame)
// ============================================================================
// 1. Inherits Position, Size, and Bounds from UI_Frame.
// 2. Uses TrueType font atlas (stb_truetype powered) for zero GDI overhead.
// 3. Automatically measures text dimensions to set Size.x and Size.y!
// 4. Supports high-contrast MMORPG drop-shadow (bShadow).
// ============================================================================
class UI_Text : public UI_Frame {
public:
    String Text;
    COLOR  Color       = COLOR_WHITE; // Text foreground color
    COLOR  ShadowColor = COLOR_BLACK; // Drop-shadow color
    BOOL   bShadow     = true;        // MMORPG drop-shadow toggle
    FLOAT  Scale       = 1.0f;        // Size scale multiplier (1.0 = native 24px)

    UI_Text() = default;

    UI_Text(StringView text,
            FLOAT x = 0.0f, FLOAT y = 0.0f,
            const COLOR &color = COLOR_WHITE,
            BOOL bShadow = true,
            FLOAT scale = 1.0f);

    void SetText(StringView text);
    inline void Set_Text(StringView text) { SetText(text); }

    [[nodiscard]] inline StringView GetText() const { return Text; }
    [[nodiscard]] inline StringView Get_Text() const { return Text; }

    inline void SetColor(const COLOR &color) { Color = color; }
    inline void SetColor(BYTE r, BYTE g, BYTE b, BYTE a = 255) { Color = COLOR(r, g, b, a); }

    inline void SetScale(FLOAT scale) {
        Scale = scale;
        UpdateSize();
    }
    inline void Set_Scale(FLOAT scale) { SetScale(scale); }

    inline void SetShadow(BOOL shadow, const COLOR &shadowColor = COLOR_BLACK) {
        bShadow     = shadow;
        ShadowColor = shadowColor;
    }

    void Render() const override;

private:
    void UpdateSize();
};

// Aliases
using F_Text = UI_Text;
using FText  = UI_Text;
