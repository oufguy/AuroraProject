#include <CodeEditorPlugin.h>
#include <M_File.h>
#include <M_Window.h>
#include <I_Mouse.h>
#include <I_Keyboard.h>
#include <M_Log.h>
#include <sstream>

static inline BOOL In_Rect(const Vector2D &m, FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
}

BOOL CodeEditorPlugin::On_Enable() {
    LOG(Info, LogLevel::Core, "CodeEditorPlugin enabled.");
    m_CodeLines.clear();

    // Default sample code or load active project file
    m_CodeLines = {
        "#include <Aurora.h>",
        "",
        "class GameClient : public B_Game {",
        "public:",
        "    void BeginPlay() override {",
        "        LOG(Info, LogLevel::Core, \"Client started!\");",
        "        M_Sound::Play(\"Assets/Audio/Chime.wav\", true, 1.0f);",
        "    }",
        "",
        "    void Tick(FLOAT dt) override {",
        "        // Game frame update",
        "    }",
        "};"
    };

    return true;
}

void CodeEditorPlugin::On_Disable() {
    LOG(Info, LogLevel::Core, "CodeEditorPlugin disabled.");
    m_CodeLines.clear();
}

void CodeEditorPlugin::Load_File(const FilePath &path) {
    if (!std::filesystem::exists(path)) return;
    String content = M_File::Open(path);
    m_CodeLines.clear();
    std::istringstream stream(content);
    String line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        m_CodeLines.push_back(line);
    }
    m_ActiveFileName = path.filename().string();
}

void CodeEditorPlugin::On_Tick(FLOAT deltaTime) {
    (void)deltaTime;
    if (I_Keyboard::Is_Pressed(Key::Up) && m_SelectedLine > 0) --m_SelectedLine;
    if (I_Keyboard::Is_Pressed(Key::Down) && m_SelectedLine + 1 < static_cast<INT>(m_CodeLines.size())) ++m_SelectedLine;
}

void CodeEditorPlugin::On_Render_UI() {
    FLOAT winW = static_cast<FLOAT>(M_Window::Get_Width());
    FLOAT winH = static_cast<FLOAT>(M_Window::Get_Height());

    FLOAT bx = 320.0f;
    FLOAT by = 70.0f;
    FLOAT bw = winW - 370.0f;
    FLOAT bh = winH - 120.0f;
    if (bw < 400.0f || bh < 300.0f) return;

    // Window frame
    UI::Draw_Box(bx, by, bw, bh, COLOR(18, 22, 30, 248));
    UI::Draw_Box(bx, by, bw, 32.0f, COLOR(26, 32, 44, 255));
    UI::Draw_Text("IN-EDITOR CODE EDITOR: " + m_ActiveFileName, bx + 15.0f, by + 8.0f, COLOR(46, 204, 113, 255), 0.85f);

    // Line numbers gutter
    FLOAT gutterW = 45.0f;
    UI::Draw_Box(bx, by + 32.0f, gutterW, bh - 32.0f, COLOR(14, 18, 24, 255));

    FLOAT lineY = by + 40.0f;
    for (size_t i = 0; i < m_CodeLines.size(); ++i) {
        if (lineY + 20.0f > by + bh) break;

        BOOL isCur = (static_cast<INT>(i) == m_SelectedLine);
        if (isCur) {
            UI::Draw_Box(bx + gutterW, lineY, bw - gutterW, 20.0f, COLOR(32, 42, 60, 255));
        }

        // Line number
        UI::Draw_Text(std::to_string(i + 1), bx + 10.0f, lineY + 2.0f, COLOR(100, 115, 140, 255), 0.75f);

        // Code text with syntax coloring
        const auto &line = m_CodeLines[i];
        COLOR textCol = COLOR(220, 230, 245, 255);
        if (line.starts_with("#include")) textCol = COLOR(155, 89, 182, 255);
        else if (line.starts_with("//")) textCol = COLOR(120, 140, 160, 255);
        else if (line.find("class ") != String::npos || line.find("void ") != String::npos) textCol = COLOR(52, 152, 219, 255);
        else if (line.find("LOG(") != String::npos || line.find("M_Sound::") != String::npos) textCol = COLOR(241, 196, 15, 255);

        UI::Draw_Text(line, bx + gutterW + 12.0f, lineY + 2.0f, textCol, 0.75f);

        lineY += 22.0f;
    }

    // Build trigger button
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);
    BOOL hovBld = In_Rect(mouse, bx + bw - 150.0f, by + 4.0f, 135.0f, 24.0f);
    UI::Draw_Box(bx + bw - 150.0f, by + 4.0f, 135.0f, 24.0f, hovBld ? COLOR(230, 126, 34, 255) : COLOR(211, 84, 0, 255));
    UI::Draw_Text("Recompile (C++)", bx + bw - 140.0f, by + 8.0f, COLOR_WHITE, 0.75f);
    if (hovBld && bClick) {
        LOG(Info, LogLevel::Core, "Triggered C++ recompile from in-editor code editor!");
    }
}
