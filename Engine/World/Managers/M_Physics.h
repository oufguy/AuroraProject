#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Raycast.h>
#include <C_Entity.h>
#include <M_Collision.h>

// ============================================================================
// M_Physics (Aurora Engine Physics Simulation & Motion Subsystem)
// ============================================================================
// Responsible strictly for physical forces, gravity, and motion simulation:
//   - Gravity & Terminal Velocity constants
//   - Surface sliding resolution (Slide_AlongSurface)
//   - Ballistic velocity calculations (Compute_BallisticVelocity)
//   - Trajectory prediction (Predict_ProjectilePath — queries M_Collision)
//
// NOTE: All raycasts, sweeps, and channel queries belong exclusively to M_Collision!
// ============================================================================
class M_Physics {
public:
    // Standard Earth gravity (9.80665 m/s^2 downwards)
    static inline constexpr FLOAT DefaultGravity = 9.80665f;

    // Terminal velocity for humanoids in air (~53 m/s)
    static inline constexpr FLOAT TerminalVelocity = 53.0f;

    // Surface Slide Resolver: Calculates sliding velocity along a collision normal
    // Prevents player/monsters from stopping dead or sticking to diagonal walls.
    [[nodiscard]] static inline Vector3D Slide_AlongSurface(
        const Vector3D &velocity,
        const Vector3D &surfaceNormal,
        FLOAT           friction = 0.0f)
    {
        FLOAT dot = velocity.x * surfaceNormal.x + velocity.y * surfaceNormal.y + velocity.z * surfaceNormal.z;
        Vector3D slide = velocity - surfaceNormal * dot;
        if (friction > 0.001f) {
            slide = slide * (1.0f - U_Math::Clamp_MinMax(friction, 0.0f, 1.0f));
        }
        return slide;
    }

    // Ballistic Velocity Solver: What launch velocity reaches a 3D target in travelTime seconds?
    [[nodiscard]] static inline Vector3D Compute_BallisticVelocity(
        const Vector3D &start,
        const Vector3D &target,
        FLOAT           travelTime,
        FLOAT           gravity = DefaultGravity)
    {
        if (travelTime <= 0.001f) return { 0.0f, 0.0f, 0.0f };
        Vector3D diff = target - start;
        FLOAT invTime = 1.0f / travelTime;
        return {
            diff.x * invTime,
            (diff.y + 0.5f * gravity * travelTime * travelTime) * invTime,
            diff.z * invTime
        };
    }

    // Predict_ProjectilePath (Unreal Engine PredictProjectilePath):
    // Simulates an arc trajectory under gravity, querying M_Collision to test impacts!
    static BOOL Predict_ProjectilePath(
        const Vector3D  &startPos,
        const Vector3D  &initialVelocity,
        RayHit          &outHit,
        Array<Vector3D> *outPathPoints     = nullptr,
        FLOAT            gravity           = DefaultGravity,
        FLOAT            maxSimulationTime = 5.0f,
        FLOAT            stepInterval      = 0.05f,
        TraceChannel     channel           = TraceChannel::WorldStatic,
        const C_Entity  *ignoreEntity      = nullptr,
        BOOL             bDebugDraw        = false,
        const COLOR     &debugColor        = COLOR_YELLOW)
    {
        outHit = RayHit{};
        Vector3D currPos = startPos;
        Vector3D currVel = initialVelocity;

        if (outPathPoints) {
            outPathPoints->push_back(currPos);
        }

        FLOAT elapsed = 0.0f;
        while (elapsed < maxSimulationTime) {
            Vector3D nextVel = currVel - Vector3D(0.0f, gravity * stepInterval, 0.0f);
            Vector3D nextPos = currPos + (currVel + nextVel) * (0.5f * stepInterval);

            // Calls M_Collision for the geometric test!
            if (M_Collision::Line_Trace(currPos, nextPos, outHit, channel, ignoreEntity, bDebugDraw, debugColor, 1.5f)) {
                if (outPathPoints) outPathPoints->push_back(outHit.Point);
                return true;
            }

            currPos = nextPos;
            currVel = nextVel;
            if (outPathPoints) outPathPoints->push_back(currPos);

            elapsed += stepInterval;
        }

        return false;
    }
};

// ============================================================================
// Procedural Global 1-Liner Aliases for Physics
// ============================================================================

inline Vector3D Slide_AlongSurface(const Vector3D &velocity, const Vector3D &surfaceNormal, FLOAT friction = 0.0f) {
    return M_Physics::Slide_AlongSurface(velocity, surfaceNormal, friction);
}

inline Vector3D Compute_BallisticVelocity(const Vector3D &start, const Vector3D &target, FLOAT travelTime, FLOAT gravity = M_Physics::DefaultGravity) {
    return M_Physics::Compute_BallisticVelocity(start, target, travelTime, gravity);
}

inline BOOL Predict_ProjectilePath(
    const Vector3D  &startPos,
    const Vector3D  &initialVelocity,
    RayHit          &outHit,
    Array<Vector3D> *outPathPoints     = nullptr,
    FLOAT            gravity           = M_Physics::DefaultGravity,
    FLOAT            maxSimulationTime = 5.0f,
    FLOAT            stepInterval      = 0.05f,
    TraceChannel     channel           = TraceChannel::WorldStatic,
    const C_Entity  *ignoreEntity      = nullptr,
    BOOL             bDebugDraw        = false,
    const COLOR     &debugColor        = COLOR_YELLOW)
{
    return M_Physics::Predict_ProjectilePath(startPos, initialVelocity, outHit, outPathPoints, gravity, maxSimulationTime, stepInterval, channel, ignoreEntity, bDebugDraw, debugColor);
}
