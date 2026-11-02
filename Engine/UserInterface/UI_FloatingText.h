#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <C_Camera.h>

// ============================================================================
// Aurora Engine — Floating Combat Damage & World Text Manager
// ============================================================================
// Single Responsibility:
//   Manages on-screen floating text popups in 3D world space:
//   - Damage numbers (normal & critical hits)
//   - Heals & Mana recovery
//   - Combat status notifications ("Evaded", "Blocked", "Miss!")
//   - Upward floating physics with easing and alpha fade-out
// ============================================================================

struct FloatingTextItem {
    Vector3D WorldPosition;
    Vector3D Velocity;
    String   Text;
    COLOR    Color;
    COLOR    ShadowColor;
    FLOAT    Scale        = 1.0f;
    FLOAT    Lifetime     = 1.2f;
    FLOAT    MaxLifetime  = 1.2f;
    BOOL     bCritical    = false;
};

class UI_FloatingText {
private:
    static std::vector<FloatingTextItem> s_Items;
    static constexpr size_t MAX_FLOATING_ITEMS = 256;

public:
    // Spawns combat damage popup at world coordinates (Yellow for normal, Red/Orange with larger font for Critical)
    static void Spawn_Damage(const Vector3D &worldPos, INT damage, BOOL bCritical = false);

    // Spawns healing popup in bright green
    static void Spawn_Heal(const Vector3D &worldPos, INT healAmount);

    // Spawns custom floating text in world space with color, font scale, and duration
    static void Spawn_Text(
        const Vector3D &worldPos,
        StringView      text,
        const COLOR    &color       = COLOR_YELLOW,
        FLOAT           scale       = 1.0f,
        FLOAT           duration    = 1.2f,
        const COLOR    &shadowColor = COLOR_BLACK);

    // Updates positions and lifetimes of all active floating text popups
    static void Tick(FLOAT deltaTime);

    // Projects active 3D world popups to 2D screen coordinates and renders drop-shadowed text
    static void Render(const C_Camera &camera);

    // Clears all active floating text
    static void Clear();

    // Returns current active text popup count
    [[nodiscard]] static inline size_t Get_ActiveCount() {
        return s_Items.size();
    }
};
