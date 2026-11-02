#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>
#include <I_Keyboard.h>
#include <I_Mouse.h>
#include <M_Window.h>

// ============================================================================
// Cross-Platform Input & Keybind Manager (SDL3 Hardware Poller + Action Binds)
// ============================================================================
// Quick Usage Example:
//   // 1. Register remappable actions:
//   M_Input::Bind("MoveForward", Key::W);
//   M_Input::Bind("Inventory",   Key::I);
//   M_Input::Bind("FirePrimary", MouseButton::MB_Left);
//
//   // 2. Inside your main while (M_Input::Update()) loop:
//   if (M_Input::Get_Action("MoveForward", InputState::Held))    { /* Fly Forward */ }
//   if (M_Input::Get_Action("Inventory",   InputState::Pressed)) { /* Open Bag once */ }
//   if (M_Input::Get_Action("FirePrimary", InputState::Held))    { /* Shoot! */ }
// ============================================================================
class M_Input {
private:
    struct ActionBinding {
        BOOL        isMouse = false;
        Key         key     = Key::Unknown;
        MouseButton button  = MouseButton::MB_Left;
    };

    // Transparent hash & equality so Get_Action(StringView) never allocates a heap String!
    struct StringHash {
        using is_transparent = void;
        size_t operator()(StringView sv) const noexcept { return std::hash<StringView>{}(sv); }
    };

    static std::unordered_map<String, ActionBinding, StringHash, std::equal_to<>> s_Bindings;

public:
    // Re-export InputState values so callers can write M_Input::Pressed, M_Input::Held, M_Input::Released!
    static constexpr InputState Pressed  = InputState::Pressed;
    static constexpr InputState Held     = InputState::Held;
    static constexpr InputState Released = InputState::Released;

    // ========================================================================
    // Core Frame Polling (Reads OS/SDL3 Hardware Events into Keyboard & Mouse)
    // ========================================================================

    // Call ONCE per frame in your main loop! Polls SDL3 Keyboard, Mouse, and Window events.
    // Returns FALSE when the user closes the window (clicks [X] or Alt+F4).
    // Example: while (M_Input::Update()) { ... }
    static BOOL Update();

    // ========================================================================
    // Bind & Rebind Actions
    // ========================================================================

    // Binds an Action Name to a Keyboard Key.
    // Example: M_Input::Bind("MoveForward", Key::W);
    static void Bind(StringView actionName, Key defaultKey);

    // Binds an Action Name to a Mouse Button.
    // Example: M_Input::Bind("FirePrimary", MouseButton::MB_Left);
    static void Bind(StringView actionName, MouseButton defaultButton);

    // Changes an existing Action's keybind at runtime.
    // Example: M_Input::Rebind("MoveForward", Key::Up);
    static void Rebind(StringView actionName, Key newKey);

    // Changes an existing Action's bind to a Mouse Button at runtime.
    // Example: M_Input::Rebind("FirePrimary", MouseButton::MB_Right);
    static void Rebind(StringView actionName, MouseButton newButton);

    // ========================================================================
    // Check Action State (Works for both Keyboard & Mouse Binds!)
    // ========================================================================

    // Checks if a bound Action is Pressed, Held, or Released this frame.
    // Example: if (M_Input::Get_Action("MoveForward", M_Input::Held)) { ... }
    static BOOL Get_Action(StringView actionName, InputState state);
};
