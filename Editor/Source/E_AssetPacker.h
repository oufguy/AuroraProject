#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_PackArchive.h>

// ============================================================================
// E_AssetPacker: Aurora Editor Asset Packing & Playlist Manager
// ============================================================================
// Allows grouping assets into distinct .pak files (e.g. Models.pak, Audio.pak)
// and building encrypted archives with a single click.
// ============================================================================
class E_AssetPacker {
private:
    static uMap<String, Array<String>> s_PackLists;
    static String                      s_ActiveConfigPath;

public:
    // Creates a new empty pack list (e.g. "GameData.pak")
    static BOOL Create_Pack(const String &packName);

    // Removes a pack list definition
    static BOOL Delete_Pack(const String &packName);

    // Adds a file path to a specific pack list
    static BOOL Add_To_Pack(const String &packName, const String &assetPath);

    // Removes a file from a pack list
    static BOOL Remove_From_Pack(const String &packName, const String &assetPath);

    // Checks if a pack exists
    static BOOL Has_Pack(const String &packName);

    // Clears all in-memory pack definitions (used when switching projects)
    static void Clear();

    // Returns all pack names (e.g. ["GameData.pak", "Audio.pak"])
    static Array<String> Get_Pack_Names();

    // Returns all files assigned to a specific pack
    static const Array<String> &Get_Pack_Files(const String &packName);

    // Saves current pack lists to an INI configuration file
    static BOOL Save_Config(const FilePath &configPath);

    // Loads pack lists from an INI configuration file
    static BOOL Load_Config(const FilePath &configPath);

    // Builds all .pak archives into the specified output folder with optional XOR encryption
    static BOOL Build_All(
        const FilePath &outputDir,
        const BYTE     *xorKey = nullptr,
        size_t          keyLen = 0);
};
