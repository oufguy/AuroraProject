#pragma once

#include <C_Entity.h>
#include <M_Mesh.h>
#include <U_Raycast.h>
#include <M_Material.h>

// ============================================================================
// C_StaticMesh (Fixed Environment Props, Rocks, Trees, Buildings)
// ============================================================================
// Inherits transform and direction vectors from C_Entity.
// Adds visual 3D mesh, texture, scale, and color tint.
// ZERO memory or CPU cost for physics velocity!
// ============================================================================
class C_StaticMesh : public C_Entity {
public:
    Vector3D           Size         = Vector3D(1.0f, 1.0f, 1.0f);
    MeshData           Mesh;
    UINT               TextureID    = 0;
    FLOAT              Transparency = 1.0f;
    COLOR              Color        = COLOR_WHITE;
    MaterialParameters Material     = M_Material::Create_Default();

    C_StaticMesh();

    void Set_Transparency(FLOAT value);
    void Set_Texture(UINT textureID)    { TextureID = textureID; }
    void Set_Size(const Vector3D &size) { Size = size; }
    void Set_Color(const COLOR &c)      { Color = c; }

    // Computes world-space axis-aligned bounding box from mesh local bounds, scale, rotation, and position
    [[nodiscard]] AABB Get_WorldAABB() const override;

    [[nodiscard]] constexpr FLOAT Get_FinalTransparency(FLOAT textureAlpha = 1.0f) const {
        return bIsVisible ? (textureAlpha * Transparency) : 0.0f;
    }
};

// Legacy compatibility alias
using E_StaticMesh = C_StaticMesh;
