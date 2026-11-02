#include <E_ProjectManager.h>
#include <E_AssetPacker.h>
#include <M_SceneSerializer.h>
#include <M_File.h>
#include <M_Log.h>
#include <sstream>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <process.h>
#endif

ProjectInfo E_ProjectManager::s_ActiveProject;

static String Get_CurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return String(buf);
}

FilePath E_ProjectManager::Get_AppDataConfigPath() {
    char *pref = SDL_GetPrefPath("Aurora", "Engine");
    FilePath configPath;
    if (pref) {
        configPath = FilePath(pref) / "RecentProjects.ini";
        SDL_free(pref);
    } else {
        const char *appData = std::getenv("APPDATA");
        if (appData) {
            configPath = FilePath(appData) / "Aurora" / "RecentProjects.ini";
        } else {
            configPath = "RecentProjects.ini";
        }
    }
    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);
    return configPath;
}

Array<RecentProjectEntry> E_ProjectManager::Get_RecentProjects() {
    Array<RecentProjectEntry> projects;
    FilePath iniPath = Get_AppDataConfigPath();
    if (!std::filesystem::exists(iniPath)) {
        return projects;
    }

    String text = M_File::Open(iniPath);
    if (text.empty()) return projects;

    std::istringstream stream(text);
    String line;
    INT count = 0;
    std::unordered_map<String, String> values;

    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t eq = line.find('=');
        if (eq == String::npos) continue;

        String key = line.substr(0, eq);
        String val = line.substr(eq + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        size_t vs = val.find_first_not_of(" \t");
        if (vs != String::npos) val = val.substr(vs);

        if (key == "Count") {
            try { count = std::stoi(val); } catch (...) { count = 0; }
        } else {
            values[key] = val;
        }
    }

    for (INT i = 0; i < count; ++i) {
        String nameKey = std::format("{}_Name", i);
        String pathKey = std::format("{}_Path", i);
        String timeKey = std::format("{}_LastOpened", i);

        auto itPath = values.find(pathKey);
        if (itPath != values.end() && !itPath->second.empty()) {
            RecentProjectEntry entry;
            entry.Path = FilePath(itPath->second);
            auto itName = values.find(nameKey);
            entry.Name = (itName != values.end() && !itName->second.empty()) ? itName->second : entry.Path.stem().string();
            auto itTime = values.find(timeKey);
            entry.LastOpened = (itTime != values.end()) ? itTime->second : "";

            // Discard invalid / dummy entries like "." or non-existent paths
            if (entry.Name == "." || entry.Path == "." || entry.Name.empty() || entry.Path.empty()) {
                continue;
            }
            std::error_code ec;
            if (!std::filesystem::exists(entry.Path, ec)) {
                continue;
            }
            projects.push_back(entry);
        }
    }

    return projects;
}

void E_ProjectManager::Add_RecentProject(const String &name, const FilePath &path) {
    if (name.empty() || name == "." || path.empty() || path == ".") return;

    std::error_code ec;
    FilePath absPath = std::filesystem::absolute(path, ec).lexically_normal();
    if (!std::filesystem::exists(absPath, ec)) return;

    Array<RecentProjectEntry> list = Get_RecentProjects();

    // Check if path already exists in recent list; remove it so it moves to top
    String normalPath = absPath.string();
    std::replace(normalPath.begin(), normalPath.end(), '\\', '/');

    std::erase_if(list, [&](const RecentProjectEntry &e) {
        String ep = e.Path.lexically_normal().string();
        std::replace(ep.begin(), ep.end(), '\\', '/');
        return ep == normalPath;
    });

    RecentProjectEntry newEntry;
    newEntry.Name       = name;
    newEntry.Path       = FilePath(normalPath);
    newEntry.LastOpened = Get_CurrentTimestamp();
    list.insert(list.begin(), newEntry);

    // Keep up to 15 recent projects
    if (list.size() > 15) list.resize(15);

    // Save back to RecentProjects.ini
    std::ostringstream ss;
    ss << "# Aurora Engine Hub - Recent Projects List\n";
    ss << "[RecentProjects]\n";
    ss << "Count = " << list.size() << "\n\n";

    for (size_t i = 0; i < list.size(); ++i) {
        ss << i << "_Name = " << list[i].Name << "\n";
        String pStr = list[i].Path.string();
        std::replace(pStr.begin(), pStr.end(), '\\', '/');
        ss << i << "_Path = " << pStr << "\n";
        ss << i << "_LastOpened = " << list[i].LastOpened << "\n\n";
    }

    FilePath iniPath = Get_AppDataConfigPath();
    M_File::Write(iniPath, ss.str(), /*bOverWrite=*/true);
}

void E_ProjectManager::Remove_RecentProject(size_t index) {
    Array<RecentProjectEntry> list = Get_RecentProjects();
    if (index >= list.size()) return;
    list.erase(list.begin() + index);

    std::ostringstream ss;
    ss << "# Aurora Engine Hub - Recent Projects List\n";
    ss << "[RecentProjects]\n";
    ss << "Count = " << list.size() << "\n\n";

    for (size_t i = 0; i < list.size(); ++i) {
        ss << i << "_Name = " << list[i].Name << "\n";
        String pStr = list[i].Path.string();
        std::replace(pStr.begin(), pStr.end(), '\\', '/');
        ss << i << "_Path = " << pStr << "\n";
        ss << i << "_LastOpened = " << list[i].LastOpened << "\n\n";
    }

    FilePath iniPath = Get_AppDataConfigPath();
    M_File::Write(iniPath, ss.str(), /*bOverWrite=*/true);
}

FilePath E_ProjectManager::Resolve_Path(const FilePath &relativePath) {
    if (!s_ActiveProject.bIsLoaded || relativePath.is_absolute()) {
        return relativePath;
    }
    return s_ActiveProject.RootDirectory / relativePath;
}

static FilePath Find_Template_Directory(const String &templateSub) {
    std::error_code ec;
    // 1. Check relative to current working directory
    FilePath p1 = FilePath("Editor/Templates") / templateSub;
    if (std::filesystem::exists(p1, ec)) return p1;

    // 2. Check relative to application binary base path (SDL_GetBasePath)
    const char *base = SDL_GetBasePath();
    if (base && *base) {
        FilePath pBase = FilePath(base) / "Editor/Templates" / templateSub;
        if (std::filesystem::exists(pBase, ec)) return pBase;
    }

    // 3. Fallbacks for running from Build/ or subfolders
    FilePath p2 = FilePath("../Editor/Templates") / templateSub;
    if (std::filesystem::exists(p2, ec)) return p2;

    FilePath p3 = FilePath("../../Editor/Templates") / templateSub;
    if (std::filesystem::exists(p3, ec)) return p3;

    // 4. Legacy root fallback
    FilePath p4 = FilePath("Templates") / templateSub;
    if (std::filesystem::exists(p4, ec)) return p4;

    return "";
}

BOOL E_ProjectManager::Open_Temporary_Project() {
    std::error_code ec;
    FilePath tempDir = std::filesystem::temp_directory_path() / "Aurora" / "UntitledProject";
    std::filesystem::create_directories(tempDir, ec);

    // Locate blank template
    FilePath tplSource = Find_Template_Directory("Blank");

    if (!tplSource.empty() && std::filesystem::exists(tplSource, ec)) {
        std::filesystem::copy(
            tplSource,
            tempDir,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
            ec
        );
    } else {
        std::filesystem::create_directories(tempDir / "Assets/Models", ec);
        std::filesystem::create_directories(tempDir / "Assets/Audio", ec);
        std::filesystem::create_directories(tempDir / "Assets/UI", ec);
        std::filesystem::create_directories(tempDir / "Scenes", ec);
        std::filesystem::create_directories(tempDir / "Config", ec);
        std::filesystem::create_directories(tempDir / "Packs", ec);

        FilePath scenePath = tempDir / "Scenes/World.scene";
        M_SceneSerializer::Create_Blank_Template(scenePath);
    }

    s_ActiveProject.Name          = "Untitled Project";
    s_ActiveProject.RootDirectory = tempDir;
    s_ActiveProject.DefaultScene  = "Scenes/World.scene";
    s_ActiveProject.RHIBackend    = "DirectX 11";
    s_ActiveProject.bIsLoaded     = true;
    s_ActiveProject.bIsTemporary  = true;

    std::filesystem::current_path(tempDir, ec);

    FilePath packsIni = tempDir / "Config/Packs.ini";
    if (M_File::Exists(packsIni)) {
        E_AssetPacker::Load_Config(packsIni);
    }

    LOG(Info, LogLevel::Core, "E_ProjectManager::Open_Temporary_Project -> Booted in temporary workspace [{}]", tempDir.string());
    return true;
}

BOOL E_ProjectManager::Create_Project(
    const FilePath  &rootDirectory,
    const String    &projectName,
    ProjectTemplate  templateType)
{
    std::error_code ec;
    std::filesystem::create_directories(rootDirectory, ec);

    // 1. Locate template directory
    String tplSub = (templateType == ProjectTemplate::Town) ? "GameExample" : "Blank";
    FilePath tplSource = Find_Template_Directory(tplSub);

    if (!tplSource.empty() && std::filesystem::exists(tplSource, ec)) {
        // Copy entire template directory into target
        std::filesystem::copy(
            tplSource,
            rootDirectory,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
            ec
        );
    } else {
        // Fallback procedural creation if templates folder is missing
        std::filesystem::create_directories(rootDirectory / "Assets/Models", ec);
        std::filesystem::create_directories(rootDirectory / "Assets/Audio", ec);
        std::filesystem::create_directories(rootDirectory / "Assets/UI", ec);
        std::filesystem::create_directories(rootDirectory / "Scenes", ec);
        std::filesystem::create_directories(rootDirectory / "Config", ec);
        std::filesystem::create_directories(rootDirectory / "Packs", ec);

        FilePath scenePath = rootDirectory / "Scenes/World.scene";
        if (templateType == ProjectTemplate::Town) {
            M_SceneSerializer::Create_Town_Template(scenePath);
        } else {
            M_SceneSerializer::Create_Blank_Template(scenePath);
        }
    }

    s_ActiveProject.Name          = projectName;
    s_ActiveProject.RootDirectory = rootDirectory;
    s_ActiveProject.DefaultScene  = "Scenes/World.scene";
    s_ActiveProject.RHIBackend    = "DirectX 11";
    s_ActiveProject.bIsLoaded     = true;
    s_ActiveProject.bIsTemporary  = false;

    Save_Project();
    Add_RecentProject(projectName, rootDirectory);

    LOG(Info, LogLevel::Core, "E_ProjectManager::Create_Project -> Created [{}] in [{}]", projectName, rootDirectory.string());
    return true;
}

BOOL E_ProjectManager::Open_Project(const FilePath &projectDirectory) {
    std::error_code ec;
    if (!std::filesystem::exists(projectDirectory, ec)) {
        LOG(Error, LogLevel::Core, "E_ProjectManager::Open_Project -> Path does not exist: [{}]", projectDirectory.string());
        return false;
    }

    FilePath projectFile = projectDirectory / "Project.aurora";
    String text = M_File::Open(projectFile);
    if (text.empty()) {
        s_ActiveProject.Name          = projectDirectory.stem().string();
        s_ActiveProject.RootDirectory = projectDirectory;
        s_ActiveProject.DefaultScene  = "Scenes/World.scene";
        s_ActiveProject.RHIBackend    = "DirectX 11";
        s_ActiveProject.bIsLoaded     = true;
        s_ActiveProject.bIsTemporary  = false;
        Save_Project();
    } else {
        s_ActiveProject.RootDirectory = projectDirectory;
        s_ActiveProject.bIsLoaded     = true;
        s_ActiveProject.bIsTemporary  = false;

        std::istringstream stream(text);
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

            if (key == "Name") s_ActiveProject.Name = val;
            else if (key == "DefaultScene") s_ActiveProject.DefaultScene = val;
            else if (key == "RHIBackend") s_ActiveProject.RHIBackend = val;
        }
    }

    if (s_ActiveProject.Name.empty()) {
        s_ActiveProject.Name = projectDirectory.stem().string();
    }

    // Set working directory to project directory for transparent asset relative lookups
    std::filesystem::current_path(projectDirectory, ec);

    // Load pack list configuration
    FilePath packsIni = projectDirectory / "Config/Packs.ini";
    if (M_File::Exists(packsIni)) {
        E_AssetPacker::Load_Config(packsIni);
    }

    Add_RecentProject(s_ActiveProject.Name, projectDirectory);

    LOG(Info, LogLevel::Core, "E_ProjectManager::Open_Project -> Successfully opened [{}] at [{}]", s_ActiveProject.Name, projectDirectory.string());
    return true;
}

BOOL E_ProjectManager::Save_Project_As(const FilePath &destinationDir, const String &projectName) {
    std::error_code ec;
    std::filesystem::create_directories(destinationDir, ec);

    // 1. If currently in a temporary directory or different workspace, copy all files to destination
    if (s_ActiveProject.bIsLoaded && s_ActiveProject.RootDirectory != destinationDir) {
        std::filesystem::copy(
            s_ActiveProject.RootDirectory,
            destinationDir,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
            ec
        );
    }

    // 2. Update active project
    s_ActiveProject.Name          = projectName.empty() ? destinationDir.stem().string() : projectName;
    s_ActiveProject.RootDirectory = destinationDir;
    s_ActiveProject.DefaultScene  = "Scenes/World.scene";
    s_ActiveProject.bIsTemporary  = false;
    s_ActiveProject.bIsLoaded     = true;

    // 3. Switch working directory
    std::filesystem::current_path(destinationDir, ec);

    // 4. Save Project.aurora
    Save_Project();

    // 5. Register in RecentProjects.ini
    Add_RecentProject(s_ActiveProject.Name, destinationDir);

    LOG(Info, LogLevel::Core, "E_ProjectManager::Save_Project_As -> Successfully saved [{}] to [{}]", s_ActiveProject.Name, destinationDir.string());
    return true;
}

BOOL E_ProjectManager::Save_Project() {
    if (!s_ActiveProject.bIsLoaded) return false;

    std::ostringstream ss;
    ss << "# ============================================================================\n";
    ss << "# Aurora Engine Project Configuration\n";
    ss << "# ============================================================================\n\n";
    ss << "[Project]\n";
    ss << "Name = " << s_ActiveProject.Name << "\n";
    ss << "DefaultScene = " << s_ActiveProject.DefaultScene << "\n";
    ss << "RHIBackend = " << s_ActiveProject.RHIBackend << "\n";

    FilePath outPath = s_ActiveProject.RootDirectory / "Project.aurora";
    return M_File::Write(outPath, ss.str(), /*bOverWrite=*/true);
}

BOOL E_ProjectManager::Launch_Editor(const FilePath &projectDirectory) {
    FilePath editorExe = "AuroraEditor.exe";
    if (!std::filesystem::exists(editorExe)) {
        editorExe = "Editor/AuroraEditor.exe";
    }
    if (!std::filesystem::exists(editorExe)) {
        editorExe = "Build/Editor/AuroraEditor.exe";
    }

#ifdef _WIN32
    String pStr = projectDirectory.string();
    intptr_t res = _spawnl(_P_NOWAIT, editorExe.string().c_str(), editorExe.string().c_str(), "--project", pStr.c_str(), nullptr);
    if (res != -1) {
        return true;
    }
    LOG(Error, LogLevel::Core, "E_ProjectManager::Launch_Editor -> _spawnl failed to start [{}]", editorExe.string());
#endif
    return false;
}
