#include <pch.h>
#include <UI_Frame.h>

UI_Frame::UI_Frame(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    Position.x   = x;
    Position.y   = y;
    Size.x       = w;
    Size.y       = h;
    Transparency = 1.0f;
}
