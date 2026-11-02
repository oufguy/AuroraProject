#pragma once

#include <C_StaticMesh.h>

// ============================================================================
// C_DynamicMesh (Moving Props, Characters, Monsters, Projectiles)
// ============================================================================
// Inherits Mesh, Texture, Transform, and Color from C_StaticMesh.
// Adds dynamic continuous Velocity, RotationSpeed, and movement physics.
// Automatically ticked every frame by M_World::Tick!
// ============================================================================
class C_DynamicMesh : public C_StaticMesh {
public:
    Vector3D Velocity      = Vector3D(0.0f, 0.0f, 0.0f); // World velocity (Units/sec)
    Vector3D RotationSpeed = Vector3D(0.0f, 0.0f, 0.0f); // Continuous rotation speed (Degrees/sec)
    FLOAT    MoveSpeed     = 5.0f;                       // Base movement speed (Units/sec)

    C_DynamicMesh();

    // Adds a velocity impulse (Units/sec)
    void Add_Velocity(const Vector3D &deltaVel);

    // Moves the entity along an arbitrary direction vector by a distance in units
    void Move_Direction(const Vector3D &dir, FLOAT distance);

    // Immediately stops all dynamic movement and rotation
    void Stop();

    // Steps movement physics every frame
    void Tick(FLOAT deltaTime) override;
};

// Legacy compatibility alias
using E_DynamicMesh = C_DynamicMesh;
