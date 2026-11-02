#pragma once

#include <C_DynamicMesh.h>

// ============================================================================
// C_Character (MMORPG Ground Player / Monster / NPC Base Entity)
// ============================================================================
// Inherits from C_DynamicMesh:
//   - C_Entity: ID, Position, Rotation
//   - C_StaticMesh: Mesh, TextureID, Size, Tint, Transparency
//   - C_DynamicMesh: Velocity, MoveSpeed, RotationSpeed
//
// Adds MMORPG Ground Character Physics:
//   - Ground clamping & gravity simulation (no falling through the world!)
//   - WASD Ground walking & turning (Yaw)
//   - Jumping & grounded state detection
//   - 3rd-person camera target point (Head / Chest level)
// ============================================================================
class C_Character : public C_DynamicMesh {
public:
    FLOAT TurnSpeed    = 180.0f; // Rotation turn speed in degrees per second
    FLOAT Gravity      = 18.0f;  // Downward gravity acceleration (m/s^2)
    FLOAT JumpStrength = 7.0f;   // Upward launch velocity when jumping
    FLOAT GroundY      = 0.0f;   // Floor height at current position
    BOOL  bIsGrounded  = true;   // TRUE when feet touch the floor
    FLOAT EyeHeight    = 1.5f;   // Camera look-at target height above feet

    C_Character();
    ~C_Character() override = default;

    // Movement inputs (forward/back: -1.0 to +1.0, right/left: -1.0 to +1.0)
    void Move_Forward(FLOAT axis, FLOAT deltaTime);
    void Move_Right(FLOAT axis, FLOAT deltaTime);

    // Turns the character left or right (modifies Yaw angle)
    void Turn(FLOAT deltaYawDegrees);

    // Makes the character jump if currently standing on the floor
    void Jump();

    // Simulates gravity and ground clamping every frame
    void Tick(FLOAT deltaTime) override;

    // Returns the 3D position the chase camera should look at (e.g. chest/head level)
    [[nodiscard]] inline Vector3D Get_CameraTarget() const {
        return { Position.x, Position.y + EyeHeight, Position.z };
    }

    // Returns axis-aligned bounding box around the character for ground/wall collision
    [[nodiscard]] AABB Get_CharacterAABB(const Vector3D &testPos) const;

    // Overrides base entity bounding box with character capsule/humanoid bounds
    [[nodiscard]] AABB Get_WorldAABB() const override { return Get_CharacterAABB(Position); }
};

// Aliases
using Character = C_Character;
