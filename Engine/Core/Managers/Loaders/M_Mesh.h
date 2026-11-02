#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Math.h>
#include <M_Log.h>
#include <M_File.h>
#include <M_Threads.h>

// ============================================================================
// 3D / 2D Mesh Vertex & Mesh Data
// ============================================================================
struct Vertex {
    Vector3D Position = { 0.0f, 0.0f, 0.0f }; // 3D local position (X, Y, Z)
    Vector3D Normal   = { 0.0f, 1.0f, 0.0f }; // Surface direction for lighting
    Vector2D UV       = { 0.0f, 0.0f };       // Texture coordinates (0.0 to 1.0)
    COLOR    Color    = COLOR_WHITE;          // Vertex tint color (R, G, B, A)
};

struct MeshData {
    String        FilePath;
    Array<Vertex> Vertices;
    Array<UINT>   Indices;
    UINT          MeshID         = 0;    // Unique Mesh ID for automatic GPU vertex/index buffer caching
    UINT          Revision       = 1;    // Incremented by Mark_Dirty() when vertices change dynamically
    FLOAT         BoundingRadius = 1.0f; // Outer bounding sphere radius for frustum culling
    AABB          LocalBounds;           // 3D local-space Axis-Aligned Bounding Box

    void Mark_Dirty() { ++Revision; }

    [[nodiscard]] UINT Get_VertexCount()   const { return static_cast<UINT>(Vertices.size()); }
    [[nodiscard]] UINT Get_IndexCount()    const { return static_cast<UINT>(Indices.size()); }
    [[nodiscard]] UINT Get_TriangleCount() const { return static_cast<UINT>(Indices.size() / 3); }
    [[nodiscard]] BOOL Is_Valid()          const { return !Vertices.empty() && !Indices.empty(); }
};

// Per-Draw GPU Constants (96 bytes: 4x4 MVP Matrix + RGBA Tint + Local Light)
struct DrawConstants {
    FLOAT MVP[16]  = {};
    FLOAT Tint[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    FLOAT Light[4] = { 0.45f, 0.65f, -0.60f, 0.0f };
};

// ============================================================================
// Built-In Primitive Mesh Generator (Triangle, Quad, Cube)
// ============================================================================
// Quick Usage Example:
//   MeshData tri  = M_Mesh::Create_Triangle(COLOR_RED, COLOR_GREEN, COLOR_BLUE);
//   MeshData quad = M_Mesh::Create_Quad(COLOR_GREEN);
//   MeshData cube = M_Mesh::Create_Cube(COLOR_CYAN);
// ============================================================================
class M_Mesh {
private:
    // Cached per-frame camera basis & projection values (updated once per frame by RHI_Manager::Begin_Frame)
    static Vector3D s_CamPos;
    static Vector3D s_CamRight;
    static Vector3D s_CamUp;
    static Vector3D s_CamFwd;
    static FLOAT    s_FocalLen;
    static FLOAT    s_NearClip;
    static FLOAT    s_FarClip;
    static FLOAT    s_ProjQ;
    static FLOAT    s_ProjZNearQ;
    static UINT     s_NextMeshID;
    static Vector3D s_SunDirection;
    static FLOAT    s_AmbientIntensity;
    static std::mutex             s_CacheMutex;
    static uMap<String, MeshData> s_MeshCache;

    struct StreamedMeshJob {
        String                   Key;
        MeshData                 Mesh;
        Function<void(MeshData)> Callback;
    };

    static std::mutex             s_StreamMutex;
    static Array<StreamedMeshJob> s_CompletedStreamQueue;
    static std::atomic<UINT>      s_PendingStreamCount;
    static std::atomic<UINT>      s_CompletedStreamCount;

public:
    // Assigns a new unique MeshID to a MeshData instance for GPU buffer caching
    static UINT Next_MeshID();

    enum PrimitiveType : BYTE {
        Triangle = 0, // 3 Vertices, 2 Triangles (Double-sided flat triangle)
        Quad     = 1, // 4 Vertices, 4 Triangles (Double-sided 1x1 flat square / plane)
        Cube     = 2  // 24 Vertices, 12 Triangles (1x1x1 3D box with normals & UVs)
    };

    // Cached camera query accessors (zero recalculation across World & RHI)
    [[nodiscard]] static inline const Vector3D &Get_CameraPos()     { return s_CamPos; }
    [[nodiscard]] static inline const Vector3D &Get_CameraRight()   { return s_CamRight; }
    [[nodiscard]] static inline const Vector3D &Get_CameraUp()      { return s_CamUp; }
    [[nodiscard]] static inline const Vector3D &Get_CameraForward() { return s_CamFwd; }
    [[nodiscard]] static inline FLOAT Get_FocalLength()             { return s_FocalLen; }
    [[nodiscard]] static inline FLOAT Get_NearClip()                { return s_NearClip; }
    [[nodiscard]] static inline FLOAT Get_FarClip()                 { return s_FarClip; }
    [[nodiscard]] static inline FLOAT Get_ProjQ()                   { return s_ProjQ; }
    [[nodiscard]] static inline FLOAT Get_ProjZNearQ()              { return s_ProjZNearQ; }

    static inline void Set_SunDirection(const Vector3D &dir) {
        s_SunDirection = dir.Normalized();
    }
    [[nodiscard]] static inline const Vector3D &Get_SunDirection()   { return s_SunDirection; }

    static inline void Set_AmbientIntensity(FLOAT ambient)           { s_AmbientIntensity = ambient; }
    [[nodiscard]] static inline FLOAT Get_AmbientIntensity()         { return s_AmbientIntensity; }

    // Updates cached camera view vectors once per frame (called automatically by RHI_Manager::Begin_Frame)
    static void Set_CameraView(
        const Vector3D &pos,
        const Vector3D &right,
        const Vector3D &up,
        const Vector3D &forward,
        FLOAT           fovDegrees,
        FLOAT           nearClip,
        FLOAT           farClip);

    // Creates a 1x1 centered Triangle with a single solid color.
    // Example: MeshData tri = M_Mesh::Create_Triangle(COLOR_RED);
    static MeshData Create_Triangle(const COLOR &color = COLOR_WHITE);

    // Creates a 1x1 centered Triangle with custom colors on each corner (great for RGB testing!).
    // Double-sided (front + back indices) so it remains visible when flying behind it in 3D.
    // Example: MeshData rgbTri = M_Mesh::Create_Triangle(COLOR_RED, COLOR_GREEN, COLOR_BLUE);
    static MeshData Create_Triangle(const COLOR &topColor, const COLOR &rightColor, const COLOR &leftColor);

    // Creates a 1x1 centered Quad / Plane (4 vertices, double-sided so visible from both sides).
    // Example: MeshData quad = M_Mesh::Create_Quad(COLOR_GREEN);
    static MeshData Create_Quad(const COLOR &color = COLOR_WHITE);

    // Creates a 1x1x1 centered 3D Cube (24 vertices, 36 indices / 12 triangles).
    // Example: MeshData cube = M_Mesh::Create_Cube(COLOR_CYAN);
    static MeshData Create_Cube(const COLOR &color = COLOR_WHITE);

    // Creates a horizontal Ground Plane on the XZ plane with normals facing up (+Y).
    // Perfect for ground floors, terrain bases, and town squares!
    // Example: MeshData ground = M_Mesh::Create_Ground(100.0f, 100.0f, COLOR(50, 120, 50, 255), 20.0f);
    static MeshData Create_Ground(FLOAT width = 100.0f, FLOAT depth = 100.0f, const COLOR &color = COLOR(55, 120, 55, 255), FLOAT uvTiling = 10.0f);

    // Creates a primitive mesh by enum type (Triangle, Quad, or Cube).
    // Example: MeshData mesh = M_Mesh::Create(M_Mesh::Cube, COLOR_YELLOW);
    static MeshData Create(PrimitiveType type, const COLOR &color = COLOR_WHITE);

    // Loads a 3D Wavefront .OBJ model from disk (supports v, vt, vn, and triangulated/quad faces f).
    // Automatically caches the parsed MeshData by file path so loading the same .obj 100x is instant O(1)!
    // If the file is missing or invalid, logs a warning and falls back to a Cube so the game never crashes.
    static MeshData Load_OBJ(const FilePath &path, const COLOR &defaultColor = COLOR_WHITE);

    // Universal 3D Model Loader (.obj raw) with O(1) path caching
    // Example: MeshData mesh = M_Mesh::Load("Assets/Sword.obj");
    static MeshData Load(const FilePath &path, const COLOR &defaultColor = COLOR_WHITE);

    // Multi-Threaded Asynchronous 3D Model Streaming Queue using M_Threads!
    // Parses .meshae or .obj on a worker thread and dispatches piece-by-piece on Core::Tick() with zero lag!
    static void Stream(
        const FilePath &path,
        Function<void(MeshData)> onLoaded,
        const COLOR &defaultColor = COLOR_WHITE);

    // Asynchronous loader (routes through the unified time-sliced streaming queue)
    static inline void Stream_OBJ(
        const FilePath &path,
        Function<void(MeshData)> onLoaded,
        const COLOR &defaultColor = COLOR_WHITE)
    {
        Stream(path, std::move(onLoaded), defaultColor);
    }

    // Asynchronous loader (automatically routes through the time-sliced streaming queue)
    static inline void Load_OBJ_Async(
        const FilePath &path,
        Function<void(MeshData)> onLoaded,
        const COLOR &defaultColor = COLOR_WHITE)
    {
        Stream_OBJ(path, std::move(onLoaded), defaultColor);
    }

    // Processes completed mesh streaming jobs piece-by-piece on the main thread during Core::Tick().
    // Limits dispatching to `budgetPerTick` meshes per frame to eliminate frame-time stutters.
    static void Tick_Streaming(UINT budgetPerTick = 1);

    // Returns the number of meshes currently loading/parsing in the background
    [[nodiscard]] static inline UINT Get_PendingStreamCount() {
        return s_PendingStreamCount.load();
    }

    // Cleans up all mesh cache and pending streaming queues cleanly
    static void Shutdown();

    // Builds the 96-byte GPU constant block (Model-View-Projection matrix, RGBA tint, and 3D lighting)
    static DrawConstants Build_DrawConstants(
        const Vector3D &pos,
        const Vector3D &rotDeg,
        const Vector3D &size,
        const COLOR    &color,
        FLOAT           transparency,
        FLOAT           aspectRatio,
        BOOL            bFlipClipY,
        BOOL            bEnable3DShading);

    // Builds orthographic 2D screen-space constants for 2D UI elements (FBox, FImage, FImageBtn)
    static DrawConstants Build_UIConstants(
        const Vector2D &pos,
        const Vector2D &size,
        const COLOR    &color,
        FLOAT           transparency,
        BOOL            bFlipClipY);
};
