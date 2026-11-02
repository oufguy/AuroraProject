#include <M_PackArchive.h>
#include <M_File.h>

std::mutex                        M_PackArchive::s_Mutex;
uMap<String, PakMountedFile>      M_PackArchive::s_MountedEntries;
Array<String>                     M_PackArchive::s_MountedPaks;

String M_PackArchive::Normalize_Path(StringView path) {
    String p(path);
    std::replace(p.begin(), p.end(), '\\', '/');
    while (!p.empty() && (p.front() == '.' || p.front() == '/')) {
        if (p.rfind("./", 0) == 0) p = p.substr(2);
        else if (p.front() == '/') p = p.substr(1);
        else break;
    }
    return p;
}

void M_PackArchive::Apply_XOR(BYTE *data, size_t size, const BYTE *key, size_t keyLen, UINT64 fileOffset) {
    if (!data || size == 0 || !key || keyLen == 0) return;
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= key[(fileOffset + i) % keyLen];
    }
}

BOOL M_PackArchive::Create_Pak(
    const FilePath      &outputPakPath,
    const Array<String> &sourceFilePaths,
    const BYTE          *xorKey,
    size_t               keyLen)
{
    if (sourceFilePaths.empty()) {
        LOG(Warn, LogLevel::Developer, "M_PackArchive::Create_Pak -> No files specified for [{}]", outputPakPath.string());
        return false;
    }

    std::error_code ec;
    FilePath parentDir = outputPakPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec)) {
        std::filesystem::create_directories(parentDir, ec);
    }

    std::ofstream out(outputPakPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG(Error, LogLevel::Developer, "M_PackArchive::Create_Pak -> Could not open [{}] for writing", outputPakPath.string());
        return false;
    }

    PakHeader header{};
    header.FileCount = static_cast<UINT32>(sourceFilePaths.size());
    header.Flags     = (xorKey && keyLen > 0) ? 1 : 0;

    // Reserve header & table of contents space
    out.write(reinterpret_cast<const char *>(&header), sizeof(PakHeader));

    Array<PakEntryHeader> entries(header.FileCount);
    // Write placeholder entries table
    out.write(reinterpret_cast<const char *>(entries.data()), entries.size() * sizeof(PakEntryHeader));

    // Write file payloads
    UINT64 currentOffset = sizeof(PakHeader) + entries.size() * sizeof(PakEntryHeader);

    for (size_t i = 0; i < sourceFilePaths.size(); ++i) {
        String cleanPath = Normalize_Path(sourceFilePaths[i]);
        Array<BYTE> fileData = M_File::Read_Bytes(sourceFilePaths[i]);
        if (fileData.empty() && !M_File::Exists(sourceFilePaths[i])) {
            LOG(Warn, LogLevel::Developer, "M_PackArchive::Create_Pak -> File missing: [{}]", sourceFilePaths[i]);
        }

        PakEntryHeader &entry = entries[i];
        std::memset(entry.Path, 0, sizeof(entry.Path));
        std::strncpy(entry.Path, cleanPath.c_str(), sizeof(entry.Path) - 1);
        entry.Offset = currentOffset;
        entry.Size   = fileData.size();

        // Calculate simple Adler/CRC checksum
        UINT32 sum = 0;
        for (BYTE b : fileData) sum = (sum * 31) + b;
        entry.Checksum = sum;

        // Apply XOR encryption if enabled
        if (header.Flags & 1) {
            Apply_XOR(fileData.data(), fileData.size(), xorKey, keyLen, 0);
        }

        if (!fileData.empty()) {
            out.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
        }

        currentOffset += fileData.size();
    }

    // Seek back and rewrite valid entry table
    out.seekp(sizeof(PakHeader), std::ios::beg);
    out.write(reinterpret_cast<const char *>(entries.data()), entries.size() * sizeof(PakEntryHeader));
    out.close();

    LOG(Info, LogLevel::Core, "M_PackArchive::Create_Pak -> Created [{}] with {} files ({} KB, Encrypted={})",
        outputPakPath.string(), header.FileCount, currentOffset / 1024, (header.Flags & 1) ? "YES" : "NO");
    return true;
}

BOOL M_PackArchive::Mount(const FilePath &pakPath, const BYTE *xorKey, size_t keyLen) {
    std::lock_guard<std::mutex> lock(s_Mutex);

    String pakStr = Normalize_Path(pakPath.string());
    std::ifstream file(pakPath, std::ios::binary);
    if (!file.is_open()) {
        LOG(Warn, LogLevel::Developer, "M_PackArchive::Mount -> Could not open pak [{}]", pakPath.string());
        return false;
    }

    PakHeader header{};
    file.read(reinterpret_cast<char *>(&header), sizeof(PakHeader));
    if (std::memcmp(header.Magic, "APAK", 4) != 0) {
        LOG(Error, LogLevel::Developer, "M_PackArchive::Mount -> Invalid magic header in [{}]", pakPath.string());
        return false;
    }

    Array<PakEntryHeader> entries(header.FileCount);
    file.read(reinterpret_cast<char *>(entries.data()), entries.size() * sizeof(PakEntryHeader));

    Array<BYTE> storedKey;
    if (xorKey && keyLen > 0) {
        storedKey.assign(xorKey, xorKey + keyLen);
    }

    for (const auto &entry : entries) {
        String pathKey = Normalize_Path(entry.Path);
        PakMountedFile mounted{};
        mounted.PakFilePath = pakPath.string();
        mounted.Offset      = entry.Offset;
        mounted.Size        = entry.Size;
        mounted.bEncrypted  = (header.Flags & 1) != 0;
        mounted.XorKey      = storedKey;
        s_MountedEntries[pathKey] = std::move(mounted);
    }

    s_MountedPaks.push_back(pakPath.string());
    LOG(Info, LogLevel::Core, "M_PackArchive::Mount -> Mounted [{}] ({} files)", pakPath.string(), header.FileCount);
    return true;
}

void M_PackArchive::Unmount(const FilePath &pakPath) {
    std::lock_guard<std::mutex> lock(s_Mutex);
    String target = pakPath.string();

    for (auto it = s_MountedEntries.begin(); it != s_MountedEntries.end(); ) {
        if (it->second.PakFilePath == target) {
            it = s_MountedEntries.erase(it);
        } else {
            ++it;
        }
    }

    s_MountedPaks.erase(
        std::remove(s_MountedPaks.begin(), s_MountedPaks.end(), target),
        s_MountedPaks.end()
    );
}

void M_PackArchive::Unmount_All() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_MountedEntries.clear();
    s_MountedPaks.clear();
}

BOOL M_PackArchive::Contains(const String &virtualPath) {
    std::lock_guard<std::mutex> lock(s_Mutex);
    String key = Normalize_Path(virtualPath);
    return s_MountedEntries.contains(key);
}

Array<BYTE> M_PackArchive::Read_Bytes(const String &virtualPath) {
    PakMountedFile fileInfo{};
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        String key = Normalize_Path(virtualPath);
        auto it = s_MountedEntries.find(key);
        if (it == s_MountedEntries.end()) return {};
        fileInfo = it->second;
    }

    if (fileInfo.Size == 0) return {};

    std::ifstream file(fileInfo.PakFilePath, std::ios::binary);
    if (!file.is_open()) return {};

    file.seekg(fileInfo.Offset, std::ios::beg);
    Array<BYTE> buffer(static_cast<size_t>(fileInfo.Size));
    file.read(reinterpret_cast<char *>(buffer.data()), fileInfo.Size);

    if (fileInfo.bEncrypted && !fileInfo.XorKey.empty()) {
        Apply_XOR(buffer.data(), buffer.size(), fileInfo.XorKey.data(), fileInfo.XorKey.size(), 0);
    }

    return buffer;
}

String M_PackArchive::Read_String(const String &virtualPath) {
    Array<BYTE> bytes = Read_Bytes(virtualPath);
    if (bytes.empty()) return "";
    return String(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

Array<String> M_PackArchive::Get_MountedPaks() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_MountedPaks;
}
