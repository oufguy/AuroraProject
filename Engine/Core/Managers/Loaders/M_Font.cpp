#include <pch.h>
#include <M_Font.h>
#include <M_File.h>
#include <M_Texture.h>
#include <M_Log.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <Core/Internal/stb_truetype.h>

// Static member definitions
UINT      M_Font::s_FontTextureID = 0;
FLOAT     M_Font::s_BakedFontSize = 24.0f;
FLOAT     M_Font::s_LineHeight    = 28.0f;
FLOAT     M_Font::s_Ascent        = 20.0f;
BOOL      M_Font::s_IsInitialized = false;
void     *M_Font::s_BakedCharData = nullptr;

static constexpr INT ATLAS_SIZE = 512;
static constexpr INT FIRST_CHAR = 32;
static constexpr INT NUM_CHARS  = 96;

void M_Font::Ensure_Initialized() {
    if (!s_IsInitialized) {
        Init();
    }
}

BOOL M_Font::Init(const FilePath &ttfPath, FLOAT fontSize) {
    if (s_IsInitialized && s_FontTextureID != 0) {
        Shutdown();
    }

    s_BakedFontSize = (fontSize > 0.0f) ? fontSize : 24.0f;

    Array<BYTE> fontBytes;
    String loadedPath;

    if (!ttfPath.empty() && M_File::Exists(ttfPath)) {
        fontBytes  = M_File::Read_Bytes(ttfPath);
        loadedPath = ttfPath.string();
    }

    if (fontBytes.empty()) {
        static const FilePath fallbackPaths[] = {
            "Assets/Fonts/Default.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf"
        };
        for (const auto &p : fallbackPaths) {
            if (M_File::Exists(p)) {
                fontBytes  = M_File::Read_Bytes(p);
                loadedPath = p.string();
                if (!fontBytes.empty()) break;
            }
        }
    }

    if (fontBytes.empty()) {
        LOG(Error, LogLevel::Core, "M_Font::Init -> Failed to find any TrueType font (.ttf) file!");
        return false;
    }

    if (!s_BakedCharData) {
        s_BakedCharData = new stbtt_bakedchar[NUM_CHARS];
    }
    stbtt_bakedchar *cdata = static_cast<stbtt_bakedchar *>(s_BakedCharData);

    Array<BYTE> alphaBitmap(ATLAS_SIZE * ATLAS_SIZE, 0);

    int bakeRes = stbtt_BakeFontBitmap(
        fontBytes.data(), 0,
        s_BakedFontSize,
        alphaBitmap.data(), ATLAS_SIZE, ATLAS_SIZE,
        FIRST_CHAR, NUM_CHARS,
        cdata);

    if (bakeRes <= 0) {
        LOG(Warn, LogLevel::Core, "M_Font::Init -> Some characters failed to fit in font atlas! bakeRes={}", bakeRes);
    }

    // Convert single-channel alpha bitmap to RGBA8 texture
    Array<COLOR> rgbaPixels(ATLAS_SIZE * ATLAS_SIZE);
    for (size_t i = 0; i < alphaBitmap.size(); ++i) {
        BYTE a = alphaBitmap[i];
        rgbaPixels[i] = COLOR(255, 255, 255, a);
    }

    s_FontTextureID = M_Texture::Create_FromPixels(ATLAS_SIZE, ATLAS_SIZE, rgbaPixels);

    // Compute vertical metrics
    float ascent = 0.0f, descent = 0.0f, lineGap = 0.0f;
    stbtt_GetScaledFontVMetrics(fontBytes.data(), 0, s_BakedFontSize, &ascent, &descent, &lineGap);
    if (ascent > 0.0f) {
        s_Ascent     = ascent;
        s_LineHeight = ascent - descent + lineGap;
    } else {
        s_Ascent     = s_BakedFontSize * 0.85f;
        s_LineHeight = s_BakedFontSize * 1.25f;
    }

    s_IsInitialized = true;
    LOG(Info, LogLevel::Core, "M_Font::Init -> Successfully baked font atlas ({}x{}) from [{}] with TextureID={}",
        ATLAS_SIZE, ATLAS_SIZE, loadedPath, s_FontTextureID);
    return true;
}

void M_Font::Shutdown() {
    if (s_BakedCharData) {
        delete[] static_cast<stbtt_bakedchar *>(s_BakedCharData);
        s_BakedCharData = nullptr;
    }
    if (s_FontTextureID != 0) {
        M_Texture::Unload(s_FontTextureID);
        s_FontTextureID = 0;
    }
    s_IsInitialized = false;
    LOG(Info, LogLevel::Core, "M_Font::Shutdown completed.");
}

Vector2D M_Font::Measure_String(StringView text, FLOAT scale) {
    Ensure_Initialized();
    if (text.empty() || !s_BakedCharData) return { 0.0f, 0.0f };

    stbtt_bakedchar *cdata = static_cast<stbtt_bakedchar *>(s_BakedCharData);
    FLOAT curX = 0.0f;
    FLOAT maxX = 0.0f;
    FLOAT curY = s_LineHeight * scale;

    for (char c : text) {
        if (c == '\n') {
            if (curX > maxX) maxX = curX;
            curX = 0.0f;
            curY += s_LineHeight * scale;
            continue;
        }
        if (c < FIRST_CHAR || c >= FIRST_CHAR + NUM_CHARS) continue;

        const stbtt_bakedchar &b = cdata[c - FIRST_CHAR];
        curX += b.xadvance * scale;
    }
    if (curX > maxX) maxX = curX;
    return { maxX, curY };
}

void M_Font::Draw_String(
    StringView     text,
    FLOAT          x,
    FLOAT          y,
    const COLOR   &color,
    FLOAT          scale)
{
    Ensure_Initialized();
    if (text.empty() || !s_BakedCharData || s_FontTextureID == 0) return;

    stbtt_bakedchar *cdata = static_cast<stbtt_bakedchar *>(s_BakedCharData);

    FLOAT curX = x;
    FLOAT curY = y + s_Ascent * scale;

    static thread_local std::vector<UIQuad> s_QuadBatch;
    s_QuadBatch.clear();
    s_QuadBatch.reserve(text.size());

    constexpr FLOAT invAtlas = 1.0f / static_cast<FLOAT>(ATLAS_SIZE);

    for (char c : text) {
        if (c == '\n') {
            curX = x;
            curY += s_LineHeight * scale;
            continue;
        }
        if (c < FIRST_CHAR || c >= FIRST_CHAR + NUM_CHARS) continue;

        const stbtt_bakedchar &b = cdata[c - FIRST_CHAR];
        FLOAT gw = static_cast<FLOAT>(b.x1 - b.x0);
        FLOAT gh = static_cast<FLOAT>(b.y1 - b.y0);

        if (gw > 0.0f && gh > 0.0f) {
            FLOAT px0 = curX + b.xoff * scale;
            FLOAT py0 = curY + b.yoff * scale;
            FLOAT px1 = px0 + gw * scale;
            FLOAT py1 = py0 + gh * scale;

            UIQuad q;
            q.Min   = { px0, py0 };
            q.Max   = { px1, py1 };
            q.UVMin = { b.x0 * invAtlas, b.y0 * invAtlas };
            q.UVMax = { b.x1 * invAtlas, b.y1 * invAtlas };
            q.Color = color;
            s_QuadBatch.push_back(q);
        }

        curX += b.xadvance * scale;
    }

    if (!s_QuadBatch.empty()) {
        RHI_Manager::Draw_UI_Quads(s_FontTextureID, s_QuadBatch.data(), static_cast<UINT>(s_QuadBatch.size()));
    }
}

void M_Font::Draw_String_Shadowed(
    StringView     text,
    FLOAT          x,
    FLOAT          y,
    const COLOR   &color,
    const COLOR   &shadowColor,
    FLOAT          scale)
{
    FLOAT offset = 1.0f * (scale < 1.0f ? 1.0f : scale);
    Draw_String(text, x + offset, y + offset, shadowColor, scale);
    Draw_String(text, x, y, color, scale);
}
