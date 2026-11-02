#include <M_File.h>
#include <M_PackArchive.h>

std::mutex M_File::g_FileMutex;

String M_File::Open(const FilePath &filePath) {
    if (M_PackArchive::Contains(filePath.string())) {
        return M_PackArchive::Read_String(filePath.string());
    }

    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    if (!std::filesystem::exists(filePath, ec) || std::filesystem::is_directory(filePath, ec)) {
        return "";
    }

    InputFile file(filePath, ModeRead);
    if (!file.is_open()) return "";

    file.seekg(0, FileEnd);
    StreamSize fileSize = file.tellg();
    if (fileSize <= 0) return "";

    file.seekg(0, FileStart);

    String buffer;
    buffer.resize(static_cast<size_t>(fileSize));
    file.read(buffer.data(), fileSize);

    return buffer;
}

BOOL M_File::Write(const FilePath &filePath, StringView data, BOOL bOverWrite) {
    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    FilePath dir = filePath.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir, ec)) {
        std::filesystem::create_directories(dir, ec);
    }

    OutputFile file(filePath, bOverWrite ? ModeOverwrite : ModeAppend);
    if (!file.is_open()) return false;

    file.write(data.data(), static_cast<StreamSize>(data.size()));
    return file.good();
}

void M_File::Close(String &loadedData) {
    loadedData.clear();
    loadedData.shrink_to_fit();
}

BOOL M_File::Clean(const FilePath &filePath) {
    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    if (!std::filesystem::exists(filePath, ec)) return false;

    OutputFile file(filePath, ModeOverwrite);
    return file.is_open();
}

BOOL M_File::Delete(const FilePath &path) {
    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    return std::filesystem::remove_all(path, ec) > 0;
}

BOOL M_File::Exists(const FilePath &filePath) {
    if (M_PackArchive::Contains(filePath.string())) {
        return true;
    }
    ErrorCode ec;
    return std::filesystem::exists(filePath, ec);
}

BOOL M_File::Find(const FilePath &filePath) {
    return Exists(filePath);
}

Array<BYTE> M_File::Read_Bytes(const FilePath &filePath) {
    if (M_PackArchive::Contains(filePath.string())) {
        return M_PackArchive::Read_Bytes(filePath.string());
    }

    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    if (!std::filesystem::exists(filePath, ec) || std::filesystem::is_directory(filePath, ec)) {
        return {};
    }

    InputFile file(filePath, ModeRead);
    if (!file.is_open()) return {};

    file.seekg(0, FileEnd);
    StreamSize fileSize = file.tellg();
    if (fileSize <= 0) return {};

    file.seekg(0, FileStart);

    Array<BYTE> buffer(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<Char *>(buffer.data()), fileSize);

    return buffer;
}

BOOL M_File::Write_Bytes(const FilePath &filePath, const void *data, size_t size) {
    ScopedLock lock(g_FileMutex);

    ErrorCode ec;
    FilePath dir = filePath.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir, ec)) {
        std::filesystem::create_directories(dir, ec);
    }

    OutputFile file(filePath, ModeOverwrite);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const Char *>(data), static_cast<StreamSize>(size));
    return file.good();
}

BOOL M_File::Mount_Pak(const FilePath &pakPath, const BYTE *xorKey, size_t keyLen) {
    return M_PackArchive::Mount(pakPath, xorKey, keyLen);
}

void M_File::Unmount_All_Paks() {
    M_PackArchive::Unmount_All();
}
