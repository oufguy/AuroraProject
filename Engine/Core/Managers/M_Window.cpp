#include <M_Window.h>
#include <I_Mouse.h>

SDL_Window *M_Window::s_Window        = nullptr;
BOOL        M_Window::s_IsOpen        = false;
BOOL        M_Window::s_IsFocused     = false;
BOOL        M_Window::s_IsMouseLocked = false;
Vector2D    M_Window::s_SavedMousePos = { 0.0f, 0.0f };
BOOL        M_Window::s_IsMinimized   = false;
BOOL        M_Window::s_WasResized    = false;
String      M_Window::s_Title         = "Aurora";
M_Window::WindowMode M_Window::s_Mode = M_Window::Windowed;
INT         M_Window::s_Width         = 1280;
INT         M_Window::s_Height        = 720;
INT         M_Window::s_PixelWidth    = 1280;
INT         M_Window::s_PixelHeight   = 720;
INT         M_Window::s_WindowedWidth  = 1280;
INT         M_Window::s_WindowedHeight = 720;

void M_Window::ApplyMode(WindowMode mode, INT width, INT height) {
    s_Mode = mode;
    if (!s_Window) return;

    if (mode == Windowed) {
        SDL_SetWindowFullscreen(s_Window, false);
        INT targetW = (width > 0) ? width : s_WindowedWidth;
        INT targetH = (height > 0) ? height : s_WindowedHeight;
        if (targetW <= 0) targetW = 1280;
        if (targetH <= 0) targetH = 720;
        SDL_SetWindowSize(s_Window, targetW, targetH);
        SDL_SetWindowPosition(s_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    } else if (mode == Fullscreen) {
        SDL_DisplayID display = SDL_GetDisplayForWindow(s_Window);
        const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(display);
        if (desktopMode) {
            SDL_SetWindowFullscreenMode(s_Window, desktopMode);
        } else {
            SDL_SetWindowFullscreenMode(s_Window, nullptr);
        }
        SDL_SetWindowFullscreen(s_Window, true);
    } else {
        SDL_SetWindowFullscreenMode(s_Window, nullptr); // borderless desktop fullscreen
        SDL_SetWindowFullscreen(s_Window, true);
    }

    if (s_IsOpen) {
        INT curW = 0, curH = 0;
        INT curPxW = 0, curPxH = 0;
        SDL_GetWindowSize(s_Window, &curW, &curH);
        SDL_GetWindowSizeInPixels(s_Window, &curPxW, &curPxH);

        // If OS fullscreen transition is asynchronous, immediately fallback to display dimensions
        if (s_Mode != Windowed) {
            SDL_DisplayID display = SDL_GetDisplayForWindow(s_Window);
            const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode(display);
            if (dm && dm->w > 0 && dm->h > 0) {
                if (curW <= s_WindowedWidth || curPxW <= s_WindowedWidth) {
                    curW = dm->w;
                    curH = dm->h;
                    curPxW = static_cast<INT>(static_cast<FLOAT>(dm->w) * dm->pixel_density);
                    curPxH = static_cast<INT>(static_cast<FLOAT>(dm->h) * dm->pixel_density);
                    if (curPxW <= 0) curPxW = dm->w;
                    if (curPxH <= 0) curPxH = dm->h;
                }
            }
        }

        s_Width       = curW;
        s_Height      = curH;
        s_PixelWidth  = curPxW;
        s_PixelHeight = curPxH;

        if (s_Mode == Windowed && s_Width > 0 && s_Height > 0) {
            s_WindowedWidth  = s_Width;
            s_WindowedHeight = s_Height;
        }
        s_WasResized = true;
    }
}

void M_Window::BeginFrame() {
    s_WasResized = false;

    // Continuous dynamic safeguard: catch any asynchronous OS fullscreen transitions or DPI changes
    if (s_IsOpen && s_Window) {
        INT curW = 0, curH = 0;
        INT curPxW = 0, curPxH = 0;
        SDL_GetWindowSize(s_Window, &curW, &curH);
        SDL_GetWindowSizeInPixels(s_Window, &curPxW, &curPxH);

        if (curW > 0 && curH > 0 && curPxW > 0 && curPxH > 0) {
            if (curW != s_Width || curH != s_Height || curPxW != s_PixelWidth || curPxH != s_PixelHeight) {
                s_Width       = curW;
                s_Height      = curH;
                s_PixelWidth  = curPxW;
                s_PixelHeight = curPxH;
                if (s_Mode == Windowed) {
                    s_WindowedWidth  = s_Width;
                    s_WindowedHeight = s_Height;
                }
                s_WasResized = true;
            }
        }
    }
}

void M_Window::OnEvent(const SDL_Event &ev) {
    switch (ev.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            s_Width      = ev.window.data1;
            s_Height     = ev.window.data2;
            s_WasResized = true;
            if (s_IsOpen && s_Window) {
                SDL_GetWindowSizeInPixels(s_Window, &s_PixelWidth, &s_PixelHeight);
                if (s_Mode == Windowed && s_Width > 0 && s_Height > 0) {
                    s_WindowedWidth  = s_Width;
                    s_WindowedHeight = s_Height;
                }
            }
            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (ev.window.data1 != s_PixelWidth || ev.window.data2 != s_PixelHeight) {
                s_PixelWidth  = ev.window.data1;
                s_PixelHeight = ev.window.data2;
                s_WasResized  = true;
                if (s_IsOpen && s_Window) {
                    SDL_GetWindowSize(s_Window, &s_Width, &s_Height);
                    if (s_Mode == Windowed && s_Width > 0 && s_Height > 0) {
                        s_WindowedWidth  = s_Width;
                        s_WindowedHeight = s_Height;
                    }
                }
            }
            break;

        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            if (s_IsOpen && s_Window) {
                SDL_GetWindowSize(s_Window, &s_Width, &s_Height);
                SDL_GetWindowSizeInPixels(s_Window, &s_PixelWidth, &s_PixelHeight);
                s_WasResized = true;
            }
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED: s_IsFocused   = true;  break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            s_IsFocused = false;
            if (s_IsMouseLocked) {
                Set_MouseLocked(false);
            }
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:    s_IsMinimized = true;  break;
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
            s_IsMinimized = false;
            if (s_IsOpen && s_Window) {
                SDL_GetWindowSize(s_Window, &s_Width, &s_Height);
                SDL_GetWindowSizeInPixels(s_Window, &s_PixelWidth, &s_PixelHeight);
                s_WasResized = true;
            }
            break;
        default: break;
    }
}

BOOL M_Window::Create(StringView title) {
    if (s_IsOpen) return true;
    if (!title.empty()) s_Title = String(title);

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        LOG(Error, LogLevel::Core, "M_Window::Create failed to init SDL video: {}", SDL_GetError());
        return false;
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    s_Window = SDL_CreateWindow(s_Title.c_str(), s_Width, s_Height, flags);
    if (!s_Window) {
        LOG(Error, LogLevel::Core, "M_Window::Create failed: {}", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    s_IsOpen      = true;
    s_IsFocused   = true;
    s_IsMinimized = false;
    ApplyMode(s_Mode, s_Width, s_Height);
    SDL_GetWindowSizeInPixels(s_Window, &s_PixelWidth, &s_PixelHeight);

    LOG(Info, LogLevel::Core, "M_Window::Create opened '{}' ({}x{}, {})", s_Title, s_Width, s_Height, ModeToString(s_Mode));
    return true;
}

void M_Window::Destroy() {
    if (!s_IsOpen) return;

    Set_MouseLocked(false);
    SDL_DestroyWindow(s_Window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    s_Window = nullptr;
    s_IsOpen = false;
}

void M_Window::Set_Title(StringView title) {
    s_Title = String(title);
    if (s_IsOpen) SDL_SetWindowTitle(s_Window, s_Title.c_str());
}

void M_Window::Set_Size(INT width, INT height) {
    if (width > 0)  s_Width  = width;
    if (height > 0) s_Height = height;
    if (s_Mode == Windowed) {
        if (width > 0)  s_WindowedWidth  = width;
        if (height > 0) s_WindowedHeight = height;
    }
    if (s_IsOpen) ApplyMode(s_Mode, s_Width, s_Height);
}

void M_Window::Set_Mode(WindowMode mode) {
    s_Mode = mode;
    if (!s_IsOpen) return;
    ApplyMode(mode, s_WindowedWidth, s_WindowedHeight);
    s_WasResized = true;
}

void M_Window::Set_MouseLocked(BOOL locked) {
    if (s_IsMouseLocked == locked) return;
    s_IsMouseLocked = locked;
    if (!s_IsOpen || !s_Window) return;

    if (locked) {
        SDL_HideCursor();
    } else {
        SDL_ShowCursor();
    }
}
