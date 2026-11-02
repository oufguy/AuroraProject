#pragma once

#include <C_Entity.h>

// ============================================================================
// C_Camera (3D Viewport Entity — Inherits from C_Entity)
// ============================================================================
// Represents the player viewpoint in 3D world space.
// - Inherits Position, Rotation, Move(), Move_Forward() directly from C_Entity.
// - Has NO Mesh or Texture (0 bytes of visual geometry bloat!).
// - Automatically updates the active view & projection matrices for RHI.
// ============================================================================
class C_Camera : public C_Entity {
private:
    static C_Camera *s_ActiveCamera;

    // Cached camera view basis & dirty tracking (Eliminates all per-frame trig calls when stationary)
    mutable Vector3D m_CachedFwd   = { 0.0f, 0.0f, 1.0f };
    mutable Vector3D m_CachedRight = { 1.0f, 0.0f, 0.0f };
    mutable Vector3D m_CachedUp    = { 0.0f, 1.0f, 0.0f };
    mutable Vector3D m_LastRot     = { 999.0f, 999.0f, 999.0f };
    mutable Vector3D m_LastPos     = { 999.0f, 999.0f, 999.0f };
    mutable FLOAT    m_LastFOV     = 0.0f;
    mutable FLOAT    m_LastNear    = 0.0f;
    mutable FLOAT    m_LastFar     = 0.0f;

    void UpdateBasis() const;

public:
    FLOAT FOV       = 60.0f;  // Vertical Field of View in degrees
    FLOAT NearClip  = 0.1f;   // Closest visible Z-buffer distance
    FLOAT FarClip   = 500.0f; // Farthest visible Z-buffer distance
    FLOAT MoveSpeed = 5.0f;   // Default camera movement speed

    [[nodiscard]] inline Vector3D Get_Forward() const { UpdateBasis(); return m_CachedFwd; }
    [[nodiscard]] inline Vector3D Get_Right()   const { UpdateBasis(); return m_CachedRight; }
    [[nodiscard]] inline Vector3D Get_Up()      const { UpdateBasis(); return m_CachedUp; }

    C_Camera();
    ~C_Camera() override;

    // Makes this C_Camera the active viewpoint used by RHI_Manager
    void Set_Active();

    // Returns the currently active C_Camera (or a built-in fallback if none exists yet)
    static C_Camera &Get_Active();

    // Sets the vertical Field of View in degrees (clamped between 20 and 140)
    void Set_FOV(FLOAT fovDegrees);

    // Sets the Near and Far Z-buffer clip planes
    void Set_ClipPlanes(FLOAT nearZ, FLOAT farZ);

    // Pushes this camera's 3D view & projection parameters to the renderer for the frame
    void Update_CameraView() const;

    // Rotates the Camera (clamps vertical pitch to [-89, +89] degrees to prevent gimbal flip)
    void Add_Rotation(FLOAT deltaPitch, FLOAT deltaYaw, FLOAT deltaRoll = 0.0f);

    // Sets the Camera's rotation in degrees (clamps vertical pitch to [-89, +89] degrees)
    void Set_Rotation(const Vector3D &rotDeg);

    // 3rd-Person Chase Camera Follow Mode (MMORPG / WoW / Lineage 2 Style)
    // Follows targetPos at distance meters behind, looking at height meters above target
    void Follow(const Vector3D &targetPos, FLOAT distance = 5.0f, FLOAT height = 1.5f);

    // Resets the camera to its default starting position and rotation
    void Reset();

    // ------------------------------------------------------------------------
    // 3D <-> 2D Coordinate Projection & Ray Math (Nameplates, Floating UI & Mouse Picking)
    // ------------------------------------------------------------------------

    // Projects a 3D world position into 2D screen pixel coordinates (0, 0 = top-left).
    // Returns TRUE if the point is in front of the camera, FALSE if behind.
    BOOL World_To_Screen(
        const Vector3D &worldPos,
        Vector2D       &outScreenPos,
        FLOAT           screenWidth  = 0.0f,
        FLOAT           screenHeight = 0.0f) const;

    // Unprojects a 2D screen pixel coordinate (e.g. mouse cursor) into a 3D world ray.
    // outRayOrigin is camera world position, outRayDir is normalized direction vector.
    BOOL Screen_To_WorldRay(
        const Vector2D &screenPos,
        Vector3D       &outRayOrigin,
        Vector3D       &outRayDir,
        FLOAT           screenWidth  = 0.0f,
        FLOAT           screenHeight = 0.0f) const;
};

// Legacy compatibility alias
using E_Camera = C_Camera;
