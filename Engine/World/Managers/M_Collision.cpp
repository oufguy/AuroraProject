#include <pch.h>
#include <M_Collision.h>
#include <M_World.h>
#include <M_Threads.h>

namespace {
    inline BOOL Test_Shape_AABB(
        TraceShape      shape,
        const Ray3D    &ray,
        const AABB     &box,
        const Vector3D &end,
        FLOAT           dist,
        FLOAT           paramA,
        FLOAT           paramB,
        FLOAT           paramC,
        FLOAT          &outHitDist,
        Vector3D       &outHitNormal)
    {
        switch (shape) {
            case TraceShape::Line:
                return U_Raycast::Intersects_Ray_AABB(ray, box, outHitDist, outHitNormal);

            case TraceShape::Sphere: {
                FLOAT radius = paramA;
                Vector3D closestPt;
                if (U_Raycast::Intersects_Sphere_AABB(end, radius, box, closestPt)) {
                    outHitDist = dist;
                    outHitNormal = U_Math::Extended::Vector_Normalize(end - closestPt);
                    return true;
                }
                return false;
            }

            case TraceShape::Capsule: {
                FLOAT radius = paramA;
                FLOAT halfH  = paramB;
                Vector3D closestPt;
                if (U_Raycast::Intersects_Capsule_AABB(end, radius, halfH, box, closestPt)) {
                    outHitDist = dist;
                    outHitNormal = U_Math::Extended::Vector_Normalize(end - closestPt);
                    return true;
                }
                return false;
            }

            case TraceShape::Box: {
                AABB expanded(
                    box.Min - Vector3D(paramA, paramB, paramC),
                    box.Max + Vector3D(paramA, paramB, paramC)
                );
                return U_Raycast::Intersects_Ray_AABB(ray, expanded, outHitDist, outHitNormal);
            }
        }
        return false;
    }
}

// ============================================================================
// Multi-threaded Batch & Async Tracing Engines
// ============================================================================
void M_Collision::Batch_Trace(
    const Array<TraceRequest> &requests,
    Array<RayHit>             &outResults)
{
    const size_t count = requests.size();
    if (count == 0) return;

    outResults.resize(count);

    if (count == 1 || !M_Threads::Is_Initialized()) {
        for (size_t i = 0; i < count; ++i) {
            const auto &req = requests[i];
            TraceByChannel(
                req.Shape, TraceMode::Single,
                req.Start, req.End, req.Channel,
                outResults[i],
                req.ParamA, req.ParamB, req.ParamC,
                req.IgnoreEntity, req.bDebugDraw, req.DebugColor, req.DebugDuration
            );
        }
        return;
    }

    // Split requests across all CPU worker threads simultaneously!
    M_Threads::Parallel_For(0, static_cast<UINT>(count), [&](UINT i) {
        const auto &req = requests[i];
        TraceByChannel(
            req.Shape, TraceMode::Single,
            req.Start, req.End, req.Channel,
            outResults[i],
            req.ParamA, req.ParamB, req.ParamC,
            req.IgnoreEntity, req.bDebugDraw, req.DebugColor, req.DebugDuration
        );
    });
}

void M_Collision::Async_Trace(
    const TraceRequest                  &request,
    const Function<void(const RayHit&)> &onComplete)
{
    M_Threads::Run([request, onComplete]() {
        RayHit hit;
        TraceByChannel(
            request.Shape, TraceMode::Single,
            request.Start, request.End, request.Channel,
            hit,
            request.ParamA, request.ParamB, request.ParamC,
            request.IgnoreEntity, request.bDebugDraw, request.DebugColor, request.DebugDuration
        );
        if (onComplete) {
            onComplete(hit);
        }
    });
}

// ============================================================================
// 1. TraceByChannel (Single Hit using TraceChannel)
// ============================================================================
BOOL M_Collision::TraceByChannel(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    TraceChannel     channel,
    RayHit          &outHit,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHit = RayHit{};

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    FLOAT closestDist = dist + 1.0f;
    BOOL bFound = false;

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        // Polymorphic trace collision filter
        if (!entPtr->Can_Hit_Trace(channel)) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist && hitDistance < closestDist) {
                closestDist = hitDistance;
                outHit.bHit      = true;
                outHit.Distance  = hitDistance;
                outHit.Point     = ray.Get_Point(hitDistance);
                outHit.Normal    = hitNormal;
                outHit.Entity    = entPtr.get();
                outHit.EntityID  = entPtr->ID;
                outHit.Channel   = entPtr->Get_HitChannel();
                outHit.Type      = entPtr->Type;
                bFound = true;
            }
        }
    }

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        Vector3D lineEnd = bFound ? outHit.Point : end;
        U_Raycast::Draw_DebugLine(start, lineEnd, debugColor, duration);
        if (bFound) {
            U_Raycast::Draw_DebugPoint(outHit.Point, COLOR_GREEN, duration);
        }
    }

    return bFound;
}

// ============================================================================
// 2. TraceByHitChannel (Single Hit using HitChannel / ObjectChannel)
// ============================================================================
BOOL M_Collision::TraceByHitChannel(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    HitChannel       hitChannel,
    RayHit          &outHit,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHit = RayHit{};

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    FLOAT closestDist = dist + 1.0f;
    BOOL bFound = false;

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        // Polymorphic hit channel filter
        if (!entPtr->Can_Hit_HitChannel(hitChannel)) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist && hitDistance < closestDist) {
                closestDist = hitDistance;
                outHit.bHit      = true;
                outHit.Distance  = hitDistance;
                outHit.Point     = ray.Get_Point(hitDistance);
                outHit.Normal    = hitNormal;
                outHit.Entity    = entPtr.get();
                outHit.EntityID  = entPtr->ID;
                outHit.Channel   = entPtr->Get_HitChannel();
                outHit.Type      = entPtr->Type;
                bFound = true;
            }
        }
    }

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        Vector3D lineEnd = bFound ? outHit.Point : end;
        U_Raycast::Draw_DebugLine(start, lineEnd, debugColor, duration);
        if (bFound) {
            U_Raycast::Draw_DebugPoint(outHit.Point, COLOR_GREEN, duration);
        }
    }

    return bFound;
}

// ============================================================================
// 3. TraceByChannel_Multi (Multi Hit using TraceChannel)
// ============================================================================
UINT M_Collision::TraceByChannel_Multi(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    TraceChannel     channel,
    Array<RayHit>   &outHits,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHits.clear();

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        if (!entPtr->Can_Hit_Trace(channel)) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist) {
                RayHit h;
                h.bHit     = true;
                h.Distance = hitDistance;
                h.Point    = ray.Get_Point(hitDistance);
                h.Normal   = hitNormal;
                h.Entity   = entPtr.get();
                h.EntityID = entPtr->ID;
                h.Channel  = entPtr->Get_HitChannel();
                h.Type     = entPtr->Type;
                outHits.push_back(h);
            }
        }
    }

    std::sort(outHits.begin(), outHits.end(), [](const RayHit &a, const RayHit &b) {
        return a.Distance < b.Distance;
    });

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        U_Raycast::Draw_DebugLine(start, end, debugColor, duration);
        for (const auto &h : outHits) {
            U_Raycast::Draw_DebugPoint(h.Point, COLOR_GREEN, duration);
        }
    }

    return static_cast<UINT>(outHits.size());
}

// ============================================================================
// 4. TraceByHitChannel_Multi (Multi Hit using HitChannel)
// ============================================================================
UINT M_Collision::TraceByHitChannel_Multi(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    HitChannel       hitChannel,
    Array<RayHit>   &outHits,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHits.clear();

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        if (!entPtr->Can_Hit_HitChannel(hitChannel)) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist) {
                RayHit h;
                h.bHit     = true;
                h.Distance = hitDistance;
                h.Point    = ray.Get_Point(hitDistance);
                h.Normal   = hitNormal;
                h.Entity   = entPtr.get();
                h.EntityID = entPtr->ID;
                h.Channel  = entPtr->Get_HitChannel();
                h.Type     = entPtr->Type;
                outHits.push_back(h);
            }
        }
    }

    std::sort(outHits.begin(), outHits.end(), [](const RayHit &a, const RayHit &b) {
        return a.Distance < b.Distance;
    });

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        U_Raycast::Draw_DebugLine(start, end, debugColor, duration);
        for (const auto &h : outHits) {
            U_Raycast::Draw_DebugPoint(h.Point, COLOR_GREEN, duration);
        }
    }

    return static_cast<UINT>(outHits.size());
}

// ============================================================================
// 5. TraceByType (Single Hit filtered directly by EntityType)
// ============================================================================
BOOL M_Collision::TraceByType(
    TraceShape       shape,
    TraceMode        mode,
    const Vector3D  &start,
    const Vector3D  &end,
    EntityType       entityType,
    RayHit          &outHit,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHit = RayHit{};

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    FLOAT closestDist = dist + 1.0f;
    BOOL bFound = false;

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        // EntityType filter: if specific type requested, reject all others!
        if (entityType != EntityType::None && entPtr->Type != entityType) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist && hitDistance < closestDist) {
                closestDist = hitDistance;
                outHit.bHit      = true;
                outHit.Distance  = hitDistance;
                outHit.Point     = ray.Get_Point(hitDistance);
                outHit.Normal    = hitNormal;
                outHit.Entity    = entPtr.get();
                outHit.EntityID  = entPtr->ID;
                outHit.Channel   = entPtr->Get_HitChannel();
                outHit.Type      = entPtr->Type;
                bFound = true;
            }
        }
    }

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        Vector3D lineEnd = bFound ? outHit.Point : end;
        U_Raycast::Draw_DebugLine(start, lineEnd, debugColor, duration);
        if (bFound) {
            U_Raycast::Draw_DebugPoint(outHit.Point, COLOR_GREEN, duration);
        }
    }

    return bFound;
}

// ============================================================================
// 6. TraceByType_Multi (Multi Hit filtered directly by EntityType)
// ============================================================================
UINT M_Collision::TraceByType_Multi(
    TraceShape       shape,
    const Vector3D  &start,
    const Vector3D  &end,
    EntityType       entityType,
    Array<RayHit>   &outHits,
    FLOAT            paramA,
    FLOAT            paramB,
    FLOAT            paramC,
    const C_Entity  *ignoreEntity,
    BOOL             bDebugDraw,
    const COLOR     &debugColor,
    FLOAT            debugDuration)
{
    outHits.clear();

    Vector3D diff = end - start;
    FLOAT dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3D dir = (dist > 1e-6f) ? (diff * (1.0f / dist)) : Vector3D{ 0.0f, 0.0f, 1.0f };
    Ray3D ray(start, dir, dist);

    const auto &entities = M_World::Get_Entities();
    for (const auto &entPtr : entities) {
        if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy || entPtr.get() == ignoreEntity) {
            continue;
        }

        if (entityType != EntityType::None && entPtr->Type != entityType) {
            continue;
        }

        AABB box = entPtr->Get_WorldAABB();
        FLOAT hitDistance = 0.0f;
        Vector3D hitNormal = { 0.0f, 1.0f, 0.0f };

        if (Test_Shape_AABB(shape, ray, box, end, dist, paramA, paramB, paramC, hitDistance, hitNormal)) {
            if (hitDistance <= dist) {
                RayHit h;
                h.bHit     = true;
                h.Distance = hitDistance;
                h.Point    = ray.Get_Point(hitDistance);
                h.Normal   = hitNormal;
                h.Entity   = entPtr.get();
                h.EntityID = entPtr->ID;
                h.Channel  = entPtr->Get_HitChannel();
                h.Type     = entPtr->Type;
                outHits.push_back(h);
            }
        }
    }

    std::sort(outHits.begin(), outHits.end(), [](const RayHit &a, const RayHit &b) {
        return a.Distance < b.Distance;
    });

    BOOL shouldDebug = bDebugDraw || s_GlobalDebugDrawing;
    FLOAT duration   = (debugDuration > 0.0f) ? debugDuration : (s_GlobalDebugDrawing ? 1.5f : 0.0f);
    if (shouldDebug) {
        U_Raycast::Draw_DebugLine(start, end, debugColor, duration);
        for (const auto &h : outHits) {
            U_Raycast::Draw_DebugPoint(h.Point, COLOR_GREEN, duration);
        }
    }

    return static_cast<UINT>(outHits.size());
}

// ============================================================================
// 7. Ground_Trace
// ============================================================================
BOOL M_Collision::Ground_Trace(
    const Vector3D &origin,
    const Vector3D &direction,
    Vector3D       &outGroundHit,
    FLOAT           groundY)
{
    Ray3D ray(origin, direction, 1000.0f);
    return U_Raycast::Intersects_Ray_Ground(ray, groundY, outGroundHit);
}
