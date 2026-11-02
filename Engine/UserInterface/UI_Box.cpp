#include <pch.h>
#include <UI_Box.h>

UI_Box::UI_Box(FLOAT x, FLOAT y, FLOAT w, FLOAT h, const COLOR &color)
    : UI_Frame(x, y, w, h), Color(color)
{
}

void UI_Box::Render() const {
    if (Size.x <= 0.0f || Size.y <= 0.0f || Transparency <= 0.001f) return;
    RHI_Manager::Draw_UI(0, Position, Size, Color, Transparency);
}
