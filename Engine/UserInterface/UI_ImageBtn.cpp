#include <pch.h>
#include <UI_ImageBtn.h>

UI_ImageBtn::UI_ImageBtn(const FilePath &normal,
                         const FilePath &hover,
                         const FilePath &pressed,
                         const FilePath &disabled,
                         FLOAT x, FLOAT y, FLOAT w, FLOAT h)
    : UI_ImageBox("", x, y, w, h)
{
    SetTextures(normal, hover, pressed, disabled);
}

UI_ImageBtn::UI_ImageBtn(UINT normal, UINT hover, UINT pressed, UINT disabled,
                         FLOAT x, FLOAT y, FLOAT w, FLOAT h)
    : UI_ImageBox(normal, x, y, w, h)
{
    SetTextures(normal, hover, pressed, disabled);
}

void UI_ImageBtn::SetTextures(const FilePath &normal,
                               const FilePath &hover,
                               const FilePath &pressed,
                               const FilePath &disabled)
{
    NormalTex   = !normal.empty()   ? M_Texture::Load_UI(normal)   : 0;
    HoverTex    = !hover.empty()    ? M_Texture::Load_UI(hover)    : 0;
    PressedTex  = !pressed.empty()  ? M_Texture::Load_UI(pressed)  : 0;
    DisabledTex = !disabled.empty() ? M_Texture::Load_UI(disabled) : 0;
    TextureID   = NormalTex;

    if ((Size.x <= 0.0f || Size.y <= 0.0f) && NormalTex != 0) {
        const TextureData &data = M_Texture::Get(NormalTex);
        if (data.Is_Valid()) {
            Size.x = static_cast<FLOAT>(data.Width);
            Size.y = static_cast<FLOAT>(data.Height);
        }
    }
}

void UI_ImageBtn::SetTextures(UINT normal, UINT hover, UINT pressed, UINT disabled) {
    NormalTex   = normal;
    HoverTex    = hover;
    PressedTex  = pressed;
    DisabledTex = disabled;
    TextureID   = NormalTex;

    if ((Size.x <= 0.0f || Size.y <= 0.0f) && NormalTex != 0) {
        const TextureData &data = M_Texture::Get(NormalTex);
        if (data.Is_Valid()) {
            Size.x = static_cast<FLOAT>(data.Width);
            Size.y = static_cast<FLOAT>(data.Height);
        }
    }
}

void UI_ImageBtn::SetNormal(const FilePath &path) {
    NormalTex = M_Texture::Load_UI(path);
    if (LockedState == -1 || LockedState == 0) TextureID = NormalTex;
}

void UI_ImageBtn::SetHover(const FilePath &path) {
    HoverTex = M_Texture::Load_UI(path);
    if (LockedState == 1) TextureID = HoverTex;
}

void UI_ImageBtn::SetPressed(const FilePath &path) {
    PressedTex = M_Texture::Load_UI(path);
    if (LockedState == 2) TextureID = PressedTex;
}

void UI_ImageBtn::SetDisabled(const FilePath &path) {
    DisabledTex = M_Texture::Load_UI(path);
    if (LockedState == 3) TextureID = DisabledTex;
}

void UI_ImageBtn::SetNormal(UINT textureID) {
    NormalTex = textureID;
    if (LockedState == -1 || LockedState == 0) TextureID = NormalTex;
}

void UI_ImageBtn::SetHover(UINT textureID) {
    HoverTex = textureID;
    if (LockedState == 1) TextureID = HoverTex;
}

void UI_ImageBtn::SetPressed(UINT textureID) {
    PressedTex = textureID;
    if (LockedState == 2) TextureID = PressedTex;
}

void UI_ImageBtn::SetDisabled(UINT textureID) {
    DisabledTex = textureID;
    if (LockedState == 3) TextureID = DisabledTex;
}

BYTE UI_ImageBtn::GetCurrentState() const {
    if (LockedState >= 0 && LockedState <= 3) {
        return static_cast<BYTE>(LockedState);
    }
    if (!Is_Hovered()) {
        return static_cast<BYTE>(ButtonState::Normal);
    }
    if (I_Mouse::Is_Down(MouseButton::MB_Left)) {
        return static_cast<BYTE>(ButtonState::Pressed);
    }
    return static_cast<BYTE>(ButtonState::Hover);
}

BOOL UI_ImageBtn::Is_Down() const {
    if (LockedState == static_cast<INT>(ButtonState::Disabled)) return false;
    return Is_Hovered() && I_Mouse::Is_Down(MouseButton::MB_Left);
}

BOOL UI_ImageBtn::Is_Clicked() const {
    if (LockedState == static_cast<INT>(ButtonState::Disabled)) return false;
    return Is_Hovered() && I_Mouse::Is_Pressed(MouseButton::MB_Left);
}

void UI_ImageBtn::Render() const {
    BYTE state = GetCurrentState();
    UINT activeTex = 0;
    COLOR activeTint = Tint;

    switch (state) {
        case 0: // Normal
            activeTex = NormalTex;
            break;
        case 1: // Hover
            activeTex = (HoverTex != 0) ? HoverTex : NormalTex;
            if (HoverTex == 0) activeTint = COLOR(240, 240, 255, 255);
            break;
        case 2: // Pressed
            activeTex = (PressedTex != 0) ? PressedTex : NormalTex;
            if (PressedTex == 0) activeTint = COLOR(180, 180, 200, 255);
            break;
        case 3: // Disabled
            activeTex = (DisabledTex != 0) ? DisabledTex : NormalTex;
            if (DisabledTex == 0) activeTint = COLOR(100, 100, 100, 160);
            break;
        default:
            activeTex = NormalTex;
            break;
    }

    if (activeTex == 0 || Size.x <= 0.0f || Size.y <= 0.0f || Transparency <= 0.001f) return;
    RHI_Manager::Draw_UI(activeTex, Position, Size, activeTint, Transparency);
}
