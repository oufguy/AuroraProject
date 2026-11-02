#include <M_Texture.h>

std::mutex                   M_Texture::s_Mutex;
uMap<UINT, TextureData>      M_Texture::s_Textures;
uMap<String, UINT>           M_Texture::s_PathCache;
UINT                         M_Texture::s_NextTextureID    = 0;
UINT                         M_Texture::s_MissingTextureID = 0;
BOOL                         M_Texture::s_Initialized      = false;

std::mutex                   M_Texture::s_StreamMutex;
Array<M_Texture::StreamedTextureJob> M_Texture::s_CompletedStreamQueue;
std::atomic<UINT>            M_Texture::s_PendingStreamCount{ 0 };
std::atomic<UINT>            M_Texture::s_CompletedStreamCount{ 0 };

void M_Texture::Ensure_Initialized() {
    if (s_Initialized) return;
    s_Initialized = true;

    // TextureID 0 = Built-in 1x1 Solid White Texture (Default for untextured meshes)
    TextureData white{};
    white.TextureID = 0;
    white.Width     = 1;
    white.Height    = 1;
    white.Revision  = 1;
    white.Pixels    = { COLOR_WHITE };
    s_Textures[0]   = std::move(white);
}

BOOL M_Texture::Decode_File(const FilePath &path, TextureData &outTex) {
    if (!M_File::Find(path)) return false;

    String pathStr = path.string();
    String ext     = path.extension().string();
    for (Char &c : ext) c = static_cast<Char>(std::tolower(static_cast<unsigned char>(c)));

    // 1. Try SDL3 built-in BMP loader (supports 8/16/24/32-bit BMPs and converts to RGBA32)
    if (ext == ".bmp") {
        SDL_Surface *surf = SDL_LoadBMP(pathStr.c_str());
        if (!surf) return false;

        SDL_Surface *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surf);
        if (!rgba) return false;

        outTex.Width  = rgba->w;
        outTex.Height = rgba->h;
        outTex.Pixels.resize(static_cast<size_t>(rgba->w) * static_cast<size_t>(rgba->h));

        const BYTE *srcBytes = static_cast<const BYTE *>(rgba->pixels);
        for (INT y = 0; y < rgba->h; ++y) {
            std::memcpy(
                &outTex.Pixels[static_cast<size_t>(y) * static_cast<size_t>(rgba->w)],
                srcBytes + static_cast<size_t>(y) * static_cast<size_t>(rgba->pitch),
                static_cast<size_t>(rgba->w) * sizeof(COLOR));
        }

        SDL_DestroySurface(rgba);
        return outTex.Is_Valid();
    }

    // 2. Built-in 24-bit / 32-bit Uncompressed TGA Loader (Zero external DLLs!)
    if (ext == ".tga") {
        String rawData = M_File::Open(path);
        if (rawData.size() < 18) return false;
        const BYTE *bytes = reinterpret_cast<const BYTE *>(rawData.data());

        const BYTE idLen     = bytes[0];
        const BYTE imageType = bytes[2];
        const INT  width     = static_cast<INT>(bytes[12] | (bytes[13] << 8));
        const INT  height    = static_cast<INT>(bytes[14] | (bytes[15] << 8));
        const BYTE bpp       = bytes[16];
        const BYTE desc      = bytes[17];

        if (imageType != 2 || width <= 0 || height <= 0 || (bpp != 24 && bpp != 32)) {
            return false;
        }

        const size_t bytesPerPixel = bpp / 8;
        const size_t dataOffset    = 18 + idLen;
        const size_t totalPixels   = static_cast<size_t>(width) * static_cast<size_t>(height);
        if (rawData.size() < dataOffset + totalPixels * bytesPerPixel) return false;

        outTex.Width  = width;
        outTex.Height = height;
        outTex.Pixels.resize(totalPixels);

        const BOOL flipY = ((desc & 0x20) == 0); // TGA default origin is bottom-left unless bit 5 is set
        for (INT y = 0; y < height; ++y) {
            const INT dstY = flipY ? (height - 1 - y) : y;
            for (INT x = 0; x < width; ++x) {
                const size_t srcIdx = dataOffset + (static_cast<size_t>(y) * width + x) * bytesPerPixel;
                const BYTE   b      = bytes[srcIdx + 0];
                const BYTE   g      = bytes[srcIdx + 1];
                const BYTE   r      = bytes[srcIdx + 2];
                const BYTE   a      = (bytesPerPixel == 4) ? bytes[srcIdx + 3] : 255;
                outTex.Pixels[static_cast<size_t>(dstY) * width + x] = COLOR(r, g, b, a);
            }
        }
        return outTex.Is_Valid();
    }

    return false;
}

UINT M_Texture::Get_MissingTexture() {
    if (s_MissingTextureID != 0) return s_MissingTextureID;
    s_MissingTextureID = Create_Checkerboard(64, 64, 8, COLOR_MAGENTA, COLOR(0, 0, 0, 255));
    return s_MissingTextureID;
}

UINT M_Texture::Get_MissingUITexture() {
    static UINT s_MissingUIID = 0;
    if (s_MissingUIID != 0) return s_MissingUIID;

    constexpr INT W = 32, H = 32;
    Array<COLOR> px(static_cast<size_t>(W) * H, COLOR(45, 48, 55, 255)); // Dark gray background

    for (INT y = 0; y < H; ++y) {
        for (INT x = 0; x < W; ++x) {
            // 2px outer red border
            if (x < 2 || x >= W - 2 || y < 2 || y >= H - 2) {
                px[static_cast<size_t>(y) * W + x] = COLOR(200, 45, 45, 255);
                continue;
            }
            // 2px diagonal 'X'
            INT d1 = std::abs(x - y);
            INT d2 = std::abs(x - (H - 1 - y));
            if (d1 <= 1 || d2 <= 1) {
                px[static_cast<size_t>(y) * W + x] = COLOR(235, 60, 60, 255);
            }
        }
    }
    s_MissingUIID = Create_FromPixels(W, H, px);
    return s_MissingUIID;
}

UINT M_Texture::Load_UI(const FilePath &path) {
    if (path.empty()) return 0;

    String key = path.string();
    {
        ScopedLock lock(s_Mutex);
        Ensure_Initialized();
        auto it = s_PathCache.find(key);
        if (it != s_PathCache.end()) return it->second;
    }

    TextureData decoded{};
    if (!Decode_File(path, decoded)) {
        LOG(Warn, LogLevel::Developer, "M_Texture::Load_UI failed to load [{}]! Using Ace Online style 'X' fallback icon.", key);
        UINT missingID = Get_MissingUITexture();
        ScopedLock lock(s_Mutex);
        s_PathCache[key] = missingID;
        return missingID;
    }

    ScopedLock lock(s_Mutex);
    decoded.TextureID = ++s_NextTextureID;
    decoded.Revision  = 1;
    UINT id           = decoded.TextureID;
    s_Textures[id]    = std::move(decoded);
    s_PathCache[key]  = id;

    LOG(Info, LogLevel::Core, "M_Texture::Load_UI -> Loaded [{}] (ID: {}, {}x{})",
        key, id, s_Textures[id].Width, s_Textures[id].Height);
    return id;
}

UINT M_Texture::Create_Solid(const COLOR &color) {
    ScopedLock lock(s_Mutex);
    Ensure_Initialized();

    TextureData tex{};
    tex.TextureID = ++s_NextTextureID;
    tex.Width     = 1;
    tex.Height    = 1;
    tex.Revision  = 1;
    tex.Pixels    = { color };

    UINT id        = tex.TextureID;
    s_Textures[id] = std::move(tex);
    return id;
}

UINT M_Texture::Create_Checkerboard(
    INT          width,
    INT          height,
    INT          cellSize,
    const COLOR &colorA,
    const COLOR &colorB)
{
    if (width <= 0)    width    = 64;
    if (height <= 0)   height   = 64;
    if (cellSize <= 0) cellSize = 8;

    TextureData tex{};
    tex.Width    = width;
    tex.Height   = height;
    tex.Revision = 1;
    tex.Pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));

    for (INT y = 0; y < height; ++y) {
        for (INT x = 0; x < width; ++x) {
            const BOOL isEven = (((x / cellSize) + (y / cellSize)) % 2) == 0;
            tex.Pixels[static_cast<size_t>(y) * width + x] = isEven ? colorA : colorB;
        }
    }

    ScopedLock lock(s_Mutex);
    Ensure_Initialized();
    tex.TextureID  = ++s_NextTextureID;
    UINT id        = tex.TextureID;
    s_Textures[id] = std::move(tex);
    return id;
}

UINT M_Texture::Create_FromPixels(INT width, INT height, const Array<COLOR> &pixels) {
    if (width <= 0 || height <= 0 || pixels.size() != static_cast<size_t>(width) * static_cast<size_t>(height)) {
        return Get_MissingTexture();
    }

    ScopedLock lock(s_Mutex);
    Ensure_Initialized();

    TextureData tex{};
    tex.TextureID = ++s_NextTextureID;
    tex.Width     = width;
    tex.Height    = height;
    tex.Revision  = 1;
    tex.Pixels    = pixels;

    UINT id        = tex.TextureID;
    s_Textures[id] = std::move(tex);
    return id;
}

UINT M_Texture::Load(const FilePath &path) {
    String key = path.string();
    {
        ScopedLock lock(s_Mutex);
        Ensure_Initialized();
        auto it = s_PathCache.find(key);
        if (it != s_PathCache.end()) {
            return it->second;
        }
    }

    TextureData decoded{};
    if (!Decode_File(path, decoded)) {
        LOG(Warn, LogLevel::Developer, "M_Texture::Load failed to load [{}]! Using fallback checkerboard.", key);
        return Get_MissingTexture();
    }

    ScopedLock lock(s_Mutex);
    decoded.TextureID = ++s_NextTextureID;
    decoded.Revision  = 1;
    UINT id           = decoded.TextureID;
    s_Textures[id]    = std::move(decoded);
    s_PathCache[key]  = id;

    LOG(Info, LogLevel::Core, "M_Texture::Load -> Loaded [{}] (ID: {}, {}x{})",
        key, id, s_Textures[id].Width, s_Textures[id].Height);
    return id;
}

void M_Texture::Stream(const FilePath &path, Function<void(UINT)> onLoaded) {
    String key = path.string();
    {
        ScopedLock lock(s_Mutex);
        Ensure_Initialized();
        auto it = s_PathCache.find(key);
        if (it != s_PathCache.end()) {
            UINT id = it->second;
            {
                std::lock_guard<std::mutex> sLock(s_StreamMutex);
                s_CompletedStreamQueue.push_back({ id, std::move(onLoaded) });
            }
            ++s_CompletedStreamCount;
            return;
        }
    }

    ++s_PendingStreamCount;
    M_Threads::Run([path, onLoaded]() {
        UINT texID = Load(path);
        {
            std::lock_guard<std::mutex> lock(s_StreamMutex);
            s_CompletedStreamQueue.push_back({ texID, std::move(onLoaded) });
        }
        ++s_CompletedStreamCount;
        if (s_PendingStreamCount > 0) {
            --s_PendingStreamCount;
        }
    });
}

void M_Texture::Tick_Streaming(UINT budgetPerTick) {
    if (budgetPerTick == 0) return;
    if (s_CompletedStreamCount.load(std::memory_order_relaxed) == 0) return;

    Array<StreamedTextureJob> jobsToDispatch;
    {
        std::lock_guard<std::mutex> lock(s_StreamMutex);
        if (s_CompletedStreamQueue.empty()) return;

        size_t count = std::min(static_cast<size_t>(budgetPerTick), s_CompletedStreamQueue.size());
        jobsToDispatch.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            jobsToDispatch.push_back(std::move(s_CompletedStreamQueue[i]));
        }
        s_CompletedStreamQueue.erase(s_CompletedStreamQueue.begin(), s_CompletedStreamQueue.begin() + count);
        s_CompletedStreamCount.store(static_cast<UINT>(s_CompletedStreamQueue.size()), std::memory_order_relaxed);
    }

    for (auto &job : jobsToDispatch) {
        if (job.Callback) {
            job.Callback(job.TextureID);
        }
    }
}

const TextureData &M_Texture::Get(UINT textureID) {
    ScopedLock lock(s_Mutex);
    Ensure_Initialized();

    auto it = s_Textures.find(textureID);
    if (it != s_Textures.end() && it->second.Is_Valid()) {
        return it->second;
    }
    return s_Textures[0];
}

void M_Texture::Unload(UINT textureID) {
    if (textureID == 0) return;
    ScopedLock lock(s_Mutex);
    auto it = s_Textures.find(textureID);
    if (it != s_Textures.end()) {
        for (auto pathIt = s_PathCache.begin(); pathIt != s_PathCache.end(); ++pathIt) {
            if (pathIt->second == textureID) {
                s_PathCache.erase(pathIt);
                break;
            }
        }
        s_Textures.erase(it);
    }
}

void M_Texture::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(s_StreamMutex);
        s_CompletedStreamQueue.clear();
        s_PendingStreamCount = 0;
    }
    ScopedLock lock(s_Mutex);
    s_Textures.clear();
    s_PathCache.clear();
    s_NextTextureID    = 0;
    s_MissingTextureID = 0;
    s_Initialized      = false;
}
