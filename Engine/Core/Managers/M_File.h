#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>

// ============================================================================
// Cross-Platform Thread-Safe File Manager
// ============================================================================
class M_File {
private:
    using InputFile  = std::ifstream;
    using OutputFile = std::ofstream;
    using ErrorCode  = std::error_code;
    using StreamSize = std::streamsize;
    using Mutex      = std::mutex;
    using ScopedLock = std::lock_guard<std::mutex>;

    static constexpr auto ModeRead      = std::ios::in | std::ios::binary;
    static constexpr auto ModeOverwrite = std::ios::out | std::ios::trunc | std::ios::binary;
    static constexpr auto ModeAppend    = std::ios::out | std::ios::app | std::ios::binary;
    static constexpr auto FileStart     = std::ios::beg;
    static constexpr auto FileEnd       = std::ios::end;

    static Mutex g_FileMutex;

public:
    // 1. Open and load a text file into RAM (returns empty string "" if file doesn't exist or is empty)
    static String Open(const FilePath &filePath);

    // 2. Write text data to a file (auto-creates folders; set bOverWrite = true to overwrite)
    static BOOL Write(const FilePath &filePath, StringView data, BOOL bOverWrite = false);

    // 3. Close a loaded file in RAM (frees memory immediately)
    static void Close(String &loadedData);

    // 4. Empty a file on disk (0 bytes) without deleting it
    static BOOL Clean(const FilePath &filePath);

    // 5. Delete a file or folder from disk
    static BOOL Delete(const FilePath &path);

    // 6. Check if a file or directory exists on disk
    [[nodiscard]] static BOOL Exists(const FilePath &filePath);
    [[nodiscard]] static BOOL Find(const FilePath &filePath);

    // 7. Reads an entire binary file into a byte array (Array<BYTE>)
    [[nodiscard]] static Array<BYTE> Read_Bytes(const FilePath &filePath);

    // 8. Writes raw binary bytes to a file on disk
    static BOOL Write_Bytes(const FilePath &filePath, const void *data, size_t size);

    // 9. Mount / Unmount .pak archive files into the virtual filesystem
    static BOOL Mount_Pak(const FilePath &pakPath, const BYTE *xorKey = nullptr, size_t keyLen = 0);
    static void Unmount_All_Paks();
};
