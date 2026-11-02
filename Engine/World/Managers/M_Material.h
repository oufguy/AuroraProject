#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Math.h>

// ============================================================================
// Aurora Engine — Material Shading & Surface Parameters (M_Material)
// ============================================================================
// Single Responsibility:
//   Defines physical and visual surface parameters for 3D meshes:
//   - Diffuse Tint Color
//   - Ambient and Specular Response Factors
//   - Shininess & Roughness
//   - Unlit & Emissive Flags
// ============================================================================

struct MaterialParameters {
    COLOR DiffuseTint    = COLOR_WHITE;
    FLOAT AmbientFactor  = 1.0f;
    FLOAT SpecularFactor = 0.5f;
    FLOAT Shininess      = 32.0f;
    FLOAT Roughness      = 0.5f;
    BOOL  bUnlit         = false;
};

class M_Material {
public:
    // Standard balanced material (smooth diffuse + moderate specular)
    [[nodiscard]] static MaterialParameters Create_Default();

    // Matte material (diffuse only, zero specular shine, rough surface e.g. dirt, rock, cloth)
    [[nodiscard]] static MaterialParameters Create_Matte(const COLOR &diffuse = COLOR_WHITE);

    // Shiny metallic material (intense specular highlight, high shininess e.g. sword, armor)
    [[nodiscard]] static MaterialParameters Create_Metallic(const COLOR &diffuse = COLOR_WHITE, FLOAT shininess = 64.0f);

    // Completely unlit material (ignores sun and shadows, full brightness e.g. UI, debug quads, lasers)
    [[nodiscard]] static MaterialParameters Create_Unlit(const COLOR &color = COLOR_WHITE);

    // Emissive glowing material (over-bright self-illumination e.g. magic runes, neon, fire)
    [[nodiscard]] static MaterialParameters Create_Emissive(const COLOR &glowColor, FLOAT intensity = 1.5f);

    // Evaluates final modulated surface color given material parameters and incident light color
    [[nodiscard]] static COLOR Evaluate_SurfaceColor(
        const MaterialParameters &material,
        const COLOR              &incidentLight,
        const COLOR              &entityBaseColor);
};
