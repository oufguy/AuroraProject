#include <M_Material.h>

MaterialParameters M_Material::Create_Default() {
    MaterialParameters mat;
    mat.DiffuseTint    = COLOR_WHITE;
    mat.AmbientFactor  = 1.0f;
    mat.SpecularFactor = 0.5f;
    mat.Shininess      = 32.0f;
    mat.Roughness      = 0.5f;
    mat.bUnlit         = false;
    return mat;
}

MaterialParameters M_Material::Create_Matte(const COLOR &diffuse) {
    MaterialParameters mat;
    mat.DiffuseTint    = diffuse;
    mat.AmbientFactor  = 1.0f;
    mat.SpecularFactor = 0.0f;
    mat.Shininess      = 4.0f;
    mat.Roughness      = 1.0f;
    mat.bUnlit         = false;
    return mat;
}

MaterialParameters M_Material::Create_Metallic(const COLOR &diffuse, FLOAT shininess) {
    MaterialParameters mat;
    mat.DiffuseTint    = diffuse;
    mat.AmbientFactor  = 0.8f;
    mat.SpecularFactor = 1.0f;
    mat.Shininess      = U_Math::Clamp_MinMax(shininess, 8.0f, 256.0f);
    mat.Roughness      = 0.15f;
    mat.bUnlit         = false;
    return mat;
}

MaterialParameters M_Material::Create_Unlit(const COLOR &color) {
    MaterialParameters mat;
    mat.DiffuseTint    = color;
    mat.AmbientFactor  = 1.0f;
    mat.SpecularFactor = 0.0f;
    mat.Shininess      = 1.0f;
    mat.Roughness      = 1.0f;
    mat.bUnlit         = true;
    return mat;
}

MaterialParameters M_Material::Create_Emissive(const COLOR &glowColor, FLOAT intensity) {
    MaterialParameters mat;
    mat.DiffuseTint    = glowColor;
    mat.AmbientFactor  = U_Math::Clamp_Min(intensity, 1.0f);
    mat.SpecularFactor = 0.0f;
    mat.Shininess      = 1.0f;
    mat.Roughness      = 0.0f;
    mat.bUnlit         = true;
    return mat;
}

COLOR M_Material::Evaluate_SurfaceColor(
    const MaterialParameters &material,
    const COLOR              &incidentLight,
    const COLOR              &entityBaseColor)
{
    if (material.bUnlit) {
        return material.DiffuseTint;
    }

    constexpr FLOAT INV_255 = 1.0f / 255.0f;

    FLOAT baseR = static_cast<FLOAT>(entityBaseColor.r) * INV_255;
    FLOAT baseG = static_cast<FLOAT>(entityBaseColor.g) * INV_255;
    FLOAT baseB = static_cast<FLOAT>(entityBaseColor.b) * INV_255;

    FLOAT tintR = static_cast<FLOAT>(material.DiffuseTint.r) * INV_255;
    FLOAT tintG = static_cast<FLOAT>(material.DiffuseTint.g) * INV_255;
    FLOAT tintB = static_cast<FLOAT>(material.DiffuseTint.b) * INV_255;

    FLOAT lightR = static_cast<FLOAT>(incidentLight.r) * INV_255;
    FLOAT lightG = static_cast<FLOAT>(incidentLight.g) * INV_255;
    FLOAT lightB = static_cast<FLOAT>(incidentLight.b) * INV_255;

    FLOAT finalR = baseR * tintR * lightR * 255.0f;
    FLOAT finalG = baseG * tintG * lightG * 255.0f;
    FLOAT finalB = baseB * tintB * lightB * 255.0f;

    BYTE outR = static_cast<BYTE>(U_Math::Clamp_MinMax(finalR, 0.0f, 255.0f));
    BYTE outG = static_cast<BYTE>(U_Math::Clamp_MinMax(finalG, 0.0f, 255.0f));
    BYTE outB = static_cast<BYTE>(U_Math::Clamp_MinMax(finalB, 0.0f, 255.0f));

    return COLOR(outR, outG, outB, entityBaseColor.a);
}
