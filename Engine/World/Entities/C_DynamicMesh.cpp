#include <C_DynamicMesh.h>

C_DynamicMesh::C_DynamicMesh() {
    Type         = EntityType::DynamicMesh;
    bIsActive    = true;
    bIsVisible   = true;
    Transparency = 1.0f;
}

void C_DynamicMesh::Add_Velocity(const Vector3D &deltaVel) {
    Velocity = Velocity + deltaVel;
}

void C_DynamicMesh::Move_Direction(const Vector3D &dir, FLOAT distance) {
    Position.x += dir.x * distance;
    Position.y += dir.y * distance;
    Position.z += dir.z * distance;
}

void C_DynamicMesh::Stop() {
    Velocity      = { 0.0f, 0.0f, 0.0f };
    RotationSpeed = { 0.0f, 0.0f, 0.0f };
}

void C_DynamicMesh::Tick(FLOAT deltaTime) {
    if (Velocity.x != 0.0f || Velocity.y != 0.0f || Velocity.z != 0.0f) {
        Move(Velocity * deltaTime);
    }

    if (RotationSpeed.x != 0.0f || RotationSpeed.y != 0.0f || RotationSpeed.z != 0.0f) {
        Add_Rotation(
            RotationSpeed.x * deltaTime,
            RotationSpeed.y * deltaTime,
            RotationSpeed.z * deltaTime);
    }
}
