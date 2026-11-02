#pragma once

#include <UI_Frame.h>
#include <U_Colors.h>
#include <M_Texture.h>
#include <RHI_Manager.h>

// ============================================================================
// UI_ImageBox: Textured 2D Image Quad (Inherits from UI_Frame)
// ============================================================================
// 1. Inherits Position, Size, and Bounds from UI_Frame.
// 2. Holds a GPU TextureID and Color Tint.
// 3. If TextureID is 0, it renders nothing.
// 4. If loaded with Size (0, 0), automatically snaps Size to native image resolution!
// 5. Missing texture files fallback to Ace Online style red "X" icon.
// ============================================================================
class UI_ImageBox : public UI_Frame {
public:
    UINT  TextureID = 0;           // GPU TextureID (0 = renders nothing)
    COLOR Tint      = COLOR_WHITE; // Color multiplier (COLOR_WHITE = untouched texture)

    UI_ImageBox() = default;

    // Direct path constructor
    UI_ImageBox(const FilePath &path, FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT w = 0.0f, FLOAT h = 0.0f);

    // Direct TextureID constructor
    UI_ImageBox(UINT textureID, FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT w = 0.0f, FLOAT h = 0.0f);

    // Sets texture from disk. If Size is currently (0, 0), snaps Size to native resolution!
    void SetTexture(const FilePath &path);
    void SetTexture(UINT textureID);

    inline void SetTint(const COLOR &color) { Tint = color; }
    inline void SetColor(const COLOR &color) { Tint = color; }
    inline void SetColor(BYTE r, BYTE g, BYTE b, BYTE a = 255) { Tint = COLOR(r, g, b, a); }

    void Render() const override;
};

// Aliases
using UI_Image = UI_ImageBox;
using F_Image  = UI_ImageBox;
using FImage   = UI_ImageBox;
