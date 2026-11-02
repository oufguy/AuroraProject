#include <pch.h>
#include <C_Character.h>
#include <U_Math.h>
#include <M_World.h>
#include <M_Collision.h>

C_Character::C_Character() {
    Type              = EntityType::Character;
    MoveSpeed         = 6.0f; // Default 6 m/s ground movement speed
    Size              = { 1.0f, 1.8f, 1.0f }; // Standard humanoid proportions
    Set_CollisionProfile(CollisionProfile::Entity);
}

AABB C_Character::Get_CharacterAABB(const Vector3D &testPos) const {
    FLOAT halfW = Size.x * 0.45f;
    FLOAT halfD = Size.z * 0.45f;
    return AABB(
        Vector3D(testPos.x - halfW, testPos.y, testPos.z - halfD),
        Vector3D(testPos.x + halfW, testPos.y + Size.y, testPos.z + halfD)
    );
}

void C_Character::Move_Forward(FLOAT axis, FLOAT deltaTime) {
    if (std::abs(axis) <= 0.001f) return;

    FLOAT yawRad = Rotation.y * (3.14159265f / 180.0f);
    FLOAT fwdX   = std::sin(yawRad);
    FLOAT fwdZ   = std::cos(yawRad);

    FLOAT step   = axis * MoveSpeed * deltaTime;
    FLOAT deltaX = fwdX * step;
    FLOAT deltaZ = fwdZ * step;

    FLOAT radius = Size.x * 0.45f;
    FLOAT halfH  = Size.y * 0.5f;

    // Slide collision: test X step independently with Capsule Trace
    RayHit hitX;
    Vector3D testPosX = { Position.x + deltaX, Position.y, Position.z };
    if (!Trace_Capsule(Position, testPosX, radius, halfH, hitX, TraceChannel::WorldStatic, this)) {
        Position.x = testPosX.x;
    }

    // Test Z step independently (enables smooth sliding along obstacle walls)
    RayHit hitZ;
    Vector3D testPosZ = { Position.x, Position.y, Position.z + deltaZ };
    if (!Trace_Capsule(Position, testPosZ, radius, halfH, hitZ, TraceChannel::WorldStatic, this)) {
        Position.z = testPosZ.z;
    }
}

void C_Character::Move_Right(FLOAT axis, FLOAT deltaTime) {
    if (std::abs(axis) <= 0.001f) return;

    FLOAT yawRad = Rotation.y * (3.14159265f / 180.0f);
    FLOAT rightX = std::cos(yawRad);
    FLOAT rightZ = -std::sin(yawRad);

    FLOAT step   = axis * MoveSpeed * deltaTime;
    FLOAT deltaX = rightX * step;
    FLOAT deltaZ = rightZ * step;

    FLOAT radius = Size.x * 0.45f;
    FLOAT halfH  = Size.y * 0.5f;

    RayHit hitX;
    Vector3D testPosX = { Position.x + deltaX, Position.y, Position.z };
    if (!Trace_Capsule(Position, testPosX, radius, halfH, hitX, TraceChannel::WorldStatic, this)) {
        Position.x = testPosX.x;
    }

    RayHit hitZ;
    Vector3D testPosZ = { Position.x, Position.y, Position.z + deltaZ };
    if (!Trace_Capsule(Position, testPosZ, radius, halfH, hitZ, TraceChannel::WorldStatic, this)) {
        Position.z = testPosZ.z;
    }
}

void C_Character::Turn(FLOAT deltaYawDegrees) {
    Rotation.y += deltaYawDegrees;
    if (Rotation.y >= 360.0f) Rotation.y -= 360.0f;
    if (Rotation.y <    0.0f) Rotation.y += 360.0f;
}

void C_Character::Jump() {
    if (bIsGrounded) {
        Velocity.y  = JumpStrength;
        bIsGrounded = false;
    }
}

void C_Character::Tick(FLOAT deltaTime) {
    // 1. Apply DynamicMesh linear velocity stepping
    C_DynamicMesh::Tick(deltaTime);

    // 2. Apply Gravity & Ground Clamping
    if (Position.y > GroundY + 0.001f || Velocity.y > 0.001f) {
        Velocity.y -= Gravity * deltaTime;
        Position.y += Velocity.y * deltaTime;
        bIsGrounded = false;

        // Hit the floor
        if (Position.y <= GroundY) {
            Position.y  = GroundY;
            Velocity.y  = 0.0f;
            bIsGrounded = true;
        }
    } else {
        Position.y  = GroundY;
        Velocity.y  = 0.0f;
        bIsGrounded = true;
    }
}
