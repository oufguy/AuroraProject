#include <M_PluginManager.h>
#include <M_File.h>
#include <M_Log.h>
#include <sstream>

Array<SharedPtr<I_Plugin>> M_PluginManager::s_Plugins;
BOOL                       M_PluginManager::s_bInitialized = false;

static FilePath Get_PluginsIniPath() {
    char *pref = SDL_GetPrefPath("Aurora", "Engine");
    FilePath p;
    if (pref) {
        p = FilePath(pref) / "Plugins.ini";
        SDL_free(pref);
    } else {
        const char *appData = std::getenv("APPDATA");
        p = appData ? (FilePath(appData) / "Aurora" / "Plugins.ini") : FilePath("Plugins.ini");
    }
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    return p;
}

void M_PluginManager::Init() {
    if (s_bInitialized) return;
    s_bInitialized = true;
    Load_Config();
    LOG(Info, LogLevel::Core, "M_PluginManager initialized with [{}] plugins.", s_Plugins.size());
}

void M_PluginManager::Shutdown() {
    Save_Config();
    for (auto &plugin : s_Plugins) {
        if (plugin && plugin->Is_Enabled()) {
            plugin->Set_Enabled(false);
        }
    }
    s_Plugins.clear();
    s_bInitialized = false;
}

void M_PluginManager::Register_Plugin(SharedPtr<I_Plugin> plugin) {
    if (!plugin) return;
    for (const auto &p : s_Plugins) {
        if (p && p->Get_ID() == plugin->Get_ID()) return;
    }
    s_Plugins.push_back(plugin);
    LOG(Info, LogLevel::Core, "Registered Plugin: [{}] ({}) - Initially Disabled", plugin->Get_Name(), plugin->Get_ID());
}

const Array<SharedPtr<I_Plugin>> &M_PluginManager::Get_Plugins() {
    return s_Plugins;
}

SharedPtr<I_Plugin> M_PluginManager::Get_Plugin(StringView id) {
    for (auto &p : s_Plugins) {
        if (p && p->Get_ID() == id) return p;
    }
    return nullptr;
}

BOOL M_PluginManager::Is_Plugin_Enabled(StringView id) {
    auto p = Get_Plugin(id);
    return p ? p->Is_Enabled() : false;
}

void M_PluginManager::Set_Plugin_Enabled(StringView id, BOOL bEnable) {
    auto p = Get_Plugin(id);
    if (p) {
        p->Set_Enabled(bEnable);
        Save_Config();
        LOG(Info, LogLevel::Core, "Plugin [{}] set to [{}]", p->Get_Name(), bEnable ? "ENABLED" : "DISABLED");
    }
}

void M_PluginManager::Tick(FLOAT deltaTime) {
    for (auto &p : s_Plugins) {
        if (p && p->Is_Enabled()) {
            try {
                p->On_Tick(deltaTime);
            } catch (...) {
                LOG(Error, LogLevel::Core, "Crash prevented in plugin [{}] during Tick!", p->Get_Name());
                p->Set_Enabled(false);
            }
        }
    }
}

void M_PluginManager::Render_UI() {
    for (auto &p : s_Plugins) {
        if (p && p->Is_Enabled()) {
            try {
                p->On_Render_UI();
            } catch (...) {
                LOG(Error, LogLevel::Core, "Crash prevented in plugin [{}] during Render_UI!", p->Get_Name());
                p->Set_Enabled(false);
            }
        }
    }
}

void M_PluginManager::Load_Config() {
    FilePath iniPath = Get_PluginsIniPath();
    if (!std::filesystem::exists(iniPath)) return;

    String content = M_File::Open(iniPath);
    if (content.empty()) return;

    std::istringstream stream(content);
    String line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t eq = line.find('=');
        if (eq == String::npos) continue;

        String key = line.substr(0, eq);
        String val = line.substr(eq + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        size_t vs = val.find_first_not_of(" \t");
        if (vs != String::npos) val = val.substr(vs);

        BOOL bEnabled = (val == "1" || val == "true" || val == "True");
        auto plugin = Get_Plugin(key);
        if (plugin) {
            plugin->Set_Enabled(bEnabled);
        }
    }
}

void M_PluginManager::Save_Config() {
    FilePath iniPath = Get_PluginsIniPath();
    std::ostringstream ss;
    ss << "# Aurora Engine Plugin Configuration\n";
    ss << "[Plugins]\n";
    for (const auto &p : s_Plugins) {
        if (p) {
            ss << p->Get_ID() << " = " << (p->Is_Enabled() ? "1" : "0") << "\n";
        }
    }
    M_File::Write(iniPath, ss.str(), /*bOverWrite=*/true);
}
