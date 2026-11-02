#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <UserInterface.h>

// ============================================================================
// E_EditorApp: Aurora Engine Studio Master Application & UI
// ============================================================================
// Dual-Mode Lifecycle:
//   1. Project Hub Mode (Flax / Unreal Engine Style)
//      - Top-Left: Engine Image / Logo container (Figma-ready placeholder)
//      - Left Navigation: Projects, ChangeLog, Settings, GitHub
//      - Launches automatically when opened directly without arguments.
//   2. Level Editor Mode (3D Viewport, Outliner, Inspector, Content Browser)
//      - Skips Hub and opens immediately when double-clicking a .aurora file!
//      - Crash-safe Plugin integration (Blueprint & CodeEditor).
// ============================================================================

enum class AppMode : BYTE {
    ProjectHub  = 0,
    LevelEditor = 1
};

enum class HubTab : BYTE {
    Projects  = 0,
    ChangeLog = 1,
    Settings  = 2
};

enum class EditorModal : BYTE {
    None          = 0,
    SaveProjectAs = 1,
    XORGenerator  = 2,
    AssetPacker   = 3,
    BuildGame     = 4,
    PluginsList   = 5,
    Settings      = 6
};

// Flax Engine & Visual Studio Style Viewport Workspaces
enum class WorkspaceTab : BYTE {
    Scene3D    = 0, // Tab 1: 3D Render Viewport (CANNOT BE CLOSED!)
    UIDesigner = 1  // Tab 2: Roblox / Figma UI Designer (can be closed with [x])
};

enum class EditorTab : BYTE {
    Assets = 0,
    Packs  = 1
};

class E_EditorApp {
private:
    static AppMode      s_AppMode;
    static HubTab       s_CurrentHubTab;
    static WorkspaceTab s_ActiveWorkspace;
    static BOOL         s_IsUIDesignerOpen;
    static BOOL         s_IsRunning;
    static BOOL         s_IsPlayMode;       // True when test-playing in editor
    static BOOL         s_IsPlayPaused;     // True when paused in PIE
    static BOOL         s_StepSingleFrame;  // Advance exactly 1 frame when paused
    static EditorModal  s_ActiveModal;
    static EditorTab    s_BottomTab;

    // Viewport layout bounds
    static Vector2D    s_ViewportMin;
    static Vector2D    s_ViewportMax;

    // XOR Generator state
    static String      s_XorPassphrase;
    static Array<BYTE> s_GeneratedKey;
    static String      s_XorStatusMsg;

    // Save As modal state
    static String      s_SaveProjName;
    static String      s_SaveProjPath;
    static INT         s_SaveActiveField;  // 1 = Name, 2 = Path

    // Project creation state (Hub & Modals)
    static BOOL        s_ShowNewProjectCard;
    static String      s_NewProjName;
    static String      s_NewProjPath;
    static INT         s_NewProjTemplate;  // 1 = GameExample, 0 = Blank
    static INT         s_NewActiveField;   // 1 = Name, 2 = Path

    // Settings state
    static String      s_GithubRepoUrl;

    // Asset browser state
    static Array<String> s_DiscoveredAssets;
    static String        s_SelectedAsset;
    static BOOL          s_ShowAssetContextMenu;
    static Vector2D      s_ContextMenuPos;

    // In-Scene Roblox-style UI Overlay
    static BOOL          s_ShowInSceneUI;

    // Drag-and-drop spawning from Content Browser to 3D Scene
    static BOOL          s_IsDraggingAsset;
    static String        s_DraggedAssetPath;
    static Vector3D      s_DragWorldHit;
    static BOOL          s_DragHasHit;

    // Status notification
    static String      s_StatusText;
    static FLOAT       s_StatusTimer;

    static void Scan_Assets();

    // Mode 1: Flax / Unreal Project Hub Renderers
    static void Draw_ProjectHub_Screen(FLOAT screenW, FLOAT screenH);
    static void Draw_Hub_ProjectsTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_Hub_ChangeLogTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_Hub_SettingsTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h);

    // Mode 2: 3D Level Editor Renderers
    static void Draw_TopToolbar(FLOAT width);
    static void Draw_TabBar(FLOAT width);
    static void Draw_Outliner(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_Inspector(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_BottomPanel(FLOAT x, FLOAT y, FLOAT w, FLOAT h);
    static void Draw_ContextMenu();
    static void Draw_Modals(FLOAT screenW, FLOAT screenH);

    // Modal renderers
    static void Draw_SaveProjectAs_Modal(FLOAT screenW, FLOAT screenH);
    static void Draw_XOR_Modal(FLOAT screenW, FLOAT screenH);
    static void Draw_BuildGame_Modal(FLOAT screenW, FLOAT screenH);
    static void Draw_PluginsList_Modal(FLOAT screenW, FLOAT screenH);
    static void Draw_Settings_Modal(FLOAT screenW, FLOAT screenH);

    // Project Actions
    static void Do_Save_Project_As();
    static void Switch_To_Project(const FilePath &projectDir);
    static void Create_And_Switch_Project();

public:
    static BOOL Init();
    static void Tick(FLOAT deltaTime);
    static void Render();
    static void Shutdown();

    static void Set_AppMode(AppMode mode);
    [[nodiscard]] static AppMode Get_AppMode() { return s_AppMode; }

    static void Set_Status(StringView text, FLOAT duration = 3.0f);
    static void Toggle_PlayMode();
    static void Start_PlayMode();
    static void Stop_PlayMode();
    static void Toggle_Pause_PlayMode();
    static void Step_Single_Frame();
    static void Save_Current_Scene();
};
