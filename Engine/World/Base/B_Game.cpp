#include <B_Game.h>
#include <M_Window.h>
#include <M_Time.h>
#include <M_Input.h>
#include <I_Keyboard.h>
#include <RHI_Manager.h>
#include <M_World.h>
#include <C_Camera.h>
#include <M_Log.h>

void B_Game::BeginPlay() {}
void B_Game::Tick(FLOAT deltaTime) { (void)deltaTime; }
void B_Game::DrawUI() {}
void B_Game::EndPlay() {}

FilePath B_Game::Get_SettingsPath() const {
    return "Settings.ini";
}

String B_Game::Get_DefaultSettings() const {
    return "# Graphics\n"
           "API = Vulkan                ; Options: Vulkan / DirectX11\n"
           "VSync = false               ; Options: true / false\n"
           "\n"
           "# Window\n"
           "Mode = Windowed             ; Options: Windowed / Borderless / Fullscreen\n"
           "Width = 1280\n"
           "Height = 720\n"
           "\n"
           "# Input\n"
           "Sensitivity = 1.0\n";
}

INT B_Game::RunGameInternal(B_Game &game) {
    // 1. Load game configuration
    game.Settings.Load(game.Get_SettingsPath(), game.Get_DefaultSettings());

    // 2. Setup Window and RHI preferences
    M_Window::Set_Title("Aurora Client");
    M_Window::Set_Size(game.Settings.Get("Width"), game.Settings.Get("Height"));
    M_Window::Set_Mode(M_Window::ModeFromString(game.Settings.Get("Mode")));

    RHI_Manager::Swap_API(RHI_Manager::Parse_API(game.Settings.Get("API")));
    RHI_Manager::Set_VSync(game.Settings.Get("VSync"));

    // 3. Boot Core Lifecycle Subsystems
    if (!Core::BeginPlay("Client", ProcessType::Window)) {
        return 1;
    }

    // 4. Initialize 3D World and setup default Camera (Zero entity overhead)
    M_World::BeginPlay("DefaultWorld");
    C_Camera *cam = &C_Camera::Get_Active();
    cam->Reset();

    // 5. Invoke Game BeginPlay
    game.BeginPlay();

    PRINT("Game Initialized! Active Backend: {} ({}) | World: [{}] ({} Entities)",
        RHI_Manager::Get_APIName(RHI_Manager::Get_ActiveAPI()),
        RHI_Manager::Get_GPUName(),
        M_World::Get_Name(),
        M_World::Get_EntityCount());

    // 6. Main Engine Loop
    DOUBLE lastFpsLog = 0.0;
    while (Core::Tick()) {
        if (M_Window::Is_Minimized()) continue;

        FLOAT dt = M_Time::Get_DeltaTime();

        // Run user gameplay logic
        game.Tick(dt);

        // Step World transforms, physics, and animations
        M_World::Tick(dt);

        // Handle window resizing
        if (M_Window::Was_Resized()) {
            if (M_Window::Get_Mode() == M_Window::Windowed) {
                game.Settings.Set("Width",  M_Window::Get_Width());
                game.Settings.Set("Height", M_Window::Get_Height());
            }

            Vector2D px = M_Window::Get_PixelSize();
            PRINT("Window resized -> {}x{} pixels ({})", px.x, px.y, M_Window::ModeToString(M_Window::Get_Mode()));
        }

        // Engine hotkeys: F9 (API swap), F10 (VSync), F11 (Fullscreen)
        if (I_Keyboard::Get_Key(Key::F10, M_Input::Pressed)) {
            BOOL vsync = game.Settings.Set("VSync", !RHI_Manager::Get_VSync());
            RHI_Manager::Set_VSync(vsync);
        }
        if (I_Keyboard::Get_Key(Key::F11, M_Input::Pressed)) {
            auto mode = M_Window::Is_Fullscreen() ? M_Window::Windowed : M_Window::Borderless;
            M_Window::Set_Mode(mode);
            game.Settings.Set("Mode", M_Window::ModeToString(mode));
        }
        if (I_Keyboard::Get_Key(Key::F9, M_Input::Pressed)) {
            API_Type target = (RHI_Manager::Get_ActiveAPI() == API_Type::Vulkan) ? API_Type::DirectX11 : API_Type::Vulkan;
            if (RHI_Manager::Swap_API(target)) {
                game.Settings.Set("API", (target == API_Type::Vulkan) ? "Vulkan" : "DirectX11");
            }
        }

        // Periodic telemetry log (clean, does NOT modify window title)
        if (M_Time::Check_Cooldown(lastFpsLog, 1.0)) {
            LOG(Info, LogLevel::Developer, "FPS: {} | VSync: {} | Backend: {} | Drawn: {} (Culled: {})",
                M_Time::Get_FPS(),
                RHI_Manager::Get_VSync() ? "ON" : "OFF",
                RHI_Manager::Get_APIName(RHI_Manager::Get_ActiveAPI()),
                M_World::Get_VisibleEntityCount(),
                M_World::Get_CulledEntityCount());
        }

        // Render Frame with active RHI backend (Vulkan or DirectX 11)
        if (RHI_Manager::Begin_Frame()) {
            M_World::Render();
            game.DrawUI();
            RHI_Manager::End_Frame();
        }
    }

    // 7. Invoke Game EndPlay and clean up
    game.EndPlay();
    game.Settings.Save(game.Get_SettingsPath());
    M_World::Shutdown();
    Core::EndPlay();

    return 0;
}
