#include <UI_FPSCounter.h>
#include <M_Time.h>
#include <M_Font.h>
#include <RHI_Manager.h>
#include <M_World.h>
#include <M_Collision.h>
#include <format>

BOOL UI_FPSCounter::s_bVisible  = true;
BOOL UI_FPSCounter::s_bDetailed = true;

void UI_FPSCounter::Toggle() {
    s_bVisible = !s_bVisible;
}

void UI_FPSCounter::Set_Visible(BOOL bVisible) {
    s_bVisible = bVisible;
}

void UI_FPSCounter::Toggle_Detailed() {
    s_bDetailed = !s_bDetailed;
}

void UI_FPSCounter::Render(FLOAT x, FLOAT y) {
    if (!s_bVisible) return;

    INT   fps = M_Time::Get_FPS();
    FLOAT dt  = M_Time::Get_DeltaTime() * 1000.0f; // ms

    // Dynamic color coding: Green > 1000 FPS, Yellow > 300 FPS, Orange > 60 FPS, Red <= 60 FPS
    COLOR fpsColor = COLOR(46, 204, 113, 255); // Green
    if (fps < 60) {
        fpsColor = COLOR(231, 76, 60, 255);   // Red
    } else if (fps < 300) {
        fpsColor = COLOR(243, 156, 18, 255);  // Orange
    } else if (fps < 1000) {
        fpsColor = COLOR(241, 196, 15, 255);  // Yellow
    }

    if (!s_bDetailed) {
        String compact = std::format("FPS: {}", fps);
        M_Font::Draw_String_Shadowed(compact, x, y, fpsColor, COLOR_BLACK, 0.95f);
        return;
    }

    // Semi-transparent backdrop box
    FLOAT panelW = 280.0f;
    FLOAT panelH = 75.0f;
    RHI_Manager::Draw_UI(0, { x - 8.0f, y - 6.0f }, { panelW, panelH }, COLOR(15, 18, 25, 220), 0.90f);

    // Line 1: FPS and frame time
    String line1 = std::format("FPS: {} ({:.2f} ms)", fps, dt);
    M_Font::Draw_String_Shadowed(line1, x, y, fpsColor, COLOR_BLACK, 0.95f);

    // Line 2: Active API backend & GPU Name
    StringView gpuName = RHI_Manager::Get_GPUName();
    String line2 = std::format("RHI: {} [{}]",
        RHI_Manager::Get_APIName(RHI_Manager::Get_ActiveAPI()),
        gpuName.empty() ? "GPU" : gpuName.substr(0, 18));
    M_Font::Draw_String_Shadowed(line2, x, y + 22.0f, COLOR(180, 210, 240, 255), COLOR_BLACK, 0.75f);

    // Line 3: World entity visibility count & F1 collision status
    INT total = M_World::Get_EntityCount();
    INT vis   = M_World::Get_VisibleEntityCount();
    INT culled = M_World::Get_CulledEntityCount();
    BOOL bF1  = M_Collision::Is_DebugDrawing();

    String line3 = std::format("Ents: {} (Vis: {} | Culled: {}) | F1: {}",
        total, vis, culled, bF1 ? "ON" : "OFF");
    M_Font::Draw_String_Shadowed(line3, x, y + 42.0f, COLOR(200, 205, 215, 255), COLOR_BLACK, 0.72f);
}
