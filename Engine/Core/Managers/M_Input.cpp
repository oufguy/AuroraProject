#include <M_Input.h>

std::unordered_map<String, M_Input::ActionBinding, M_Input::StringHash, std::equal_to<>> M_Input::s_Bindings;

BOOL M_Input::Update() {
    // Fast-path: If no Window is open (Console / Headless Server), skip immediately in 1 CPU cycle!
    if (!M_Window::Is_Open()) [[likely]] return true;

    I_Keyboard::BeginFrame();
    I_Mouse::BeginFrame();
    M_Window::BeginFrame();

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        // Forward all window events (resize, focus, minimize) to the Window Manager
        if (ev.type >= SDL_EVENT_WINDOW_FIRST && ev.type <= SDL_EVENT_WINDOW_LAST) {
            M_Window::OnEvent(ev);
        }

        switch (ev.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                return false;

            case SDL_EVENT_KEY_DOWN:
                if (!ev.key.repeat) {
                    I_Keyboard::OnKeyEvent(static_cast<UINT16>(ev.key.scancode), true);
                }
                break;

            case SDL_EVENT_KEY_UP:
                I_Keyboard::OnKeyEvent(static_cast<UINT16>(ev.key.scancode), false);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                I_Mouse::OnButtonEvent(ev.button.button, true);
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                I_Mouse::OnButtonEvent(ev.button.button, false);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                I_Mouse::OnMoveEvent(ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                I_Mouse::OnScrollEvent(ev.wheel.y);
                break;

            default:
                break;
        }
    }
    return true;
}

void M_Input::Bind(StringView actionName, Key defaultKey) {
    s_Bindings[String(actionName)] = { false, defaultKey, MouseButton::MB_Left };
}

void M_Input::Bind(StringView actionName, MouseButton defaultButton) {
    s_Bindings[String(actionName)] = { true, Key::Unknown, defaultButton };
}

void M_Input::Rebind(StringView actionName, Key newKey) {
    s_Bindings[String(actionName)] = { false, newKey, MouseButton::MB_Left };
}

void M_Input::Rebind(StringView actionName, MouseButton newButton) {
    s_Bindings[String(actionName)] = { true, Key::Unknown, newButton };
}

BOOL M_Input::Get_Action(StringView actionName, InputState state) {
    auto it = s_Bindings.find(actionName);
    if (it == s_Bindings.end()) return false;

    const ActionBinding &bind = it->second;
    if (bind.isMouse) {
        return I_Mouse::Get_Button(bind.button, state);
    }
    return I_Keyboard::Get_Key(bind.key, state);
}
