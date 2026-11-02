#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Math.h>
#include <C_Entity.h>

// ============================================================================
// 1. Ray & Hit Data Structures
// ============================================================================

// 3D Ray with origin, normalized direction arrow, and max reach length
struct Ray3D {
    Vector3D Origin    = { 0.0f, 0.0f, 0.0f };
    Vector3D Direction = { 0.0f, 0.0f, 1.0f }; // Unit vector (normalized)
    FLOAT    Length    = 1000.0f;

    constexpr Ray3D() = default;
    constexpr Ray3D(const Vector3D &origin, const Vector3D &direction, FLOAT length = 1000.0f)
        : Origin(origin), Direction(direction), Length(length) {}

    [[nodiscard]] inline Vector3D Get_Point(FLOAT distance) const {
        return Origin + Direction * distance;
    }
};

// Ray / Trace Hit Result (Unreal FHitResult style)
struct RayHit {
    BOOL             bHit      = false;
    Vector3D         Point     = { 0.0f, 0.0f, 0.0f }; // Exact 3D world impact coordinate
    Vector3D         Normal    = { 0.0f, 1.0f, 0.0f }; // Surface normal at impact point
    FLOAT            Distance  = 0.0f;                 // Distance from trace start to impact
    C_Entity        *Entity    = nullptr;              // Pointer to struck entity (or nullptr for terrain)
    INT              EntityID  = 0;
    HitChannel       Channel   = HitChannel::WorldStatic;
    EntityType       Type      = EntityType::None;     // Category of hit entity (Monster, NPC, Character, etc.)

    constexpr RayHit() = default;
};

// Optional Trace Settings Builder (Unreal 5 style)
struct TraceParams {
    TraceShape       Shape         = TraceShape::Line;
    TraceMode        Mode          = TraceMode::Single;
    TraceChannel     Channel       = TraceChannel::Visibility;
    FLOAT            Radius        = 0.5f;                 // For Sphere / Capsule
    FLOAT            HalfHeight    = 0.9f;                 // For Capsule
    Vector3D         BoxExtents    = { 0.5f, 0.5f, 0.5f }; // For Box
    const C_Entity  *IgnoreEntity  = nullptr;
    BOOL             bDebugDraw    = false;
    COLOR            DebugColor    = COLOR_RED;
    FLOAT            DebugDuration = 0.0f;
};

// ============================================================================
// 3. U_Raycast (Pure Geometric Math, Direction Vectors & Projection Library)
// ============================================================================
class U_Raycast {
public:
    // ------------------------------------------------------------------------
    // A. Unreal-Style Rotation Vectors (Pure Math - No Camera/Entity required)
    // ------------------------------------------------------------------------

    // Returns normalized forward direction vector from Pitch, Yaw, Roll degrees
    [[nodiscard]] static Vector3D Get_ForwardVector(const Vector3D &rotationDeg);

    // Returns normalized right direction vector from Pitch, Yaw, Roll degrees
    [[nodiscard]] static Vector3D Get_RightVector(const Vector3D &rotationDeg);

    // Returns normalized up direction vector from Pitch, Yaw, Roll degrees
    [[nodiscard]] static Vector3D Get_UpVector(const Vector3D &rotationDeg);

    // ------------------------------------------------------------------------
    // B. Unreal-Style Screen & World Projection (Beginner-Friendly Names)
    // ------------------------------------------------------------------------

    // Screen_To_World: Unprojects 2D screen pixels (mouse) to a 3D world ray
    [[nodiscard]] static Ray3D Screen_To_World(
        const Vector2D &screenPos,
        const Vector3D &camPos,
        const Vector3D &camRotDeg,
        FLOAT           fovDegrees,
        FLOAT           screenWidth  = 0.0f,
        FLOAT           screenHeight = 0.0f);

    // World_To_Screen: Projects a 3D world coordinate to 2D screen pixels (floating nameplates)
    static BOOL World_To_Screen(
        const Vector3D &worldPos,
        const Vector3D &camPos,
        const Vector3D &camRotDeg,
        FLOAT           fovDegrees,
        Vector2D       &outScreenPos,
        FLOAT           screenWidth  = 0.0f,
        FLOAT           screenHeight = 0.0f);

    // ------------------------------------------------------------------------
    // C. Pure Geometric Intersections (Zero Entity/World Dependency)
    // ------------------------------------------------------------------------

    // Tests ray against an Axis-Aligned Bounding Box (AABB Slab method)
    static BOOL Intersects_Ray_AABB(
        const Ray3D    &ray,
        const AABB     &aabb,
        FLOAT          &outDistance,
        Vector3D       &outNormal);

    // Tests ray against a 3D sphere
    static BOOL Intersects_Ray_Sphere(
        const Ray3D    &ray,
        const Vector3D &center,
        FLOAT           radius,
        FLOAT          &outDistance,
        Vector3D       &outNormal);

    // Tests ray against a horizontal ground plane at groundY
    static BOOL Intersects_Ray_Ground(
        const Ray3D    &ray,
        FLOAT           groundY,
        Vector3D       &outGroundPoint);

    // Tests ray against an arbitrary 3D plane (normal + distance from origin)
    static BOOL Intersects_Ray_Plane(
        const Ray3D    &ray,
        const Vector3D &planeNormal,
        FLOAT           planeDist,
        FLOAT          &outDistance,
        Vector3D       &outPoint);

    // Möller–Trumbore algorithm: tests ray against a 3D polygon triangle
    static BOOL Intersects_Ray_Triangle(
        const Ray3D    &ray,
        const Vector3D &v0,
        const Vector3D &v1,
        const Vector3D &v2,
        FLOAT          &outDistance,
        Vector2D       &outUV);

    // Tests if a moving sphere collides with an AABB
    static BOOL Intersects_Sphere_AABB(
        const Vector3D &center,
        FLOAT           radius,
        const AABB     &aabb,
        Vector3D       &outClosestPoint);

    // Tests if a vertical capsule (character shape) collides with an AABB
    static BOOL Intersects_Capsule_AABB(
        const Vector3D &capsuleBase,
        FLOAT           radius,
        FLOAT           halfHeight,
        const AABB     &aabb,
        Vector3D       &outClosestPoint);

    // ------------------------------------------------------------------------
    // D. Visual Debug Drawing (Unreal DrawDebugLine Style)
    // ------------------------------------------------------------------------

    // Draws a visible 3D line segment that stays in the world for duration seconds
    static void Draw_DebugLine(
        const Vector3D &start,
        const Vector3D &end,
        const COLOR    &color    = COLOR_RED,
        FLOAT           duration = 0.0f);

    // Draws a visible ray from an origin point along a direction
    static void Draw_DebugRay(
        const Vector3D &origin,
        const Vector3D &direction,
        FLOAT           length   = 10.0f,
        const COLOR    &color    = COLOR_RED,
        FLOAT           duration = 0.0f);

    // Draws a visible wireframe 3D bounding box
    static void Draw_DebugBox(
        const AABB  &box,
        const COLOR &color    = COLOR_GREEN,
        FLOAT        duration = 0.0f);

    // Draws a visible 3D point dot
    static void Draw_DebugPoint(
        const Vector3D &point,
        const COLOR    &color    = COLOR_YELLOW,
        FLOAT           duration = 0.0f);

    // Clears all currently active debug lines immediately
    static void Clear_Debug();

    // Steps debug line expiration timers
    static void Tick_Debug(FLOAT deltaTime);

    // Renders active debug lines to the 3D scene
    static void Render_Debug();
};
