#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <RHI_Manager.h>

// ============================================================================
// M_Font: TrueType Font Atlas Manager (stb_truetype powered, Zero GDI Overhead)
// ============================================================================
// 1. Reads any .ttf file from disk into RAM.
// 2. Automatically bakes all printable ASCII characters (32..126) into a crisp
//    GPU texture atlas once at startup.
// 3. 100% GPU quad rendered: zero Windows GDI calls during runtime, zero lag!
// 4. Default fallback: automatically detects Windows system fonts (Segoe UI, Arial, Tahoma)
//    if no custom game font is placed in Assets/Fonts/.
// ============================================================================
class M_Font {
private:
    static UINT      s_FontTextureID;
    static FLOAT     s_BakedFontSize;
    static FLOAT     s_LineHeight;
    static FLOAT     s_Ascent;
    static BOOL      s_IsInitialized;
    static void     *s_BakedCharData; // Internal stbtt_bakedchar[96]

    static void Ensure_Initialized();

public:
    // Initializes or reloads font from a .ttf file at specified pixel height (default 24px)
    static BOOL Init(const FilePath &ttfPath = "", FLOAT fontSize = 24.0f);

    // Shuts down font atlas and releases texture
    static void Shutdown();

    // Measures the width and height of a string in screen pixels
    [[nodiscard]] static Vector2D Measure_String(StringView text, FLOAT scale = 1.0f);

    // Draws raw unshadowed text at screen coordinates (x, y is top-left in pixels)
    static void Draw_String(
        StringView     text,
        FLOAT          x,
        FLOAT          y,
        const COLOR   &color = COLOR_WHITE,
        FLOAT          scale = 1.0f);

    // Draws crisp MMORPG-style shadowed text (readable against any 3D environment)
    static void Draw_String_Shadowed(
        StringView     text,
        FLOAT          x,
        FLOAT          y,
        const COLOR   &color       = COLOR_WHITE,
        const COLOR   &shadowColor = COLOR_BLACK,
        FLOAT          scale       = 1.0f);

    // Accessors
    [[nodiscard]] static inline UINT    Get_TextureID()   { Ensure_Initialized(); return s_FontTextureID; }
    [[nodiscard]] static inline FLOAT   Get_LineHeight()  { Ensure_Initialized(); return s_LineHeight; }
    [[nodiscard]] static inline FLOAT   Get_Ascent()      { Ensure_Initialized(); return s_Ascent; }
    [[nodiscard]] static inline FLOAT   Get_BakedSize()   { return s_BakedFontSize; }
    [[nodiscard]] static inline BOOL    Is_Initialized()  { return s_IsInitialized; }
};
