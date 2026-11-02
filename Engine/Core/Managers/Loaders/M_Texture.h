#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <M_Log.h>
#include <M_File.h>
#include <M_Threads.h>

// ============================================================================
// 2D RGBA Texture Data (RAM Pixel Buffer + Unique TextureID for GPU Caching)
// ============================================================================
struct TextureData {
    UINT         TextureID = 0; // 0 = Default 1x1 White Texture
    INT          Width     = 1;
    INT          Height    = 1;
    UINT         Revision  = 1;
    Array<COLOR> Pixels;        // Row-major RGBA8 pixels (Width * Height * 4 bytes)

    void Mark_Dirty() { ++Revision; }

    [[nodiscard]] BOOL Is_Valid() const {
        return Width > 0 && Height > 0 && Pixels.size() == static_cast<size_t>(Width) * static_cast<size_t>(Height);
    }
};

// ============================================================================
// Texture Manager (Procedural Generation, File Loading, Path Cache & Async Load)
// ============================================================================
// Quick Usage Example:
//   UINT checkerTex = M_Texture::Create_Checkerboard(64, 64, 8, COLOR_WHITE, COLOR_GRAY);
//   UINT brickTex   = M_Texture::Load("Assets/Textures/Brick.bmp");
//   cube.Set_Texture(checkerTex);
// ============================================================================
class M_Texture {
private:
    using Mutex      = std::mutex;
    using ScopedLock = std::lock_guard<std::mutex>;

    static Mutex                   s_Mutex;
    static uMap<UINT, TextureData> s_Textures;
    static uMap<String, UINT>      s_PathCache;
    static UINT                    s_NextTextureID;
    static UINT                    s_MissingTextureID;
    static BOOL                    s_Initialized;

    struct StreamedTextureJob {
        UINT                 TextureID = 0;
        Function<void(UINT)> Callback;
    };

    static std::mutex                s_StreamMutex;
    static Array<StreamedTextureJob> s_CompletedStreamQueue;
    static std::atomic<UINT>         s_PendingStreamCount;
    static std::atomic<UINT>         s_CompletedStreamCount;

    static void Ensure_Initialized();
    static BOOL Decode_File(const FilePath &path, TextureData &outTex);

public:
    // Returns the default 1x1 White TextureID (0)
    [[nodiscard]] static constexpr UINT Get_White() {
        return 0;
    }

    // Returns a 64x64 Magenta/Black missing-texture fallback checkerboard (never crashes on missing files!)
    [[nodiscard]] static UINT Get_MissingTexture();

    // Returns a 32x32 Ace Online style missing UI "Red X" button icon (never crashes on missing UI files!)
    [[nodiscard]] static UINT Get_MissingUITexture();

    // Loads a UI texture from disk. If missing, logs warning and returns Get_MissingUITexture().
    static UINT Load_UI(const FilePath &path);

    // Creates a 1x1 solid color texture and returns its Unique TextureID
    static UINT Create_Solid(const COLOR &color);

    // Creates a procedural 2D checkerboard texture (great for UV testing, floors, cubes & debug grids!)
    // Example: UINT checker = M_Texture::Create_Checkerboard(64, 64, 8, COLOR_WHITE, COLOR_GRAY);
    static UINT Create_Checkerboard(
        INT          width    = 64,
        INT          height   = 64,
        INT          cellSize = 8,
        const COLOR &colorA   = COLOR_WHITE,
        const COLOR &colorB   = COLOR(50, 65, 85, 255));

    // Creates a custom texture directly from an RGBA8 pixel array
    static UINT Create_FromPixels(INT width, INT height, const Array<COLOR> &pixels);

    // Loads a .bmp or .tga texture from disk (cached by path so duplicate loads are instant O(1)!).
    // If the file is missing or invalid, logs a warning and returns Get_MissingTexture().
    static UINT Load(const FilePath &path);

    // Multi-Threaded Asynchronous Texture Streaming Queue using M_Threads!
    // Decodes the image on a background worker thread, adds it to the streaming array,
    // and dispatches it piece-by-piece on the main thread during Core::Tick() so frame rates remain rock solid!
    static void Stream(const FilePath &path, Function<void(UINT)> onLoaded);

    // Multi-Threaded Asynchronous Texture Loader (routes through the time-sliced streaming queue)
    static inline void Load_Async(const FilePath &path, Function<void(UINT)> onLoaded) {
        Stream(path, std::move(onLoaded));
    }

    // Processes completed texture streaming jobs piece-by-piece on the main thread during Core::Tick().
    // Limits dispatching to `budgetPerTick` textures per frame to eliminate frame-time stutters.
    static void Tick_Streaming(UINT budgetPerTick = 1);

    // Returns the number of textures currently decoding in the background
    [[nodiscard]] static inline UINT Get_PendingStreamCount() {
        return s_PendingStreamCount.load();
    }

    // Thread-safe lookup of TextureData by TextureID (falls back to 1x1 White TextureID 0 if not found)
    [[nodiscard]] static const TextureData &Get(UINT textureID);

    // Unloads a texture from RAM and frees its pixel memory
    // (Default White TextureID 0 cannot be unloaded)
    static void Unload(UINT textureID);

    // Frees all cached CPU texture pixel data and clears the streaming queue cleanly on engine shutdown
    static void Shutdown();
};
