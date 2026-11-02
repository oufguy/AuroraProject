#include <pch.h>
#include <U_Raycast.h>
#include <M_Window.h>
#include <M_Mesh.h>
#include <RHI_Manager.h>

// ============================================================================
// Internal Debug Drawing Queue
// ============================================================================
struct DebugLine {
    Vector3D Start;
    Vector3D End;
    COLOR    Color;
    FLOAT    Duration = 0.0f;
};

static Array<DebugLine> s_DebugLines;
static std::mutex       s_DebugMutex;
static MeshData         s_BeamMesh;
static BOOL             s_BeamMeshInit = false;

// ============================================================================
// A. Unreal-Style Rotation Vectors (Pitch, Yaw, Roll in Degrees)
// ============================================================================
Vector3D U_Raycast::Get_ForwardVector(const Vector3D &rotationDeg) {
    FLOAT pitch = rotationDeg.x * U_Math::DegToRad;
    FLOAT yaw   = rotationDeg.y * U_Math::DegToRad;
    FLOAT cp = U_Math::Radians_Cos(pitch), sp = U_Math::Radians_Sin(pitch);
    FLOAT cy = U_Math::Radians_Cos(yaw),   sy = U_Math::Radians_Sin(yaw);
    return { sy * cp, -sp, cy * cp };
}

Vector3D U_Raycast::Get_RightVector(const Vector3D &rotationDeg) {
    FLOAT yaw = rotationDeg.y * U_Math::DegToRad;
    return { U_Math::Radians_Cos(yaw), 0.0f, -U_Math::Radians_Sin(yaw) };
}

Vector3D U_Raycast::Get_UpVector(const Vector3D &rotationDeg) {
    FLOAT pitch = rotationDeg.x * U_Math::DegToRad;
    FLOAT yaw   = rotationDeg.y * U_Math::DegToRad;
    FLOAT cp = U_Math::Radians_Cos(pitch), sp = U_Math::Radians_Sin(pitch);
    FLOAT cy = U_Math::Radians_Cos(yaw),   sy = U_Math::Radians_Sin(yaw);
    return { sy * sp, cp, cy * sp };
}

// ============================================================================
// B. Unreal-Style Screen & World Projection
// ============================================================================
Ray3D U_Raycast::Screen_To_World(
    const Vector2D &screenPos,
    const Vector3D &camPos,
    const Vector3D &camRotDeg,
    FLOAT           fovDegrees,
    FLOAT           screenWidth,
    FLOAT           screenHeight)
{
    FLOAT w = (screenWidth > 0.0f) ? screenWidth : static_cast<FLOAT>(M_Window::Get_Width());
    FLOAT h = (screenHeight > 0.0f) ? screenHeight : static_cast<FLOAT>(M_Window::Get_Height());
    if (w <= 0.0f) w = 1280.0f;
    if (h <= 0.0f) h = 720.0f;

    Vector3D fwd   = Get_ForwardVector(camRotDeg);
    Vector3D right = Get_RightVector(camRotDeg);
    Vector3D up    = Get_UpVector(camRotDeg);

    FLOAT pxW = static_cast<FLOAT>(M_Window::Get_PixelWidth());
    FLOAT pxH = static_cast<FLOAT>(M_Window::Get_PixelHeight());
    FLOAT aspectRatio = (pxH > 0.0f) ? (pxW / pxH) : (w / h);
    FLOAT halfFov     = U_Math::Clamp_MinMax(fovDegrees, 20.0f, 140.0f) * 0.5f * U_Math::DegToRad;
    FLOAT focalLen    = U_Math::Radians_Cos(halfFov) / U_Math::Radians_Sin(halfFov);
    FLOAT xScale      = focalLen / aspectRatio;
    FLOAT yScale      = focalLen;

    FLOAT ndcX = (screenPos.x / w) * 2.0f - 1.0f;
    FLOAT ndcY = -((screenPos.y / h) * 2.0f - 1.0f);

    FLOAT viewX = ndcX / xScale;
    FLOAT viewY = ndcY / yScale;
    FLOAT viewZ = 1.0f;

    Vector3D dirWorld = (right * viewX) + (up * viewY) + (fwd * viewZ);
    FLOAT lenSq = dirWorld.x * dirWorld.x + dirWorld.y * dirWorld.y + dirWorld.z * dirWorld.z;
    Vector3D dirNorm = (lenSq > 1e-8f) ? (dirWorld * (1.0f / std::sqrt(lenSq))) : fwd;

    return Ray3D(camPos, dirNorm, 1000.0f);
}

BOOL U_Raycast::World_To_Screen(
    const Vector3D &worldPos,
    const Vector3D &camPos,
    const Vector3D &camRotDeg,
    FLOAT           fovDegrees,
    Vector2D       &outScreenPos,
    FLOAT           screenWidth,
    FLOAT           screenHeight)
{
    Vector3D fwd   = Get_ForwardVector(camRotDeg);
    Vector3D right = Get_RightVector(camRotDeg);
    Vector3D up    = Get_UpVector(camRotDeg);

    Vector3D rel = worldPos - camPos;
    FLOAT viewX = rel.x * right.x + rel.y * right.y + rel.z * right.z;
    FLOAT viewY = rel.x * up.x    + rel.y * up.y    + rel.z * up.z;
    FLOAT viewZ = rel.x * fwd.x   + rel.y * fwd.y   + rel.z * fwd.z;

    if (viewZ <= 0.05f) return false;

    FLOAT w = (screenWidth > 0.0f) ? screenWidth : static_cast<FLOAT>(M_Window::Get_PixelWidth());
    FLOAT h = (screenHeight > 0.0f) ? screenHeight : static_cast<FLOAT>(M_Window::Get_PixelHeight());
    if (w <= 0.0f) w = 1280.0f;
    if (h <= 0.0f) h = 720.0f;

    FLOAT aspectRatio = w / h;
    FLOAT halfFov     = U_Math::Clamp_MinMax(fovDegrees, 20.0f, 140.0f) * 0.5f * U_Math::DegToRad;
    FLOAT focalLen    = U_Math::Radians_Cos(halfFov) / U_Math::Radians_Sin(halfFov);
    FLOAT xScale      = focalLen / aspectRatio;
    FLOAT yScale      = focalLen;

    FLOAT ndcX = (viewX * xScale) / viewZ;
    FLOAT ndcY = (viewY * yScale) / viewZ;

    outScreenPos.x = (ndcX * 0.5f + 0.5f) * w;
    outScreenPos.y = (-ndcY * 0.5f + 0.5f) * h;
    return true;
}

// ============================================================================
// C. Pure Geometric Intersections
// ============================================================================
BOOL U_Raycast::Intersects_Ray_AABB(
    const Ray3D &ray,
    const AABB  &aabb,
    FLOAT       &outDistance,
    Vector3D    &outNormal)
{
    FLOAT tmin = 0.0f;
    FLOAT tmax = ray.Length;
    Vector3D normalMin = { 0.0f, 0.0f, 0.0f };

    // X slab
    if (std::abs(ray.Direction.x) > 1e-6f) {
        FLOAT invD = 1.0f / ray.Direction.x;
        FLOAT t1 = (aabb.Min.x - ray.Origin.x) * invD;
        FLOAT t2 = (aabb.Max.x - ray.Origin.x) * invD;
        Vector3D n1 = { -1.0f, 0.0f, 0.0f };
        Vector3D n2 = {  1.0f, 0.0f, 0.0f };
        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
        if (t1 > tmin) { tmin = t1; normalMin = n1; }
        if (t2 < tmax) { tmax = t2; }
        if (tmin > tmax) return false;
    } else if (ray.Origin.x < aabb.Min.x || ray.Origin.x > aabb.Max.x) {
        return false;
    }

    // Y slab
    if (std::abs(ray.Direction.y) > 1e-6f) {
        FLOAT invD = 1.0f / ray.Direction.y;
        FLOAT t1 = (aabb.Min.y - ray.Origin.y) * invD;
        FLOAT t2 = (aabb.Max.y - ray.Origin.y) * invD;
        Vector3D n1 = { 0.0f, -1.0f, 0.0f };
        Vector3D n2 = { 0.0f,  1.0f, 0.0f };
        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
        if (t1 > tmin) { tmin = t1; normalMin = n1; }
        if (t2 < tmax) { tmax = t2; }
        if (tmin > tmax) return false;
    } else if (ray.Origin.y < aabb.Min.y || ray.Origin.y > aabb.Max.y) {
        return false;
    }

    // Z slab
    if (std::abs(ray.Direction.z) > 1e-6f) {
        FLOAT invD = 1.0f / ray.Direction.z;
        FLOAT t1 = (aabb.Min.z - ray.Origin.z) * invD;
        FLOAT t2 = (aabb.Max.z - ray.Origin.z) * invD;
        Vector3D n1 = { 0.0f, 0.0f, -1.0f };
        Vector3D n2 = { 0.0f, 0.0f,  1.0f };
        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
        if (t1 > tmin) { tmin = t1; normalMin = n1; }
        if (t2 < tmax) { tmax = t2; }
        if (tmin > tmax) return false;
    } else if (ray.Origin.z < aabb.Min.z || ray.Origin.z > aabb.Max.z) {
        return false;
    }

    if (tmin < 0.0f) tmin = 0.0f;
    outDistance = tmin;
    outNormal   = normalMin;
    return true;
}

BOOL U_Raycast::Intersects_Ray_Sphere(
    const Ray3D    &ray,
    const Vector3D &center,
    FLOAT           radius,
    FLOAT          &outDistance,
    Vector3D       &outNormal)
{
    Vector3D m = ray.Origin - center;
    FLOAT b = m.x * ray.Direction.x + m.y * ray.Direction.y + m.z * ray.Direction.z;
    FLOAT c = (m.x * m.x + m.y * m.y + m.z * m.z) - radius * radius;

    // Ray origin outside sphere and pointing away
    if (c > 0.0f && b > 0.0f) return false;

    FLOAT discr = b * b - c;
    if (discr < 0.0f) return false;

    FLOAT t = -b - std::sqrt(discr);
    if (t < 0.0f) t = 0.0f;
    if (t > ray.Length) return false;

    outDistance = t;
    Vector3D hitPt = ray.Get_Point(t);
    Vector3D norm = hitPt - center;
    FLOAT nLen = std::sqrt(norm.x * norm.x + norm.y * norm.y + norm.z * norm.z);
    outNormal = (nLen > 1e-6f) ? (norm * (1.0f / nLen)) : Vector3D{ 0.0f, 1.0f, 0.0f };
    return true;
}

BOOL U_Raycast::Intersects_Ray_Ground(
    const Ray3D &ray,
    FLOAT        groundY,
    Vector3D    &outGroundPoint)
{
    if (std::abs(ray.Direction.y) < 1e-6f) return false;
    FLOAT t = (groundY - ray.Origin.y) / ray.Direction.y;
    if (t < 0.0f || t > ray.Length) return false;

    outGroundPoint = ray.Get_Point(t);
    return true;
}

BOOL U_Raycast::Intersects_Ray_Plane(
    const Ray3D    &ray,
    const Vector3D &planeNormal,
    FLOAT           planeDist,
    FLOAT          &outDistance,
    Vector3D       &outPoint)
{
    FLOAT denom = planeNormal.x * ray.Direction.x + planeNormal.y * ray.Direction.y + planeNormal.z * ray.Direction.z;
    if (std::abs(denom) < 1e-6f) return false;

    FLOAT t = (planeDist - (planeNormal.x * ray.Origin.x + planeNormal.y * ray.Origin.y + planeNormal.z * ray.Origin.z)) / denom;
    if (t < 0.0f || t > ray.Length) return false;

    outDistance = t;
    outPoint    = ray.Get_Point(t);
    return true;
}

BOOL U_Raycast::Intersects_Ray_Triangle(
    const Ray3D    &ray,
    const Vector3D &v0,
    const Vector3D &v1,
    const Vector3D &v2,
    FLOAT          &outDistance,
    Vector2D       &outUV)
{
    Vector3D edge1 = v1 - v0;
    Vector3D edge2 = v2 - v0;

    // Cross product: ray.dir x edge2
    Vector3D pvec = {
        ray.Direction.y * edge2.z - ray.Direction.z * edge2.y,
        ray.Direction.z * edge2.x - ray.Direction.x * edge2.z,
        ray.Direction.x * edge2.y - ray.Direction.y * edge2.x
    };

    FLOAT det = edge1.x * pvec.x + edge1.y * pvec.y + edge1.z * pvec.z;
    if (std::abs(det) < 1e-7f) return false;
    FLOAT invDet = 1.0f / det;

    Vector3D tvec = ray.Origin - v0;
    FLOAT u = (tvec.x * pvec.x + tvec.y * pvec.y + tvec.z * pvec.z) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    // Cross product: tvec x edge1
    Vector3D qvec = {
        tvec.y * edge1.z - tvec.z * edge1.y,
        tvec.z * edge1.x - tvec.x * edge1.z,
        tvec.x * edge1.y - tvec.y * edge1.x
    };

    FLOAT v = (ray.Direction.x * qvec.x + ray.Direction.y * qvec.y + ray.Direction.z * qvec.z) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    FLOAT t = (edge2.x * qvec.x + edge2.y * qvec.y + edge2.z * qvec.z) * invDet;
    if (t < 0.0f || t > ray.Length) return false;

    outDistance = t;
    outUV       = { u, v };
    return true;
}

BOOL U_Raycast::Intersects_Sphere_AABB(
    const Vector3D &center,
    FLOAT           radius,
    const AABB     &aabb,
    Vector3D       &outClosestPoint)
{
    outClosestPoint.x = U_Math::Clamp_MinMax(center.x, aabb.Min.x, aabb.Max.x);
    outClosestPoint.y = U_Math::Clamp_MinMax(center.y, aabb.Min.y, aabb.Max.y);
    outClosestPoint.z = U_Math::Clamp_MinMax(center.z, aabb.Min.z, aabb.Max.z);

    FLOAT dx = center.x - outClosestPoint.x;
    FLOAT dy = center.y - outClosestPoint.y;
    FLOAT dz = center.z - outClosestPoint.z;

    return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
}

BOOL U_Raycast::Intersects_Capsule_AABB(
    const Vector3D &capsuleBase,
    FLOAT           radius,
    FLOAT           halfHeight,
    const AABB     &aabb,
    Vector3D       &outClosestPoint)
{
    // Vertical capsule segment runs from capsuleBase to capsuleBase + (0, 2*halfHeight, 0)
    FLOAT botY = capsuleBase.y + radius;
    FLOAT topY = capsuleBase.y + halfHeight * 2.0f - radius;
    if (topY < botY) topY = botY;

    // Clamp AABB center height to capsule spine segment
    FLOAT boxCenterY = (aabb.Min.y + aabb.Max.y) * 0.5f;
    FLOAT spineY = U_Math::Clamp_MinMax(boxCenterY, botY, topY);

    Vector3D sphereCenter = { capsuleBase.x, spineY, capsuleBase.z };
    return Intersects_Sphere_AABB(sphereCenter, radius, aabb, outClosestPoint);
}

// ============================================================================
// D. Visual Debug Drawing
// ============================================================================
void U_Raycast::Draw_DebugLine(const Vector3D &start, const Vector3D &end, const COLOR &color, FLOAT duration) {
    std::lock_guard<std::mutex> lock(s_DebugMutex);
    constexpr size_t MAX_DEBUG_LINES = 2048;
    if (s_DebugLines.size() >= MAX_DEBUG_LINES) {
        s_DebugLines.erase(s_DebugLines.begin(), s_DebugLines.begin() + 256);
    }
    s_DebugLines.push_back({ start, end, color, duration });
}

void U_Raycast::Draw_DebugRay(const Vector3D &origin, const Vector3D &direction, FLOAT length, const COLOR &color, FLOAT duration) {
    Vector3D end = origin + direction * length;
    Draw_DebugLine(origin, end, color, duration);
}

void U_Raycast::Draw_DebugBox(const AABB &box, const COLOR &color, FLOAT duration) {
    const Vector3D &min = box.Min;
    const Vector3D &max = box.Max;

    // Bottom square
    Draw_DebugLine({ min.x, min.y, min.z }, { max.x, min.y, min.z }, color, duration);
    Draw_DebugLine({ max.x, min.y, min.z }, { max.x, min.y, max.z }, color, duration);
    Draw_DebugLine({ max.x, min.y, max.z }, { min.x, min.y, max.z }, color, duration);
    Draw_DebugLine({ min.x, min.y, max.z }, { min.x, min.y, min.z }, color, duration);

    // Top square
    Draw_DebugLine({ min.x, max.y, min.z }, { max.x, max.y, min.z }, color, duration);
    Draw_DebugLine({ max.x, max.y, min.z }, { max.x, max.y, max.z }, color, duration);
    Draw_DebugLine({ max.x, max.y, max.z }, { min.x, max.y, max.z }, color, duration);
    Draw_DebugLine({ min.x, max.y, max.z }, { min.x, max.y, min.z }, color, duration);

    // 4 Vertical Pillars
    Draw_DebugLine({ min.x, min.y, min.z }, { min.x, max.y, min.z }, color, duration);
    Draw_DebugLine({ max.x, min.y, min.z }, { max.x, max.y, min.z }, color, duration);
    Draw_DebugLine({ max.x, min.y, max.z }, { max.x, max.y, max.z }, color, duration);
    Draw_DebugLine({ min.x, min.y, max.z }, { min.x, max.y, max.z }, color, duration);
}

void U_Raycast::Draw_DebugPoint(const Vector3D &point, const COLOR &color, FLOAT duration) {
    FLOAT s = 0.1f;
    Draw_DebugLine({ point.x - s, point.y, point.z }, { point.x + s, point.y, point.z }, color, duration);
    Draw_DebugLine({ point.x, point.y - s, point.z }, { point.x, point.y + s, point.z }, color, duration);
    Draw_DebugLine({ point.x, point.y, point.z - s }, { point.x, point.y, point.z + s }, color, duration);
}

void U_Raycast::Clear_Debug() {
    std::lock_guard<std::mutex> lock(s_DebugMutex);
    s_DebugLines.clear();
}

void U_Raycast::Tick_Debug(FLOAT deltaTime) {
    std::lock_guard<std::mutex> lock(s_DebugMutex);
    std::erase_if(s_DebugLines, [deltaTime](DebugLine &line) {
        line.Duration -= deltaTime;
        return line.Duration <= 0.0f;
    });
}

void U_Raycast::Render_Debug() {
    std::lock_guard<std::mutex> lock(s_DebugMutex);
    if (s_DebugLines.empty()) return;

    if (!s_BeamMeshInit) {
        s_BeamMesh = M_Mesh::Create_Cube(COLOR_WHITE);
        s_BeamMeshInit = true;
    }

    for (const auto &line : s_DebugLines) {
        Vector3D diff = line.End - line.Start;
        FLOAT len = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        if (len < 0.001f) continue;

        Vector3D center = (line.Start + line.End) * 0.5f;
        FLOAT yawDeg   = std::atan2(diff.x, diff.z) * U_Math::RadToDeg;
        FLOAT pitchDeg = -std::asin(U_Math::Clamp_MinMax(diff.y / len, -1.0f, 1.0f)) * U_Math::RadToDeg;

        Vector3D rot  = { pitchDeg, yawDeg, 0.0f };
        Vector3D size = { 0.04f, 0.04f, len };

        RHI_Manager::Draw_Mesh(s_BeamMesh, center, rot, size, line.Color, 0.95f, 0);
    }
}
