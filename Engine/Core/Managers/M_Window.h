#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>

// ============================================================================
// Cross-Platform Window Manager (SDL3 - Windows, Linux X11 & Wayland)
// ============================================================================
// Quick Usage Example:
//   M_Window::Set_Title("Aurora Client");
//   M_Window::Set_Size(1280, 720);
//   M_Window::Set_Mode(M_Window::Windowed);
//
//   while (Core::Tick()) {
//       if (I_Keyboard::Get_Key(Key::F11, M_Input::Pressed)) {
//           M_Window::Set_Mode(M_Window::Is_Fullscreen() ? M_Window::Windowed : M_Window::Borderless);
//       }
//   }
// ============================================================================
class M_Window {
    friend class M_Input; // M_Input::Update() polls SDL events and forwards window events here

public:
    // Display mode for Set_Mode(...)
    enum WindowMode : BYTE {
        Windowed   = 0, // Normal window with title bar (resizable)
        Borderless = 1, // Fullscreen at desktop resolution (instant Alt+Tab, recommended!)
        Fullscreen = 2  // Exclusive fullscreen (changes monitor resolution to Width x Height)
    };

    static inline WindowMode ModeFromString(StringView text) {
        if (text == "Borderless") return Borderless;
        if (text == "Fullscreen") return Fullscreen;
        return Windowed;
    }

    static inline const Char *ModeToString(WindowMode mode) {
        switch (mode) {
            case Borderless: return "Borderless";
            case Fullscreen: return "Fullscreen";
            default:         return "Windowed";
        }
    }

private:
    static SDL_Window *s_Window;
    static BOOL        s_IsOpen;
    static BOOL        s_IsFocused;
    static BOOL        s_IsMouseLocked;
    static Vector2D    s_SavedMousePos;
    static BOOL        s_IsMinimized;
    static BOOL        s_WasResized;
    static String      s_Title;
    static WindowMode  s_Mode;
    static INT         s_Width;
    static INT         s_Height;
    static INT         s_PixelWidth;
    static INT         s_PixelHeight;
    static INT         s_WindowedWidth;
    static INT         s_WindowedHeight;

    // Applies Windowed / Borderless / Exclusive Fullscreen to the live SDL window
    static void ApplyMode(WindowMode mode, INT width, INT height);

    // Called by M_Input::Update() at the start of every frame
    static void BeginFrame();

    // Called by M_Input::Update() for every SDL window event
    static void OnEvent(const SDL_Event &ev);

public:
    // ========================================================================
    // Create & Destroy (Called automatically by Core::BeginPlay / EndPlay)
    // ========================================================================

    // Opens the OS window using current Title, Width, Height, and Mode.
    // Returns FALSE if the OS refused to create a window (no display, e.g. headless Linux server).
    static BOOL Create(StringView title = "");

    // Closes the window and shuts down SDL video (called automatically by Core::EndPlay).
    static void Destroy();

    // ========================================================================
    // Set_... (Title, Size, Mode, Mouse Lock — Works before OR after Core::BeginPlay!)
    // ========================================================================

    // Changes the text in the window title bar.
    // Example: M_Window::Set_Title("Aurora Client");
    static void Set_Title(StringView title);

    // Sets the window size (Width, Height).
    // Example: M_Window::Set_Size(1920, 1080);
    static void Set_Size(INT width, INT height);

    // Switches Windowed / Borderless / Fullscreen mode.
    // Example: M_Window::Set_Mode(M_Window::Borderless);
    static void Set_Mode(WindowMode mode);

    // Locks & hides the cursor inside the window -> I_Mouse::Get_Delta() keeps working endlessly.
    // Perfect for rotating the camera while holding Right Mouse Button (MMO style!).
    // Automatically guards against redundant Win32 / SDL OS calls when the state is unchanged.
    // Example: M_Window::Set_MouseLocked(I_Mouse::Get_Button(MouseButton::MB_Right, M_Input::Held));
    static void Set_MouseLocked(BOOL locked);

    // ========================================================================
    // Get_... / Is_... / Was_...
    // ========================================================================

    // Returns the window size in screen units (use for UI layout & mouse coordinates).
    // Example: Vector2D size = M_Window::Get_Size();
    static inline Vector2D Get_Size() {
        return { static_cast<FLOAT>(s_Width), static_cast<FLOAT>(s_Height) };
    }

    static inline INT Get_Width()  { return s_Width; }
    static inline INT Get_Height() { return s_Height; }

    static inline INT Get_PixelWidth()  { return s_PixelWidth; }
    static inline INT Get_PixelHeight() { return s_PixelHeight; }

    static inline INT Get_WindowedWidth()  { return s_WindowedWidth; }
    static inline INT Get_WindowedHeight() { return s_WindowedHeight; }

    // Returns the window size in real screen PIXELS (use for Vulkan swapchain / render resolution).
    // (On a 150% scaled monitor, a 1280x720 window is 1920x1080 pixels!)
    // Example: Vector2D pixels = M_Window::Get_PixelSize();
    static inline Vector2D Get_PixelSize() {
        return { static_cast<FLOAT>(s_PixelWidth), static_cast<FLOAT>(s_PixelHeight) };
    }

    // Returns the current mode (Windowed / Borderless / Fullscreen).
    static inline WindowMode Get_Mode()    { return s_Mode; }

    // Returns the raw SDL_Window* (used by RHI to create its graphics surface).
    static inline SDL_Window *Get_Handle() { return s_Window; }

    // TRUE while the window exists.
    static inline BOOL Is_Open()       { return s_IsOpen; }

    // TRUE while the window has keyboard focus (FALSE after Alt+Tab).
    static inline BOOL Is_Focused()    { return s_IsFocused; }

    // TRUE while minimized to the taskbar.
    static inline BOOL Is_Minimized()  { return s_IsMinimized; }

    // TRUE while the mouse is locked and relative mode is active (e.g. right-click orbiting).
    static inline BOOL Is_MouseLocked() { return s_IsMouseLocked; }

    // Returns the cursor position saved right before mouse lock was engaged.
    static inline Vector2D Get_SavedMousePos() { return s_SavedMousePos; }

    // TRUE in Borderless or exclusive Fullscreen mode.
    static inline BOOL Is_Fullscreen() { return s_Mode != Windowed; }

    // TRUE only on the frame the window's pixel size changed (resize, fullscreen toggle, DPI change).
    static inline BOOL Was_Resized()   { return s_WasResized; }
};
