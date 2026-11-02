#include <pch.h>
#include <UI_Text.h>

UI_Text::UI_Text(StringView text, FLOAT x, FLOAT y, const COLOR &color, BOOL bShadow, FLOAT scale)
    : UI_Frame(x, y, 0.0f, 0.0f), Text(text), Color(color), ShadowColor(COLOR_BLACK), bShadow(bShadow), Scale(scale)
{
    UpdateSize();
}

void UI_Text::SetText(StringView text) {
    Text = String(text);
    UpdateSize();
}

void UI_Text::UpdateSize() {
    Vector2D dim = M_Font::Measure_String(Text, Scale);
    Size.x = dim.x;
    Size.y = dim.y;
}

void UI_Text::Render() const {
    if (Text.empty() || Transparency <= 0.001f) return;

    if (bShadow) {
        M_Font::Draw_String_Shadowed(Text, Position.x, Position.y, Color, ShadowColor, Scale);
    } else {
        M_Font::Draw_String(Text, Position.x, Position.y, Color, Scale);
    }
}
