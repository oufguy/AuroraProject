#pragma once

#include <pch.h>
#include <I_Plugin.h>
#include <U_Variables.h>

// ============================================================================
// M_PluginManager: Central Plugin Registry & Lifecycle Coordinator
// ============================================================================
// Crash-safe coordinator that isolates plugin failures and maintains user
// enable/disable preferences in %APPDATA%/Aurora/Engine/Plugins.ini.
// ============================================================================
class M_PluginManager {
private:
    static Array<SharedPtr<I_Plugin>> s_Plugins;
    static BOOL                       s_bInitialized;

public:
    static void Init();
    static void Shutdown();

    // Registers a plugin into the central engine registry (starts disabled)
    static void Register_Plugin(SharedPtr<I_Plugin> plugin);

    // Returns all registered plugins
    [[nodiscard]] static const Array<SharedPtr<I_Plugin>> &Get_Plugins();

    // Finds a plugin by its unique ID
    [[nodiscard]] static SharedPtr<I_Plugin> Get_Plugin(StringView id);

    // Queries or updates plugin enable status
    [[nodiscard]] static BOOL Is_Plugin_Enabled(StringView id);
    static void Set_Plugin_Enabled(StringView id, BOOL bEnable);

    // Crash-safe execution of plugin frame logic
    static void Tick(FLOAT deltaTime);

    // Crash-safe execution of plugin UI rendering
    static void Render_UI();

    // Configuration persistence
    static void Load_Config();
    static void Save_Config();
};
