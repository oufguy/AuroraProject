#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <I_Mouse.h>

// ============================================================================
// UI_Frame: Mother Base Class of all 2D User Interface Elements
// ============================================================================
// 1. Represents the pure 2D geometric layout frame (Position, Size, Bounds).
// 2. 100% manual rendering: if you call .Render(), it renders.
//    If you don't call it, it disappears. Easy to store in your own arrays & structs!
// 3. Provides ergonomic setters (SetPosition(x, y), SetSize(w, h)) with NO Vector2D boilerplate.
// 4. Provides mouse hit-testing: Contains(point), Is_Hovered().
// ============================================================================
class UI_Frame {
public:
    Vector2D Position     = { 0.0f, 0.0f }; // Top-left screen pixel position (0, 0 is top-left)
    Vector2D Size         = { 0.0f, 0.0f }; // Width & Height in screen pixels
    FLOAT    Transparency = 1.0f;           // 1.0f = opaque, 0.0f = invisible

    UI_Frame() = default;
    virtual ~UI_Frame() = default;

    UI_Frame(FLOAT x, FLOAT y, FLOAT w = 0.0f, FLOAT h = 0.0f);

    // ------------------------------------------------------------------------
    // Multi-Value Setters (No Vector2D constructor needed!)
    // ------------------------------------------------------------------------
    inline void SetPosition(FLOAT x, FLOAT y) {
        Position.x = x;
        Position.y = y;
    }
    inline void Set_Position(FLOAT x, FLOAT y) { SetPosition(x, y); }

    inline void SetSize(FLOAT w, FLOAT h) {
        Size.x = w;
        Size.y = h;
    }
    inline void Set_Size(FLOAT w, FLOAT h) { SetSize(w, h); }

    inline void SetTransparency(FLOAT alpha) { Transparency = alpha; }
    inline void Set_Transparency(FLOAT alpha) { SetTransparency(alpha); }

    // ------------------------------------------------------------------------
    // Ergonomic Getters
    // ------------------------------------------------------------------------
    [[nodiscard]] inline FLOAT GetWidth()   const { return Size.x; }
    [[nodiscard]] inline FLOAT GetHeight()  const { return Size.y; }
    [[nodiscard]] inline FLOAT GetX()       const { return Position.x; }
    [[nodiscard]] inline FLOAT GetY()       const { return Position.y; }

    // Checks if the given screen point lies inside this element's bounding box
    [[nodiscard]] inline BOOL Contains(const Vector2D &point) const {
        return (point.x >= Position.x && point.x <= Position.x + Size.x &&
                point.y >= Position.y && point.y <= Position.y + Size.y);
    }

    // Checks if the mouse cursor is currently hovering over this frame
    [[nodiscard]] inline BOOL Is_Hovered() const {
        return Contains(I_Mouse::Get_Position());
    }

    // ------------------------------------------------------------------------
    // Manual Render
    // ------------------------------------------------------------------------
    virtual void Render() const {}
    inline  void Draw()   const { Render(); }
};

// Aliases
using UI_FrameBox = UI_Frame;
