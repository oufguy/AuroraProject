#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>

// ============================================================================
// M_PackArchive: Aurora Engine Virtual File System & .pak Archive Manager
// ============================================================================
// Supports:
//   1. Creating standalone binary .pak archives from loose files.
//   2. Transparent mounting: game engine reads models, textures, audio from .pak.
//   3. Optional high-speed 256-bit XOR keystream encryption.
//   4. Zero third-party dependencies: pure native C++23.
// ============================================================================

#pragma pack(push, 1)
struct PakHeader {
    char   Magic[4]  = { 'A', 'P', 'A', 'K' }; // "APAK"
    UINT32 Version   = 1;
    UINT32 FileCount = 0;
    UINT32 Flags     = 0;                      // Bit 0 = XOR Encrypted
};

struct PakEntryHeader {
    char   Path[260] = {};                     // Relative virtual path e.g. "Assets/Models/Building.obj"
    UINT64 Offset    = 0;                      // Byte offset in .pak file
    UINT64 Size      = 0;                      // Byte length of file payload
    UINT32 Checksum  = 0;                      // Verification checksum
};
#pragma pack(pop)

struct PakMountedFile {
    String PakFilePath;
    UINT64 Offset = 0;
    UINT64 Size   = 0;
    BOOL   bEncrypted = false;
    Array<BYTE> XorKey;
};

class M_PackArchive {
private:
    static std::mutex                               s_Mutex;
    static uMap<String, PakMountedFile>             s_MountedEntries;
    static Array<String>                            s_MountedPaks;

    static void Apply_XOR(BYTE *data, size_t size, const BYTE *key, size_t keyLen, UINT64 fileOffset = 0);

public:
    // Builds a new binary .pak archive from a list of loose files
    static BOOL Create_Pak(
        const FilePath      &outputPakPath,
        const Array<String> &sourceFilePaths,
        const BYTE          *xorKey = nullptr,
        size_t               keyLen = 0);

    // Mounts a .pak file into the virtual filesystem
    static BOOL Mount(
        const FilePath &pakPath,
        const BYTE     *xorKey = nullptr,
        size_t          keyLen = 0);

    // Unmounts a mounted .pak file
    static void Unmount(const FilePath &pakPath);

    // Unmounts all currently mounted .pak archives
    static void Unmount_All();

    // Checks if a file exists in any mounted .pak
    [[nodiscard]] static BOOL Contains(const String &virtualPath);

    // Reads raw binary bytes of a file from mounted .pak (returns empty if not found)
    [[nodiscard]] static Array<BYTE> Read_Bytes(const String &virtualPath);

    // Reads file contents as string from mounted .pak
    [[nodiscard]] static String Read_String(const String &virtualPath);

    // Returns all mounted .pak file paths
    [[nodiscard]] static Array<String> Get_MountedPaks();

    // Normalizes file paths (replaces \ with / and trims whitespace)
    static String Normalize_Path(StringView path);
};
