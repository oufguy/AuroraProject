#include <M_Lighting.h>
#include <M_Mesh.h>
#include <algorithm>

Vector3D               M_Lighting::s_SunDirection      = Vector3D(0.45f, 0.65f, -0.60f).Normalized();
COLOR                  M_Lighting::s_SunColor          = COLOR(255, 250, 235, 255);
FLOAT                  M_Lighting::s_SunIntensity      = 1.0f;
COLOR                  M_Lighting::s_AmbientColor      = COLOR(70, 75, 90, 255);
FLOAT                  M_Lighting::s_AmbientIntensity  = 0.35f;

UINT                   M_Lighting::s_NextPointLightID  = 0;
std::vector<PointLight> M_Lighting::s_PointLights;

void M_Lighting::Init() {
    s_SunDirection     = Vector3D(0.45f, 0.65f, -0.60f).Normalized();
    s_SunColor         = COLOR(255, 250, 235, 255);
    s_SunIntensity     = 1.0f;
    s_AmbientColor     = COLOR(70, 75, 90, 255);
    s_AmbientIntensity = 0.35f;
    s_PointLights.clear();
    Sync_To_Renderer();
}

void M_Lighting::Set_SunDirection(const Vector3D &direction) {
    s_SunDirection = direction.Normalized();
    Sync_To_Renderer();
}

void M_Lighting::Set_SunAngles(FLOAT pitchDeg, FLOAT yawDeg) {
    FLOAT rx = pitchDeg * U_Math::DegToRad;
    FLOAT ry = yawDeg   * U_Math::DegToRad;

    FLOAT cosPitch = U_Math::Radians_Cos(rx);
    FLOAT sinPitch = U_Math::Radians_Sin(rx);
    FLOAT cosYaw   = U_Math::Radians_Cos(ry);
    FLOAT sinYaw   = U_Math::Radians_Sin(ry);

    Vector3D dir;
    dir.x = cosPitch * sinYaw;
    dir.y = sinPitch;
    dir.z = -cosPitch * cosYaw;

    Set_SunDirection(dir);
}

void M_Lighting::Set_SunColor(const COLOR &color) {
    s_SunColor = color;
}

void M_Lighting::Set_SunIntensity(FLOAT intensity) {
    s_SunIntensity = U_Math::Clamp_Min(intensity, 0.0f);
}

void M_Lighting::Set_AmbientColor(const COLOR &color) {
    s_AmbientColor = color;
}

void M_Lighting::Set_AmbientIntensity(FLOAT intensity) {
    s_AmbientIntensity = U_Math::Clamp_MinMax(intensity, 0.0f, 1.0f);
    Sync_To_Renderer();
}

UINT M_Lighting::Add_PointLight(
    const Vector3D &position,
    FLOAT           radius,
    const COLOR    &color,
    FLOAT           intensity)
{
    PointLight light;
    light.ID        = ++s_NextPointLightID;
    light.Position  = position;
    light.Radius    = U_Math::Clamp_Min(radius, 0.1f);
    light.Color     = color;
    light.Intensity = U_Math::Clamp_Min(intensity, 0.0f);
    light.bActive   = true;

    s_PointLights.push_back(light);
    return light.ID;
}

BOOL M_Lighting::Remove_PointLight(UINT lightID) {
    auto it = std::remove_if(s_PointLights.begin(), s_PointLights.end(),
        [lightID](const PointLight &pl) { return pl.ID == lightID; });

    if (it != s_PointLights.end()) {
        s_PointLights.erase(it, s_PointLights.end());
        return true;
    }
    return false;
}

PointLight *M_Lighting::Get_PointLight(UINT lightID) {
    for (auto &pl : s_PointLights) {
        if (pl.ID == lightID) return &pl;
    }
    return nullptr;
}

void M_Lighting::Clear_PointLights() {
    s_PointLights.clear();
}

COLOR M_Lighting::Evaluate_Lighting(const Vector3D &worldPos, const Vector3D &surfaceNormal) {
    // 1. Ambient lighting base
    FLOAT r = (static_cast<FLOAT>(s_AmbientColor.r) / 255.0f) * s_AmbientIntensity;
    FLOAT g = (static_cast<FLOAT>(s_AmbientColor.g) / 255.0f) * s_AmbientIntensity;
    FLOAT b = (static_cast<FLOAT>(s_AmbientColor.b) / 255.0f) * s_AmbientIntensity;

    // 2. Sunlight directional diffuse (N . L)
    FLOAT dotNL = surfaceNormal.Dot(s_SunDirection);
    if (dotNL > 0.0f) {
        FLOAT sunFactor = dotNL * s_SunIntensity;
        r += (static_cast<FLOAT>(s_SunColor.r) / 255.0f) * sunFactor;
        g += (static_cast<FLOAT>(s_SunColor.g) / 255.0f) * sunFactor;
        b += (static_cast<FLOAT>(s_SunColor.b) / 255.0f) * sunFactor;
    }

    // 3. Point lights accumulation with quadratic falloff
    for (const auto &pl : s_PointLights) {
        if (!pl.bActive) continue;

        Vector3D toLight = pl.Position - worldPos;
        FLOAT distSq = toLight.LengthSq();
        FLOAT radiusSq = pl.Radius * pl.Radius;

        if (distSq < radiusSq) {
            FLOAT dist = toLight.Length();
            FLOAT atten = 1.0f - (dist / pl.Radius);
            atten = atten * atten; // Quadratic falloff

            Vector3D lightDir = (dist > 0.0001f) ? (toLight / dist) : Vector3D(0.0f, 1.0f, 0.0f);
            FLOAT pointDot = surfaceNormal.Dot(lightDir);
            if (pointDot > 0.0f) {
                FLOAT lightFactor = pointDot * pl.Intensity * atten;
                r += (static_cast<FLOAT>(pl.Color.r) / 255.0f) * lightFactor;
                g += (static_cast<FLOAT>(pl.Color.g) / 255.0f) * lightFactor;
                b += (static_cast<FLOAT>(pl.Color.b) / 255.0f) * lightFactor;
            }
        }
    }

    // Clamp to [0, 255]
    BYTE outR = static_cast<BYTE>(U_Math::Clamp_MinMax(r * 255.0f, 0.0f, 255.0f));
    BYTE outG = static_cast<BYTE>(U_Math::Clamp_MinMax(g * 255.0f, 0.0f, 255.0f));
    BYTE outB = static_cast<BYTE>(U_Math::Clamp_MinMax(b * 255.0f, 0.0f, 255.0f));

    return COLOR(outR, outG, outB, 255);
}

FLOAT M_Lighting::Evaluate_IntensityAt(const Vector3D &worldPos) {
    FLOAT intensity = s_AmbientIntensity + s_SunIntensity * 0.5f;

    for (const auto &pl : s_PointLights) {
        if (!pl.bActive) continue;
        FLOAT dist = (pl.Position - worldPos).Length();
        if (dist < pl.Radius) {
            FLOAT atten = 1.0f - (dist / pl.Radius);
            intensity += pl.Intensity * atten * atten;
        }
    }

    return intensity;
}

void M_Lighting::Sync_To_Renderer() {
    M_Mesh::Set_SunDirection(s_SunDirection);
    M_Mesh::Set_AmbientIntensity(s_AmbientIntensity);
}
