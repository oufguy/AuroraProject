#pragma once

#include <UI_ImageBox.h>

// ============================================================================
// Button States (0, 1, 2, 3)
// ============================================================================
enum class ButtonState : BYTE {
    Normal   = 0, // State 0: Default idle state
    Hover    = 1, // State 1: Mouse cursor is hovering over button
    Pressed  = 2, // State 2: Left click is held down on button (or locked pressed)
    Disabled = 3  // State 3: Disabled / Off (grayed out, ignores all mouse clicks)
};

// ============================================================================
// UI_ImageBtn: 4-State Interactive Button (Inherits from UI_ImageBox)
// ============================================================================
class UI_ImageBtn : public UI_ImageBox {
public:
    // Textures for each of the 4 states:
    UINT NormalTex   = 0; // State 0 (Normal / Idle)
    UINT HoverTex    = 0; // State 1 (Mouse Hover)
    UINT PressedTex  = 0; // State 2 (Mouse Pressed / Clicked)
    UINT DisabledTex = 0; // State 3 (Disabled / Off)

    // State Locking: -1 = Auto (follows mouse), 0..3 = locked to that specific state
    INT LockedState = -1;

    UI_ImageBtn() = default;

    // 4-Texture path constructor (Ace Online style)
    UI_ImageBtn(const FilePath &normal,
                const FilePath &hover    = "",
                const FilePath &pressed  = "",
                const FilePath &disabled = "",
                FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT w = 0.0f, FLOAT h = 0.0f);

    // 4-Texture ID constructor
    UI_ImageBtn(UINT normal, UINT hover = 0, UINT pressed = 0, UINT disabled = 0,
                FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT w = 0.0f, FLOAT h = 0.0f);

    // ------------------------------------------------------------------------
    // Change / Set Textures Later
    // ------------------------------------------------------------------------
    void SetTextures(const FilePath &normal,
                     const FilePath &hover    = "",
                     const FilePath &pressed  = "",
                     const FilePath &disabled = "");

    void SetTextures(UINT normal, UINT hover = 0, UINT pressed = 0, UINT disabled = 0);

    void SetNormal(const FilePath &path);
    void SetHover(const FilePath &path);
    void SetPressed(const FilePath &path);
    void SetDisabled(const FilePath &path);

    void SetNormal(UINT textureID);
    void SetHover(UINT textureID);
    void SetPressed(UINT textureID);
    void SetDisabled(UINT textureID);

    // ------------------------------------------------------------------------
    // State Locking (0 = Normal, 1 = Hover, 2 = Pressed, 3 = Disabled/Off)
    // ------------------------------------------------------------------------
    inline void LockState(BYTE state) {
        LockedState = state;
    }
    inline void LockState(ButtonState state) {
        LockedState = static_cast<INT>(state);
    }

    inline void UnlockState() {
        LockedState = -1;
    }

    inline void SetState(INT state) {
        LockedState = state;
    }
    inline void SetState(ButtonState state) {
        LockedState = static_cast<INT>(state);
    }

    inline void SetDisabled(BOOL bDisable) {
        LockedState = bDisable ? static_cast<INT>(ButtonState::Disabled) : -1;
    }

    [[nodiscard]] inline BOOL IsLocked() const {
        return LockedState >= 0;
    }

    [[nodiscard]] BYTE GetCurrentState() const;

    // ------------------------------------------------------------------------
    // Mouse Interaction Queries
    // ------------------------------------------------------------------------
    [[nodiscard]] BOOL Is_Down() const;
    [[nodiscard]] BOOL Is_Clicked() const;

    // ------------------------------------------------------------------------
    // Manual Render (Renders active state's texture)
    // ------------------------------------------------------------------------
    void Render() const override;
};

// Aliases
using UI_Button  = UI_ImageBtn;
using F_ImageBtn = UI_ImageBtn;
using FImageBtn  = UI_ImageBtn;
