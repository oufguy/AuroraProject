#include <E_AssetPacker.h>
#include <M_File.h>
#include <sstream>

uMap<String, Array<String>> E_AssetPacker::s_PackLists;
String                      E_AssetPacker::s_ActiveConfigPath;

static const Array<String> s_EmptyList;

BOOL E_AssetPacker::Create_Pack(const String &packName) {
    if (packName.empty()) return false;
    String name = packName;
    if (!name.ends_with(".pak")) name += ".pak";

    if (s_PackLists.contains(name)) return false;
    s_PackLists[name] = {};
    return true;
}

BOOL E_AssetPacker::Delete_Pack(const String &packName) {
    return s_PackLists.erase(packName) > 0;
}

BOOL E_AssetPacker::Add_To_Pack(const String &packName, const String &assetPath) {
    String name = packName;
    if (!name.ends_with(".pak")) name += ".pak";

    if (!s_PackLists.contains(name)) {
        s_PackLists[name] = {};
    }

    String clean = M_PackArchive::Normalize_Path(assetPath);
    auto &list = s_PackLists[name];
    if (std::find(list.begin(), list.end(), clean) == list.end()) {
        list.push_back(clean);
        return true;
    }
    return false;
}

BOOL E_AssetPacker::Remove_From_Pack(const String &packName, const String &assetPath) {
    auto it = s_PackLists.find(packName);
    if (it == s_PackLists.end()) return false;

    String clean = M_PackArchive::Normalize_Path(assetPath);
    auto fit = std::find(it->second.begin(), it->second.end(), clean);
    if (fit != it->second.end()) {
        it->second.erase(fit);
        return true;
    }
    return false;
}

BOOL E_AssetPacker::Has_Pack(const String &packName) {
    return s_PackLists.contains(packName);
}

void E_AssetPacker::Clear() {
    s_PackLists.clear();
    s_ActiveConfigPath.clear();
}

Array<String> E_AssetPacker::Get_Pack_Names() {
    Array<String> names;
    names.reserve(s_PackLists.size());
    for (const auto &[name, _] : s_PackLists) {
        names.push_back(name);
    }
    return names;
}

const Array<String> &E_AssetPacker::Get_Pack_Files(const String &packName) {
    auto it = s_PackLists.find(packName);
    if (it != s_PackLists.end()) return it->second;
    return s_EmptyList;
}

BOOL E_AssetPacker::Save_Config(const FilePath &configPath) {
    std::ostringstream ss;
    ss << "# ============================================================================\n";
    ss << "# Aurora Engine - Asset Pack List Definitions\n";
    ss << "# ============================================================================\n\n";

    ss << "[Packs]\n";
    ss << "Count = " << s_PackLists.size() << "\n";
    INT pIdx = 0;
    for (const auto &[name, _] : s_PackLists) {
        ss << "Pack_" << pIdx++ << " = " << name << "\n";
    }
    ss << "\n";

    for (const auto &[name, files] : s_PackLists) {
        ss << "[Pack:" << name << "]\n";
        ss << "FileCount = " << files.size() << "\n";
        for (size_t i = 0; i < files.size(); ++i) {
            ss << "File_" << i << " = " << files[i] << "\n";
        }
        ss << "\n";
    }

    s_ActiveConfigPath = configPath.string();
    return M_File::Write(configPath, ss.str(), /*bOverWrite=*/true);
}

BOOL E_AssetPacker::Load_Config(const FilePath &configPath) {
    String text = M_File::Open(configPath);
    if (text.empty()) return false;

    s_PackLists.clear();
    std::istringstream stream(text);
    String line;
    String currentPack = "";

    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t start = line.find_first_not_of(" \t");
        if (start == String::npos) continue;
        line = line.substr(start);

        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            String sec = line.substr(1, line.size() - 2);
            if (sec.rfind("Pack:", 0) == 0) {
                currentPack = sec.substr(5);
                s_PackLists[currentPack] = {};
            } else {
                currentPack = "";
            }
            continue;
        }

        if (currentPack.empty()) continue;

        size_t eq = line.find('=');
        if (eq == String::npos) continue;

        String key = line.substr(0, eq);
        String val = line.substr(eq + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        size_t vstart = val.find_first_not_of(" \t");
        if (vstart != String::npos) val = val.substr(vstart);

        if (key.rfind("File_", 0) == 0) {
            s_PackLists[currentPack].push_back(val);
        }
    }

    s_ActiveConfigPath = configPath.string();
    return true;
}

BOOL E_AssetPacker::Build_All(const FilePath &outputDir, const BYTE *xorKey, size_t keyLen) {
    if (s_PackLists.empty()) {
        LOG(Warn, LogLevel::Developer, "E_AssetPacker::Build_All -> No packs configured to build.");
        return false;
    }

    BOOL allSuccess = true;
    for (const auto &[packName, files] : s_PackLists) {
        if (files.empty()) continue;
        FilePath targetPath = outputDir / packName;
        if (!M_PackArchive::Create_Pak(targetPath, files, xorKey, keyLen)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}
