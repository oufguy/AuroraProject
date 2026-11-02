#pragma once

#include <pch.h>

// ============================================================================
// Primitive Number Types (Clean, Explicit Sizing)
// ============================================================================

// Boolean (true / false). Example: BOOL bIsFlying = true;
using BOOL = bool;

// 1-Byte Unsigned (0 to 255) - Raw packet/file bytes, colors, level.
// Use INT8 for signed 1-byte values (-128 to +127).
using BYTE = uint8_t;
// 2-Byte Signed (-32,768 to +32,767) - Small stats, elemental resistances, grid coords.
// Use UINT16 for unsigned 2-byte values (0 to 65,535) like packet opcodes or Map IDs.
using SHORT = int16_t;

// 4-Byte Signed (-2.1B to +2.1B) - Default for HP, MP, damage, item counts, loops.
// Use DWORD for unsigned 4-byte values (0 to 4.2B) like bitflags.
using INT = int32_t;
// 4-Byte (0 to 4.2B). Example: DWORD bitFlags = 0xFF00;
using DWORD = uint32_t;
using UINT  = uint32_t;
using UINT32 = uint32_t;
// 8-Byte Signed (up to 9.2 Quintillion) - Player Gold/SPI, Total EXP, database Account IDs.
// Use UINT64 for unsigned 8-byte values (0 to 18.4 Quintillion) like file hashes.
using INT64 = int64_t;

// 4-Byte Decimal (~7 digits of precision) - World positions, movement speed, delta time.
// Use DOUBLE when you need high-precision timers that never lose accuracy over days.
using FLOAT = float;
// 8-Byte Decimal (~15-17 digits of precision) - High-precision server clock, long-running timers.
// Use FLOAT for normal 3D math, positions, and rendering.
using DOUBLE = double;

// 1 byte (-128 to 127).
// Use BYTE for unsigned 1-byte values (0 to 255) like raw packet bytes or colors.
using INT8 = int8_t;
// 2 bytes (0 to 65,535). Example: UINT16 packetOpcode = 0x1001;
// Use SHORT for signed 2-byte values (-32,768 to +32,767) like small stats or grid coords.
using UINT16 = uint16_t;
// 8-Byte (0 to 18 Quintillion). Example: UINT64 fileHash = 0;
// Use INT64 for signed 8-byte values (-9 Quintillion to +9 Quintillion) like player gold or total EXP.
using UINT64 = uint64_t;

// ============================================================================
// Text, Characters & Strings
// ============================================================================

// Standard 1-byte character. Example: Char inputKey = 'W'; // Keyboard key, delimiter
using Char = char;
// Wide 2-byte Unicode character. Example: WChar asianGlyph = L'仙'; // Asian fantasy character glyph
using WChar = wchar_t;

// Dynamic text string. Example: String playerName = "Li Wei"; // Character name, chat message
using String = std::string;
// Wide Unicode text string. Example: WString title = L"诛仙在线"; // Localized Asian fantasy text
using WString = std::wstring;
// Fast read-only string view (zero memory allocation). Example: void SendChat(StringView msg); // Function parameter
using StringView = std::string_view;
// Fast read-only wide string view (zero memory allocation). Example: void SetTitle(WStringView title);
using WStringView = std::wstring_view;

// Standard filesystem path. Example: FilePath cfgPath = "Settings.ini";
using FilePath = std::filesystem::path;

// Formats text with placeholders. Example: String msg = StringFormat("Player {} spawned at {}", name, pos);
template<typename... Args>
inline String StringFormat(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

// ============================================================================
// Vectors & Colors
// ============================================================================

// 2D Vector (x, y). Example: Vector2D uvCoord = { 0.5f, 0.5f }; // Screen UI coords, texture UV
struct Vector2D {
    FLOAT x = 0.0f;
    FLOAT y = 0.0f;

    constexpr Vector2D operator+(const Vector2D &o) const { return { x + o.x, y + o.y }; }
    constexpr Vector2D operator-(const Vector2D &o) const { return { x - o.x, y - o.y }; }
    constexpr Vector2D operator*(FLOAT s) const { return { x * s, y * s }; }
    constexpr Vector2D operator/(FLOAT s) const { return (s != 0.0f) ? Vector2D{ x / s, y / s } : Vector2D{}; }

    constexpr Vector2D &operator+=(const Vector2D &o) { x += o.x; y += o.y; return *this; }
    constexpr Vector2D &operator-=(const Vector2D &o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vector2D &operator*=(FLOAT s) { x *= s; y *= s; return *this; }
};
// 3D Vector (x, y, z). Example: Vector3D playerPos = { 100.0f, 20.0f, -50.0f }; // World position, rotation, scale
struct Vector3D {
    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    FLOAT z = 0.0f;

    constexpr Vector3D operator+(const Vector3D &o) const { return { x + o.x, y + o.y, z + o.z }; }
    constexpr Vector3D operator-(const Vector3D &o) const { return { x - o.x, y - o.y, z - o.z }; }
    constexpr Vector3D operator*(FLOAT s) const { return { x * s, y * s, z * s }; }
    constexpr Vector3D operator/(FLOAT s) const { return (s != 0.0f) ? Vector3D{ x / s, y / s, z / s } : Vector3D{}; }

    constexpr Vector3D &operator+=(const Vector3D &o) { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr Vector3D &operator-=(const Vector3D &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr Vector3D &operator*=(FLOAT s) { x *= s; y *= s; z *= s; return *this; }

    [[nodiscard]] constexpr FLOAT LengthSq() const { return x * x + y * y + z * z; }
    [[nodiscard]] inline FLOAT Length() const { return std::sqrt(x * x + y * y + z * z); }
    [[nodiscard]] inline Vector3D Normalized() const {
        FLOAT lsq = LengthSq();
        if (lsq > 0.000001f) {
            FLOAT inv = 1.0f / std::sqrt(lsq);
            return { x * inv, y * inv, z * inv };
        }
        return { 0.0f, 0.0f, 0.0f };
    }
    [[nodiscard]] constexpr FLOAT Dot(const Vector3D &o) const { return x * o.x + y * o.y + z * o.z; }
};
// 4D Vector (x, y, z, w). Example: Vector4D tintColor = { 1.0f, 0.8f, 0.2f, 1.0f }; // RGBA colors, quaternions
struct Vector4D {
    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    FLOAT z = 0.0f;
    FLOAT w = 0.0f;
};

// ============================================================================
// 3D Axis-Aligned Bounding Box (AABB)
// ============================================================================
// Used for:
//   1. 3D Collision Detection (Player vs Buildings / Obstacles / Terrain)
//   2. 3D Mouse Ray Picking (Clicking on Characters, NPCs, Buildings)
//   3. Frustum & Occlusion Culling
// ============================================================================
struct AABB {
    Vector3D Min = {  1e9f,  1e9f,  1e9f };
    Vector3D Max = { -1e9f, -1e9f, -1e9f };

    constexpr AABB() = default;
    constexpr AABB(const Vector3D &min, const Vector3D &max) : Min(min), Max(max) {}

    // Resets box to inverted infinity for point accumulation
    constexpr void Reset() {
        Min = {  1e9f,  1e9f,  1e9f };
        Max = { -1e9f, -1e9f, -1e9f };
    }

    [[nodiscard]] constexpr BOOL Is_Valid() const {
        return Min.x <= Max.x && Min.y <= Max.y && Min.z <= Max.z;
    }

    // Expands the bounding box to enclose a 3D point
    constexpr void Encompass(const Vector3D &pt) {
        if (pt.x < Min.x) Min.x = pt.x;
        if (pt.y < Min.y) Min.y = pt.y;
        if (pt.z < Min.z) Min.z = pt.z;
        if (pt.x > Max.x) Max.x = pt.x;
        if (pt.y > Max.y) Max.y = pt.y;
        if (pt.z > Max.z) Max.z = pt.z;
    }

    // Expands the bounding box to enclose another bounding box
    constexpr void Encompass(const AABB &other) {
        if (!other.Is_Valid()) return;
        if (other.Min.x < Min.x) Min.x = other.Min.x;
        if (other.Min.y < Min.y) Min.y = other.Min.y;
        if (other.Min.z < Min.z) Min.z = other.Min.z;
        if (other.Max.x > Max.x) Max.x = other.Max.x;
        if (other.Max.y > Max.y) Max.y = other.Max.y;
        if (other.Max.z > Max.z) Max.z = other.Max.z;
    }

    [[nodiscard]] constexpr Vector3D Get_Center() const {
        return { (Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f, (Min.z + Max.z) * 0.5f };
    }

    [[nodiscard]] constexpr Vector3D Get_Extents() const {
        return { (Max.x - Min.x) * 0.5f, (Max.y - Min.y) * 0.5f, (Max.z - Min.z) * 0.5f };
    }

    [[nodiscard]] constexpr Vector3D Get_Size() const {
        return { Max.x - Min.x, Max.y - Min.y, Max.z - Min.z };
    }

    // Checks whether this AABB intersects another AABB
    [[nodiscard]] constexpr BOOL Intersects(const AABB &other) const {
        return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
               (Min.y <= other.Max.y && Max.y >= other.Min.y) &&
               (Min.z <= other.Max.z && Max.z >= other.Min.z);
    }

    // Checks whether a 3D point is inside this AABB
    [[nodiscard]] constexpr BOOL Contains(const Vector3D &pt) const {
        return (pt.x >= Min.x && pt.x <= Max.x) &&
               (pt.y >= Min.y && pt.y <= Max.y) &&
               (pt.z >= Min.z && pt.z <= Max.z);
    }
};

// RGBA Color (0 to 255). Works with { R, G, B } or HEX (0xRRGGBB)!
// Example: Color red = { 255, 0, 0 };  or  Color green = 0x2ECC71;
struct COLOR {
    BYTE r = 255, g = 255, b = 255, a = 255;
    constexpr COLOR() = default;
    constexpr COLOR(BYTE R, BYTE G, BYTE B, BYTE A = 255) : r(R), g(G), b(B), a(A) {}
    constexpr COLOR(DWORD hex, BYTE A = 255) : r(BYTE(hex >> 16)), g(BYTE(hex >> 8)), b(BYTE(hex)), a(A) {}
    // Allows Color from Vector3D: COLOR(Vector3D(255, 0, 0)) or normalized COLOR(Vector3D(1.0f, 0.0f, 0.0f))
    constexpr COLOR(const Vector3D &v, BYTE A = 255)
        : r(static_cast<BYTE>(v.x > 1.0f ? v.x : (v.x < 0.0f ? 0.0f : v.x * 255.0f))),
          g(static_cast<BYTE>(v.y > 1.0f ? v.y : (v.y < 0.0f ? 0.0f : v.y * 255.0f))),
          b(static_cast<BYTE>(v.z > 1.0f ? v.z : (v.z < 0.0f ? 0.0f : v.z * 255.0f))),
          a(A) {}
    // Allows COLOR("#2ECC71") so editor Color Picker UIs can click & edit it!
    constexpr COLOR(StringView hex, BYTE A = 255) : a(A) {
        auto Hex = [](Char c) constexpr -> BYTE {
            return (c >= '0' && c <= '9') ? BYTE(c - '0') :
                (c >= 'a' && c <= 'f') ? BYTE(c - 'a' + 10) :
                (c >= 'A' && c <= 'F') ? BYTE(c - 'A' + 10) : 0;
        };
        if (!hex.empty() && hex[0] == '#') hex.remove_prefix(1);
        if (hex.size() >= 6) {
            r = BYTE((Hex(hex[0]) << 4) | Hex(hex[1]));
            g = BYTE((Hex(hex[2]) << 4) | Hex(hex[3]));
            b = BYTE((Hex(hex[4]) << 4) | Hex(hex[5]));
        }
    }
};

// 2D Screen-space quad for GPU UI batching (font glyphs, sprites, HUD panels)
struct UIQuad {
    Vector2D Min;   // Screen pixel top-left (x0, y0)
    Vector2D Max;   // Screen pixel bottom-right (x1, y1)
    Vector2D UVMin; // Texture UV top-left (u0, v0)
    Vector2D UVMax; // Texture UV bottom-right (u1, v1)
    COLOR    Color = { 255, 255, 255, 255 };
};

// ============================================================================
// C++20 std::formatter Specializations for Vectors
// Enables direct logging with {}: PRINT("Position: {}", myVector3D);
// ============================================================================

template<>
struct std::formatter<Vector2D> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const Vector2D &v, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({:.2f}, {:.2f})", v.x, v.y);
    }
};
template<>
struct std::formatter<Vector3D> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const Vector3D &v, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({:.2f}, {:.2f}, {:.2f})", v.x, v.y, v.z);
    }
};
template<>
struct std::formatter<Vector4D> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const Vector4D &v, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", v.x, v.y, v.z, v.w);
    }
};
template<>
struct std::formatter<AABB> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const AABB &b, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "[Min: ({:.2f}, {:.2f}, {:.2f}), Max: ({:.2f}, {:.2f}, {:.2f})]",
            b.Min.x, b.Min.y, b.Min.z, b.Max.x, b.Max.y, b.Max.z);
    }
};

// ============================================================================
// Game Utilities
// ============================================================================

// Dynamic heap array (std::vector). Example: Array<UINT> learnedSkillIds; // Resizable list of skills, monsters
template<typename T>
using Array = std::vector<T>;
// Fixed-size stack array (std::array). Example: fArray<FLOAT, 4> quat; // Bone rotations, fixed 4-element buffers
template<typename T, size_t N>
using fArray = std::array<T, N>;

// Sorted map by key (std::map, O(log N)). Example: Map<UINT, String> itemNames; // Leaderboards, sorted data
template<typename Key, typename Value>
using Map = std::map<Key, Value>;
// Fast hash map (std::unordered_map, O(1)). Example: uMap<UINT64, PlayerData> activePlayers; // Fast ID lookup
template<typename Key, typename Value>
using uMap = std::unordered_map<Key, Value>;

// Sorted unique set (std::set). Example: Set<UINT> completedQuestIds; // Unique, ordered IDs
template<typename T>
using Set = std::set<T>;
// Fast hash set (std::unordered_set). Example: uSet<UINT64> onlineAccountIds; // Fast membership check
template<typename T>
using uSet = std::unordered_set<T>;

// Shared reference-counting smart pointer. Example: SharedPtr<StaticObject> swordMesh; // Shared game asset
template<typename T>
using SharedPtr = std::shared_ptr<T>;
// Unique single-owner smart pointer. Example: UniquePtr<Camera> mainCamera; // Single exclusive owner
template<typename T>
using UniquePtr = std::unique_ptr<T>;
// Weak non-owning reference to SharedPtr. Example: WeakPtr<StaticObject> currentTarget; // Selected target without keeping alive
template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Creates a new SharedPtr instance. Example: auto sword = MakeShared<StaticObject>("FlyingSword");
template<typename T, typename... Args>
inline SharedPtr<T> MakeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}
// Creates a new UniquePtr instance. Example: auto cam = MakeUnique<Camera>();
template<typename T, typename... Args>
inline UniquePtr<T> MakeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}


// Callable function wrapper (std::function). Example: Function<void()> onClick;
template<typename T>
using Function = std::function<T>;

// Generic task callback. Example: Task myTask = []() { ... };
using Task = std::function<void()>;
