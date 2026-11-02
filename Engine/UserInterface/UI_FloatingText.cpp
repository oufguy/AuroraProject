#include <UI_FloatingText.h>
#include <M_Font.h>
#include <U_Math.h>
#include <format>
#include <algorithm>

std::vector<FloatingTextItem> UI_FloatingText::s_Items;

void UI_FloatingText::Spawn_Damage(const Vector3D &worldPos, INT damage, BOOL bCritical) {
    FloatingTextItem item;
    item.WorldPosition = worldPos + Vector3D(0.0f, 1.5f, 0.0f);
    item.Velocity      = Vector3D(0.0f, bCritical ? 1.6f : 1.2f, 0.0f);
    item.Lifetime      = bCritical ? 1.4f : 1.1f;
    item.MaxLifetime   = item.Lifetime;
    item.bCritical     = bCritical;

    if (bCritical) {
        item.Text        = std::format("! {} !", damage);
        item.Color       = COLOR(255, 65, 54, 255); // Vivid Crimson Red
        item.ShadowColor = COLOR(0, 0, 0, 255);
        item.Scale       = 1.25f;
    } else {
        item.Text        = std::format("-{}", damage);
        item.Color       = COLOR(255, 220, 50, 255); // Golden Combat Yellow
        item.ShadowColor = COLOR(0, 0, 0, 255);
        item.Scale       = 1.0f;
    }

    if (s_Items.size() >= MAX_FLOATING_ITEMS) {
        s_Items.erase(s_Items.begin());
    }
    s_Items.push_back(std::move(item));
}

void UI_FloatingText::Spawn_Heal(const Vector3D &worldPos, INT healAmount) {
    FloatingTextItem item;
    item.WorldPosition = worldPos + Vector3D(0.0f, 1.5f, 0.0f);
    item.Velocity      = Vector3D(0.0f, 1.0f, 0.0f);
    item.Lifetime      = 1.2f;
    item.MaxLifetime   = 1.2f;
    item.Text          = std::format("+{}", healAmount);
    item.Color         = COLOR(46, 204, 113, 255); // Emerald Green
    item.ShadowColor   = COLOR(0, 0, 0, 255);
    item.Scale         = 1.05f;
    item.bCritical     = false;

    if (s_Items.size() >= MAX_FLOATING_ITEMS) {
        s_Items.erase(s_Items.begin());
    }
    s_Items.push_back(std::move(item));
}

void UI_FloatingText::Spawn_Text(
    const Vector3D &worldPos,
    StringView      text,
    const COLOR    &color,
    FLOAT           scale,
    FLOAT           duration,
    const COLOR    &shadowColor)
{
    FloatingTextItem item;
    item.WorldPosition = worldPos + Vector3D(0.0f, 1.5f, 0.0f);
    item.Velocity      = Vector3D(0.0f, 1.0f, 0.0f);
    item.Lifetime      = U_Math::Clamp_Min(duration, 0.2f);
    item.MaxLifetime   = item.Lifetime;
    item.Text          = String(text);
    item.Color         = color;
    item.ShadowColor   = shadowColor;
    item.Scale         = scale;
    item.bCritical     = false;

    if (s_Items.size() >= MAX_FLOATING_ITEMS) {
        s_Items.erase(s_Items.begin());
    }
    s_Items.push_back(std::move(item));
}

void UI_FloatingText::Tick(FLOAT deltaTime) {
    for (auto &item : s_Items) {
        item.WorldPosition += item.Velocity * deltaTime;
        item.Lifetime      -= deltaTime;
        // Subtle upward drag
        item.Velocity.y    *= 0.985f;
    }

    std::erase_if(s_Items, [](const FloatingTextItem &item) {
        return item.Lifetime <= 0.0f;
    });
}

void UI_FloatingText::Render(const C_Camera &camera) {
    if (s_Items.empty()) return;

    for (const auto &item : s_Items) {
        Vector2D screenPos;
        if (!camera.World_To_Screen(item.WorldPosition, screenPos)) {
            continue;
        }

        // Smooth alpha fade during final 35% of lifetime
        FLOAT alphaFactor = 1.0f;
        FLOAT fadeWindow = item.MaxLifetime * 0.35f;
        if (item.Lifetime < fadeWindow && fadeWindow > 0.001f) {
            alphaFactor = item.Lifetime / fadeWindow;
        }
        alphaFactor = U_Math::Clamp_MinMax(alphaFactor, 0.0f, 1.0f);

        COLOR drawColor = item.Color;
        drawColor.a = static_cast<BYTE>(static_cast<FLOAT>(item.Color.a) * alphaFactor);

        COLOR shadowColor = item.ShadowColor;
        shadowColor.a = static_cast<BYTE>(static_cast<FLOAT>(item.ShadowColor.a) * alphaFactor);

        Vector2D textSize = M_Font::Measure_String(item.Text, item.Scale);
        FLOAT centeredX = screenPos.x - (textSize.x * 0.5f);
        FLOAT centeredY = screenPos.y - (textSize.y * 0.5f);

        M_Font::Draw_String_Shadowed(item.Text, centeredX, centeredY, drawColor, shadowColor, item.Scale);
    }
}

void UI_FloatingText::Clear() {
    s_Items.clear();
}
