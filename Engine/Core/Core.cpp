#include <Core.h>
#include <M_Sound.h>

BOOL        Core::s_IsRunning   = false;
BOOL        Core::s_IsDebugMode = false;
ProcessType Core::s_ProcessType = ProcessType::Window;

BOOL Core::BeginPlay(
    StringView appName,
    ProcessType type,
    StringView windowTitle)
{
    s_IsRunning   = true;
    s_ProcessType = type;

    // 1. Crash Manager -> "{appName}Crash.log"
    M_Crash::Init(StringFormat("{}Crash.log", appName));

    // 2. SDL3 Base Events
    SDL_Init(SDL_INIT_EVENTS);

    // 3. Time System
    M_Time::Init();

    // 4. Worker Thread Pool (Auto-detects CPU cores, runs on Client & Server)
    M_Threads::Init();

    // 5. If Window process, create the OS Window & boot the RHI graphics backend!
    if (s_ProcessType == ProcessType::Window) {
        if (!M_Window::Create(windowTitle)) {
            LOG(Error, LogLevel::Core, "Core::BeginPlay failed to create Window!");
            Stop();
            return false;
        }

        // Initializes RHI graphics backend
        if (!RHI_Manager::Init()) {
            LOG(Error, LogLevel::Core, "Core::BeginPlay failed to initialize RHI!");
            Stop();
            return false;
        }
    }

    LOG(Info, LogLevel::Core, "Core::BeginPlay [{}] booted successfully ({})",
        appName,
        (s_ProcessType == ProcessType::Window) ? "Window" : "Headless"
    );
    return true;
}

BOOL Core::Tick() {
    if (!s_IsRunning) return false;

    M_Time::Update();

    // Time-sliced mesh and texture streaming processing (1 asset per tick to prevent hitches)
    M_Mesh::Tick_Streaming(1);
    M_Texture::Tick_Streaming(1);

    if (s_ProcessType == ProcessType::Window) {
        M_Sound::Tick(M_Time::Get_DeltaTime());
        if (!M_Input::Update()) {
            s_IsRunning = false;
            return false;
        }

        // Automatically throttle CPU loop when window is minimized to the taskbar
        if (M_Window::Is_Minimized()) {
            M_Time::Sleep(10);
        }
    }

    return true;
}

void Core::Stop() {
    s_IsRunning = false;
}

void Core::EndPlay() {
    if (s_ProcessType == ProcessType::Window) {
        M_Sound::Shutdown();
        RHI_Manager::Shutdown();
        M_Window::Destroy();
    }

    M_Texture::Shutdown();
    M_Mesh::Shutdown();
    M_Threads::Shutdown();
    SDL_Quit();
    s_IsRunning = false;
    LOG(Info, LogLevel::Core, "Core::EndPlay finished.");
}
