#include <C_StaticMesh.h>

C_StaticMesh::C_StaticMesh() {
    Type         = EntityType::StaticMesh;
    bIsActive    = true;
    bIsVisible   = true;
    Transparency = 1.0f;
}

void C_StaticMesh::Set_Transparency(FLOAT value) {
    Transparency = U_Math::Clamp_MinMax(value, 0.0f, 1.0f);
}

AABB C_StaticMesh::Get_WorldAABB() const {
    AABB worldBox;
    if (Mesh.LocalBounds.Is_Valid()) {
        // Transform the 8 corners of the local bounding box by Scale, Yaw rotation, and Position
        FLOAT yawRad = Rotation.y * (3.14159265f / 180.0f);
        FLOAT cosY   = std::cos(yawRad);
        FLOAT sinY   = std::sin(yawRad);

        const Vector3D &min = Mesh.LocalBounds.Min;
        const Vector3D &max = Mesh.LocalBounds.Max;

        const Vector3D corners[8] = {
            { min.x * Size.x, min.y * Size.y, min.z * Size.z },
            { max.x * Size.x, min.y * Size.y, min.z * Size.z },
            { min.x * Size.x, max.y * Size.y, min.z * Size.z },
            { max.x * Size.x, max.y * Size.y, min.z * Size.z },
            { min.x * Size.x, min.y * Size.y, max.z * Size.z },
            { max.x * Size.x, min.y * Size.y, max.z * Size.z },
            { min.x * Size.x, max.y * Size.y, max.z * Size.z },
            { max.x * Size.x, max.y * Size.y, max.z * Size.z }
        };

        for (int i = 0; i < 8; ++i) {
            FLOAT rx = corners[i].x * cosY + corners[i].z * sinY;
            FLOAT ry = corners[i].y;
            FLOAT rz = -corners[i].x * sinY + corners[i].z * cosY;

            worldBox.Encompass(Position + Vector3D(rx, ry, rz));
        }
    } else {
        Vector3D halfExtents = Size * 0.5f;
        worldBox.Min = Position - halfExtents;
        worldBox.Max = Position + halfExtents;
    }
    return worldBox;
}
