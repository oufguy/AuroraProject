#include <RHI_Manager.h>
#include <R_Vulkan.h>
#include <R_DirectX.h>

API_Type RHI_Manager::s_ActiveAPI     = API_Type::Vulkan;
BOOL     RHI_Manager::s_VSync         = false;
BOOL     RHI_Manager::s_IsInitialized = false;
String   RHI_Manager::s_ActiveWarning = "";

BOOL RHI_Manager::InitBackend(API_Type api, SDL_Window *window) {
    switch (api) {
        case API_Type::Vulkan:    return R_Vulkan::Init(window);
        case API_Type::DirectX11: return R_DirectX::Init(window);
        default:                  return false;
    }
}

void RHI_Manager::ShutdownBackend(API_Type api) {
    switch (api) {
        case API_Type::Vulkan:    R_Vulkan::Shutdown();  break;
        case API_Type::DirectX11: R_DirectX::Shutdown(); break;
        default: break;
    }
}

DeviceCompatibility RHI_Manager::Detect_Device() {
    DeviceCompatibility dev{};

    const Char *plat = SDL_GetPlatform();
    dev.Platform = plat ? plat : "Unknown";

    dev.GPU = String(Get_GPUName());
    if (dev.GPU.empty() || dev.GPU == "None" || dev.GPU == "Unknown GPU") {
        StringView vulkanGPU = R_Vulkan::Get_GPUName();
        dev.GPU = vulkanGPU.empty() ? "Generic Graphics Adapter" : String(vulkanGPU);
    }

    SDL_SharedObject *ntdll = SDL_LoadObject("ntdll.dll");
    if (ntdll) {
        if (SDL_LoadFunction(ntdll, "wine_get_version") != nullptr) {
            dev.bIsWineProton = true;
        }
        SDL_UnloadObject(ntdll);
    }

    dev.bHasDirectX11 = R_DirectX::Is_Supported();

    SDL_SharedObject *d3d9 = SDL_LoadObject("d3d9.dll");
    if (d3d9) {
        dev.bHasDirectX9 = (SDL_LoadFunction(d3d9, "Direct3DCreate9") != nullptr);
        SDL_UnloadObject(d3d9);
    }

    dev.bHasVulkan = true;

    if (dev.bIsWineProton) {
        dev.CompatibilityNote = std::format(
            "Notice: Running via Proton/Wine on [{}]. Native Vulkan provides better performance.",
            dev.GPU);
    } else if (dev.Platform != "Windows" && !dev.bHasDirectX11) {
        dev.CompatibilityNote = std::format(
            "DirectX is unsupported on {} ({}). Native Vulkan is active.",
            dev.Platform, dev.GPU);
    } else if (!dev.bHasDirectX11) {
        dev.CompatibilityNote = std::format(
            "DirectX runtime was not found. Native Vulkan is active.");
    }

    return dev;
}

BOOL RHI_Manager::Is_APISupported(API_Type api) {
    switch (api) {
        case API_Type::Vulkan:
            return true;
        case API_Type::DirectX11:
            return R_DirectX::Is_Supported();
        case API_Type::DirectX9: {
            SDL_SharedObject *d3d9 = SDL_LoadObject("d3d9.dll");
            if (!d3d9) return false;
            BOOL bHas = (SDL_LoadFunction(d3d9, "Direct3DCreate9") != nullptr);
            SDL_UnloadObject(d3d9);
            return bHas;
        }
        default:
            return false;
    }
}

String RHI_Manager::Get_APIStatusText(API_Type api) {
    DeviceCompatibility dev = Detect_Device();
    switch (api) {
        case API_Type::Vulkan:
            return "Recommended (Native Cross-Platform)";
        case API_Type::DirectX11:
            if (!dev.bHasDirectX11) {
                return std::format("Unsupported on {} ({})", dev.Platform, dev.GPU);
            }
            if (dev.bIsWineProton) {
                return "Proton/Wine Detected (Vulkan is preferred)";
            }
            return "Compatible (Windows D3D11 / DXVK)";
        case API_Type::DirectX9:
            if (!dev.bHasDirectX9) {
                return std::format("DirectX 9 not found on {} ({})", dev.Platform, dev.GPU);
            }
            return "Compatible (Legacy D3D9 / DXVK)";
        default:
            return "Unknown";
    }
}

BOOL RHI_Manager::Init() {
    if (s_IsInitialized) return true;

    SDL_Window *window = M_Window::Get_Handle();
    if (!window) {
        LOG(Info, LogLevel::Core, "RHI_Manager: Headless process detected (Zero graphics overhead).");
        return true;
    }

    if (s_ActiveAPI == API_Type::None) {
        s_ActiveAPI = API_Type::Vulkan;
    }

    if (!Is_APISupported(s_ActiveAPI)) {
        s_ActiveWarning = std::format(
            "[{}] not found! Defaulting to [Vulkan].",
            Get_APIName(s_ActiveAPI));
        LOG(Warn, LogLevel::Core, "[RHI] {}", s_ActiveWarning);
        s_ActiveAPI = API_Type::Vulkan;
    }

    if (!InitBackend(s_ActiveAPI, window)) {
        if (s_ActiveAPI != API_Type::Vulkan) {
            s_ActiveWarning = std::format(
                "[{}] failed to initialize! Defaulting to [Vulkan].",
                Get_APIName(s_ActiveAPI));
            LOG(Warn, LogLevel::Core, "[RHI] {}", s_ActiveWarning);

            s_ActiveAPI = API_Type::Vulkan;
            if (!InitBackend(s_ActiveAPI, window)) {
                s_ActiveWarning = "Vulkan not found or failed! Defaulting to [None].";
                LOG(Error, LogLevel::Core, "[RHI] {}", s_ActiveWarning);
                s_ActiveAPI = API_Type::None;
                return false;
            }
        } else {
            s_ActiveWarning = "Vulkan not found or failed! Defaulting to [None].";
            LOG(Error, LogLevel::Core, "[RHI] {}", s_ActiveWarning);
            s_ActiveAPI = API_Type::None;
            return false;
        }
    }

    Set_VSync(s_VSync);

    s_IsInitialized = true;
    LOG(Info, LogLevel::Core, "RHI_Manager initialized with [{}] backend (VSync: {}) on [{}]",
        Get_APIName(s_ActiveAPI), s_VSync ? "ON" : "OFF", Get_GPUName());
    return true;
}

void RHI_Manager::Shutdown() {
    if (!s_IsInitialized) return;

    ShutdownBackend(s_ActiveAPI);
    s_ActiveAPI     = API_Type::Vulkan;
    s_IsInitialized = false;
    LOG(Info, LogLevel::Core, "RHI_Manager shutdown cleanly.");
}

BOOL RHI_Manager::Swap_API(API_Type newAPI) {
    if (newAPI == API_Type::None) return false;
    if (!s_IsInitialized) {
        s_ActiveAPI = newAPI;
        return true;
    }
    if (newAPI == s_ActiveAPI) return true;

    API_Type previousAPI = s_ActiveAPI;

    if (!Is_APISupported(newAPI)) {
        s_ActiveWarning = std::format(
            "[{}] not found! Reverting changes and keeping [{}].",
            Get_APIName(newAPI), Get_APIName(previousAPI));
        LOG(Warn, LogLevel::Core, "[RHI] {}", s_ActiveWarning);
        return false;
    }

    SDL_Window *window = M_Window::Get_Handle();
    if (!window) return false;

    LOG(Info, LogLevel::Core, "RHI_Manager: Swapping backend from [{}] to [{}]...",
        Get_APIName(previousAPI), Get_APIName(newAPI));

    BOOL  previousVSync      = Get_VSync();
    COLOR previousClearColor = Get_ClearColor();

    Set_ClearColor(COLOR(0, 0, 0, 255));
    if (Begin_Frame()) {
        End_Frame();
    }

    ShutdownBackend(previousAPI);

    s_ActiveAPI = newAPI;
    if (!InitBackend(s_ActiveAPI, window)) {
        s_ActiveWarning = std::format(
            "[{}] not found or failed to initialize! Reverting changes to [{}].",
            Get_APIName(newAPI), Get_APIName(previousAPI));
        LOG(Warn, LogLevel::Core, "[RHI] {}", s_ActiveWarning);

        s_ActiveAPI = previousAPI;
        if (!InitBackend(s_ActiveAPI, window)) {
            LOG(Warn, LogLevel::Core, "[RHI] Previous backend failed! Defaulting to [Vulkan].");
            s_ActiveAPI = API_Type::Vulkan;
            if (!InitBackend(s_ActiveAPI, window)) {
                LOG(Error, LogLevel::Core, "[RHI] Vulkan not found! Defaulting to [None].");
                s_ActiveAPI = API_Type::None;
            }
        }
        Set_VSync(previousVSync);
        Set_ClearColor(previousClearColor);
        return false;
    }

    Set_VSync(previousVSync);
    Set_ClearColor(previousClearColor);

    LOG(Info, LogLevel::Core, "RHI_Manager: Successfully swapped backend to [{}]!", Get_APIName(s_ActiveAPI));
    return true;
}

BOOL RHI_Manager::Begin_Frame() {
    if (!s_IsInitialized || M_Window::Is_Minimized()) return false;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    return R_Vulkan::Begin_Frame();
        case API_Type::DirectX11: return R_DirectX::Begin_Frame();
        default:                  return false;
    }
}

void RHI_Manager::Draw_Mesh(
    const MeshData &mesh,
    const Vector3D &pos,
    const Vector3D &rot,
    const Vector3D &size,
    const COLOR    &color,
    FLOAT           transparency,
    UINT            textureID)
{
    if (!s_IsInitialized) return;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Draw_Mesh(mesh, pos, rot, size, color, transparency, textureID);  break;
        case API_Type::DirectX11: R_DirectX::Draw_Mesh(mesh, pos, rot, size, color, transparency, textureID); break;
        default: break;
    }
}

void RHI_Manager::Draw_UI(
    UINT            textureID,
    const Vector2D &pos,
    const Vector2D &size,
    const COLOR    &color,
    FLOAT           transparency)
{
    if (!s_IsInitialized) return;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Draw_UI(textureID, pos, size, color, transparency);  break;
        case API_Type::DirectX11: R_DirectX::Draw_UI(textureID, pos, size, color, transparency); break;
        default: break;
    }
}

void RHI_Manager::Draw_UI_SubRect(
    UINT            textureID,
    const Vector2D &pos,
    const Vector2D &size,
    const Vector2D &uvMin,
    const Vector2D &uvMax,
    const COLOR    &color,
    FLOAT           transparency)
{
    if (!s_IsInitialized || size.x <= 0.0f || size.y <= 0.0f || transparency <= 0.001f) return;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Draw_UI_SubRect(textureID, pos, size, uvMin, uvMax, color, transparency);  break;
        case API_Type::DirectX11: R_DirectX::Draw_UI_SubRect(textureID, pos, size, uvMin, uvMax, color, transparency); break;
        default: break;
    }
}

void RHI_Manager::Draw_UI_Quads(
    UINT          textureID,
    const UIQuad *quads,
    UINT          count)
{
    if (!s_IsInitialized || !quads || count == 0) return;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Draw_UI_Quads(textureID, quads, count);  break;
        case API_Type::DirectX11: R_DirectX::Draw_UI_Quads(textureID, quads, count); break;
        default: break;
    }
}

void RHI_Manager::End_Frame() {
    if (!s_IsInitialized) return;

    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::End_Frame();  break;
        case API_Type::DirectX11: R_DirectX::End_Frame(); break;
        default: break;
    }
}

void RHI_Manager::Set_VSync(BOOL enabled) {
    s_VSync = enabled;
    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Set_VSync(enabled);  break;
        case API_Type::DirectX11: R_DirectX::Set_VSync(enabled); break;
        default: break;
    }
}

BOOL RHI_Manager::Get_VSync() {
    if (!s_IsInitialized) return s_VSync;
    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    return R_Vulkan::Get_VSync();
        case API_Type::DirectX11: return R_DirectX::Get_VSync();
        default:                  return s_VSync;
    }
}

void RHI_Manager::Set_ClearColor(const COLOR &color) {
    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    R_Vulkan::Set_ClearColor(color);  break;
        case API_Type::DirectX11: R_DirectX::Set_ClearColor(color); break;
        default: break;
    }
}

COLOR RHI_Manager::Get_ClearColor() {
    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    return R_Vulkan::Get_ClearColor();
        case API_Type::DirectX11: return R_DirectX::Get_ClearColor();
        default:                  return COLOR(0, 0, 0, 255);
    }
}

StringView RHI_Manager::Get_GPUName() {
    switch (s_ActiveAPI) {
        case API_Type::Vulkan:    return R_Vulkan::Get_GPUName();
        case API_Type::DirectX11: return R_DirectX::Get_GPUName();
        default:                  return "None";
    }
}
