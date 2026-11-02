#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Math.h>
#include <U_Colors.h>

// ============================================================================
// Aurora Engine — World Lighting Manager (Sun Direction, Point Lights, Ambient)
// ============================================================================
// Single Responsibility:
//   Authoritative manager for 3D world lighting:
//   - Directional Sunlight (Direction vector, pitch/yaw, sun color, intensity)
//   - Sky Ambient Light (Color, ambient intensity)
//   - Dynamic 3D Point Lights (Position, radius, color, intensity falloff)
//   - CPU lighting evaluation & renderer synchronization
// ============================================================================

struct PointLight {
    UINT     ID        = 0;
    Vector3D Position  = { 0.0f, 0.0f, 0.0f };
    FLOAT    Radius    = 10.0f;
    COLOR    Color     = COLOR_WHITE;
    FLOAT    Intensity = 1.0f;
    BOOL     bActive   = true;
};

class M_Lighting {
private:
    static Vector3D               s_SunDirection;
    static COLOR                  s_SunColor;
    static FLOAT                  s_SunIntensity;
    static COLOR                  s_AmbientColor;
    static FLOAT                  s_AmbientIntensity;

    static UINT                   s_NextPointLightID;
    static std::vector<PointLight> s_PointLights;

public:
    // Initializes default sunlight (warm daylight) and ambient sky
    static void Init();

    // ========================================================================
    // Directional Sunlight
    // ========================================================================

    // Sets the normalized 3D direction vector pointing toward the sun
    static void Set_SunDirection(const Vector3D &direction);

    // Sets sun direction from Pitch and Yaw angles in degrees (e.g. Pitch = 45.0f, Yaw = 30.0f)
    static void Set_SunAngles(FLOAT pitchDeg, FLOAT yawDeg);

    [[nodiscard]] static inline const Vector3D &Get_SunDirection() {
        return s_SunDirection;
    }

    // Sets the color of the directional sunlight
    static void Set_SunColor(const COLOR &color);

    [[nodiscard]] static inline const COLOR &Get_SunColor() {
        return s_SunColor;
    }

    // Sets the brightness multiplier of sunlight (1.0f = standard daylight)
    static void Set_SunIntensity(FLOAT intensity);

    [[nodiscard]] static inline FLOAT Get_SunIntensity() {
        return s_SunIntensity;
    }

    // ========================================================================
    // Ambient Sky Shading
    // ========================================================================

    // Sets the ambient sky color reflected off surfaces not facing direct sun
    static void Set_AmbientColor(const COLOR &color);

    [[nodiscard]] static inline const COLOR &Get_AmbientColor() {
        return s_AmbientColor;
    }

    // Sets the ambient brightness factor (0.0f = completely black shadows, 1.0f = fully lit)
    static void Set_AmbientIntensity(FLOAT intensity);

    [[nodiscard]] static inline FLOAT Get_AmbientIntensity() {
        return s_AmbientIntensity;
    }

    // ========================================================================
    // 3D Point Lights
    // ========================================================================

    // Spawns a new point light in the world. Returns unique light ID.
    static UINT Add_PointLight(
        const Vector3D &position,
        FLOAT           radius    = 10.0f,
        const COLOR    &color     = COLOR_WHITE,
        FLOAT           intensity = 1.0f);

    // Removes a point light by ID
    static BOOL Remove_PointLight(UINT lightID);

    // Returns a pointer to a point light by ID, or nullptr if not found
    static PointLight *Get_PointLight(UINT lightID);

    // Clears all active point lights
    static void Clear_PointLights();

    [[nodiscard]] static inline const std::vector<PointLight> &Get_PointLights() {
        return s_PointLights;
    }

    [[nodiscard]] static inline size_t Get_PointLightCount() {
        return s_PointLights.size();
    }

    // ========================================================================
    // Lighting Calculations & Pipeline Synchronization
    // ========================================================================

    // Evaluates the full combined surface lighting (ambient + sunlight + point lights) at a given world position and surface normal
    [[nodiscard]] static COLOR Evaluate_Lighting(const Vector3D &worldPos, const Vector3D &surfaceNormal);

    // Returns total light intensity (0.0f to 1.0f+) at a world position
    [[nodiscard]] static FLOAT Evaluate_IntensityAt(const Vector3D &worldPos);

    // Synchronizes active sun direction and ambient intensity directly into M_Mesh GPU shader constants
    static void Sync_To_Renderer();
};
