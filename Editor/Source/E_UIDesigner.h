#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <UserInterface.h>

// ============================================================================
// E_UIDesigner: Roblox / Figma-Style 2D UI & HUD Designer for Aurora Studio
// ============================================================================
// Features:
//   - Infinite 2D Pan & Zoom Canvas (Space+Drag, Middle Click, Mouse Wheel)
//   - 1920x1080 Game Artboard Frame with visual grid
//   - PNG & Texture Drag-and-Drop Placement from Assets/UI/
//   - Visual bounding box selection & 4-corner interactive resizing
//   - Left Sidebar: UI Layer Hierarchy + Project PNG Image Browser
//   - Right Sidebar: Properties Inspector (X, Y, W, H, Texture, Tint, Text, Scale)
//   - Bottom Floating Pill Toolbar: Quick-add Box, Image, Button, Text
//   - 1-Click "Save Layout (.auroraui)" and "Copy C++ Code"
// ============================================================================

enum class UIElementType : BYTE {
    Box    = 0, // Solid colored panel / backdrop
    Image  = 1, // Hardware-accelerated PNG texture quad
    Button = 2, // Interactive 4-state image/color button
    Text   = 3  // TrueType dynamic text
};

struct DesignerElement {
    String        Name        = "Element";
    UIElementType Type        = UIElementType::Box;
    Vector2D      Position    = { 100.0f, 100.0f }; // In Artboard coordinates (0..1920, 0..1080)
    Vector2D      Size        = { 200.0f, 120.0f };
    COLOR         TintColor   = COLOR_WHITE;
    FLOAT         Alpha       = 1.0f;
    String        TexturePath = "";
    UINT          TextureID   = 0;
    String        TextContent = "Label";
    FLOAT         FontScale   = 1.0f;
    BOOL          bVisible    = true;
    BOOL          bLocked     = false;
};

class E_UIDesigner {
private:
    // Canvas transform
    static Vector2D s_CanvasPan;
    static FLOAT    s_CanvasZoom;
    static BOOL     s_IsPanning;
    static Vector2D s_LastPanMouse;

    // Artboard resolution (default 1920x1080 MMO standard)
    static Vector2D s_ArtboardSize;

    // Elements hierarchy (Rendered in z-order)
    static Array<DesignerElement> s_Elements;
    static INT                    s_SelectedIndex;
    static INT                    s_NextElementID;

    // Element dragging & resizing state
    static BOOL     s_IsDraggingElement;
    static INT      s_ActiveResizeHandle; // -1: none, 0: TopLeft, 1: TopRight, 2: BottomRight, 3: BottomLeft
    static Vector2D s_DragStartMouse;
    static Vector2D s_ElementStartPos;
    static Vector2D s_ElementStartSize;

    // Asset browser
    static Array<String> s_DiscoveredImages;
    static INT           s_LeftTab;       // 0 = Hierarchy, 1 = PNG Assets
    static String        s_StatusMessage;
    static FLOAT         s_StatusTimer;

    // Text editing in Inspector
    static BOOL          s_IsEditingText;
    static BOOL          s_IsEditingName;

    // Coordinate conversion
    static Vector2D Canvas_To_Screen(const Vector2D &canvasPos, const Vector2D &viewportMin);
    static Vector2D Screen_To_Canvas(const Vector2D &screenPos, const Vector2D &viewportMin);

    // Helpers
    static void Scan_UI_Assets();
    static void Ensure_Sample_Elements();

public:
    static void Init();
    static void Tick(FLOAT deltaTime, const Vector2D &viewportMin, const Vector2D &viewportMax);
    static void Render(const Vector2D &viewportMin, const Vector2D &viewportMax);

    // Render Subsystems
    static void Draw_Canvas(const Vector2D &viewportMin, const Vector2D &viewportMax);
    static void Draw_InScene_Overlay(const Vector2D &viewportMin, const Vector2D &viewportMax);
    static void Draw_LeftSidebar(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_RightInspector(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_FloatingPill(FLOAT x, FLOAT y, FLOAT w, FLOAT h);

    // Element Management
    static void Add_Box();
    static void Add_Image(StringView texturePath = "");
    static void Add_Button();
    static void Add_Text();
    static void Delete_Selected();
    static void Duplicate_Selected();

    // 1-Click MMORPG UI Presets (Figma / Roblox Style)
    static void Add_Inventory_Preset();
    static void Add_ActionHotbar_Preset();
    static void Add_PlayerStatus_Preset();
    static void Add_Minimap_Preset();
    static void Add_ChatBox_Preset();

    // Export & Persistence
    static void Save_Layout(const FilePath &path = "Assets/UI/HUD.auroraui");
    static void Load_Layout(const FilePath &path = "Assets/UI/HUD.auroraui");
    static String Generate_Cpp_Snippet();
};
