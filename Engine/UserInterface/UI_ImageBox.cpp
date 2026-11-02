#include <pch.h>
#include <UI_ImageBox.h>

UI_ImageBox::UI_ImageBox(const FilePath &path, FLOAT x, FLOAT y, FLOAT w, FLOAT h)
    : UI_Frame(x, y, w, h)
{
    SetTexture(path);
}

UI_ImageBox::UI_ImageBox(UINT textureID, FLOAT x, FLOAT y, FLOAT w, FLOAT h)
    : UI_Frame(x, y, w, h)
{
    SetTexture(textureID);
}

void UI_ImageBox::SetTexture(const FilePath &path) {
    if (path.empty()) {
        TextureID = 0;
        return;
    }

    TextureID = M_Texture::Load_UI(path);
    if ((Size.x <= 0.0f || Size.y <= 0.0f) && TextureID != 0) {
        const TextureData &data = M_Texture::Get(TextureID);
        if (data.Is_Valid()) {
            Size.x = static_cast<FLOAT>(data.Width);
            Size.y = static_cast<FLOAT>(data.Height);
        }
    }
}

void UI_ImageBox::SetTexture(UINT textureID) {
    TextureID = textureID;
    if ((Size.x <= 0.0f || Size.y <= 0.0f) && TextureID != 0) {
        const TextureData &data = M_Texture::Get(TextureID);
        if (data.Is_Valid()) {
            Size.x = static_cast<FLOAT>(data.Width);
            Size.y = static_cast<FLOAT>(data.Height);
        }
    }
}

void UI_ImageBox::Render() const {
    if (TextureID == 0 || Size.x <= 0.0f || Size.y <= 0.0f || Transparency <= 0.001f) return;
    RHI_Manager::Draw_UI(TextureID, Position, Size, Tint, Transparency);
}
