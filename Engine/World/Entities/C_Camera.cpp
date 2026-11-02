#include <C_Camera.h>
#include <M_Mesh.h>
#include <M_Window.h>
#include <U_Raycast.h>

C_Camera *C_Camera::s_ActiveCamera = nullptr;

C_Camera::C_Camera() {
    Type       = EntityType::Camera;
    bIsActive  = true;
    bIsVisible = false; // Cameras are never drawn as visible geometry!
    Position   = { 0.0f, 0.0f, -3.0f };
    Rotation   = { 0.0f, 0.0f, 0.0f };

    if (!s_ActiveCamera) {
        s_ActiveCamera = this;
    }
}

C_Camera::~C_Camera() {
    if (s_ActiveCamera == this) {
        s_ActiveCamera = nullptr;
    }
}

void C_Camera::Set_Active() {
    s_ActiveCamera = this;
}

C_Camera &C_Camera::Get_Active() {
    if (!s_ActiveCamera) {
        static C_Camera s_FallbackCamera;
        return s_FallbackCamera;
    }
    return *s_ActiveCamera;
}

void C_Camera::Set_FOV(FLOAT fovDegrees) {
    FOV = U_Math::Clamp_MinMax(fovDegrees, 20.0f, 140.0f);
}

void C_Camera::Set_ClipPlanes(FLOAT nearZ, FLOAT farZ) {
    NearClip = (nearZ > 0.001f) ? nearZ : 0.001f;
    FarClip  = (farZ > NearClip) ? farZ : NearClip + 1.0f;
}

void C_Camera::UpdateBasis() const {
    if (Rotation.x == m_LastRot.x && Rotation.y == m_LastRot.y && Rotation.z == m_LastRot.z) {
        return;
    }

    FLOAT pitch = Rotation.x * U_Math::DegToRad;
    FLOAT yaw   = Rotation.y * U_Math::DegToRad;
    FLOAT sp    = U_Math::Radians_Sin(pitch);
    FLOAT cp    = U_Math::Radians_Cos(pitch);
    FLOAT sy    = U_Math::Radians_Sin(yaw);
    FLOAT cy    = U_Math::Radians_Cos(yaw);

    m_CachedFwd   = { sy * cp, -sp, cy * cp };
    m_CachedRight = { cy, 0.0f, -sy };
    m_CachedUp    = { sy * sp, cp, cy * sp };
    m_LastRot     = Rotation;
}

void C_Camera::Update_CameraView() const {
    if (Position.x == m_LastPos.x && Position.y == m_LastPos.y && Position.z == m_LastPos.z &&
        Rotation.x == m_LastRot.x && Rotation.y == m_LastRot.y && Rotation.z == m_LastRot.z &&
        FOV == m_LastFOV && NearClip == m_LastNear && FarClip == m_LastFar) {
        return;
    }

    m_LastPos  = Position;
    m_LastFOV  = FOV;
    m_LastNear = NearClip;
    m_LastFar  = FarClip;

    UpdateBasis();
    M_Mesh::Set_CameraView(Position, m_CachedRight, m_CachedUp, m_CachedFwd, FOV, NearClip, FarClip);
}

void C_Camera::Add_Rotation(FLOAT deltaPitch, FLOAT deltaYaw, FLOAT deltaRoll) {
    Rotation.x = U_Math::Clamp_MinMax(Rotation.x + deltaPitch, -89.0f, 89.0f);
    Rotation.y += deltaYaw;
    Rotation.z += deltaRoll;
}

void C_Camera::Set_Rotation(const Vector3D &rotDeg) {
    Rotation.x = U_Math::Clamp_MinMax(rotDeg.x, -89.0f, 89.0f);
    Rotation.y = rotDeg.y;
    Rotation.z = rotDeg.z;
}

void C_Camera::Follow(const Vector3D &targetPos, FLOAT distance, FLOAT height) {
    Vector3D lookAt = { targetPos.x, targetPos.y + height, targetPos.z };

    FLOAT pitchRad = Rotation.x * (3.14159265f / 180.0f);
    FLOAT yawRad   = Rotation.y * (3.14159265f / 180.0f);

    FLOAT cosP = std::cos(pitchRad);
    FLOAT sinP = std::sin(pitchRad);
    FLOAT cosY = std::cos(yawRad);
    FLOAT sinY = std::sin(yawRad);

    Position.x = lookAt.x - sinY * cosP * distance;
    Position.y = lookAt.y + sinP * distance;
    Position.z = lookAt.z - cosY * cosP * distance;
}

void C_Camera::Reset() {
    Position   = { 0.0f, 0.0f, -3.0f };
    Rotation   = { 0.0f, 0.0f, 0.0f };
    FOV        = 60.0f;
    NearClip   = 0.1f;
    FarClip    = 500.0f;
    m_LastRot  = { 999.0f, 999.0f, 999.0f };
    m_LastPos  = { 999.0f, 999.0f, 999.0f };
}

BOOL C_Camera::World_To_Screen(
    const Vector3D &worldPos,
    Vector2D       &outScreenPos,
    FLOAT           screenWidth,
    FLOAT           screenHeight) const
{
    return U_Raycast::World_To_Screen(worldPos, Position, Rotation, FOV, outScreenPos, screenWidth, screenHeight);
}

BOOL C_Camera::Screen_To_WorldRay(
    const Vector2D &screenPos,
    Vector3D       &outRayOrigin,
    Vector3D       &outRayDir,
    FLOAT           screenWidth,
    FLOAT           screenHeight) const
{
    Ray3D ray = U_Raycast::Screen_To_World(screenPos, Position, Rotation, FOV, screenWidth, screenHeight);
    outRayOrigin = ray.Origin;
    outRayDir    = ray.Direction;
    return true;
}
