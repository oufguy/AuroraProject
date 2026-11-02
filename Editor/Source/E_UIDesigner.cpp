#include <E_UIDesigner.h>
#include <E_ProjectManager.h>
#include <I_Mouse.h>
#include <I_Keyboard.h>
#include <M_Window.h>
#include <M_File.h>
#include <M_Asset.h>
#include <M_Sound.h>
#include <RHI_Manager.h>

Vector2D              E_UIDesigner::s_CanvasPan          = { 40.0f, 30.0f };
FLOAT                 E_UIDesigner::s_CanvasZoom         = 0.50f;
BOOL                  E_UIDesigner::s_IsPanning          = false;
Vector2D              E_UIDesigner::s_LastPanMouse       = { 0.0f, 0.0f };
Vector2D              E_UIDesigner::s_ArtboardSize       = { 1920.0f, 1080.0f };
Array<DesignerElement> E_UIDesigner::s_Elements;
INT                   E_UIDesigner::s_SelectedIndex      = -1;
INT                   E_UIDesigner::s_NextElementID      = 1;
BOOL                  E_UIDesigner::s_IsDraggingElement  = false;
INT                   E_UIDesigner::s_ActiveResizeHandle = -1;
Vector2D              E_UIDesigner::s_DragStartMouse     = { 0.0f, 0.0f };
Vector2D              E_UIDesigner::s_ElementStartPos    = { 0.0f, 0.0f };
Vector2D              E_UIDesigner::s_ElementStartSize   = { 0.0f, 0.0f };
Array<String>         E_UIDesigner::s_DiscoveredImages;
INT                   E_UIDesigner::s_LeftTab            = 0;
String                E_UIDesigner::s_StatusMessage      = "";
FLOAT                 E_UIDesigner::s_StatusTimer        = 0.0f;
BOOL                  E_UIDesigner::s_IsEditingText      = false;
BOOL                  E_UIDesigner::s_IsEditingName      = false;

namespace {
    inline BOOL Check_Rect(const Vector2D &m, FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
        return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
    }
}

Vector2D E_UIDesigner::Canvas_To_Screen(const Vector2D &canvasPos, const Vector2D &viewportMin) {
    return {
        viewportMin.x + s_CanvasPan.x + (canvasPos.x * s_CanvasZoom),
        viewportMin.y + s_CanvasPan.y + (canvasPos.y * s_CanvasZoom)
    };
}

Vector2D E_UIDesigner::Screen_To_Canvas(const Vector2D &screenPos, const Vector2D &viewportMin) {
    if (s_CanvasZoom <= 0.001f) return { 0.0f, 0.0f };
    return {
        (screenPos.x - viewportMin.x - s_CanvasPan.x) / s_CanvasZoom,
        (screenPos.y - viewportMin.y - s_CanvasPan.y) / s_CanvasZoom
    };
}

void E_UIDesigner::Scan_UI_Assets() {
    s_DiscoveredImages.clear();

    FilePath projRoot = E_ProjectManager::Get_ActiveProject().RootDirectory;
    if (projRoot.empty()) projRoot = ".";

    FilePath uiDir = projRoot / "Assets" / "UI";
    if (std::filesystem::exists(uiDir)) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(uiDir)) {
            if (entry.is_regular_file()) {
                String ext = entry.path().extension().string();
                for (char &c : ext) c = static_cast<char>(std::tolower(c));
                if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") {
                    s_DiscoveredImages.push_back(entry.path().lexically_relative(projRoot).generic_string());
                }
            }
        }
    }

    // Also scan Assets/ for loose textures
    FilePath assetsDir = projRoot / "Assets";
    if (std::filesystem::exists(assetsDir)) {
        for (const auto &entry : std::filesystem::directory_iterator(assetsDir)) {
            if (entry.is_regular_file()) {
                String ext = entry.path().extension().string();
                for (char &c : ext) c = static_cast<char>(std::tolower(c));
                if (ext == ".png" || ext == ".bmp" || ext == ".jpg") {
                    String rel = entry.path().lexically_relative(projRoot).generic_string();
                    if (std::find(s_DiscoveredImages.begin(), s_DiscoveredImages.end(), rel) == s_DiscoveredImages.end()) {
                        s_DiscoveredImages.push_back(rel);
                    }
                }
            }
        }
    }
}

void E_UIDesigner::Ensure_Sample_Elements() {
    if (!s_Elements.empty()) return;

    // 1. Dark Backdrop Panel
    DesignerElement bg;
    bg.Name      = "Inventory_Backdrop";
    bg.Type      = UIElementType::Box;
    bg.Position  = { 480.0f, 180.0f };
    bg.Size      = { 460.0f, 540.0f };
    bg.TintColor = COLOR(24, 30, 42, 230);
    bg.Alpha     = 0.92f;
    s_Elements.push_back(bg);

    // 2. Window Header Title
    DesignerElement title;
    title.Name        = "Header_Title";
    title.Type        = UIElementType::Text;
    title.Position    = { 505.0f, 205.0f };
    title.Size        = { 260.0f, 32.0f };
    title.TextContent = "Character Inventory";
    title.TintColor   = COLOR_GOLD;
    title.FontScale   = 1.1f;
    s_Elements.push_back(title);

    // 3. Close Button
    DesignerElement closeBtn;
    closeBtn.Name        = "Btn_Close";
    closeBtn.Type        = UIElementType::Button;
    closeBtn.Position    = { 885.0f, 195.0f };
    closeBtn.Size        = { 36.0f, 32.0f };
    closeBtn.TintColor   = COLOR(192, 57, 43, 255);
    closeBtn.TextContent = "X";
    closeBtn.FontScale   = 0.9f;
    s_Elements.push_back(closeBtn);

    // 4. Sample Item Slots Grid (4x4)
    for (INT row = 0; row < 3; ++row) {
        for (INT col = 0; col < 4; ++col) {
            DesignerElement slot;
            slot.Name      = std::format("Slot_{}_{}", row + 1, col + 1);
            slot.Type      = UIElementType::Box;
            slot.Position  = { 510.0f + col * 95.0f, 270.0f + row * 95.0f };
            slot.Size      = { 80.0f, 80.0f };
            slot.TintColor = COLOR(38, 48, 65, 255);
            slot.Alpha     = 0.95f;
            s_Elements.push_back(slot);
        }
    }

    s_SelectedIndex = 0;
}

void E_UIDesigner::Init() {
    Scan_UI_Assets();
    Ensure_Sample_Elements();
}

void E_UIDesigner::Tick(FLOAT deltaTime, const Vector2D &viewportMin, const Vector2D &viewportMax) {
    if (s_StatusTimer > 0.0f) {
        s_StatusTimer -= deltaTime;
    }

    Vector2D mouse = I_Mouse::Get_Position();
    FLOAT vpW = viewportMax.x - viewportMin.x;
    FLOAT vpH = viewportMax.y - viewportMin.y;
    BOOL bInViewport = Check_Rect(mouse, viewportMin.x, viewportMin.y, vpW, vpH);

    // 1. Canvas Pan (Middle Mouse Drag or Space + Left Click Drag)
    BOOL bMiddleDown = I_Mouse::Is_Down(MouseButton::MB_Middle);
    BOOL bSpaceLeft  = I_Keyboard::Is_Down(Key::Space) && I_Mouse::Is_Down(MouseButton::MB_Left);

    if (bInViewport && (bMiddleDown || bSpaceLeft)) {
        if (!s_IsPanning) {
            s_IsPanning     = true;
            s_LastPanMouse  = mouse;
        } else {
            Vector2D delta = mouse - s_LastPanMouse;
            s_CanvasPan   += delta;
            s_LastPanMouse = mouse;
        }
    } else {
        s_IsPanning = false;
    }

    // 2. Canvas Zoom (Mouse Wheel smoothly scaling around cursor)
    if (bInViewport && !s_IsPanning) {
        FLOAT scroll = I_Mouse::Get_Scroll();
        if (std::abs(scroll) > 0.01f) {
            Vector2D mouseCanvasBefore = Screen_To_Canvas(mouse, viewportMin);
            s_CanvasZoom = std::clamp(s_CanvasZoom + (scroll * 0.06f), 0.15f, 3.0f);
            Vector2D mouseCanvasAfter = Screen_To_Canvas(mouse, viewportMin);
            s_CanvasPan += (mouseCanvasAfter - mouseCanvasBefore) * s_CanvasZoom;
        }
    }

    // 3. Selection & Manipulation of Elements
    BOOL bLeftPressed  = I_Mouse::Is_Pressed(MouseButton::MB_Left);
    BOOL bLeftHeld     = I_Mouse::Is_Down(MouseButton::MB_Left);
    BOOL bLeftReleased = I_Mouse::Is_Released(MouseButton::MB_Left);

    Vector2D mouseCanvas = Screen_To_Canvas(mouse, viewportMin);

    if (bInViewport && bLeftPressed && !bMiddleDown && !I_Keyboard::Is_Down(Key::Space)) {
        // First check if clicking a resize handle of current selection
        s_ActiveResizeHandle = -1;
        if (s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
            const auto &sel = s_Elements[s_SelectedIndex];
            Vector2D scTopLeft = Canvas_To_Screen(sel.Position, viewportMin);
            Vector2D scBotRight = Canvas_To_Screen(sel.Position + sel.Size, viewportMin);
            FLOAT hSize = 8.0f;

            if (Check_Rect(mouse, scTopLeft.x - hSize, scTopLeft.y - hSize, hSize * 2, hSize * 2)) s_ActiveResizeHandle = 0;
            else if (Check_Rect(mouse, scBotRight.x - hSize, scTopLeft.y - hSize, hSize * 2, hSize * 2)) s_ActiveResizeHandle = 1;
            else if (Check_Rect(mouse, scBotRight.x - hSize, scBotRight.y - hSize, hSize * 2, hSize * 2)) s_ActiveResizeHandle = 2;
            else if (Check_Rect(mouse, scTopLeft.x - hSize, scBotRight.y - hSize, hSize * 2, hSize * 2)) s_ActiveResizeHandle = 3;

            if (s_ActiveResizeHandle >= 0) {
                s_DragStartMouse   = mouse;
                s_ElementStartPos  = sel.Position;
                s_ElementStartSize = sel.Size;
            }
        }

        // If no resize handle was clicked, check elements from top to bottom (reverse order)
        if (s_ActiveResizeHandle < 0) {
            INT hitIndex = -1;
            for (INT i = static_cast<INT>(s_Elements.size()) - 1; i >= 0; --i) {
                const auto &el = s_Elements[i];
                if (!el.bVisible || el.bLocked) continue;
                if (mouseCanvas.x >= el.Position.x && mouseCanvas.x <= el.Position.x + el.Size.x &&
                    mouseCanvas.y >= el.Position.y && mouseCanvas.y <= el.Position.y + el.Size.y) {
                    hitIndex = i;
                    break;
                }
            }

            s_SelectedIndex = hitIndex;
            if (s_SelectedIndex >= 0) {
                s_IsDraggingElement = true;
                s_DragStartMouse    = mouse;
                s_ElementStartPos   = s_Elements[s_SelectedIndex].Position;
                s_ElementStartSize  = s_Elements[s_SelectedIndex].Size;
            }
        }
    }

    // Process Active Dragging (Position Move)
    if (s_IsDraggingElement && bLeftHeld && s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
        Vector2D mouseDelta = (mouse - s_DragStartMouse) * (1.0f / s_CanvasZoom);
        auto &sel = s_Elements[s_SelectedIndex];
        sel.Position = s_ElementStartPos + mouseDelta;
        // Snap to grid (10px) if Ctrl is held
        if (I_Keyboard::Is_Down(Key::Ctrl)) {
            sel.Position.x = std::round(sel.Position.x / 10.0f) * 10.0f;
            sel.Position.y = std::round(sel.Position.y / 10.0f) * 10.0f;
        }
    }

    // Process Active Resizing
    if (s_ActiveResizeHandle >= 0 && bLeftHeld && s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
        Vector2D mouseDelta = (mouse - s_DragStartMouse) * (1.0f / s_CanvasZoom);
        auto &sel = s_Elements[s_SelectedIndex];

        if (s_ActiveResizeHandle == 2) { // Bottom Right
            sel.Size.x = std::max(20.0f, s_ElementStartSize.x + mouseDelta.x);
            sel.Size.y = std::max(20.0f, s_ElementStartSize.y + mouseDelta.y);
        } else if (s_ActiveResizeHandle == 1) { // Top Right
            sel.Size.x = std::max(20.0f, s_ElementStartSize.x + mouseDelta.x);
            sel.Position.y = s_ElementStartPos.y + mouseDelta.y;
            sel.Size.y = std::max(20.0f, s_ElementStartSize.y - mouseDelta.y);
        } else if (s_ActiveResizeHandle == 0) { // Top Left
            sel.Position.x = s_ElementStartPos.x + mouseDelta.x;
            sel.Size.x = std::max(20.0f, s_ElementStartSize.x - mouseDelta.x);
            sel.Position.y = s_ElementStartPos.y + mouseDelta.y;
            sel.Size.y = std::max(20.0f, s_ElementStartSize.y - mouseDelta.y);
        } else if (s_ActiveResizeHandle == 3) { // Bottom Left
            sel.Position.x = s_ElementStartPos.x + mouseDelta.x;
            sel.Size.x = std::max(20.0f, s_ElementStartSize.x - mouseDelta.x);
            sel.Size.y = std::max(20.0f, s_ElementStartSize.y + mouseDelta.y);
        }
    }

    if (bLeftReleased) {
        s_IsDraggingElement  = false;
        s_ActiveResizeHandle = -1;
    }

    // 4. Keyboard Shortcuts
    if (bInViewport && !s_IsEditingName && !s_IsEditingText) {
        if (I_Keyboard::Is_Pressed(Key::Delete) || I_Keyboard::Is_Pressed(Key::Backspace)) {
            Delete_Selected();
        }
        if (I_Keyboard::Is_Down(Key::Ctrl) && I_Keyboard::Is_Pressed(Key::D)) {
            Duplicate_Selected();
        }
        if (I_Keyboard::Is_Down(Key::Ctrl) && I_Keyboard::Is_Pressed(Key::S)) {
            Save_Layout();
        }

        // Figma-style Arrow Keys Nudge
        if (s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
            FLOAT step = (I_Keyboard::Is_Down(Key::Shift) ? 10.0f : 1.0f);
            if (I_Keyboard::Is_Pressed(Key::Left))  s_Elements[s_SelectedIndex].Position.x -= step;
            if (I_Keyboard::Is_Pressed(Key::Right)) s_Elements[s_SelectedIndex].Position.x += step;
            if (I_Keyboard::Is_Pressed(Key::Up))    s_Elements[s_SelectedIndex].Position.y -= step;
            if (I_Keyboard::Is_Pressed(Key::Down))  s_Elements[s_SelectedIndex].Position.y += step;
        }
    }
}

void E_UIDesigner::Render(const Vector2D &viewportMin, const Vector2D &viewportMax) {
    FLOAT totalW = viewportMax.x - viewportMin.x;
    FLOAT totalH = viewportMax.y - viewportMin.y;

    FLOAT sideW = 270.0f;
    FLOAT inspW = 300.0f;
    FLOAT canvasX = viewportMin.x + sideW;
    FLOAT canvasY = viewportMin.y;
    FLOAT canvasW = totalW - sideW - inspW;
    FLOAT canvasH = totalH;

    // 1. Central Design Canvas
    Draw_Canvas({ canvasX, canvasY }, { canvasX + canvasW, canvasY + canvasH });

    // 2. Floating Bottom Pill Toolbar
    Draw_FloatingPill(canvasX, canvasY, canvasW, canvasH);

    // 3. Left Sidebar: Layers & PNG Assets
    Draw_LeftSidebar(viewportMin.x, viewportMin.y, sideW, totalH);

    // 4. Right Sidebar: Properties Inspector
    Draw_RightInspector(viewportMax.x - inspW, viewportMin.y, inspW, totalH);

    // Status Notification Toast
    if (s_StatusTimer > 0.0f) {
        FLOAT a = std::clamp(s_StatusTimer, 0.0f, 1.0f);
        UI::Draw_Box(canvasX + (canvasW * 0.5f) - 180.0f, canvasY + 20.0f, 360.0f, 32.0f, COLOR(20, 30, 42, static_cast<BYTE>(240 * a)));
        UI::Draw_Text(s_StatusMessage, canvasX + (canvasW * 0.5f) - 165.0f, canvasY + 28.0f, COLOR(46, 204, 113, static_cast<BYTE>(255 * a)), 0.85f);
    }
}

void E_UIDesigner::Draw_Canvas(const Vector2D &viewportMin, const Vector2D &viewportMax) {
    FLOAT cw = viewportMax.x - viewportMin.x;
    FLOAT ch = viewportMax.y - viewportMin.y;

    // Canvas Background (Figma dark slate)
    UI::Draw_Box(viewportMin.x, viewportMin.y, cw, ch, COLOR(18, 20, 26, 255));

    // Artboard Bounds in screen space
    Vector2D abTopLeft  = Canvas_To_Screen({ 0.0f, 0.0f }, viewportMin);
    Vector2D abBotRight = Canvas_To_Screen(s_ArtboardSize, viewportMin);
    FLOAT abW = abBotRight.x - abTopLeft.x;
    FLOAT abH = abBotRight.y - abTopLeft.y;

    // Artboard Backdrop (Game screen preview)
    UI::Draw_Box(abTopLeft.x, abTopLeft.y, abW, abH, COLOR(12, 14, 18, 255));

    // Artboard Border Outline
    UI::Draw_Box(abTopLeft.x - 1.0f, abTopLeft.y - 1.0f, abW + 2.0f, 1.0f, COLOR(70, 85, 110, 255));
    UI::Draw_Box(abTopLeft.x - 1.0f, abBotRight.y,        abW + 2.0f, 1.0f, COLOR(70, 85, 110, 255));
    UI::Draw_Box(abTopLeft.x - 1.0f, abTopLeft.y,        1.0f, abH,         COLOR(70, 85, 110, 255));
    UI::Draw_Box(abBotRight.x,       abTopLeft.y,        1.0f, abH,         COLOR(70, 85, 110, 255));

    // Artboard Title Header
    String abTitle = std::format("Game Artboard — 1920x1080 (Zoom: {}%)", static_cast<INT>(s_CanvasZoom * 100.0f));
    UI::Draw_Text(abTitle, abTopLeft.x + 5.0f, abTopLeft.y - 20.0f, COLOR(140, 160, 190, 255), 0.75f);

    // Render Elements in z-order
    for (size_t i = 0; i < s_Elements.size(); ++i) {
        const auto &el = s_Elements[i];
        if (!el.bVisible) continue;

        Vector2D elScreenPos = Canvas_To_Screen(el.Position, viewportMin);
        Vector2D elScreenSize = el.Size * s_CanvasZoom;

        if (el.Type == UIElementType::Box) {
            UI::Draw_Box(elScreenPos.x, elScreenPos.y, elScreenSize.x, elScreenSize.y, el.TintColor, el.Alpha);
        } else if (el.Type == UIElementType::Image) {
            if (el.TextureID != 0) {
                RHI_Manager::Draw_UI(el.TextureID, elScreenPos, elScreenSize, el.TintColor, el.Alpha);
            } else {
                UI::Draw_Box(elScreenPos.x, elScreenPos.y, elScreenSize.x, elScreenSize.y, COLOR(45, 60, 85, 255), el.Alpha);
                UI::Draw_Text("[🖼 Image]", elScreenPos.x + 10.0f, elScreenPos.y + (elScreenSize.y * 0.5f) - 8.0f, COLOR_WHITE, 0.75f);
            }
        } else if (el.Type == UIElementType::Button) {
            UI::Draw_Box(elScreenPos.x, elScreenPos.y, elScreenSize.x, elScreenSize.y, el.TintColor, el.Alpha);
            Vector2D txtSize = UI::Measure_Text(el.TextContent, el.FontScale * s_CanvasZoom);
            FLOAT tx = elScreenPos.x + (elScreenSize.x - txtSize.x) * 0.5f;
            FLOAT ty = elScreenPos.y + (elScreenSize.y - txtSize.y) * 0.5f;
            UI::Draw_Text_Shadowed(el.TextContent, tx, ty, COLOR_WHITE, COLOR_BLACK, el.FontScale * s_CanvasZoom);
        } else if (el.Type == UIElementType::Text) {
            UI::Draw_Text_Shadowed(el.TextContent, elScreenPos.x, elScreenPos.y, el.TintColor, COLOR_BLACK, el.FontScale * s_CanvasZoom);
        }

        // Selection Bounding Box & 4 Resize Handles
        if (static_cast<INT>(i) == s_SelectedIndex) {
            UI::Draw_Box(elScreenPos.x - 1.0f, elScreenPos.y - 1.0f, elScreenSize.x + 2.0f, 1.0f, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x - 1.0f, elScreenPos.y + elScreenSize.y, elScreenSize.x + 2.0f, 1.0f, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x - 1.0f, elScreenPos.y, 1.0f, elScreenSize.y, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x + elScreenSize.x, elScreenPos.y, 1.0f, elScreenSize.y, COLOR_CYAN);

            FLOAT hs = 6.0f;
            // 4 Corner handles
            UI::Draw_Box(elScreenPos.x - hs * 0.5f, elScreenPos.y - hs * 0.5f, hs, hs, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x + elScreenSize.x - hs * 0.5f, elScreenPos.y - hs * 0.5f, hs, hs, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x + elScreenSize.x - hs * 0.5f, elScreenPos.y + elScreenSize.y - hs * 0.5f, hs, hs, COLOR_CYAN);
            UI::Draw_Box(elScreenPos.x - hs * 0.5f, elScreenPos.y + elScreenSize.y - hs * 0.5f, hs, hs, COLOR_CYAN);
        }
    }
}

void E_UIDesigner::Draw_InScene_Overlay(const Vector2D &viewportMin, const Vector2D &viewportMax) {
    FLOAT vpW = viewportMax.x - viewportMin.x;
    FLOAT vpH = viewportMax.y - viewportMin.y;
    if (vpW <= 0.0f || vpH <= 0.0f) return;

    // Scale artboard (1920x1080) to viewport
    FLOAT scaleX = vpW / s_ArtboardSize.x;
    FLOAT scaleY = vpH / s_ArtboardSize.y;

    for (size_t i = 0; i < s_Elements.size(); ++i) {
        const auto &el = s_Elements[i];
        if (!el.bVisible) continue;

        Vector2D elPos = {
            viewportMin.x + (el.Position.x * scaleX),
            viewportMin.y + (el.Position.y * scaleY)
        };
        Vector2D elSize = {
            el.Size.x * scaleX,
            el.Size.y * scaleY
        };

        if (el.Type == UIElementType::Box) {
            UI::Draw_Box(elPos.x, elPos.y, elSize.x, elSize.y, el.TintColor, el.Alpha);
        } else if (el.Type == UIElementType::Image) {
            if (el.TextureID != 0) {
                RHI_Manager::Draw_UI(el.TextureID, elPos, elSize, el.TintColor, el.Alpha);
            } else {
                UI::Draw_Box(elPos.x, elPos.y, elSize.x, elSize.y, COLOR(45, 60, 85, 200), el.Alpha);
            }
        } else if (el.Type == UIElementType::Button) {
            UI::Draw_Box(elPos.x, elPos.y, elSize.x, elSize.y, el.TintColor, el.Alpha);
            FLOAT fontS = el.FontScale * std::min(scaleX, scaleY);
            Vector2D txtSize = UI::Measure_Text(el.TextContent, fontS);
            FLOAT tx = elPos.x + (elSize.x - txtSize.x) * 0.5f;
            FLOAT ty = elPos.y + (elSize.y - txtSize.y) * 0.5f;
            UI::Draw_Text_Shadowed(el.TextContent, tx, ty, COLOR_WHITE, COLOR_BLACK, fontS);
        } else if (el.Type == UIElementType::Text) {
            FLOAT fontS = el.FontScale * std::min(scaleX, scaleY);
            UI::Draw_Text_Shadowed(el.TextContent, elPos.x, elPos.y, el.TintColor, COLOR_BLACK, fontS);
        }
    }
}

void E_UIDesigner::Draw_LeftSidebar(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Sidebar Backdrop
    UI::Draw_Box(x, y, w, h, COLOR(22, 26, 35, 255));
    UI::Draw_Box(x + w - 1.0f, y, 1.0f, h, COLOR(45, 55, 75, 255));

    // Tab Header: [ Layers ]  [ PNG Assets ]
    FLOAT tabW = w * 0.5f;
    BOOL hovT0 = Check_Rect(mouse, x, y, tabW, 32.0f);
    BOOL hovT1 = Check_Rect(mouse, x + tabW, y, tabW, 32.0f);

    UI::Draw_Box(x, y, tabW, 32.0f, (s_LeftTab == 0) ? COLOR(35, 45, 62, 255) : (hovT0 ? COLOR(28, 35, 48, 255) : COLOR(18, 22, 30, 255)));
    UI::Draw_Text("Layers Hierarchy", x + 15.0f, y + 9.0f, (s_LeftTab == 0) ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.8f);

    UI::Draw_Box(x + tabW, y, tabW, 32.0f, (s_LeftTab == 1) ? COLOR(35, 45, 62, 255) : (hovT1 ? COLOR(28, 35, 48, 255) : COLOR(18, 22, 30, 255)));
    UI::Draw_Text("PNG Assets", x + tabW + 20.0f, y + 9.0f, (s_LeftTab == 1) ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.8f);

    if (hovT0 && bClick) s_LeftTab = 0;
    if (hovT1 && bClick) s_LeftTab = 1;

    FLOAT curY = y + 40.0f;

    // TAB 0: LAYERS HIERARCHY
    if (s_LeftTab == 0) {
        UI::Draw_Text("ROOT CANVAS (1920x1080)", x + 12.0f, curY, COLOR(120, 140, 170, 255), 0.75f);
        curY += 22.0f;

        for (INT i = static_cast<INT>(s_Elements.size()) - 1; i >= 0; --i) {
            auto &el = s_Elements[i];
            BOOL isSel = (i == s_SelectedIndex);
            BOOL hov = Check_Rect(mouse, x + 6.0f, curY, w - 12.0f, 26.0f);

            COLOR rowCol = isSel ? COLOR(41, 128, 185, 255) : (hov ? COLOR(32, 40, 55, 255) : COLOR(22, 26, 35, 255));
            UI::Draw_Box(x + 6.0f, curY, w - 12.0f, 26.0f, rowCol);

            // Icon by type
            const char *typeIcon = "[□]";
            if (el.Type == UIElementType::Image) typeIcon = "[🖼]";
            else if (el.Type == UIElementType::Button) typeIcon = "[❖]";
            else if (el.Type == UIElementType::Text) typeIcon = "[T]";

            UI::Draw_Text(typeIcon, x + 12.0f, curY + 6.0f, isSel ? COLOR_CYAN : COLOR(180, 200, 230, 255), 0.75f);
            UI::Draw_Text(el.Name, x + 38.0f, curY + 6.0f, isSel ? COLOR_WHITE : COLOR(210, 225, 245, 255), 0.8f);

            if (hov && bClick) {
                s_SelectedIndex = i;
            }

            curY += 28.0f;
            if (curY > y + h - 40.0f) break;
        }
    }
    // TAB 1: PNG ASSETS (Roblox Drag-and-Drop / Click-to-Place style)
    else {
        UI::Draw_Text("Click to place PNG on Canvas:", x + 12.0f, curY, COLOR(120, 140, 170, 255), 0.75f);
        curY += 22.0f;

        if (s_DiscoveredImages.empty()) {
            UI::Draw_Text("No PNG images found in Assets/UI/.", x + 12.0f, curY, COLOR(180, 130, 90, 255), 0.75f);
            UI::Draw_Text("Drop your UI PNGs into Assets/UI/!", x + 12.0f, curY + 20.0f, COLOR(140, 155, 180, 255), 0.75f);
        } else {
            for (const auto &imgPath : s_DiscoveredImages) {
                BOOL hov = Check_Rect(mouse, x + 6.0f, curY, w - 12.0f, 32.0f);
                UI::Draw_Box(x + 6.0f, curY, w - 12.0f, 32.0f, hov ? COLOR(35, 45, 65, 255) : COLOR(25, 30, 42, 255));
                UI::Draw_Text("[PNG]", x + 12.0f, curY + 9.0f, COLOR_CYAN, 0.75f);

                FilePath p(imgPath);
                String filename = p.filename().string();
                UI::Draw_Text(filename, x + 55.0f, curY + 9.0f, COLOR_WHITE, 0.8f);

                if (hov && bClick) {
                    Add_Image(imgPath);
                }

                curY += 36.0f;
                if (curY > y + h - 40.0f) break;
            }
        }
    }
}

void E_UIDesigner::Draw_RightInspector(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Inspector Backdrop
    UI::Draw_Box(x, y, w, h, COLOR(22, 26, 35, 255));
    UI::Draw_Box(x, y, 1.0f, h, COLOR(45, 55, 75, 255));

    // Header
    UI::Draw_Text("PROPERTIES INSPECTOR", x + 14.0f, y + 10.0f, COLOR_WHITE, 0.95f);
    UI::Draw_Box(x, y + 32.0f, w, 1.0f, COLOR(45, 55, 75, 255));

    FLOAT curY = y + 44.0f;

    if (s_SelectedIndex < 0 || s_SelectedIndex >= static_cast<INT>(s_Elements.size())) {
        UI::Draw_Text("No UI element selected.", x + 14.0f, curY + 20.0f, COLOR(140, 155, 180, 255), 0.85f);
        UI::Draw_Text("Click an element on the canvas", x + 14.0f, curY + 45.0f, COLOR(100, 115, 140, 255), 0.75f);
        UI::Draw_Text("or use the bottom toolbar to add one.", x + 14.0f, curY + 65.0f, COLOR(100, 115, 140, 255), 0.75f);
        return;
    }

    auto &sel = s_Elements[s_SelectedIndex];

    // Element Name
    UI::Draw_Text("Name:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
    UI::Draw_Box(x + 65.0f, curY - 4.0f, w - 80.0f, 24.0f, COLOR(16, 20, 28, 255));
    UI::Draw_Text(sel.Name, x + 72.0f, curY + 1.0f, COLOR_WHITE, 0.8f);
    curY += 34.0f;

    // Transform: Position (X, Y)
    UI::Draw_Text("Position:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
    UI::Draw_Text(std::format("X: {:.0f}   Y: {:.0f}", sel.Position.x, sel.Position.y), x + 85.0f, curY, COLOR_CYAN, 0.85f);

    // Nudge buttons for X
    BOOL hovXMinus = Check_Rect(mouse, x + 210.0f, curY - 2.0f, 18.0f, 18.0f);
    BOOL hovXPlus  = Check_Rect(mouse, x + 232.0f, curY - 2.0f, 18.0f, 18.0f);
    UI::Draw_Box(x + 210.0f, curY - 2.0f, 18.0f, 18.0f, hovXMinus ? COLOR(52, 152, 219, 255) : COLOR(35, 45, 60, 255));
    UI::Draw_Text("<", x + 215.0f, curY + 1.0f, COLOR_WHITE, 0.75f);
    UI::Draw_Box(x + 232.0f, curY - 2.0f, 18.0f, 18.0f, hovXPlus ? COLOR(52, 152, 219, 255) : COLOR(35, 45, 60, 255));
    UI::Draw_Text(">", x + 237.0f, curY + 1.0f, COLOR_WHITE, 0.75f);

    if (hovXMinus && bClick) sel.Position.x -= (I_Keyboard::Is_Down(Key::Shift) ? 10.0f : 1.0f);
    if (hovXPlus && bClick)  sel.Position.x += (I_Keyboard::Is_Down(Key::Shift) ? 10.0f : 1.0f);

    curY += 28.0f;

    // Transform: Size (W, H)
    UI::Draw_Text("Size:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
    UI::Draw_Text(std::format("W: {:.0f}   H: {:.0f}", sel.Size.x, sel.Size.y), x + 85.0f, curY, COLOR_CYAN, 0.85f);

    BOOL hovWMinus = Check_Rect(mouse, x + 210.0f, curY - 2.0f, 18.0f, 18.0f);
    BOOL hovWPlus  = Check_Rect(mouse, x + 232.0f, curY - 2.0f, 18.0f, 18.0f);
    UI::Draw_Box(x + 210.0f, curY - 2.0f, 18.0f, 18.0f, hovWMinus ? COLOR(52, 152, 219, 255) : COLOR(35, 45, 60, 255));
    UI::Draw_Text("-", x + 216.0f, curY + 1.0f, COLOR_WHITE, 0.75f);
    UI::Draw_Box(x + 232.0f, curY - 2.0f, 18.0f, 18.0f, hovWPlus ? COLOR(52, 152, 219, 255) : COLOR(35, 45, 60, 255));
    UI::Draw_Text("+", x + 236.0f, curY + 1.0f, COLOR_WHITE, 0.75f);

    if (hovWMinus && bClick) sel.Size.x = std::max(20.0f, sel.Size.x - 10.0f);
    if (hovWPlus && bClick)  sel.Size.x += 10.0f;

    curY += 34.0f;

    // Color Tint Presets
    UI::Draw_Text("Tint Color:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
    curY += 20.0f;

    static const COLOR swatches[] = {
        COLOR_WHITE,
        COLOR(24, 30, 42, 255),
        COLOR(41, 128, 185, 255),
        COLOR(46, 204, 113, 255),
        COLOR(231, 76, 60, 255),
        COLOR(241, 196, 15, 255),
        COLOR(155, 89, 182, 255)
    };

    FLOAT sx = x + 14.0f;
    for (const auto &col : swatches) {
        BOOL hovCol = Check_Rect(mouse, sx, curY, 28.0f, 22.0f);
        UI::Draw_Box(sx, curY, 28.0f, 22.0f, col);
        if (hovCol) {
            UI::Draw_Box(sx - 1.0f, curY - 1.0f, 30.0f, 24.0f, COLOR_WHITE, 0.5f);
            if (bClick) sel.TintColor = col;
        }
        sx += 36.0f;
    }
    curY += 36.0f;

    // Opacity / Alpha
    UI::Draw_Text(std::format("Opacity: {:.0f}%", sel.Alpha * 100.0f), x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
    BOOL hovA0 = Check_Rect(mouse, x + 130.0f, curY - 2.0f, 36.0f, 20.0f);
    BOOL hovA1 = Check_Rect(mouse, x + 172.0f, curY - 2.0f, 36.0f, 20.0f);
    BOOL hovA2 = Check_Rect(mouse, x + 214.0f, curY - 2.0f, 36.0f, 20.0f);

    UI::Draw_Box(x + 130.0f, curY - 2.0f, 36.0f, 20.0f, hovA0 ? COLOR(52, 152, 219, 255) : COLOR(30, 40, 55, 255));
    UI::Draw_Text("50%", x + 136.0f, curY + 2.0f, COLOR_WHITE, 0.7f);
    UI::Draw_Box(x + 172.0f, curY - 2.0f, 36.0f, 20.0f, hovA1 ? COLOR(52, 152, 219, 255) : COLOR(30, 40, 55, 255));
    UI::Draw_Text("80%", x + 178.0f, curY + 2.0f, COLOR_WHITE, 0.7f);
    UI::Draw_Box(x + 214.0f, curY - 2.0f, 36.0f, 20.0f, hovA2 ? COLOR(52, 152, 219, 255) : COLOR(30, 40, 55, 255));
    UI::Draw_Text("100%", x + 218.0f, curY + 2.0f, COLOR_WHITE, 0.7f);

    if (hovA0 && bClick) sel.Alpha = 0.50f;
    if (hovA1 && bClick) sel.Alpha = 0.80f;
    if (hovA2 && bClick) sel.Alpha = 1.00f;
    curY += 34.0f;

    // Text Properties (if Button or Text)
    if (sel.Type == UIElementType::Text || sel.Type == UIElementType::Button) {
        UI::Draw_Text("Text Content:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
        UI::Draw_Box(x + 14.0f, curY + 18.0f, w - 28.0f, 26.0f, COLOR(16, 20, 28, 255));
        UI::Draw_Text(sel.TextContent, x + 20.0f, curY + 23.0f, COLOR_WHITE, 0.8f);
        curY += 56.0f;
    }

    // Texture Path (if Image)
    if (sel.Type == UIElementType::Image) {
        UI::Draw_Text("PNG Texture:", x + 14.0f, curY, COLOR(160, 175, 200, 255), 0.8f);
        FilePath tp(sel.TexturePath);
        String tName = tp.empty() ? "(None — select from PNG Assets tab)" : tp.filename().string();
        UI::Draw_Text(tName, x + 14.0f, curY + 20.0f, COLOR_CYAN, 0.75f);
        curY += 46.0f;
    }

    // Export Buttons at Bottom of Inspector
    curY = y + h - 90.0f;

    BOOL hovSave = Check_Rect(mouse, x + 14.0f, curY, w - 28.0f, 32.0f);
    UI::Draw_Box(x + 14.0f, curY, w - 28.0f, 32.0f, hovSave ? COLOR(39, 174, 96, 255) : COLOR(46, 204, 113, 255));
    UI::Draw_Text("💾 Save Layout (.auroraui)", x + 35.0f, curY + 8.0f, COLOR_WHITE, 0.85f);
    if (hovSave && bClick) {
        Save_Layout();
    }

    curY += 40.0f;

    BOOL hovCopy = Check_Rect(mouse, x + 14.0f, curY, w - 28.0f, 32.0f);
    UI::Draw_Box(x + 14.0f, curY, w - 28.0f, 32.0f, hovCopy ? COLOR(41, 128, 185, 255) : COLOR(52, 152, 219, 255));
    UI::Draw_Text("📋 Copy C++ Code", x + 65.0f, curY + 8.0f, COLOR_WHITE, 0.85f);
    if (hovCopy && bClick) {
        String cppCode = Generate_Cpp_Snippet();
        s_StatusMessage = "C++ code generated & copied to log!";
        s_StatusTimer   = 3.5f;
    }
}

void E_UIDesigner::Draw_FloatingPill(FLOAT canvasX, FLOAT canvasY, FLOAT canvasW, FLOAT canvasH) {
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Pill geometry
    FLOAT pillW = 755.0f;
    FLOAT pillH = 42.0f;
    FLOAT pillX = canvasX + (canvasW - pillW) * 0.5f;
    FLOAT pillY = canvasY + canvasH - pillH - 24.0f;

    // Dark glass pill backdrop with border
    UI::Draw_Box(pillX, pillY, pillW, pillH, COLOR(22, 26, 36, 240));
    UI::Draw_Box(pillX - 1.0f, pillY - 1.0f, pillW + 2.0f, 1.0f, COLOR(55, 68, 92, 255));
    UI::Draw_Box(pillX - 1.0f, pillY + pillH, pillW + 2.0f, 1.0f, COLOR(55, 68, 92, 255));

    struct PillTool {
        const char *Label;
        FLOAT W;
        int ToolID;
    };

    static const PillTool tools[] = {
        { "[+ Box]",    68.0f, 1 },
        { "[+ Image]",  78.0f, 2 },
        { "[+ Button]", 80.0f, 3 },
        { "[+ Text]",   68.0f, 4 },
        { "[+ Inv]",    68.0f, 10 },
        { "[+ Hotbar]", 78.0f, 11 },
        { "[+ Status]", 78.0f, 12 },
        { "[+ Map]",    62.0f, 13 },
        { "[+ Chat]",   66.0f, 14 },
        { "[Delete]",   65.0f, 5 }
    };

    FLOAT curX = pillX + 10.0f;
    for (const auto &t : tools) {
        BOOL hov = Check_Rect(mouse, curX, pillY + 6.0f, t.W, 30.0f);
        COLOR bgCol = hov ? COLOR(41, 128, 185, 255) : COLOR(30, 38, 52, 255);
        if (t.ToolID >= 10 && t.ToolID <= 14) {
            bgCol = hov ? COLOR(142, 68, 173, 255) : COLOR(45, 30, 60, 255);
        } else if (t.ToolID == 5) {
            bgCol = hov ? COLOR(192, 57, 43, 255) : COLOR(40, 28, 35, 255);
        }

        UI::Draw_Box(curX, pillY + 6.0f, t.W, 30.0f, bgCol);
        UI::Draw_Text(t.Label, curX + 8.0f, pillY + 13.0f, COLOR_WHITE, 0.78f);

        if (hov && bClick) {
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
            if (t.ToolID == 1)       Add_Box();
            else if (t.ToolID == 2)  Add_Image();
            else if (t.ToolID == 3)  Add_Button();
            else if (t.ToolID == 4)  Add_Text();
            else if (t.ToolID == 10) Add_Inventory_Preset();
            else if (t.ToolID == 11) Add_ActionHotbar_Preset();
            else if (t.ToolID == 12) Add_PlayerStatus_Preset();
            else if (t.ToolID == 13) Add_Minimap_Preset();
            else if (t.ToolID == 14) Add_ChatBox_Preset();
            else if (t.ToolID == 5)  Delete_Selected();
        }

        curX += t.W + 6.0f;
    }
}

void E_UIDesigner::Add_Inventory_Preset() {
    Vector2D basePos = { 1350.0f, 280.0f };

    // Backdrop
    DesignerElement bg;
    bg.Name      = "Inv_Window";
    bg.Type      = UIElementType::Box;
    bg.Position  = basePos;
    bg.Size      = { 420.0f, 560.0f };
    bg.TintColor = COLOR(20, 24, 32, 245);
    bg.Alpha     = 0.95f;
    s_Elements.push_back(bg);

    // Title
    DesignerElement title;
    title.Name        = "Inv_Title";
    title.Type        = UIElementType::Text;
    title.Position    = { basePos.x + 20.0f, basePos.y + 16.0f };
    title.Size        = { 200.0f, 26.0f };
    title.TextContent = "Inventory";
    title.TintColor   = COLOR_GOLD;
    title.FontScale   = 1.0f;
    s_Elements.push_back(title);

    // Close button
    DesignerElement close;
    close.Name        = "Inv_Btn_Close";
    close.Type        = UIElementType::Button;
    close.Position    = { basePos.x + 375.0f, basePos.y + 12.0f };
    close.Size        = { 30.0f, 28.0f };
    close.TintColor   = COLOR(192, 57, 43, 255);
    close.TextContent = "X";
    close.FontScale   = 0.8f;
    s_Elements.push_back(close);

    // 4x6 Item Grid (24 slots)
    for (INT r = 0; r < 6; ++r) {
        for (INT c = 0; c < 4; ++c) {
            DesignerElement slot;
            slot.Name      = std::format("Inv_Slot_{}", r * 4 + c + 1);
            slot.Type      = UIElementType::Box;
            slot.Position  = { basePos.x + 20.0f + (c * 95.0f), basePos.y + 55.0f + (r * 75.0f) };
            slot.Size      = { 85.0f, 68.0f };
            slot.TintColor = COLOR(35, 42, 56, 255);
            slot.Alpha     = 0.95f;
            s_Elements.push_back(slot);
        }
    }

    // Gold counter
    DesignerElement gold;
    gold.Name        = "Inv_Gold_Text";
    gold.Type        = UIElementType::Text;
    gold.Position    = { basePos.x + 25.0f, basePos.y + 518.0f };
    gold.Size        = { 250.0f, 24.0f };
    gold.TextContent = "Gold: 250,000";
    gold.TintColor   = COLOR_GOLD;
    gold.FontScale   = 0.85f;
    s_Elements.push_back(gold);

    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    s_StatusMessage = "Spawned MMORPG Inventory Preset (24 Slots)!";
    s_StatusTimer   = 3.0f;
}

void E_UIDesigner::Add_ActionHotbar_Preset() {
    Vector2D basePos = { 600.0f, 980.0f };

    // 10 Action Slots
    const char *keys[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };
    for (INT i = 0; i < 10; ++i) {
        DesignerElement slot;
        slot.Name        = std::format("Hotbar_Slot_{}", keys[i]);
        slot.Type        = UIElementType::Button;
        slot.Position    = { basePos.x + (i * 72.0f), basePos.y };
        slot.Size        = { 64.0f, 64.0f };
        slot.TintColor   = COLOR(28, 36, 52, 240);
        slot.TextContent = keys[i];
        slot.FontScale   = 0.85f;
        s_Elements.push_back(slot);
    }

    // EXP Bar at bottom
    DesignerElement expBar;
    expBar.Name      = "Hotbar_EXP_Bar";
    expBar.Type      = UIElementType::Box;
    expBar.Position  = { basePos.x, basePos.y + 70.0f };
    expBar.Size      = { 712.0f, 12.0f };
    expBar.TintColor = COLOR(46, 204, 113, 255);
    expBar.Alpha     = 0.9f;
    s_Elements.push_back(expBar);

    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    s_StatusMessage = "Spawned Action Hotbar Preset (1-0 Keys + EXP Bar)!";
    s_StatusTimer   = 3.0f;
}

void E_UIDesigner::Add_PlayerStatus_Preset() {
    Vector2D basePos = { 40.0f, 35.0f };

    // Portrait Box
    DesignerElement portrait;
    portrait.Name      = "Status_Portrait";
    portrait.Type      = UIElementType::Box;
    portrait.Position  = basePos;
    portrait.Size      = { 70.0f, 70.0f };
    portrait.TintColor = COLOR(41, 128, 185, 255);
    s_Elements.push_back(portrait);

    // Player Name & Level
    DesignerElement name;
    name.Name        = "Status_PlayerName";
    name.Type        = UIElementType::Text;
    name.Position    = { basePos.x + 85.0f, basePos.y };
    name.Size        = { 180.0f, 22.0f };
    name.TextContent = "AuroraKnight  [Lv. 75]";
    name.TintColor   = COLOR_GOLD;
    name.FontScale   = 0.85f;
    s_Elements.push_back(name);

    // HP Bar (Red)
    DesignerElement hp;
    hp.Name      = "Status_HP_Bar";
    hp.Type      = UIElementType::Box;
    hp.Position  = { basePos.x + 85.0f, basePos.y + 26.0f };
    hp.Size      = { 220.0f, 18.0f };
    hp.TintColor = COLOR(231, 76, 60, 255);
    s_Elements.push_back(hp);

    // MP Bar (Blue)
    DesignerElement mp;
    mp.Name      = "Status_MP_Bar";
    mp.Type      = UIElementType::Box;
    mp.Position  = { basePos.x + 85.0f, basePos.y + 48.0f };
    mp.Size      = { 180.0f, 16.0f };
    mp.TintColor = COLOR(52, 152, 219, 255);
    s_Elements.push_back(mp);

    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    s_StatusMessage = "Spawned Player Status Preset (HP / MP / Portrait)!";
    s_StatusTimer   = 3.0f;
}

void E_UIDesigner::Add_Minimap_Preset() {
    Vector2D basePos = { 1680.0f, 35.0f };

    // Minimap frame
    DesignerElement map;
    map.Name      = "Minimap_Frame";
    map.Type      = UIElementType::Box;
    map.Position  = basePos;
    map.Size      = { 190.0f, 190.0f };
    map.TintColor = COLOR(18, 22, 30, 220);
    s_Elements.push_back(map);

    // Zone Title
    DesignerElement zone;
    zone.Name        = "Minimap_Zone";
    zone.Type        = UIElementType::Text;
    zone.Position    = { basePos.x + 10.0f, basePos.y + 195.0f };
    zone.Size        = { 170.0f, 20.0f };
    zone.TextContent = "Aurora Capital";
    zone.TintColor   = COLOR_CYAN;
    zone.FontScale   = 0.8f;
    s_Elements.push_back(zone);

    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    s_StatusMessage = "Spawned Minimap Preset!";
    s_StatusTimer   = 3.0f;
}

void E_UIDesigner::Add_ChatBox_Preset() {
    Vector2D basePos = { 40.0f, 780.0f };

    // Chat Box Backdrop
    DesignerElement bg;
    bg.Name      = "Chat_Backdrop";
    bg.Type      = UIElementType::Box;
    bg.Position  = basePos;
    bg.Size      = { 420.0f, 220.0f };
    bg.TintColor = COLOR(16, 20, 28, 215);
    bg.Alpha     = 0.88f;
    s_Elements.push_back(bg);

    // Chat Tabs: [All], [Guild], [Party], [Whisper]
    const char *tabs[] = { "All", "Guild", "Party", "Whisper" };
    for (INT i = 0; i < 4; ++i) {
        DesignerElement tab;
        tab.Name        = std::format("Chat_Tab_{}", tabs[i]);
        tab.Type        = UIElementType::Button;
        tab.Position    = { basePos.x + 8.0f + (i * 68.0f), basePos.y + 6.0f };
        tab.Size        = { 62.0f, 24.0f };
        tab.TintColor   = (i == 0) ? COLOR(52, 152, 219, 255) : COLOR(28, 36, 48, 255);
        tab.TextContent = tabs[i];
        tab.FontScale   = 0.72f;
        s_Elements.push_back(tab);
    }

    // Chat Message Log
    DesignerElement log;
    log.Name        = "Chat_Message_Log";
    log.Type        = UIElementType::Text;
    log.Position    = { basePos.x + 12.0f, basePos.y + 42.0f };
    log.Size        = { 395.0f, 130.0f };
    log.TextContent = "[System]: Welcome to Aurora World!\n[Guild] Player1: Anyone for Dungeon raid?\n[Party] Healer: Ready!";
    log.TintColor   = COLOR(220, 230, 245, 255);
    log.FontScale   = 0.75f;
    s_Elements.push_back(log);

    // Chat Input Bar
    DesignerElement input;
    input.Name      = "Chat_Input_Field";
    input.Type      = UIElementType::Box;
    input.Position  = { basePos.x + 8.0f, basePos.y + 182.0f };
    input.Size      = { 404.0f, 30.0f };
    input.TintColor = COLOR(24, 30, 42, 255);
    input.Alpha     = 0.95f;
    s_Elements.push_back(input);

    DesignerElement prompt;
    prompt.Name        = "Chat_Input_Placeholder";
    prompt.Type        = UIElementType::Text;
    prompt.Position    = { basePos.x + 18.0f, basePos.y + 188.0f };
    prompt.Size        = { 300.0f, 20.0f };
    prompt.TextContent = "Press Enter to chat...";
    prompt.TintColor   = COLOR(130, 145, 170, 255);
    prompt.FontScale   = 0.75f;
    s_Elements.push_back(prompt);

    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    s_StatusMessage = "Spawned MMORPG Chat Box Preset (Tabs + History + Input)!";
    s_StatusTimer   = 3.0f;
}

void E_UIDesigner::Add_Box() {
    DesignerElement el;
    el.Name      = std::format("Panel_Box_{}", s_NextElementID++);
    el.Type      = UIElementType::Box;
    el.Position  = { 600.0f, 350.0f };
    el.Size      = { 260.0f, 160.0f };
    el.TintColor = COLOR(28, 36, 50, 240);
    el.Alpha     = 0.95f;
    s_Elements.push_back(el);
    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
}

void E_UIDesigner::Add_Image(StringView texturePath) {
    DesignerElement el;
    el.Name      = std::format("Image_{}", s_NextElementID++);
    el.Type      = UIElementType::Image;
    el.Position  = { 640.0f, 360.0f };
    el.Size      = { 128.0f, 128.0f };
    el.TintColor = COLOR_WHITE;
    el.Alpha     = 1.0f;

    if (!texturePath.empty()) {
        el.TexturePath = String(texturePath);
        el.TextureID   = M_Asset::Load_Texture(el.TexturePath);
    } else if (!s_DiscoveredImages.empty()) {
        el.TexturePath = s_DiscoveredImages[0];
        el.TextureID   = M_Asset::Load_Texture(el.TexturePath);
    }

    s_Elements.push_back(el);
    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
}

void E_UIDesigner::Add_Button() {
    DesignerElement el;
    el.Name        = std::format("Btn_Action_{}", s_NextElementID++);
    el.Type        = UIElementType::Button;
    el.Position    = { 650.0f, 400.0f };
    el.Size        = { 160.0f, 45.0f };
    el.TintColor   = COLOR(52, 152, 219, 255);
    el.TextContent = "Click Me";
    el.FontScale   = 0.9f;
    s_Elements.push_back(el);
    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
}

void E_UIDesigner::Add_Text() {
    DesignerElement el;
    el.Name        = std::format("Text_Label_{}", s_NextElementID++);
    el.Type        = UIElementType::Text;
    el.Position    = { 650.0f, 320.0f };
    el.Size        = { 200.0f, 30.0f };
    el.TintColor   = COLOR_WHITE;
    el.TextContent = "New Text Label";
    el.FontScale   = 1.0f;
    s_Elements.push_back(el);
    s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
}

void E_UIDesigner::Delete_Selected() {
    if (s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
        s_Elements.erase(s_Elements.begin() + s_SelectedIndex);
        s_SelectedIndex = s_Elements.empty() ? -1 : std::clamp(s_SelectedIndex - 1, 0, static_cast<INT>(s_Elements.size()) - 1);
    }
}

void E_UIDesigner::Duplicate_Selected() {
    if (s_SelectedIndex >= 0 && s_SelectedIndex < static_cast<INT>(s_Elements.size())) {
        DesignerElement dup = s_Elements[s_SelectedIndex];
        dup.Name += "_Copy";
        dup.Position.x += 20.0f;
        dup.Position.y += 20.0f;
        s_Elements.push_back(dup);
        s_SelectedIndex = static_cast<INT>(s_Elements.size()) - 1;
    }
}

void E_UIDesigner::Save_Layout(const FilePath &path) {
    FilePath projRoot = E_ProjectManager::Get_ActiveProject().RootDirectory;
    FilePath targetPath = (projRoot.empty() ? FilePath(".") : projRoot) / path;

    std::filesystem::create_directories(targetPath.parent_path());

    String content = "# Aurora Studio UI Layout\n[Artboard]\nWidth = 1920\nHeight = 1080\n\n";

    for (size_t i = 0; i < s_Elements.size(); ++i) {
        const auto &el = s_Elements[i];
        content += std::format("[Element_{}]\n", i);
        content += std::format("Name = {}\n", el.Name);
        content += std::format("Type = {}\n", static_cast<INT>(el.Type));
        content += std::format("Position = {:.1f}, {:.1f}\n", el.Position.x, el.Position.y);
        content += std::format("Size = {:.1f}, {:.1f}\n", el.Size.x, el.Size.y);
        content += std::format("Tint = {}, {}, {}, {}\n", el.TintColor.r, el.TintColor.g, el.TintColor.b, el.TintColor.a);
        content += std::format("Alpha = {:.2f}\n", el.Alpha);
        content += std::format("Texture = {}\n", el.TexturePath);
        content += std::format("Text = {}\n", el.TextContent);
        content += std::format("FontScale = {:.2f}\n\n", el.FontScale);
    }

    M_File::Write(targetPath, content, true);
    s_StatusMessage = std::format("Saved layout to [{}]!", targetPath.generic_string());
    s_StatusTimer   = 3.5f;
}

void E_UIDesigner::Load_Layout(const FilePath &path) {
    (void)path;
}

String E_UIDesigner::Generate_Cpp_Snippet() {
    String out = "// ============================================================================\n";
    out += "// Generated Aurora Engine UI Code (from UI Designer)\n";
    out += "// Paste into Client::DrawUI() or your custom UI class\n";
    out += "// ============================================================================\n\n";

    for (const auto &el : s_Elements) {
        if (!el.bVisible) continue;
        if (el.Type == UIElementType::Box) {
            out += std::format("UI::Draw_Box({:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, COLOR({}, {}, {}, {}), {:.2f}f);\n",
                el.Position.x, el.Position.y, el.Size.x, el.Size.y,
                el.TintColor.r, el.TintColor.g, el.TintColor.b, el.TintColor.a, el.Alpha);
        } else if (el.Type == UIElementType::Image) {
            out += std::format("RHI_Manager::Draw_UI(M_Asset::Load_Texture(\"{}\"), {{ {:.1f}f, {:.1f}f }}, {{ {:.1f}f, {:.1f}f }});\n",
                el.TexturePath.empty() ? "Assets/UI/Inventory.png" : el.TexturePath,
                el.Position.x, el.Position.y, el.Size.x, el.Size.y);
        } else if (el.Type == UIElementType::Button) {
            out += std::format("UI::Draw_Box({:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, COLOR({}, {}, {}, {}));\n",
                el.Position.x, el.Position.y, el.Size.x, el.Size.y,
                el.TintColor.r, el.TintColor.g, el.TintColor.b, el.TintColor.a);
            out += std::format("UI::Draw_Text_Shadowed(\"{}\", {:.1f}f, {:.1f}f, COLOR_WHITE, COLOR_BLACK, {:.2f}f);\n",
                el.TextContent, el.Position.x + 10.0f, el.Position.y + 8.0f, el.FontScale);
        } else if (el.Type == UIElementType::Text) {
            out += std::format("UI::Draw_Text_Shadowed(\"{}\", {:.1f}f, {:.1f}f, COLOR({}, {}, {}, {}), COLOR_BLACK, {:.2f}f);\n",
                el.TextContent, el.Position.x, el.Position.y,
                el.TintColor.r, el.TintColor.g, el.TintColor.b, el.TintColor.a, el.FontScale);
        }
    }

    PRINT("{}", out);
    return out;
}
