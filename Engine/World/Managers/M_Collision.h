#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Raycast.h>
#include <C_Entity.h>
#include <C_StaticMesh.h>

// ============================================================================
// TraceRequest: Batched & Async Query Request (Multithreaded Collision Queries)
// ============================================================================
struct TraceRequest {
    TraceShape       Shape         = TraceShape::Line;
    Vector3D         Start         = { 0.0f, 0.0f, 0.0f };
    Vector3D         End           = { 0.0f, 0.0f, 0.0f };
    TraceChannel     Channel       = TraceChannel::Visibility;
    FLOAT            ParamA        = 0.5f; // Radius (Sphere/Capsule) or BoxExtent.X
    FLOAT            ParamB        = 0.9f; // HalfHeight (Capsule) or BoxExtent.Y
    FLOAT            ParamC        = 0.5f; // BoxExtent.Z
    const C_Entity  *IgnoreEntity  = nullptr;
    BOOL             bDebugDraw    = false;
    COLOR            DebugColor    = COLOR_RED;
    FLOAT            DebugDuration = 0.0f;
};

// ============================================================================
// M_Collision (World Collision & Tracing Manager — Unreal Engine Style)
// ============================================================================
// Features:
//   - Supports 4 3D Shapes: Line, Sphere, Box, Capsule (Character Shape!)
//   - Collision Channels: Visibility, Camera, WorldStatic, WorldDynamic, Pawn, Monster, Projectile
//   - Unreal Channel Architecture: TraceChannel vs HitChannel
//   - Single vs Multi Trace modes (Stops at 1st hit or collects all pierced entities)
//   - Visual Debug Drawing with duration timers & Global F1 toggle
//   - Multithreaded Batch & Async Tracing via M_Threads worker pool
//   - Global 1-liner functions: Line_Trace, Ray_Trace, Capsule_Trace, Sphere_Trace, Box_Trace, Multi_Trace, Has_LineOfSight, Ground_Trace
// ============================================================================
class M_Collision {
private:
    static inline BOOL s_GlobalDebugDrawing = false; // Toggled via F1 hotkey!

public:
    // Global Visual Debug Drawing Mode (Toggled via F1 hotkey)
    static inline void Toggle_DebugDrawing() { s_GlobalDebugDrawing = !s_GlobalDebugDrawing; }
    static inline void Set_DebugDrawing(BOOL bEnable) { s_GlobalDebugDrawing = bEnable; }
    [[nodiscard]] static inline BOOL Is_DebugDrawing() { return s_GlobalDebugDrawing; }

    // Multi-threaded Batch Tracing: Splits 500+ requests across all CPU worker threads simultaneously!
    static void Batch_Trace(
        const Array<TraceRequest> &requests,
        Array<RayHit>             &outResults);

    // Asynchronous Tracing: Queues a trace to run on a background worker thread with a callback
    static void Async_Trace(
        const TraceRequest                  &request,
        const Function<void(const RayHit&)> &onComplete);

    // ========================================================================
    // 1. Core Master Tracing Engines (TraceChannel & HitChannel Overloads)
    // ========================================================================

    // TraceByChannel (Single Hit): Primary trace query by TraceChannel
    static BOOL TraceByChannel(
        TraceShape       shape,
        TraceMode        mode,
        const Vector3D  &start,
        const Vector3D  &end,
        TraceChannel     channel,
        RayHit          &outHit,
        FLOAT            paramA        = 0.5f, // Radius (Sphere/Capsule) or BoxExtent.X
        FLOAT            paramB        = 0.9f, // HalfHeight (Capsule) or BoxExtent.Y
        FLOAT            paramC        = 0.5f, // BoxExtent.Z
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    // TraceByHitChannel (Single Hit): Filter directly by entity's HitChannel (Unreal Object Channel)
    static BOOL TraceByHitChannel(
        TraceShape       shape,
        TraceMode        mode,
        const Vector3D  &start,
        const Vector3D  &end,
        HitChannel       hitChannel,
        RayHit          &outHit,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    // Overload for HitChannel directly in TraceByChannel
    static inline BOOL TraceByChannel(
        TraceShape       shape,
        TraceMode        mode,
        const Vector3D  &start,
        const Vector3D  &end,
        HitChannel       channel,
        RayHit          &outHit,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel(shape, mode, start, end, channel, outHit, paramA, paramB, paramC, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // TraceByChannel_Multi (Multi Hit): Penetrates and collects all pierced entities
    static UINT TraceByChannel_Multi(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        TraceChannel     channel,
        Array<RayHit>   &outHits,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    static UINT TraceByHitChannel_Multi(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        HitChannel       hitChannel,
        Array<RayHit>   &outHits,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    static inline UINT TraceByChannel_Multi(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        HitChannel       channel,
        Array<RayHit>   &outHits,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel_Multi(shape, start, end, channel, outHits, paramA, paramB, paramC, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // ========================================================================
    // 2. Beginner-Friendly 1-Liner Shortcuts (TraceChannel & HitChannel)
    // ========================================================================

    // Line_Trace: Shoots a straight line between two 3D points
    static inline BOOL Line_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        RayHit          &outHit,
        TraceChannel     channel       = TraceChannel::Visibility,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByChannel(TraceShape::Line, TraceMode::Single, start, end, channel, outHit, 0.0f, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Line_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        RayHit          &outHit,
        HitChannel       channel,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel(TraceShape::Line, TraceMode::Single, start, end, channel, outHit, 0.0f, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Aliases for Line_Trace
    static inline BOOL Trace_Line(const Vector3D &start, const Vector3D &end, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline BOOL Trace_Line(const Vector3D &start, const Vector3D &end, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline BOOL Raycast(const Vector3D &start, const Vector3D &end, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Ray_Trace: Shoots a ray along a direction vector for a maximum distance
    static inline BOOL Ray_Trace(
        const Vector3D  &origin,
        const Vector3D  &direction,
        FLOAT            maxDistance,
        RayHit          &outHit,
        TraceChannel     channel       = TraceChannel::Visibility,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        Vector3D end = origin + direction * maxDistance;
        return Line_Trace(origin, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Ray_Trace(
        const Vector3D  &origin,
        const Vector3D  &direction,
        FLOAT            maxDistance,
        RayHit          &outHit,
        HitChannel       channel,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        Vector3D end = origin + direction * maxDistance;
        return Line_Trace(origin, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Sphere_Trace: Swept sphere between two points (Melee swings, AoE blasts, Fireballs)
    static inline BOOL Sphere_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        RayHit          &outHit,
        TraceChannel     channel       = TraceChannel::Visibility,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByChannel(TraceShape::Sphere, TraceMode::Single, start, end, channel, outHit, radius, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Sphere_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        RayHit          &outHit,
        HitChannel       channel,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel(TraceShape::Sphere, TraceMode::Single, start, end, channel, outHit, radius, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Sphere(const Vector3D &start, const Vector3D &end, FLOAT radius, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline BOOL Trace_Sphere(const Vector3D &start, const Vector3D &end, FLOAT radius, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Capsule_Trace: Standard 3D Character Capsule sweep! (Ground walking, Wall sliding)
    static inline BOOL Capsule_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        FLOAT            halfHeight,
        RayHit          &outHit,
        TraceChannel     channel       = TraceChannel::WorldStatic,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByChannel(TraceShape::Capsule, TraceMode::Single, start, end, channel, outHit, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Capsule_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        FLOAT            halfHeight,
        RayHit          &outHit,
        HitChannel       channel,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel(TraceShape::Capsule, TraceMode::Single, start, end, channel, outHit, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Capsule(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, RayHit &outHit, TraceChannel channel = TraceChannel::WorldStatic, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline BOOL Trace_Capsule(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Box_Trace: Swept 3D Box between two points (Doors, Platforms, Rectangular barriers)
    static inline BOOL Box_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        const Vector3D  &extents,
        RayHit          &outHit,
        TraceChannel     channel       = TraceChannel::WorldStatic,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByChannel(TraceShape::Box, TraceMode::Single, start, end, channel, outHit, extents.x, extents.y, extents.z, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Box_Trace(
        const Vector3D  &start,
        const Vector3D  &end,
        const Vector3D  &extents,
        RayHit          &outHit,
        HitChannel       channel,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel(TraceShape::Box, TraceMode::Single, start, end, channel, outHit, extents.x, extents.y, extents.z, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Box(const Vector3D &start, const Vector3D &end, const Vector3D &extents, RayHit &outHit, TraceChannel channel = TraceChannel::WorldStatic, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline BOOL Trace_Box(const Vector3D &start, const Vector3D &end, const Vector3D &extents, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Multi_Trace: Collects ALL entities pierced by the shape (Piercing lasers, Shotgun spread)
    static inline UINT Multi_Trace(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        Array<RayHit>   &outHits,
        TraceChannel     channel       = TraceChannel::Visibility,
        FLOAT            radius        = 0.5f,
        FLOAT            halfHeight    = 0.9f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByChannel_Multi(shape, start, end, channel, outHits, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline UINT Multi_Trace(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        Array<RayHit>   &outHits,
        HitChannel       channel,
        FLOAT            radius        = 0.5f,
        FLOAT            halfHeight    = 0.9f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByHitChannel_Multi(shape, start, end, channel, outHits, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline UINT Trace_Multi(TraceShape shape, const Vector3D &start, const Vector3D &end, Array<RayHit> &outHits, TraceChannel channel = TraceChannel::Visibility, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
    static inline UINT Trace_Multi(TraceShape shape, const Vector3D &start, const Vector3D &end, Array<RayHit> &outHits, HitChannel channel, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    // Has_LineOfSight: Fast true/false check if Point A can see Point B without obstruction
    static inline BOOL Has_LineOfSight(
        const Vector3D  &start,
        const Vector3D  &end,
        TraceChannel     channel      = TraceChannel::Visibility,
        const C_Entity  *ignoreEntity = nullptr)
    {
        RayHit hit;
        return !Line_Trace(start, end, hit, channel, ignoreEntity, false);
    }

    static inline BOOL Has_LineOfSight(
        const Vector3D  &start,
        const Vector3D  &end,
        HitChannel       channel,
        const C_Entity  *ignoreEntity = nullptr)
    {
        RayHit hit;
        return !Line_Trace(start, end, hit, channel, ignoreEntity, false);
    }

    // Ground_Trace: Finds where a ray hits the horizontal ground plane
    static BOOL Ground_Trace(
        const Vector3D &origin,
        const Vector3D &direction,
        Vector3D       &outGroundHit,
        FLOAT           groundY = 0.0f);

    static inline BOOL Trace_Ground(const Vector3D &origin, const Vector3D &direction, Vector3D &outGroundHit, FLOAT groundY = 0.0f) {
        return Ground_Trace(origin, direction, outGroundHit, groundY);
    }

    // ========================================================================
    // 3. TraceByType (Filters directly by EntityType: Monster, NPC, Character, StaticMesh, ItemDrop)
    // ========================================================================
    static BOOL TraceByType(
        TraceShape       shape,
        TraceMode        mode,
        const Vector3D  &start,
        const Vector3D  &end,
        EntityType       entityType,
        RayHit          &outHit,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    static UINT TraceByType_Multi(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        EntityType       entityType,
        Array<RayHit>   &outHits,
        FLOAT            paramA        = 0.5f,
        FLOAT            paramB        = 0.9f,
        FLOAT            paramC        = 0.5f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f);

    // Ergonomic 1-Liners for TraceByType
    static inline BOOL Line_Trace_ByType(
        const Vector3D  &start,
        const Vector3D  &end,
        EntityType       entityType,
        RayHit          &outHit,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByType(TraceShape::Line, TraceMode::Single, start, end, entityType, outHit, 0.0f, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Line_ByType(const Vector3D &start, const Vector3D &end, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Line_Trace_ByType(start, end, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Ray_Trace_ByType(
        const Vector3D  &origin,
        const Vector3D  &direction,
        FLOAT            maxDistance,
        EntityType       entityType,
        RayHit          &outHit,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        Vector3D end = origin + direction * maxDistance;
        return Line_Trace_ByType(origin, end, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Sphere_Trace_ByType(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        EntityType       entityType,
        RayHit          &outHit,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByType(TraceShape::Sphere, TraceMode::Single, start, end, entityType, outHit, radius, 0.0f, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Sphere_ByType(const Vector3D &start, const Vector3D &end, FLOAT radius, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Sphere_Trace_ByType(start, end, radius, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Capsule_Trace_ByType(
        const Vector3D  &start,
        const Vector3D  &end,
        FLOAT            radius,
        FLOAT            halfHeight,
        EntityType       entityType,
        RayHit          &outHit,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByType(TraceShape::Capsule, TraceMode::Single, start, end, entityType, outHit, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Capsule_ByType(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Capsule_Trace_ByType(start, end, radius, halfHeight, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Box_Trace_ByType(
        const Vector3D  &start,
        const Vector3D  &end,
        const Vector3D  &extents,
        EntityType       entityType,
        RayHit          &outHit,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByType(TraceShape::Box, TraceMode::Single, start, end, entityType, outHit, extents.x, extents.y, extents.z, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline BOOL Trace_Box_ByType(const Vector3D &start, const Vector3D &end, const Vector3D &extents, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Box_Trace_ByType(start, end, extents, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline UINT Multi_Trace_ByType(
        TraceShape       shape,
        const Vector3D  &start,
        const Vector3D  &end,
        EntityType       entityType,
        Array<RayHit>   &outHits,
        FLOAT            radius        = 0.5f,
        FLOAT            halfHeight    = 0.9f,
        const C_Entity  *ignoreEntity  = nullptr,
        BOOL             bDebugDraw    = false,
        const COLOR     &debugColor    = COLOR_RED,
        FLOAT            debugDuration = 0.0f)
    {
        return TraceByType_Multi(shape, start, end, entityType, outHits, radius, halfHeight, 0.0f, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }

    static inline UINT Trace_Multi_ByType(TraceShape shape, const Vector3D &start, const Vector3D &end, EntityType entityType, Array<RayHit> &outHits, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
        return Multi_Trace_ByType(shape, start, end, entityType, outHits, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
    }
};

// ============================================================================
// 4. Global Procedural Functions (Direct Calling Without Class Prefix!)
// ============================================================================

inline BOOL TraceByChannel(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    TraceChannel     channel,
    RayHit          &outHit,
    FLOAT            paramA        = 0.5f,
    FLOAT            paramB        = 0.9f,
    FLOAT            paramC        = 0.5f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::TraceByChannel(shape, mode, start, end, channel, outHit, paramA, paramB, paramC, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL TraceByChannel(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    HitChannel       channel,
    RayHit          &outHit,
    FLOAT            paramA        = 0.5f,
    FLOAT            paramB        = 0.9f,
    FLOAT            paramC        = 0.5f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::TraceByHitChannel(shape, mode, start, end, channel, outHit, paramA, paramB, paramC, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Line_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    RayHit          &outHit,
    TraceChannel     channel       = TraceChannel::Visibility,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Line_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    RayHit          &outHit,
    HitChannel       channel,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Line(const Vector3D &start, const Vector3D &end, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline BOOL Trace_Line(const Vector3D &start, const Vector3D &end, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline BOOL Raycast(const Vector3D &start, const Vector3D &end, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Line_Trace(start, end, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Ray_Trace(
    const Vector3D  &origin,
    const Vector3D  &direction,
    FLOAT            maxDistance,
    RayHit          &outHit,
    TraceChannel     channel       = TraceChannel::Visibility,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Ray_Trace(origin, direction, maxDistance, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Ray_Trace(
    const Vector3D  &origin,
    const Vector3D  &direction,
    FLOAT            maxDistance,
    RayHit          &outHit,
    HitChannel       channel,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Ray_Trace(origin, direction, maxDistance, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Sphere_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    RayHit          &outHit,
    TraceChannel     channel       = TraceChannel::Visibility,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Sphere_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    RayHit          &outHit,
    HitChannel       channel,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Sphere(const Vector3D &start, const Vector3D &end, FLOAT radius, RayHit &outHit, TraceChannel channel = TraceChannel::Visibility, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline BOOL Trace_Sphere(const Vector3D &start, const Vector3D &end, FLOAT radius, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Sphere_Trace(start, end, radius, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Capsule_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    FLOAT            halfHeight,
    RayHit          &outHit,
    TraceChannel     channel       = TraceChannel::WorldStatic,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Capsule_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    FLOAT            halfHeight,
    RayHit          &outHit,
    HitChannel       channel,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Capsule(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, RayHit &outHit, TraceChannel channel = TraceChannel::WorldStatic, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline BOOL Trace_Capsule(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Capsule_Trace(start, end, radius, halfHeight, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Box_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    const Vector3D  &extents,
    RayHit          &outHit,
    TraceChannel     channel       = TraceChannel::WorldStatic,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Box_Trace(
    const Vector3D  &start,
    const Vector3D  &end,
    const Vector3D  &extents,
    RayHit          &outHit,
    HitChannel       channel,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Box(const Vector3D &start, const Vector3D &end, const Vector3D &extents, RayHit &outHit, TraceChannel channel = TraceChannel::WorldStatic, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline BOOL Trace_Box(const Vector3D &start, const Vector3D &end, const Vector3D &extents, RayHit &outHit, HitChannel channel, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Box_Trace(start, end, extents, outHit, channel, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline UINT Multi_Trace(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    Array<RayHit>   &outHits,
    TraceChannel     channel       = TraceChannel::Visibility,
    FLOAT            radius        = 0.5f,
    FLOAT            halfHeight    = 0.9f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline UINT Multi_Trace(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    Array<RayHit>   &outHits,
    HitChannel       channel,
    FLOAT            radius        = 0.5f,
    FLOAT            halfHeight    = 0.9f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline UINT Trace_Multi(TraceShape shape, const Vector3D &start, const Vector3D &end, Array<RayHit> &outHits, TraceChannel channel = TraceChannel::Visibility, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}
inline UINT Trace_Multi(TraceShape shape, const Vector3D &start, const Vector3D &end, Array<RayHit> &outHits, HitChannel channel, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Multi_Trace(shape, start, end, outHits, channel, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Has_LineOfSight(
    const Vector3D  &start,
    const Vector3D  &end,
    TraceChannel     channel      = TraceChannel::Visibility,
    const C_Entity  *ignoreEntity = nullptr)
{
    return M_Collision::Has_LineOfSight(start, end, channel, ignoreEntity);
}

inline BOOL Has_LineOfSight(
    const Vector3D  &start,
    const Vector3D  &end,
    HitChannel       channel,
    const C_Entity  *ignoreEntity = nullptr)
{
    return M_Collision::Has_LineOfSight(start, end, channel, ignoreEntity);
}

inline BOOL Ground_Trace(
    const Vector3D &origin,
    const Vector3D &direction,
    Vector3D       &outGroundHit,
    FLOAT           groundY = 0.0f)
{
    return M_Collision::Ground_Trace(origin, direction, outGroundHit, groundY);
}

inline BOOL Trace_Ground(const Vector3D &origin, const Vector3D &direction, Vector3D &outGroundHit, FLOAT groundY = 0.0f) {
    return M_Collision::Ground_Trace(origin, direction, outGroundHit, groundY);
}

// ----------------------------------------------------------------------------
// Global TraceByType Procedural Helpers
// ----------------------------------------------------------------------------

inline BOOL TraceByType(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    EntityType       entityType,
    RayHit          &outHit,
    FLOAT            paramA        = 0.5f,
    FLOAT            paramB        = 0.9f,
    FLOAT            paramC        = 0.5f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::TraceByType(shape, mode, start, end, entityType, outHit, paramA, paramB, paramC, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Line_Trace_ByType(
    const Vector3D  &start,
    const Vector3D  &end,
    EntityType       entityType,
    RayHit          &outHit,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Line_Trace_ByType(start, end, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Line_ByType(const Vector3D &start, const Vector3D &end, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Line_Trace_ByType(start, end, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Ray_Trace_ByType(
    const Vector3D  &origin,
    const Vector3D  &direction,
    FLOAT            maxDistance,
    EntityType       entityType,
    RayHit          &outHit,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Ray_Trace_ByType(origin, direction, maxDistance, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Sphere_Trace_ByType(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    EntityType       entityType,
    RayHit          &outHit,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Sphere_Trace_ByType(start, end, radius, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Sphere_ByType(const Vector3D &start, const Vector3D &end, FLOAT radius, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Sphere_Trace_ByType(start, end, radius, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Capsule_Trace_ByType(
    const Vector3D  &start,
    const Vector3D  &end,
    FLOAT            radius,
    FLOAT            halfHeight,
    EntityType       entityType,
    RayHit          &outHit,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Capsule_Trace_ByType(start, end, radius, halfHeight, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Capsule_ByType(const Vector3D &start, const Vector3D &end, FLOAT radius, FLOAT halfHeight, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Capsule_Trace_ByType(start, end, radius, halfHeight, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Box_Trace_ByType(
    const Vector3D  &start,
    const Vector3D  &end,
    const Vector3D  &extents,
    EntityType       entityType,
    RayHit          &outHit,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Box_Trace_ByType(start, end, extents, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline BOOL Trace_Box_ByType(const Vector3D &start, const Vector3D &end, const Vector3D &extents, EntityType entityType, RayHit &outHit, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Box_Trace_ByType(start, end, extents, entityType, outHit, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline UINT Multi_Trace_ByType(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    EntityType       entityType,
    Array<RayHit>   &outHits,
    FLOAT            radius        = 0.5f,
    FLOAT            halfHeight    = 0.9f,
    const C_Entity  *ignoreEntity  = nullptr,
    BOOL             bDebugDraw    = false,
    const COLOR     &debugColor    = COLOR_RED,
    FLOAT            debugDuration = 0.0f)
{
    return M_Collision::Multi_Trace_ByType(shape, start, end, entityType, outHits, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

inline UINT Trace_Multi_ByType(TraceShape shape, const Vector3D &start, const Vector3D &end, EntityType entityType, Array<RayHit> &outHits, FLOAT radius = 0.5f, FLOAT halfHeight = 0.9f, const C_Entity *ignoreEntity = nullptr, BOOL bDebugDraw = false, const COLOR &debugColor = COLOR_RED, FLOAT debugDuration = 0.0f) {
    return M_Collision::Multi_Trace_ByType(shape, start, end, entityType, outHits, radius, halfHeight, ignoreEntity, bDebugDraw, debugColor, debugDuration);
}

// ----------------------------------------------------------------------------
// Multithreaded & Asynchronous Tracing Helpers
// ----------------------------------------------------------------------------

inline void Batch_Trace(const Array<TraceRequest> &requests, Array<RayHit> &outResults) {
    M_Collision::Batch_Trace(requests, outResults);
}

inline void Async_Trace(const TraceRequest &request, const Function<void(const RayHit&)> &onComplete) {
    M_Collision::Async_Trace(request, onComplete);
}
