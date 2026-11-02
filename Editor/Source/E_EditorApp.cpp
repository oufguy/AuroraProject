#include <E_EditorApp.h>
#include <E_SceneEditor.h>
#include <E_UIDesigner.h>
#include <E_ProjectManager.h>
#include <E_AssetPacker.h>
#include <E_XORGenerator.h>
#include <M_Collision.h>
#include <M_Input.h>
#include <M_Mesh.h>
#include <M_SceneSerializer.h>
#include <M_World.h>
#include <M_Window.h>
#include <I_Mouse.h>
#include <I_Keyboard.h>
#include <M_Sound.h>
#include <C_StaticMesh.h>
#include <C_Character.h>
#include <M_File.h>
#include <M_Asset.h>
#include <M_Time.h>
#include <M_PluginManager.h>
#include <BlueprintPlugin.h>
#include <CodeEditorPlugin.h>
#include <SDL3/SDL_misc.h>

AppMode      E_EditorApp::s_AppMode              = AppMode::ProjectHub;
HubTab       E_EditorApp::s_CurrentHubTab        = HubTab::Projects;
WorkspaceTab E_EditorApp::s_ActiveWorkspace      = WorkspaceTab::Scene3D;
BOOL         E_EditorApp::s_IsUIDesignerOpen      = true;
BOOL         E_EditorApp::s_IsRunning            = true;
BOOL         E_EditorApp::s_IsPlayMode           = false;
BOOL         E_EditorApp::s_IsPlayPaused         = false;
BOOL         E_EditorApp::s_StepSingleFrame      = false;
EditorModal  E_EditorApp::s_ActiveModal          = EditorModal::None;
EditorTab    E_EditorApp::s_BottomTab            = EditorTab::Assets;
Vector2D     E_EditorApp::s_ViewportMin          = { 270.0f, 72.0f };
Vector2D     E_EditorApp::s_ViewportMax          = { 960.0f, 520.0f };

String      E_EditorApp::s_XorPassphrase        = "AuroraGameSecretKey2026!";
Array<BYTE> E_EditorApp::s_GeneratedKey         = {};
String      E_EditorApp::s_XorStatusMsg         = "";

String      E_EditorApp::s_SaveProjName         = "MyGame";
String      E_EditorApp::s_SaveProjPath         = "D:/MyGame";
INT         E_EditorApp::s_SaveActiveField      = 1; // 1 = Name, 2 = Path

BOOL        E_EditorApp::s_ShowNewProjectCard   = false;
String      E_EditorApp::s_NewProjName          = "AuroraWorldOnline";
String      E_EditorApp::s_NewProjPath          = "D:/AuroraWorldOnline";
INT         E_EditorApp::s_NewProjTemplate      = 1; // 1 = GameExample, 0 = Blank
INT         E_EditorApp::s_NewActiveField       = 1; // 1 = Name, 2 = Path

String      E_EditorApp::s_GithubRepoUrl        = "https://github.com/oufguy/AuroraProject";

Array<String> E_EditorApp::s_DiscoveredAssets   = {};
String        E_EditorApp::s_SelectedAsset      = "";
BOOL          E_EditorApp::s_ShowAssetContextMenu = false;
Vector2D      E_EditorApp::s_ContextMenuPos     = {};

BOOL          E_EditorApp::s_ShowInSceneUI        = true;
BOOL          E_EditorApp::s_IsDraggingAsset      = false;
String        E_EditorApp::s_DraggedAssetPath     = "";
Vector3D      E_EditorApp::s_DragWorldHit         = { 0.0f, 0.0f, 0.0f };
BOOL          E_EditorApp::s_DragHasHit           = false;

String      E_EditorApp::s_StatusText           = "Aurora Studio Ready.";
FLOAT       E_EditorApp::s_StatusTimer          = 3.0f;

static inline BOOL Check_Click_Rect(const Vector2D &mouse, FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    return (mouse.x >= x && mouse.x <= x + w && mouse.y >= y && mouse.y <= y + h);
}

void E_EditorApp::Set_AppMode(AppMode mode) {
    s_AppMode = mode;
    if (s_AppMode == AppMode::ProjectHub) {
        M_Window::Set_Title("Aurora Engine — Project Hub");
    } else if (E_ProjectManager::Has_ActiveProject()) {
        const auto &proj = E_ProjectManager::Get_ActiveProject();
        String title = std::format("Aurora Studio — [{}]", proj.Name);
        if (proj.bIsTemporary) title += " *";
        M_Window::Set_Title(title);
    }
}

void E_EditorApp::Set_Status(StringView text, FLOAT duration) {
    s_StatusText  = String(text);
    s_StatusTimer = duration;
}

void E_EditorApp::Scan_Assets() {
    s_DiscoveredAssets.clear();
    std::error_code ec;

    const char *folders[] = { "Assets/Models", "Assets/Audio", "Assets/UI" };
    for (const char *dir : folders) {
        if (!std::filesystem::exists(dir, ec)) continue;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file()) {
                String pathStr = entry.path().string();
                std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                s_DiscoveredAssets.push_back(pathStr);
            }
        }
    }
}

BOOL E_EditorApp::Init() {
    // 1. Register Default Plugins (Disabled by default)
    M_PluginManager::Register_Plugin(std::make_shared<BlueprintPlugin>());
    M_PluginManager::Register_Plugin(std::make_shared<CodeEditorPlugin>());
    M_PluginManager::Init();

    // 2. Initialize Scene Editor, UI Designer, and Keystream
    E_SceneEditor::Init();
    E_UIDesigner::Init();
    s_GeneratedKey = E_XORGenerator::Generate_Key(s_XorPassphrase);

    // 3. If starting in LevelEditor mode, ensure active project is loaded
    if (s_AppMode == AppMode::LevelEditor) {
        if (!E_ProjectManager::Has_ActiveProject()) {
            E_ProjectManager::Open_Temporary_Project();
        }

        Scan_Assets();

        FilePath scenePath = "Scenes/World.scene";
        if (!M_File::Exists(scenePath)) {
            M_SceneSerializer::Create_Blank_Template(scenePath);
        }
        M_SceneSerializer::Load(scenePath);

        FilePath packsPath = "Config/Packs.ini";
        if (M_File::Exists(packsPath)) {
            E_AssetPacker::Load_Config(packsPath);
        }

        Set_Status(StringFormat("Loaded project [{}] successfully.", E_ProjectManager::Get_ActiveProject().Name));
    }

    return true;
}

void E_EditorApp::Save_Current_Scene() {
    if (E_ProjectManager::Is_Temporary()) {
        s_SaveProjName    = "MyGame";
        s_SaveProjPath    = "D:/MyGame";
        s_SaveActiveField = 1;
        s_ActiveModal     = EditorModal::SaveProjectAs;
        Set_Status("Untitled Project: Please select a location to save.");
        return;
    }

    FilePath path = "Scenes/World.scene";
    if (M_SceneSerializer::Save(path, E_ProjectManager::Get_ActiveProject().Name)) {
        E_ProjectManager::Save_Project();
        Set_Status(StringFormat("Scene and project saved to [{}]!", E_ProjectManager::Get_ActiveProject().RootDirectory.string()));
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
    } else {
        Set_Status("ERROR: Failed to save scene!");
    }
}

void E_EditorApp::Do_Save_Project_As() {
    if (s_SaveProjPath.empty()) {
        Set_Status("ERROR: Please enter a valid save path!");
        return;
    }

    M_SceneSerializer::Save("Scenes/World.scene", s_SaveProjName);

    if (E_ProjectManager::Save_Project_As(s_SaveProjPath, s_SaveProjName)) {
        M_Window::Set_Title(std::format("Aurora Studio — [{}]", s_SaveProjName));
        Scan_Assets();
        E_AssetPacker::Load_Config("Config/Packs.ini");

        s_ActiveModal = EditorModal::None;
        Set_Status(StringFormat("Project saved permanently to [{}]!", s_SaveProjPath));
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
    } else {
        Set_Status("ERROR: Failed to save project to specified location!");
    }
}

void E_EditorApp::Switch_To_Project(const FilePath &projectDir) {
    if (!std::filesystem::exists(projectDir)) {
        Set_Status("ERROR: Project path does not exist!");
        return;
    }

    if (E_ProjectManager::Open_Project(projectDir)) {
        M_World::Clear();
        FilePath scenePath = "Scenes/World.scene";
        if (M_File::Exists(scenePath)) {
            M_SceneSerializer::Load(scenePath);
        } else {
            M_SceneSerializer::Create_Blank_Template(scenePath);
            M_SceneSerializer::Load(scenePath);
        }

        E_AssetPacker::Clear();
        FilePath packsPath = "Config/Packs.ini";
        if (M_File::Exists(packsPath)) {
            E_AssetPacker::Load_Config(packsPath);
        }

        Scan_Assets();
        Set_AppMode(AppMode::LevelEditor);
        s_ActiveModal = EditorModal::None;
        Set_Status(StringFormat("Opened project [{}]", E_ProjectManager::Get_ActiveProject().Name));
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
    }
}

void E_EditorApp::Create_And_Switch_Project() {
    if (s_NewProjPath.empty()) {
        Set_Status("ERROR: Please enter a project path!");
        return;
    }

    ProjectTemplate tpl = (s_NewProjTemplate == 1) ? ProjectTemplate::Town : ProjectTemplate::Blank;
    if (E_ProjectManager::Create_Project(s_NewProjPath, s_NewProjName, tpl)) {
        M_World::Clear();
        M_SceneSerializer::Load("Scenes/World.scene");

        E_AssetPacker::Clear();
        E_AssetPacker::Load_Config("Config/Packs.ini");

        Scan_Assets();
        Set_AppMode(AppMode::LevelEditor);
        s_ActiveModal = EditorModal::None;
        s_ShowNewProjectCard = false;
        Set_Status(StringFormat("Created and opened [{}]", s_NewProjName));
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
    }
}

void E_EditorApp::Toggle_PlayMode() {
    if (s_IsPlayMode) {
        Stop_PlayMode();
    } else {
        Start_PlayMode();
    }
}

void E_EditorApp::Start_PlayMode() {
    s_IsPlayMode      = true;
    s_IsPlayPaused    = false;
    s_StepSingleFrame = false;

    // Snapshot scene so changes can be reverted on Stop
    FilePath snapDir = "Scenes";
    std::filesystem::create_directories(snapDir);
    M_SceneSerializer::Save(snapDir / ".pie_snapshot.scene");

    Set_Status("Play in Editor (PIE) ACTIVE. [F5 Stop | F6 Pause | Esc Exit]");
    M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
}

void E_EditorApp::Stop_PlayMode() {
    s_IsPlayMode      = false;
    s_IsPlayPaused    = false;
    s_StepSingleFrame = false;

    // Restore scene snapshot
    FilePath snapFile = "Scenes/.pie_snapshot.scene";
    if (M_File::Exists(snapFile)) {
        M_SceneSerializer::Load(snapFile);
    }

    Set_Status("Edit Mode ACTIVE. Scene restored to pre-play state.");
    M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
}

void E_EditorApp::Toggle_Pause_PlayMode() {
    if (!s_IsPlayMode) return;
    s_IsPlayPaused = !s_IsPlayPaused;
    if (s_IsPlayPaused) {
        Set_Status("PIE PAUSED. [F6 Continue | F7 Step Frame | Freecam Inspect]");
    } else {
        Set_Status("PIE RESUMED.");
    }
    M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
}

void E_EditorApp::Step_Single_Frame() {
    if (!s_IsPlayMode || !s_IsPlayPaused) return;
    s_StepSingleFrame = true;
    Set_Status("Stepped 1 Frame.");
}

void E_EditorApp::Tick(FLOAT deltaTime) {
    if (s_StatusTimer > 0.0f) {
        s_StatusTimer -= deltaTime;
    }

    // MODE 1: Flax / Unreal Project Hub
    if (s_AppMode == AppMode::ProjectHub) {
        // Typing into New Project card in Hub
        if (s_ShowNewProjectCard && s_NewActiveField > 0) {
            String &target = (s_NewActiveField == 1) ? s_NewProjName : s_NewProjPath;
            if (I_Keyboard::Is_Pressed(Key::Backspace) && !target.empty()) {
                target.pop_back();
                if (s_NewActiveField == 1) s_NewProjPath = "D:/" + s_NewProjName;
            }
            if (I_Keyboard::Is_Pressed(Key::Enter)) {
                Create_And_Switch_Project();
            }
            for (INT k = static_cast<INT>(Key::A); k <= static_cast<INT>(Key::Z); ++k) {
                Key key = static_cast<Key>(k);
                if (I_Keyboard::Is_Pressed(key)) {
                    Char c = static_cast<Char>('a' + (k - static_cast<INT>(Key::A)));
                    if (I_Keyboard::Is_Down(Key::ShiftLeft) || I_Keyboard::Is_Down(Key::ShiftRight)) c = static_cast<Char>(std::toupper(c));
                    target += c;
                    if (s_NewActiveField == 1) s_NewProjPath = "D:/" + s_NewProjName;
                }
            }
            for (INT k = static_cast<INT>(Key::Num0); k <= static_cast<INT>(Key::Num9); ++k) {
                Key key = static_cast<Key>(k);
                if (I_Keyboard::Is_Pressed(key)) {
                    target += static_cast<Char>('0' + (k - static_cast<INT>(Key::Num0)));
                    if (s_NewActiveField == 1) s_NewProjPath = "D:/" + s_NewProjName;
                }
            }
            if (I_Keyboard::Is_Pressed(Key::Slash) || I_Keyboard::Is_Pressed(Key::Backslash)) target += '/';
            if (I_Keyboard::Is_Pressed(Key::Minus)) target += (I_Keyboard::Is_Down(Key::ShiftLeft) ? '_' : '-');
            if (I_Keyboard::Is_Pressed(Key::Period)) target += '.';
        }
        return;
    }

    // MODE 2: 3D Level Editor
    Vector2D winSize = { static_cast<FLOAT>(M_Window::Get_Width()), static_cast<FLOAT>(M_Window::Get_Height()) };
    s_ViewportMin = { 270.0f, 72.0f };
    s_ViewportMax = { winSize.x - 320.0f, winSize.y - 190.0f };

    // Global Hotkeys
    if (I_Keyboard::Is_Pressed(Key::F5)) {
        Toggle_PlayMode();
    }
    if (I_Keyboard::Is_Down(Key::Ctrl) && I_Keyboard::Is_Pressed(Key::S)) {
        Save_Current_Scene();
    }
    if (I_Keyboard::Is_Pressed(Key::Escape)) {
        if (s_ActiveModal != EditorModal::None) {
            s_ActiveModal = EditorModal::None;
        } else if (s_IsPlayMode) {
            Toggle_PlayMode();
        } else if (s_ShowAssetContextMenu) {
            s_ShowAssetContextMenu = false;
        }
    }

    // Modal Text Input Processing
    if (s_ActiveModal == EditorModal::SaveProjectAs && s_SaveActiveField > 0) {
        String &target = (s_SaveActiveField == 1) ? s_SaveProjName : s_SaveProjPath;
        if (I_Keyboard::Is_Pressed(Key::Backspace) && !target.empty()) {
            target.pop_back();
            if (s_SaveActiveField == 1) s_SaveProjPath = "D:/" + s_SaveProjName;
        }
        if (I_Keyboard::Is_Pressed(Key::Enter)) {
            Do_Save_Project_As();
        }
        for (INT k = static_cast<INT>(Key::A); k <= static_cast<INT>(Key::Z); ++k) {
            Key key = static_cast<Key>(k);
            if (I_Keyboard::Is_Pressed(key)) {
                Char c = static_cast<Char>('a' + (k - static_cast<INT>(Key::A)));
                if (I_Keyboard::Is_Down(Key::ShiftLeft) || I_Keyboard::Is_Down(Key::ShiftRight)) c = static_cast<Char>(std::toupper(c));
                target += c;
                if (s_SaveActiveField == 1) s_SaveProjPath = "D:/" + s_SaveProjName;
            }
        }
        for (INT k = static_cast<INT>(Key::Num0); k <= static_cast<INT>(Key::Num9); ++k) {
            Key key = static_cast<Key>(k);
            if (I_Keyboard::Is_Pressed(key)) {
                target += static_cast<Char>('0' + (k - static_cast<INT>(Key::Num0)));
                if (s_SaveActiveField == 1) s_SaveProjPath = "D:/" + s_SaveProjName;
            }
        }
        if (I_Keyboard::Is_Pressed(Key::Slash) || I_Keyboard::Is_Pressed(Key::Backslash)) target += '/';
        if (I_Keyboard::Is_Pressed(Key::Minus)) target += (I_Keyboard::Is_Down(Key::ShiftLeft) ? '_' : '-');
        if (I_Keyboard::Is_Pressed(Key::Period)) target += '.';
    }

    if (s_IsPlayMode) {
        if (I_Keyboard::Is_Pressed(Key::F6)) {
            Toggle_Pause_PlayMode();
        }
        if (I_Keyboard::Is_Pressed(Key::F7)) {
            Step_Single_Frame();
        }

        if (!s_IsPlayPaused || s_StepSingleFrame) {
            // Player / Gameplay Tick during PIE
            C_Character *player = nullptr;
            for (const auto &ent : M_World::Get_Entities()) {
                if (ent && ent->Type == EntityType::Character) {
                    player = ent->As<C_Character>();
                    break;
                }
            }

            if (player) {
                if (I_Keyboard::Is_Down(Key::W)) player->Move_Forward(1.0f, deltaTime);
                if (I_Keyboard::Is_Down(Key::S)) player->Move_Forward(-1.0f, deltaTime);
                if (I_Keyboard::Is_Down(Key::D)) player->Turn(player->TurnSpeed * deltaTime);
                if (I_Keyboard::Is_Down(Key::A)) player->Turn(-player->TurnSpeed * deltaTime);
                if (I_Keyboard::Is_Pressed(Key::Space)) {
                    player->Jump();
                    M_Sound::Play("Assets/Audio/Jump.wav", /*bIsLocal=*/true, 0.9f);
                }
                player->Tick(deltaTime);

                C_Camera &cam = C_Camera::Get_Active();
                cam.Follow(player->Position, 10.0f, 3.5f);
            }
            s_StepSingleFrame = false;
        } else {
            // Paused inspection: Developer can fly around with Freecam to inspect scene
            E_SceneEditor::Tick(deltaTime, s_ViewportMin, s_ViewportMax);
        }
    } else {
        // Edit Mode Viewport Tick
        if (s_ActiveModal == EditorModal::None) {
            if (s_ActiveWorkspace == WorkspaceTab::Scene3D) {
                // Freecam & 3D Level Editor Tick
                E_SceneEditor::Tick(deltaTime, s_ViewportMin, s_ViewportMax);
                if (I_Keyboard::Is_Pressed(Key::F1)) {
                    M_Collision::Toggle_DebugDrawing();
                    Set_Status(M_Collision::Is_DebugDrawing() ? "Collision Bounds View (F1): ON" : "Collision Bounds View (F1): OFF");
                }
                if (I_Keyboard::Is_Pressed(Key::F2)) {
                    s_ShowInSceneUI = !s_ShowInSceneUI;
                    Set_Status(s_ShowInSceneUI ? "HUD Overlay (F2): ON" : "HUD Overlay (F2): OFF");
                }

                // Process Content Browser Drag-and-Drop Spawning into 3D Viewport
                if (s_IsDraggingAsset) {
                    Vector2D mouse = I_Mouse::Get_Position();
                    BOOL bInVp = (mouse.x >= s_ViewportMin.x && mouse.x <= s_ViewportMax.x &&
                                  mouse.y >= s_ViewportMin.y && mouse.y <= s_ViewportMax.y);

                    if (bInVp && E_SceneEditor::Get_Camera()) {
                        C_Camera *cam = E_SceneEditor::Get_Camera();
                        Ray3D ray = U_Raycast::Screen_To_World(mouse, cam->Position, cam->Rotation, cam->FOV);
                        // Intersect ray with ground plane y = 0.0f
                        if (std::abs(ray.Direction.y) > 0.001f) {
                            FLOAT t = -ray.Origin.y / ray.Direction.y;
                            if (t > 0.0f) {
                                s_DragWorldHit = ray.Origin + (ray.Direction * t);
                                s_DragHasHit   = true;
                            }
                        }
                    } else {
                        s_DragHasHit = false;
                    }

                    if (I_Mouse::Is_Released(MouseButton::MB_Left) || !I_Mouse::Is_Down(MouseButton::MB_Left)) {
                        if (s_DragHasHit && bInVp) {
                            if (s_DraggedAssetPath.ends_with(".obj")) {
                                auto *sm = M_World::Spawn_Entity<C_StaticMesh>();
                                if (sm) {
                                    sm->Mesh = M_Mesh::Load(s_DraggedAssetPath);
                                    sm->Position = s_DragWorldHit;
                                    E_SceneEditor::Select_Entity(sm);
                                    M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
                                    Set_Status("Spawned entity into 3D scene!");
                                }
                            } else if (s_DraggedAssetPath.ends_with(".png") || s_DraggedAssetPath.ends_with(".jpg")) {
                                E_UIDesigner::Add_Image(s_DraggedAssetPath);
                                Set_Status("Placed UI image onto Artboard!");
                            }
                        }
                        s_IsDraggingAsset = false;
                        s_DragHasHit      = false;
                    }
                }
            } else if (s_ActiveWorkspace == WorkspaceTab::UIDesigner) {
                // Roblox / Figma 2D UI Designer Tick
                E_UIDesigner::Tick(deltaTime, { 0.0f, 72.0f }, { winSize.x, winSize.y });
            }
        }
    }

    // Windows File Explorer Drag-and-Drop (SDL_EVENT_DROP_FILE)
    if (M_Input::Has_DroppedFile()) {
        String dropped = M_Input::Pop_DroppedFile();
        FilePath dropPath(dropped);
        if (std::filesystem::exists(dropPath)) {
            String ext = dropPath.extension().string();
            for (char &c : ext) c = static_cast<char>(std::tolower(c));

            FilePath projRoot = E_ProjectManager::Get_ActiveProject().RootDirectory;
            if (projRoot.empty()) projRoot = ".";

            if (ext == ".aurora") {
                Switch_To_Project(dropPath.parent_path());
            } else if (ext == ".scene") {
                M_SceneSerializer::Load(dropPath);
                Set_Status("Loaded scene: " + dropPath.filename().string());
            } else if (ext == ".obj") {
                FilePath dest = projRoot / "Assets" / "Models" / dropPath.filename();
                std::filesystem::create_directories(dest.parent_path());
                std::filesystem::copy_file(dropPath, dest, std::filesystem::copy_options::overwrite_existing);
                Scan_Assets();
                auto *sm = M_World::Spawn_Entity<C_StaticMesh>();
                if (sm) {
                    sm->Mesh = M_Mesh::Load(dest.lexically_relative(projRoot).generic_string());
                    sm->Position = { 0.0f, 0.0f, 0.0f };
                    E_SceneEditor::Select_Entity(sm);
                }
                Set_Status("Imported 3D Model: " + dropPath.filename().string());
            } else if (ext == ".png" || ext == ".bmp" || ext == ".jpg") {
                FilePath dest = projRoot / "Assets" / "UI" / dropPath.filename();
                std::filesystem::create_directories(dest.parent_path());
                std::filesystem::copy_file(dropPath, dest, std::filesystem::copy_options::overwrite_existing);
                Scan_Assets();
                E_UIDesigner::Add_Image(dest.lexically_relative(projRoot).generic_string());
                Set_Status("Imported UI Image: " + dropPath.filename().string());
            } else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
                FilePath dest = projRoot / "Assets" / "Audio" / dropPath.filename();
                std::filesystem::create_directories(dest.parent_path());
                std::filesystem::copy_file(dropPath, dest, std::filesystem::copy_options::overwrite_existing);
                Scan_Assets();
                Set_Status("Imported Audio: " + dropPath.filename().string());
            }
        }
    }

    // Tick active plugins safely
    M_PluginManager::Tick(deltaTime);
}

void E_EditorApp::Draw_ProjectHub_Screen(FLOAT screenW, FLOAT screenH) {
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // 1. Hub Background
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(16, 20, 26, 255));

    // 2. Left Navigation Sidebar
    FLOAT sideW = 230.0f;
    UI::Draw_Box(0, 0, sideW, screenH, COLOR(22, 27, 36, 255));
    UI::Draw_Box(sideW - 1.0f, 0, 1.0f, screenH, COLOR(38, 46, 62, 255));

    // Top-Left: Engine Image / Logo Container (Ready for user Figma design)
    FLOAT imgX = 15.0f;
    FLOAT imgY = 15.0f;
    FLOAT imgW = sideW - 30.0f;
    FLOAT imgH = 80.0f;

    UI::Draw_Box(imgX, imgY, imgW, imgH, COLOR(28, 34, 46, 255));
    UI::Draw_Box(imgX, imgY, imgW, 3.0f, COLOR(52, 152, 219, 255));
    UI::Draw_Text("AURORA ENGINE", imgX + 16.0f, imgY + 16.0f, COLOR(52, 152, 219, 255), 1.0f);
    UI::Draw_Text("[ Figma Image Area ]", imgX + 16.0f, imgY + 38.0f, COLOR(140, 155, 180, 255), 0.75f);
    UI::Draw_Text("v0.2.0 Community Edition", imgX + 16.0f, imgY + 56.0f, COLOR(241, 196, 15, 255), 0.7f);

    // Sidebar Navigation Buttons
    struct NavBtn {
        const char *Label;
        HubTab Tab;
        bool IsUrl;
    };

    NavBtn navs[] = {
        { "Projects",  HubTab::Projects,  false },
        { "ChangeLog", HubTab::ChangeLog, false },
        { "Settings",  HubTab::Settings,  false },
        { "GitHub",    HubTab::Projects,  true  }
    };

    FLOAT btnY = 115.0f;
    for (const auto &n : navs) {
        BOOL isCur = (!n.IsUrl && s_CurrentHubTab == n.Tab);
        BOOL hov = Check_Click_Rect(mouse, 10.0f, btnY, sideW - 20.0f, 38.0f);

        UI::Draw_Box(10.0f, btnY, sideW - 20.0f, 38.0f, isCur ? COLOR(41, 128, 185, 255) : (hov ? COLOR(32, 40, 54, 255) : COLOR(22, 27, 36, 255)));
        if (isCur) {
            UI::Draw_Box(10.0f, btnY, 4.0f, 38.0f, COLOR(52, 152, 219, 255));
        }

        COLOR txtCol = isCur ? COLOR_WHITE : (hov ? COLOR(230, 240, 255, 255) : COLOR(160, 175, 200, 255));
        UI::Draw_Text(n.Label, 26.0f, btnY + 11.0f, txtCol, 0.9f);

        if (hov && bClick) {
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
            if (n.IsUrl) {
                SDL_OpenURL(s_GithubRepoUrl.c_str());
                Set_Status("Opened GitHub repository in browser.");
            } else {
                s_CurrentHubTab = n.Tab;
            }
        }

        btnY += 46.0f;
    }

    // Bottom Sidebar System Info
    UI::Draw_Text("RHI: DirectX 11 / Vulkan", 15.0f, screenH - 45.0f, COLOR(100, 115, 140, 255), 0.7f);
    UI::Draw_Text("Platform: Windows x64", 15.0f, screenH - 25.0f, COLOR(100, 115, 140, 255), 0.7f);

    // 3. Main Content Area
    FLOAT cx = sideW + 25.0f;
    FLOAT cy = 20.0f;
    FLOAT cw = screenW - sideW - 50.0f;
    FLOAT ch = screenH - 40.0f;

    if (s_CurrentHubTab == HubTab::Projects) {
        Draw_Hub_ProjectsTab(cx, cy, cw, ch);
    } else if (s_CurrentHubTab == HubTab::ChangeLog) {
        Draw_Hub_ChangeLogTab(cx, cy, cw, ch);
    } else if (s_CurrentHubTab == HubTab::Settings) {
        Draw_Hub_SettingsTab(cx, cy, cw, ch);
    }

    // Status Notification Banner
    if (s_StatusTimer > 0.0f) {
        FLOAT alpha = std::clamp(s_StatusTimer, 0.0f, 1.0f);
        UI::Draw_Box(screenW - 400.0f, screenH - 45.0f, 380.0f, 28.0f, COLOR(25, 35, 45, static_cast<BYTE>(230 * alpha)));
        UI::Draw_Text(s_StatusText, screenW - 390.0f, screenH - 39.0f, COLOR(241, 196, 15, static_cast<BYTE>(255 * alpha)), 0.8f);
    }
}

void E_EditorApp::Draw_Hub_ProjectsTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    (void)h;
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Header
    UI::Draw_Text("PROJECTS WORKSPACE", x, y + 4.0f, COLOR_WHITE, 1.1f);
    UI::Draw_Text("Open an existing project or create a new one from template.", x, y + 30.0f, COLOR(140, 155, 180, 255), 0.8f);

    // "+ New Project" Button on right
    BOOL hovNew = Check_Click_Rect(mouse, x + w - 160.0f, y + 6.0f, 150.0f, 36.0f);
    UI::Draw_Box(x + w - 160.0f, y + 6.0f, 150.0f, 36.0f, s_ShowNewProjectCard ? COLOR(155, 89, 182, 255) : (hovNew ? COLOR(41, 128, 185, 255) : COLOR(32, 42, 60, 255)));
    UI::Draw_Text("+ New Project", x + w - 138.0f, y + 16.0f, COLOR_WHITE, 0.85f);
    if (hovNew && bClick) {
        s_ShowNewProjectCard = !s_ShowNewProjectCard;
        s_NewActiveField = 1;
    }

    FLOAT curY = y + 70.0f;

    // Optional: New Project Creation Box
    if (s_ShowNewProjectCard) {
        UI::Draw_Box(x, curY, w, 195.0f, COLOR(24, 30, 42, 255));
        UI::Draw_Box(x, curY, w, 2.0f, COLOR(155, 89, 182, 255));

        UI::Draw_Text("CREATE NEW PROJECT", x + 15.0f, curY + 12.0f, COLOR_WHITE, 0.9f);

        // Template selector
        UI::Draw_Text("Template:", x + 15.0f, curY + 40.0f, COLOR(180, 195, 220, 255), 0.8f);
        BOOL hovT1 = Check_Click_Rect(mouse, x + 90.0f, curY + 34.0f, 160.0f, 28.0f);
        UI::Draw_Box(x + 90.0f, curY + 34.0f, 160.0f, 28.0f, (s_NewProjTemplate == 1) ? COLOR(41, 128, 185, 255) : (hovT1 ? COLOR(35, 45, 60, 255) : COLOR(18, 24, 34, 255)));
        UI::Draw_Text("GameExample", x + 115.0f, curY + 40.0f, COLOR_WHITE, 0.8f);
        if (hovT1 && bClick) s_NewProjTemplate = 1;

        BOOL hovT2 = Check_Click_Rect(mouse, x + 260.0f, curY + 34.0f, 160.0f, 28.0f);
        UI::Draw_Box(x + 260.0f, curY + 34.0f, 160.0f, 28.0f, (s_NewProjTemplate == 0) ? COLOR(41, 128, 185, 255) : (hovT2 ? COLOR(35, 45, 60, 255) : COLOR(18, 24, 34, 255)));
        UI::Draw_Text("Blank Project", x + 295.0f, curY + 40.0f, COLOR_WHITE, 0.8f);
        if (hovT2 && bClick) s_NewProjTemplate = 0;

        // Project Name
        UI::Draw_Text("Name:", x + 15.0f, curY + 80.0f, COLOR(180, 195, 220, 255), 0.8f);
        BOOL hovN = Check_Click_Rect(mouse, x + 90.0f, curY + 74.0f, 330.0f, 28.0f);
        UI::Draw_Box(x + 90.0f, curY + 74.0f, 330.0f, 28.0f, (s_NewActiveField == 1) ? COLOR(35, 48, 68, 255) : COLOR(18, 24, 34, 255));
        UI::Draw_Text(s_NewProjName + (s_NewActiveField == 1 ? "_" : ""), x + 100.0f, curY + 80.0f, COLOR_WHITE, 0.8f);
        if (hovN && bClick) s_NewActiveField = 1;

        // Project Path
        UI::Draw_Text("Path:", x + 15.0f, curY + 116.0f, COLOR(180, 195, 220, 255), 0.8f);
        BOOL hovP = Check_Click_Rect(mouse, x + 90.0f, curY + 110.0f, 330.0f, 28.0f);
        UI::Draw_Box(x + 90.0f, curY + 110.0f, 330.0f, 28.0f, (s_NewActiveField == 2) ? COLOR(35, 48, 68, 255) : COLOR(18, 24, 34, 255));
        UI::Draw_Text(s_NewProjPath + (s_NewActiveField == 2 ? "_" : ""), x + 100.0f, curY + 116.0f, COLOR_WHITE, 0.8f);
        if (hovP && bClick) s_NewActiveField = 2;

        // Action Buttons
        BOOL hovCreate = Check_Click_Rect(mouse, x + 90.0f, curY + 150.0f, 150.0f, 32.0f);
        UI::Draw_Box(x + 90.0f, curY + 150.0f, 150.0f, 32.0f, hovCreate ? COLOR(46, 204, 113, 255) : COLOR(39, 174, 96, 255));
        UI::Draw_Text("Create & Open", x + 110.0f, curY + 158.0f, COLOR_WHITE, 0.85f);
        if (hovCreate && bClick) {
            Create_And_Switch_Project();
            return;
        }

        BOOL hovClose = Check_Click_Rect(mouse, x + 250.0f, curY + 150.0f, 90.0f, 32.0f);
        UI::Draw_Box(x + 250.0f, curY + 150.0f, 90.0f, 32.0f, hovClose ? COLOR(70, 80, 95, 255) : COLOR(40, 50, 65, 255));
        UI::Draw_Text("Cancel", x + 272.0f, curY + 158.0f, COLOR_WHITE, 0.85f);
        if (hovClose && bClick) {
            s_ShowNewProjectCard = false;
        }

        curY += 210.0f;
    }

    // Recent Projects Section
    UI::Draw_Text("RECENT PROJECTS", x, curY, COLOR(200, 215, 235, 255), 0.85f);
    curY += 25.0f;

    auto recents = E_ProjectManager::Get_RecentProjects();
    if (recents.empty()) {
        UI::Draw_Box(x, curY, w, 80.0f, COLOR(22, 27, 36, 255));
        UI::Draw_Text("No projects found. Click '+ New Project' above to create one!", x + 20.0f, curY + 30.0f, COLOR(140, 155, 180, 255), 0.85f);
    } else {
        for (size_t i = 0; i < recents.size() && i < 5; ++i) {
            const auto &p = recents[i];
            BOOL hovCard = Check_Click_Rect(mouse, x, curY, w, 68.0f);
            UI::Draw_Box(x, curY, w, 68.0f, hovCard ? COLOR(28, 36, 48, 255) : COLOR(22, 27, 36, 255));
            UI::Draw_Box(x, curY, 4.0f, 68.0f, COLOR(52, 152, 219, 255));

            UI::Draw_Text(p.Name, x + 18.0f, curY + 12.0f, COLOR_WHITE, 0.95f);
            UI::Draw_Text(p.Path.string(), x + 18.0f, curY + 36.0f, COLOR(140, 160, 190, 255), 0.75f);
            if (!p.LastOpened.empty()) {
                UI::Draw_Text("Last opened: " + p.LastOpened, x + w - 260.0f, curY + 36.0f, COLOR(110, 125, 150, 255), 0.7f);
            }

            // [ Open ] Button
            BOOL hovOpen = Check_Click_Rect(mouse, x + w - 120.0f, curY + 18.0f, 100.0f, 32.0f);
            UI::Draw_Box(x + w - 120.0f, curY + 18.0f, 100.0f, 32.0f, hovOpen ? COLOR(46, 204, 113, 255) : COLOR(39, 174, 96, 255));
            UI::Draw_Text("Open", x + w - 85.0f, curY + 26.0f, COLOR_WHITE, 0.85f);
            if (hovOpen && bClick) {
                Switch_To_Project(p.Path);
                return;
            }

            curY += 76.0f;
        }
    }
}

void E_EditorApp::Draw_Hub_ChangeLogTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    (void)h;
    UI::Draw_Text("AURORA ENGINE CHANGELOG & ROADMAP", x, y + 4.0f, COLOR_WHITE, 1.1f);
    UI::Draw_Text("Track engine milestones, feature releases, and architecture updates.", x, y + 30.0f, COLOR(140, 155, 180, 255), 0.8f);

    FLOAT curY = y + 70.0f;

    struct LogItem {
        const char *Version;
        const char *Date;
        Array<const char *> Items;
    };

    LogItem logs[] = {
        { "v0.2.0 - Standalone Engine Hub & Modular Plugins", "November 2026", {
            "Flax / Unreal Engine style Project Hub screen inside single AuroraEditor.exe binary.",
            "Direct .aurora file double-click association support (skips Hub directly into 3D Editor).",
            "Modular Crash-Safe Plugin System (Blueprint Visual C++ and In-Editor Code Editor base plugins).",
            "100% clean repository architecture (Idea 1): Engine and Editor separated from Game projects."
        }},
        { "v0.1.5 - Multi-RHI Graphics & Scene Serialization", "October 2026", {
            "Runtime hot-swappable Render Hardware Interface (DirectX 11 & Vulkan 1.3).",
            "3D Scene serializer with entity hierarchy, OBJ mesh loading, and AABB culling.",
            "3D Audio spatial system and encrypted .pak packaging with XOR keystream protection."
        }}
    };

    for (const auto &log : logs) {
        FLOAT cardH = 35.0f + log.Items.size() * 24.0f;
        UI::Draw_Box(x, curY, w, cardH, COLOR(22, 27, 36, 255));
        UI::Draw_Box(x, curY, w, 2.0f, COLOR(46, 204, 113, 255));

        UI::Draw_Text(log.Version, x + 16.0f, curY + 10.0f, COLOR(46, 204, 113, 255), 0.9f);
        UI::Draw_Text(log.Date, x + w - 160.0f, curY + 10.0f, COLOR(140, 155, 180, 255), 0.75f);

        FLOAT itemY = curY + 36.0f;
        for (const char *it : log.Items) {
            UI::Draw_Text(StringFormat(" * {}", it), x + 20.0f, itemY, COLOR(210, 220, 240, 255), 0.8f);
            itemY += 24.0f;
        }

        curY += cardH + 15.0f;
    }
}

void E_EditorApp::Draw_Hub_SettingsTab(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    (void)h;
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    UI::Draw_Text("ENGINE & EDITOR SETTINGS", x, y + 4.0f, COLOR_WHITE, 1.1f);
    UI::Draw_Text("Configure graphics backend, developer tools, and modular plugins.", x, y + 30.0f, COLOR(140, 155, 180, 255), 0.8f);

    FLOAT curY = y + 70.0f;

    // Card 1: Graphics & RHI Configuration
    UI::Draw_Box(x, curY, w, 110.0f, COLOR(22, 27, 36, 255));
    UI::Draw_Box(x, curY, w, 2.0f, COLOR(52, 152, 219, 255));
    UI::Draw_Text("GRAPHICS & DISPLAY (RHI)", x + 16.0f, curY + 10.0f, COLOR(52, 152, 219, 255), 0.9f);

    UI::Draw_Text("Default Render Backend: DirectX 11 (Vulkan 1.3 Available)", x + 20.0f, curY + 38.0f, COLOR(210, 220, 240, 255), 0.8f);
    UI::Draw_Text("VSync: OFF (Unlocked High-FPS Engine Architecture)", x + 20.0f, curY + 62.0f, COLOR(210, 220, 240, 255), 0.8f);
    UI::Draw_Text("Window Mode: Resizable Windowed (1280x720 Base)", x + 20.0f, curY + 86.0f, COLOR(210, 220, 240, 255), 0.8f);

    curY += 130.0f;

    // Card 2: Plugins Manager
    const auto &plugins = M_PluginManager::Get_Plugins();
    FLOAT plugH = 45.0f + plugins.size() * 65.0f;
    UI::Draw_Box(x, curY, w, plugH, COLOR(22, 27, 36, 255));
    UI::Draw_Box(x, curY, w, 2.0f, COLOR(241, 196, 15, 255));
    UI::Draw_Text("MODULAR PLUGINS (All Disabled by Default)", x + 16.0f, curY + 10.0f, COLOR(241, 196, 15, 255), 0.9f);

    FLOAT pY = curY + 40.0f;
    for (const auto &p : plugins) {
        if (!p) continue;
        BOOL isEn = p->Is_Enabled();
        UI::Draw_Box(x + 15.0f, pY, w - 30.0f, 55.0f, COLOR(28, 34, 46, 255));

        UI::Draw_Text(p->Get_Name(), x + 25.0f, pY + 8.0f, COLOR_WHITE, 0.85f);
        UI::Draw_Text(StringFormat("Category: {}  |  Version: {}", p->Get_Category(), p->Get_Version()), x + 25.0f, pY + 28.0f, COLOR(140, 155, 180, 255), 0.7f);

        // Toggle button
        BOOL hovTog = Check_Click_Rect(mouse, x + w - 140.0f, pY + 12.0f, 95.0f, 30.0f);
        UI::Draw_Box(x + w - 140.0f, pY + 12.0f, 95.0f, 30.0f, isEn ? COLOR(46, 204, 113, 255) : (hovTog ? COLOR(60, 70, 85, 255) : COLOR(40, 48, 60, 255)));
        UI::Draw_Text(isEn ? "ENABLED" : "DISABLED", x + w - 128.0f, pY + 20.0f, isEn ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.75f);

        if (hovTog && bClick) {
            M_PluginManager::Set_Plugin_Enabled(p->Get_ID(), !isEn);
            Set_Status(StringFormat("Plugin [{}] is now {}", p->Get_Name(), !isEn ? "ENABLED" : "DISABLED"));
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
        }

        pY += 62.0f;
    }
}

void E_EditorApp::Draw_TopToolbar(FLOAT width) {
    UI::Draw_Box(0, 0, width, 42.0f, COLOR(20, 24, 32, 255));
    UI::Draw_Box(0, 41.0f, width, 1.0f, COLOR(45, 55, 75, 255));

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // [ < Hub ] Button: Returns to Flax/Unreal Project Hub
    BOOL hovHub = Check_Click_Rect(mouse, 8.0f, 6.0f, 80.0f, 30.0f);
    UI::Draw_Box(8.0f, 6.0f, 80.0f, 30.0f, hovHub ? COLOR(52, 73, 94, 255) : COLOR(35, 45, 60, 255));
    UI::Draw_Text("< Hub", 26.0f, 13.0f, COLOR_WHITE, 0.85f);
    if (hovHub && bClick) {
        Set_AppMode(AppMode::ProjectHub);
        return;
    }

    // Logo & Project Title
    UI::Draw_Text("AURORA STUDIO", 100.0f, 11.0f, COLOR(52, 152, 219, 255), 1.0f);

    String projLabel;
    if (E_ProjectManager::Is_Temporary()) {
        projLabel = "|  Untitled Project * (Unsaved)";
    } else {
        projLabel = StringFormat("|  Project: {}", E_ProjectManager::Get_ActiveProject().Name);
    }
    UI::Draw_Text(projLabel, 235.0f, 12.0f, E_ProjectManager::Is_Temporary() ? COLOR(241, 196, 15, 255) : COLOR(160, 175, 200, 255), 0.85f);

    struct ToolBtn {
        const char *Label;
        FLOAT X;
        FLOAT W;
        COLOR Tint;
        EditorModal TargetModal;
        bool IsAction;
        int ActionID; // 1 = Play/Stop, 2 = Save, 3 = Pause/Continue, 4 = Step Frame
    };

    std::vector<ToolBtn> btns;
    if (!s_IsPlayMode) {
        btns.push_back({ "[ > Play (F5) ]", 440.0f, 100.0f, COLOR(46, 204, 113, 255), EditorModal::None, true, 1 });
        btns.push_back({ "Save (Ctrl+S)", 545.0f, 95.0f, COLOR(52, 152, 219, 255), EditorModal::None, true, 2 });
        btns.push_back({ "Save As...", 645.0f, 85.0f, COLOR(41, 128, 185, 255), EditorModal::SaveProjectAs, false, 0 });
        btns.push_back({ "Settings", 735.0f, 75.0f, COLOR(52, 152, 219, 255), EditorModal::Settings, false, 0 });
        btns.push_back({ "Plugins", 815.0f, 75.0f, COLOR(155, 89, 182, 255), EditorModal::PluginsList, false, 0 });
        btns.push_back({ "Asset Packer", 895.0f, 100.0f, COLOR(241, 196, 15, 255), EditorModal::AssetPacker, false, 0 });
        btns.push_back({ "XOR Gen", 1000.0f, 85.0f, COLOR(26, 188, 156, 255), EditorModal::XORGenerator, false, 0 });
        btns.push_back({ "Build Game", 1090.0f, 90.0f, COLOR(231, 76, 60, 255), EditorModal::BuildGame, false, 0 });
    } else {
        btns.push_back({ "[ [] Stop (Esc) ]", 440.0f, 105.0f, COLOR(231, 76, 60, 255), EditorModal::None, true, 1 });
        if (s_IsPlayPaused) {
            btns.push_back({ "[ > Continue (F6) ]", 550.0f, 115.0f, COLOR(46, 204, 113, 255), EditorModal::None, true, 3 });
        } else {
            btns.push_back({ "[ || Pause (F6) ]", 550.0f, 105.0f, COLOR(241, 196, 15, 255), EditorModal::None, true, 3 });
        }
        btns.push_back({ "[ >> Step (F7) ]", 670.0f, 100.0f, COLOR(52, 152, 219, 255), EditorModal::None, true, 4 });
        btns.push_back({ "Settings", 775.0f, 75.0f, COLOR(52, 152, 219, 255), EditorModal::Settings, false, 0 });
        btns.push_back({ "Plugins", 855.0f, 75.0f, COLOR(155, 89, 182, 255), EditorModal::PluginsList, false, 0 });
        btns.push_back({ "Build Game", 935.0f, 90.0f, COLOR(231, 76, 60, 255), EditorModal::BuildGame, false, 0 });
    }

    for (const auto &b : btns) {
        BOOL hov = Check_Click_Rect(mouse, b.X, 6.0f, b.W, 30.0f);
        UI::Draw_Box(b.X, 6.0f, b.W, 30.0f, hov ? COLOR(40, 50, 70, 255) : COLOR(28, 34, 46, 255));
        UI::Draw_Box(b.X, 6.0f, b.W, 2.0f, b.Tint);
        UI::Draw_Text(b.Label, b.X + 8.0f, 13.0f, hov ? COLOR_WHITE : COLOR(200, 210, 230, 255), 0.85f);

        if (hov && bClick) {
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
            if (b.IsAction) {
                if (b.ActionID == 1) Toggle_PlayMode();
                else if (b.ActionID == 2) Save_Current_Scene();
                else if (b.ActionID == 3) Toggle_Pause_PlayMode();
                else if (b.ActionID == 4) Step_Single_Frame();
            } else {
                s_ActiveModal = b.TargetModal;
                if (s_ActiveModal == EditorModal::SaveProjectAs) {
                    s_SaveActiveField = 1;
                    if (s_SaveProjName.empty() || s_SaveProjName == "Untitled Project") {
                        s_SaveProjName = "MyGame";
                        s_SaveProjPath = "D:/MyGame";
                    }
                }
            }
        }
    }
}

void E_EditorApp::Draw_TabBar(FLOAT width) {
    UI::Draw_Box(0, 42.0f, width, 30.0f, COLOR(22, 26, 36, 255));
    UI::Draw_Box(0, 71.0f, width, 1.0f, COLOR(45, 55, 75, 255));

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Tab 1: [ 🎮 Scene ] (Permanent — Cannot be closed! Visual Studio / Flax style!)
    BOOL isSceneActive = (s_ActiveWorkspace == WorkspaceTab::Scene3D);
    FLOAT tab1W = 105.0f;
    BOOL hovTab1 = Check_Click_Rect(mouse, 8.0f, 44.0f, tab1W, 27.0f);
    UI::Draw_Box(8.0f, 44.0f, tab1W, 27.0f, isSceneActive ? COLOR(32, 40, 56, 255) : (hovTab1 ? COLOR(28, 34, 46, 255) : COLOR(18, 22, 30, 255)));
    if (isSceneActive) {
        UI::Draw_Box(8.0f, 44.0f, tab1W, 2.0f, COLOR(52, 152, 219, 255));
    }
    UI::Draw_Text("🎮 Scene", 26.0f, 50.0f, isSceneActive ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.85f);
    if (hovTab1 && bClick) {
        s_ActiveWorkspace = WorkspaceTab::Scene3D;
    }

    // Tab 2: [ 🎨 UI Designer  × ] (Closable!)
    FLOAT nextX = 120.0f;
    if (s_IsUIDesignerOpen) {
        BOOL isUIActive = (s_ActiveWorkspace == WorkspaceTab::UIDesigner);
        FLOAT tab2W = 145.0f;
        BOOL hovTab2 = Check_Click_Rect(mouse, nextX, 44.0f, tab2W, 27.0f);
        BOOL hovCloseUI = Check_Click_Rect(mouse, nextX + tab2W - 22.0f, 48.0f, 16.0f, 18.0f);

        UI::Draw_Box(nextX, 44.0f, tab2W, 27.0f, isUIActive ? COLOR(32, 40, 56, 255) : (hovTab2 ? COLOR(28, 34, 46, 255) : COLOR(18, 22, 30, 255)));
        if (isUIActive) {
            UI::Draw_Box(nextX, 44.0f, tab2W, 2.0f, COLOR(155, 89, 182, 255));
        }
        UI::Draw_Text("🎨 UI Designer", nextX + 12.0f, 50.0f, isUIActive ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.85f);

        // Close button 'x'
        UI::Draw_Box(nextX + tab2W - 22.0f, 48.0f, 16.0f, 18.0f, hovCloseUI ? COLOR(192, 57, 43, 255) : COLOR(0, 0, 0, 0));
        UI::Draw_Text("x", nextX + tab2W - 17.0f, 49.0f, hovCloseUI ? COLOR_WHITE : COLOR(140, 155, 180, 255), 0.75f);

        if (hovCloseUI && bClick) {
            s_IsUIDesignerOpen = false;
            s_ActiveWorkspace  = WorkspaceTab::Scene3D;
        } else if (hovTab2 && bClick) {
            s_ActiveWorkspace  = WorkspaceTab::UIDesigner;
        }

        nextX += tab2W + 6.0f;
    } else {
        // [ + UI Designer ] Add Tab Button
        FLOAT addW = 120.0f;
        BOOL hovAdd = Check_Click_Rect(mouse, nextX, 44.0f, addW, 27.0f);
        UI::Draw_Box(nextX, 44.0f, addW, 27.0f, hovAdd ? COLOR(35, 45, 62, 255) : COLOR(22, 27, 36, 255));
        UI::Draw_Text("+ UI Designer", nextX + 12.0f, 50.0f, hovAdd ? COLOR_WHITE : COLOR(130, 145, 170, 255), 0.8f);
        if (hovAdd && bClick) {
            s_IsUIDesignerOpen = true;
            s_ActiveWorkspace  = WorkspaceTab::UIDesigner;
        }
    }

    // Roblox-Style [ 🎨 HUD Overlay: ON/OFF ] toggle button on right side of TabBar!
    FLOAT uiToggleW = 145.0f;
    FLOAT uiToggleX = width - uiToggleW - 12.0f;
    BOOL hovUIToggle = Check_Click_Rect(mouse, uiToggleX, 44.0f, uiToggleW, 25.0f);
    UI::Draw_Box(uiToggleX, 44.0f, uiToggleW, 25.0f, s_ShowInSceneUI ? COLOR(39, 174, 96, 220) : (hovUIToggle ? COLOR(45, 55, 75, 255) : COLOR(26, 32, 44, 255)));
    UI::Draw_Text(s_ShowInSceneUI ? "HUD Overlay (F2): ON" : "HUD Overlay (F2): OFF", uiToggleX + 10.0f, 49.0f, COLOR_WHITE, 0.78f);
    if (hovUIToggle && bClick) {
        s_ShowInSceneUI = !s_ShowInSceneUI;
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.6f);
    }
}

void E_EditorApp::Draw_Outliner(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    UI::Draw_Box(x, y, w, h, COLOR(24, 28, 38, 250));
    UI::Draw_Box(x + w - 1.0f, y, 1.0f, h, COLOR(45, 55, 75, 255));

    // Header
    UI::Draw_Box(x, y, w, 28.0f, COLOR(18, 22, 30, 255));
    UI::Draw_Text("SCENE HIERARCHY", x + 12.0f, y + 6.0f, COLOR(180, 195, 220, 255), 0.85f);

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    FLOAT itemY = y + 36.0f;
    const auto &entities = M_World::Get_Entities();
    C_Entity *selected = E_SceneEditor::Get_Selected();

    for (const auto &ent : entities) {
        if (!ent) continue;
        if (itemY + 24.0f > y + h - 50.0f) break;

        BOOL isSel = (ent.get() == selected);
        BOOL hov = Check_Click_Rect(mouse, x + 4.0f, itemY, w - 8.0f, 22.0f);

        if (isSel) {
            UI::Draw_Box(x + 4.0f, itemY, w - 8.0f, 22.0f, COLOR(41, 128, 185, 255));
        } else if (hov) {
            UI::Draw_Box(x + 4.0f, itemY, w - 8.0f, 22.0f, COLOR(38, 46, 62, 255));
        }

        String label = StringFormat("[ID {}] {}", ent->ID,
            (ent->Type == EntityType::Character) ? "Player Character" :
            (ent->Type == EntityType::Camera) ? "Main Camera" : "Static Building");

        UI::Draw_Text(label, x + 12.0f, itemY + 4.0f, isSel ? COLOR_WHITE : COLOR(200, 210, 230, 255), 0.8f);

        if (hov && bClick) {
            E_SceneEditor::Select_Entity(ent.get());
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.6f);
        }

        itemY += 24.0f;
    }

    // Action buttons at bottom of outliner
    FLOAT btnY = y + h - 42.0f;
    UI::Draw_Box(x, btnY, w, 42.0f, COLOR(18, 22, 30, 255));

    // + Building Button
    BOOL hovB = Check_Click_Rect(mouse, x + 8.0f, btnY + 7.0f, 115.0f, 28.0f);
    UI::Draw_Box(x + 8.0f, btnY + 7.0f, 115.0f, 28.0f, hovB ? COLOR(52, 73, 94, 255) : COLOR(41, 55, 72, 255));
    UI::Draw_Text("+ Building", x + 24.0f, btnY + 13.0f, COLOR_WHITE, 0.8f);
    if (hovB && bClick) {
        auto *b = M_World::Spawn<C_StaticMesh>();
        b->Position = { 0.0f, 0.0f, 0.0f };
        b->Mesh     = M_Asset::Load_Mesh("Assets/Models/Building.obj");
        b->Color    = COLOR(180, 140, 100, 255);
        b->Material = M_Material::Create_Matte(b->Color);
        b->Set_CollisionProfile(CollisionProfile::BlockAll);
        E_SceneEditor::Select_Entity(b);
        Set_Status("Spawned new Building at center.");
    }

    // Delete Button
    BOOL hovD = Check_Click_Rect(mouse, x + 130.0f, btnY + 7.0f, 115.0f, 28.0f);
    UI::Draw_Box(x + 130.0f, btnY + 7.0f, 115.0f, 28.0f, hovD ? COLOR(192, 57, 43, 255) : COLOR(150, 40, 30, 255));
    UI::Draw_Text("Delete", x + 165.0f, btnY + 13.0f, COLOR_WHITE, 0.8f);
    if (hovD && bClick && selected) {
        INT selID = selected->ID;
        E_SceneEditor::Select_Entity(nullptr);
        M_World::Destroy_Entity(selID);
        Set_Status("Deleted selected entity.");
    }
}

void E_EditorApp::Draw_Inspector(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    UI::Draw_Box(x, y, w, h, COLOR(24, 28, 38, 250));
    UI::Draw_Box(x, y, 1.0f, h, COLOR(45, 55, 75, 255));

    // Header
    UI::Draw_Box(x, y, w, 28.0f, COLOR(18, 22, 30, 255));
    UI::Draw_Text("ENTITY INSPECTOR", x + 12.0f, y + 6.0f, COLOR(180, 195, 220, 255), 0.85f);

    C_Entity *sel = E_SceneEditor::Get_Selected();
    if (!sel) {
        UI::Draw_Text("Select an entity to view properties.", x + 16.0f, y + 45.0f, COLOR(120, 135, 160, 255), 0.8f);
        return;
    }

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    FLOAT curY = y + 36.0f;

    // Entity Title & ID
    UI::Draw_Text(StringFormat("Entity #{} ({})", sel->ID,
        (sel->Type == EntityType::Character) ? "Character" :
        (sel->Type == EntityType::Camera) ? "Camera" : "StaticMesh"), x + 14.0f, curY, COLOR(52, 152, 219, 255), 0.85f);
    curY += 28.0f;

    // Transform Section Header
    UI::Draw_Box(x + 10.0f, curY, w - 20.0f, 22.0f, COLOR(32, 38, 50, 255));
    UI::Draw_Text("TRANSFORM", x + 16.0f, curY + 3.0f, COLOR(200, 215, 240, 255), 0.75f);
    curY += 28.0f;

    // Position (X, Y, Z)
    UI::Draw_Text(StringFormat("Pos:  X:{:6.1f}  Y:{:6.1f}  Z:{:6.1f}", sel->Position.x, sel->Position.y, sel->Position.z),
        x + 14.0f, curY, COLOR(220, 230, 245, 255), 0.8f);
    curY += 22.0f;

    auto DrawAdjRow = [&](const char *axis, FLOAT &val, FLOAT step) {
        UI::Draw_Text(axis, x + 14.0f, curY + 3.0f, COLOR(180, 190, 210, 255), 0.8f);

        BOOL hovM = Check_Click_Rect(mouse, x + 60.0f, curY, 35.0f, 20.0f);
        UI::Draw_Box(x + 60.0f, curY, 35.0f, 20.0f, hovM ? COLOR(52, 73, 94, 255) : COLOR(41, 55, 72, 255));
        UI::Draw_Text("-1", x + 72.0f, curY + 2.0f, COLOR_WHITE, 0.75f);
        if (hovM && bClick) val -= step;

        BOOL hovP = Check_Click_Rect(mouse, x + 105.0f, curY, 35.0f, 20.0f);
        UI::Draw_Box(x + 105.0f, curY, 35.0f, 20.0f, hovP ? COLOR(52, 73, 94, 255) : COLOR(41, 55, 72, 255));
        UI::Draw_Text("+1", x + 115.0f, curY + 2.0f, COLOR_WHITE, 0.75f);
        if (hovP && bClick) val += step;

        curY += 24.0f;
    };

    DrawAdjRow("Adj X:", sel->Position.x, 1.0f);
    DrawAdjRow("Adj Y:", sel->Position.y, 1.0f);
    DrawAdjRow("Adj Z:", sel->Position.z, 1.0f);
    curY += 6.0f;

    // Rotation
    UI::Draw_Text(StringFormat("Yaw:  {:5.1f} deg", sel->Rotation.y), x + 14.0f, curY, COLOR(220, 230, 245, 255), 0.8f);
    curY += 20.0f;
    DrawAdjRow("Turn:", sel->Rotation.y, 15.0f);
    curY += 12.0f;

    // Mesh & Material Section
    UI::Draw_Box(x + 10.0f, curY, w - 20.0f, 22.0f, COLOR(32, 38, 50, 255));
    UI::Draw_Text("MESH & MATERIAL", x + 16.0f, curY + 3.0f, COLOR(200, 215, 240, 255), 0.75f);
    curY += 28.0f;

    String meshStr = "None";
    if (auto *sm = sel->As<C_StaticMesh>()) meshStr = sm->Mesh.FilePath.empty() ? "Assets/Models/Building.obj" : sm->Mesh.FilePath;
    else if (auto *ch = sel->As<C_Character>()) meshStr = ch->Mesh.FilePath.empty() ? "Assets/Models/Character.obj" : ch->Mesh.FilePath;

    UI::Draw_Text(StringFormat("Mesh: {}", meshStr), x + 14.0f, curY, COLOR(180, 200, 220, 255), 0.75f);
    curY += 24.0f;

    // Collision profile
    const char *colStr = (sel->Profile == CollisionProfile::NoCollision) ? "NoCollision" : "BlockAll";
    UI::Draw_Text(StringFormat("Collision: {}", colStr),
        x + 14.0f, curY, COLOR(46, 204, 113, 255), 0.8f);
}

void E_EditorApp::Draw_BottomPanel(FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    UI::Draw_Box(x, y, w, h, COLOR(20, 24, 32, 255));
    UI::Draw_Box(x, y, w, 1.0f, COLOR(45, 55, 75, 255));

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);
    BOOL bRightClick = I_Mouse::Is_Pressed(MouseButton::MB_Right);

    // Tab Header
    UI::Draw_Box(x, y, w, 28.0f, COLOR(16, 20, 28, 255));

    // Tab 1: Asset Browser
    BOOL hovT1 = Check_Click_Rect(mouse, x + 10.0f, y + 2.0f, 120.0f, 24.0f);
    UI::Draw_Box(x + 10.0f, y + 2.0f, 120.0f, 24.0f, (s_BottomTab == EditorTab::Assets) ? COLOR(41, 128, 185, 255) : (hovT1 ? COLOR(35, 45, 60, 255) : COLOR(24, 28, 38, 255)));
    UI::Draw_Text("Content Browser", x + 20.0f, y + 6.0f, COLOR_WHITE, 0.8f);
    if (hovT1 && bClick) s_BottomTab = EditorTab::Assets;

    // Tab 2: Pack Manager
    BOOL hovT2 = Check_Click_Rect(mouse, x + 140.0f, y + 2.0f, 120.0f, 24.0f);
    UI::Draw_Box(x + 140.0f, y + 2.0f, 120.0f, 24.0f, (s_BottomTab == EditorTab::Packs) ? COLOR(41, 128, 185, 255) : (hovT2 ? COLOR(35, 45, 60, 255) : COLOR(24, 28, 38, 255)));
    UI::Draw_Text("Pack Manager", x + 155.0f, y + 6.0f, COLOR_WHITE, 0.8f);
    if (hovT2 && bClick) s_BottomTab = EditorTab::Packs;

    // Tab 1 Content: Asset Cards
    if (s_BottomTab == EditorTab::Assets) {
        FLOAT cardX = x + 15.0f;
        FLOAT cardY = y + 36.0f;

        for (const auto &asset : s_DiscoveredAssets) {
            if (cardX + 170.0f > x + w - 20.0f) {
                cardX = x + 15.0f;
                cardY += 60.0f;
                if (cardY + 50.0f > y + h) break;
            }

            BOOL isSel = (s_SelectedAsset == asset);
            BOOL hov = Check_Click_Rect(mouse, cardX, cardY, 160.0f, 50.0f);

            UI::Draw_Box(cardX, cardY, 160.0f, 50.0f, isSel ? COLOR(41, 128, 185, 255) : (hov ? COLOR(38, 48, 65, 255) : COLOR(26, 32, 44, 255)));

            size_t slash = asset.find_last_of('/');
            String name = (slash != String::npos) ? asset.substr(slash + 1) : asset;
            String type = asset.ends_with(".obj") ? "[Model 3D]" : (asset.ends_with(".wav") ? "[Audio 2D]" : "[Texture]");

            UI::Draw_Text(type, cardX + 8.0f, cardY + 6.0f, COLOR(241, 196, 15, 255), 0.7f);
            UI::Draw_Text(name, cardX + 8.0f, cardY + 24.0f, COLOR_WHITE, 0.75f);

            if (hov && bClick) {
                s_SelectedAsset        = asset;
                s_IsDraggingAsset      = true;
                s_DraggedAssetPath     = asset;
                s_ShowAssetContextMenu = false;
                M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.5f);
            }

            if (hov && bRightClick) {
                s_SelectedAsset = asset;
                s_ShowAssetContextMenu = true;
                s_ContextMenuPos = mouse;
            }

            cardX += 175.0f;
        }
    } else {
        // Tab 2 Content: Packs List
        FLOAT packX = x + 15.0f;
        FLOAT packY = y + 36.0f;

        auto packNames = E_AssetPacker::Get_Pack_Names();
        for (const auto &pName : packNames) {
            const auto &files = E_AssetPacker::Get_Pack_Files(pName);

            UI::Draw_Box(packX, packY, 260.0f, 135.0f, COLOR(26, 32, 44, 255));
            UI::Draw_Box(packX, packY, 260.0f, 24.0f, COLOR(38, 48, 65, 255));
            UI::Draw_Text(StringFormat("Pack: {} ({} files)", pName, files.size()), packX + 8.0f, packY + 4.0f, COLOR(46, 204, 113, 255), 0.8f);

            FLOAT fY = packY + 28.0f;
            for (size_t fi = 0; fi < files.size() && fi < 4; ++fi) {
                size_t sl = files[fi].find_last_of('/');
                String fn = (sl != String::npos) ? files[fi].substr(sl + 1) : files[fi];
                UI::Draw_Text(StringFormat(" * {}", fn), packX + 12.0f, fY, COLOR(180, 195, 220, 255), 0.75f);
                fY += 20.0f;
            }
            if (files.size() > 4) {
                UI::Draw_Text(StringFormat("   ... +{} more files", files.size() - 4), packX + 12.0f, fY, COLOR(120, 135, 160, 255), 0.7f);
            }

            packX += 275.0f;
        }

        BOOL hovBld = Check_Click_Rect(mouse, x + w - 160.0f, y + 4.0f, 140.0f, 22.0f);
        UI::Draw_Box(x + w - 160.0f, y + 4.0f, 140.0f, 22.0f, hovBld ? COLOR(231, 76, 60, 255) : COLOR(192, 57, 43, 255));
        UI::Draw_Text("Build All Packs", x + w - 145.0f, y + 7.0f, COLOR_WHITE, 0.75f);
        if (hovBld && bClick) {
            E_AssetPacker::Build_All("Packs", s_GeneratedKey.data(), s_GeneratedKey.size());
            Set_Status("Built encrypted .pak files in Packs/!");
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
        }
    }
}

void E_EditorApp::Draw_ContextMenu() {
    if (!s_ShowAssetContextMenu || s_SelectedAsset.empty()) return;

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    FLOAT mx = s_ContextMenuPos.x;
    FLOAT my = s_ContextMenuPos.y;
    FLOAT mw = 180.0f;
    FLOAT mh = 100.0f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 28, 38, 255));
    UI::Draw_Box(mx, my, mw, mh, COLOR(52, 73, 94, 255));

    BOOL hov1 = Check_Click_Rect(mouse, mx + 2.0f, my + 4.0f, mw - 4.0f, 26.0f);
    UI::Draw_Box(mx + 2.0f, my + 4.0f, mw - 4.0f, 26.0f, hov1 ? COLOR(41, 128, 185, 255) : COLOR(28, 34, 46, 255));
    UI::Draw_Text("Add to GameData.pak", mx + 10.0f, my + 9.0f, COLOR_WHITE, 0.75f);
    if (hov1 && bClick) {
        E_AssetPacker::Add_To_Pack("GameData.pak", s_SelectedAsset);
        E_AssetPacker::Save_Config("Config/Packs.ini");
        Set_Status(StringFormat("Added [{}] to GameData.pak", s_SelectedAsset));
        s_ShowAssetContextMenu = false;
    }

    BOOL hov2 = Check_Click_Rect(mouse, mx + 2.0f, my + 34.0f, mw - 4.0f, 26.0f);
    UI::Draw_Box(mx + 2.0f, my + 34.0f, mw - 4.0f, 26.0f, hov2 ? COLOR(41, 128, 185, 255) : COLOR(28, 34, 46, 255));
    UI::Draw_Text("Add to Audio.pak", mx + 10.0f, my + 39.0f, COLOR_WHITE, 0.75f);
    if (hov2 && bClick) {
        E_AssetPacker::Add_To_Pack("Audio.pak", s_SelectedAsset);
        E_AssetPacker::Save_Config("Config/Packs.ini");
        Set_Status(StringFormat("Added [{}] to Audio.pak", s_SelectedAsset));
        s_ShowAssetContextMenu = false;
    }

    BOOL hov3 = Check_Click_Rect(mouse, mx + 2.0f, my + 64.0f, mw - 4.0f, 26.0f);
    UI::Draw_Box(mx + 2.0f, my + 64.0f, mw - 4.0f, 26.0f, hov3 ? COLOR(39, 174, 96, 255) : COLOR(28, 34, 46, 255));
    UI::Draw_Text("Spawn into Scene", mx + 10.0f, my + 69.0f, COLOR_WHITE, 0.75f);
    if (hov3 && bClick) {
        if (s_SelectedAsset.ends_with(".obj")) {
            auto *sm = M_World::Spawn<C_StaticMesh>();
            sm->Position = { 0.0f, 0.0f, 0.0f };
            sm->Mesh     = M_Asset::Load_Mesh(s_SelectedAsset);
            sm->Color    = COLOR(200, 160, 120, 255);
            sm->Material = M_Material::Create_Matte(sm->Color);
            sm->Set_CollisionProfile(CollisionProfile::BlockAll);
            E_SceneEditor::Select_Entity(sm);
            Set_Status(StringFormat("Spawned mesh [{}] in scene.", s_SelectedAsset));
        }
        s_ShowAssetContextMenu = false;
    }

    if (bClick && !Check_Click_Rect(mouse, mx, my, mw, mh)) {
        s_ShowAssetContextMenu = false;
    }
}

void E_EditorApp::Draw_SaveProjectAs_Modal(FLOAT screenW, FLOAT screenH) {
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(0, 0, 0, 190));

    FLOAT mw = 560.0f;
    FLOAT mh = 310.0f;
    FLOAT mx = (screenW - mw) * 0.5f;
    FLOAT my = (screenH - mh) * 0.5f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 28, 38, 255));
    UI::Draw_Box(mx, my, mw, 3.0f, COLOR(41, 128, 185, 255));

    UI::Draw_Text("SAVE PROJECT AS", mx + 25.0f, my + 20.0f, COLOR_WHITE, 1.0f);
    UI::Draw_Text("Select a folder location to permanently save your project workspace.", mx + 25.0f, my + 46.0f, COLOR(160, 175, 200, 255), 0.8f);

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Field 1: Project Name
    UI::Draw_Text("Project Name:", mx + 25.0f, my + 85.0f, COLOR(200, 215, 235, 255), 0.85f);
    BOOL hovF1 = Check_Click_Rect(mouse, mx + 25.0f, my + 105.0f, mw - 50.0f, 32.0f);
    UI::Draw_Box(mx + 25.0f, my + 105.0f, mw - 50.0f, 32.0f, (s_SaveActiveField == 1) ? COLOR(30, 42, 60, 255) : COLOR(16, 20, 28, 255));
    if (s_SaveActiveField == 1) UI::Draw_Box(mx + 25.0f, my + 135.0f, mw - 50.0f, 2.0f, COLOR(52, 152, 219, 255));
    UI::Draw_Text(s_SaveProjName + (s_SaveActiveField == 1 ? "_" : ""), mx + 35.0f, my + 112.0f, COLOR_WHITE, 0.85f);
    if (hovF1 && bClick) s_SaveActiveField = 1;

    // Field 2: Save Path
    UI::Draw_Text("Project Directory Path:", mx + 25.0f, my + 150.0f, COLOR(200, 215, 235, 255), 0.85f);
    BOOL hovF2 = Check_Click_Rect(mouse, mx + 25.0f, my + 170.0f, mw - 50.0f, 32.0f);
    UI::Draw_Box(mx + 25.0f, my + 170.0f, mw - 50.0f, 32.0f, (s_SaveActiveField == 2) ? COLOR(30, 42, 60, 255) : COLOR(16, 20, 28, 255));
    if (s_SaveActiveField == 2) UI::Draw_Box(mx + 25.0f, my + 200.0f, mw - 50.0f, 2.0f, COLOR(52, 152, 219, 255));
    UI::Draw_Text(s_SaveProjPath + (s_SaveActiveField == 2 ? "_" : ""), mx + 35.0f, my + 177.0f, COLOR_WHITE, 0.85f);
    if (hovF2 && bClick) s_SaveActiveField = 2;

    // [ Save Project ] button
    BOOL hovSave = Check_Click_Rect(mouse, mx + 25.0f, my + 235.0f, 160.0f, 38.0f);
    UI::Draw_Box(mx + 25.0f, my + 235.0f, 160.0f, 38.0f, hovSave ? COLOR(46, 204, 113, 255) : COLOR(39, 174, 96, 255));
    UI::Draw_Text("Save Project", mx + 55.0f, my + 246.0f, COLOR_WHITE, 0.9f);
    if (hovSave && bClick) {
        Do_Save_Project_As();
    }

    // [ Cancel ] button
    BOOL hovCancel = Check_Click_Rect(mouse, mx + 200.0f, my + 235.0f, 110.0f, 38.0f);
    UI::Draw_Box(mx + 200.0f, my + 235.0f, 110.0f, 38.0f, hovCancel ? COLOR(70, 80, 95, 255) : COLOR(40, 50, 65, 255));
    UI::Draw_Text("Cancel", mx + 230.0f, my + 246.0f, COLOR_WHITE, 0.85f);
    if (hovCancel && bClick) {
        s_ActiveModal = EditorModal::None;
    }
}

void E_EditorApp::Draw_PluginsList_Modal(FLOAT screenW, FLOAT screenH) {
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(0, 0, 0, 190));

    FLOAT mw = 620.0f;
    FLOAT mh = 380.0f;
    FLOAT mx = (screenW - mw) * 0.5f;
    FLOAT my = (screenH - mh) * 0.5f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 28, 38, 255));
    UI::Draw_Box(mx, my, mw, 3.0f, COLOR(155, 89, 182, 255));

    UI::Draw_Text("MODULAR PLUGINS MANAGER", mx + 25.0f, my + 20.0f, COLOR_WHITE, 1.0f);
    UI::Draw_Text("Toggle built-in tools (Blueprint Visual Scripting, In-Editor Code Editor).", mx + 25.0f, my + 44.0f, COLOR(160, 175, 200, 255), 0.8f);

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    const auto &plugins = M_PluginManager::Get_Plugins();
    FLOAT py = my + 80.0f;

    for (const auto &p : plugins) {
        if (!p) continue;
        BOOL isEn = p->Is_Enabled();

        UI::Draw_Box(mx + 25.0f, py, mw - 50.0f, 65.0f, COLOR(18, 22, 30, 255));
        UI::Draw_Box(mx + 25.0f, py, 3.0f, 65.0f, isEn ? COLOR(46, 204, 113, 255) : COLOR(100, 115, 140, 255));

        UI::Draw_Text(p->Get_Name(), mx + 40.0f, py + 10.0f, COLOR_WHITE, 0.9f);
        UI::Draw_Text(p->Get_Description(), mx + 40.0f, py + 32.0f, COLOR(140, 160, 190, 255), 0.75f);

        // Toggle Button
        BOOL hovBtn = Check_Click_Rect(mouse, mx + mw - 145.0f, py + 16.0f, 95.0f, 32.0f);
        UI::Draw_Box(mx + mw - 145.0f, py + 16.0f, 95.0f, 32.0f, isEn ? COLOR(46, 204, 113, 255) : (hovBtn ? COLOR(55, 65, 80, 255) : COLOR(35, 45, 58, 255)));
        UI::Draw_Text(isEn ? "ENABLED" : "DISABLED", mx + mw - 132.0f, py + 24.0f, isEn ? COLOR_WHITE : COLOR(160, 175, 200, 255), 0.75f);

        if (hovBtn && bClick) {
            M_PluginManager::Set_Plugin_Enabled(p->Get_ID(), !isEn);
            Set_Status(StringFormat("Plugin [{}] is now {}", p->Get_Name(), !isEn ? "ENABLED" : "DISABLED"));
            M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 0.8f);
        }

        py += 75.0f;
    }

    // [ Close ] button
    BOOL hovClose = Check_Click_Rect(mouse, mx + mw - 130.0f, my + mh - 45.0f, 105.0f, 32.0f);
    UI::Draw_Box(mx + mw - 130.0f, my + mh - 45.0f, 105.0f, 32.0f, hovClose ? COLOR(70, 80, 95, 255) : COLOR(40, 50, 65, 255));
    UI::Draw_Text("Close", mx + mw - 95.0f, my + mh - 36.0f, COLOR_WHITE, 0.85f);
    if (hovClose && bClick) {
        s_ActiveModal = EditorModal::None;
    }
}

void E_EditorApp::Draw_XOR_Modal(FLOAT screenW, FLOAT screenH) {
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(0, 0, 0, 180));

    FLOAT mw = 550.0f;
    FLOAT mh = 360.0f;
    FLOAT mx = (screenW - mw) * 0.5f;
    FLOAT my = (screenH - mh) * 0.5f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 28, 38, 255));
    UI::Draw_Box(mx, my, mw, 3.0f, COLOR(26, 188, 156, 255));

    UI::Draw_Text("XOR ENCRYPTION KEY GENERATOR", mx + 20.0f, my + 18.0f, COLOR_WHITE, 1.0f);
    UI::Draw_Text("Generates 256-bit cryptographic keystream for standalone .pak protection.", mx + 20.0f, my + 44.0f, COLOR(160, 175, 200, 255), 0.8f);

    UI::Draw_Box(mx + 20.0f, my + 75.0f, mw - 40.0f, 35.0f, COLOR(16, 20, 28, 255));
    UI::Draw_Text(StringFormat("Passphrase: \"{}\"", s_XorPassphrase), mx + 30.0f, my + 84.0f, COLOR(241, 196, 15, 255), 0.85f);

    UI::Draw_Box(mx + 20.0f, my + 125.0f, mw - 40.0f, 110.0f, COLOR(16, 20, 28, 255));
    UI::Draw_Text("Generated 256-Bit Keystream:", mx + 30.0f, my + 135.0f, COLOR(120, 135, 160, 255), 0.75f);

    String hex = E_XORGenerator::Format_HexBytes(s_GeneratedKey);
    UI::Draw_Text(hex, mx + 30.0f, my + 155.0f, COLOR(46, 204, 113, 255), 0.75f);

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    BOOL hovSave = Check_Click_Rect(mouse, mx + 20.0f, my + 260.0f, 240.0f, 35.0f);
    UI::Draw_Box(mx + 20.0f, my + 260.0f, 240.0f, 35.0f, hovSave ? COLOR(26, 188, 156, 255) : COLOR(22, 160, 133, 255));
    UI::Draw_Text("Save Config/EncryptionKey.h", mx + 30.0f, my + 270.0f, COLOR_WHITE, 0.8f);
    if (hovSave && bClick) {
        E_XORGenerator::Save_Header_To_File("Config/EncryptionKey.h", s_XorPassphrase, s_GeneratedKey);
        s_XorStatusMsg = "Saved to Config/EncryptionKey.h!";
        Set_Status("Exported XOR key header!");
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
    }

    BOOL hovClose = Check_Click_Rect(mouse, mx + mw - 120.0f, my + 260.0f, 100.0f, 35.0f);
    UI::Draw_Box(mx + mw - 120.0f, my + 260.0f, 100.0f, 35.0f, hovClose ? COLOR(231, 76, 60, 255) : COLOR(192, 57, 43, 255));
    UI::Draw_Text("Close", mx + mw - 85.0f, my + 270.0f, COLOR_WHITE, 0.85f);
    if (hovClose && bClick) {
        s_ActiveModal = EditorModal::None;
    }

    if (!s_XorStatusMsg.empty()) {
        UI::Draw_Text(s_XorStatusMsg, mx + 20.0f, my + 310.0f, COLOR(46, 204, 113, 255), 0.8f);
    }
}

void E_EditorApp::Draw_BuildGame_Modal(FLOAT screenW, FLOAT screenH) {
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(0, 0, 0, 180));

    FLOAT mw = 480.0f;
    FLOAT mh = 240.0f;
    FLOAT mx = (screenW - mw) * 0.5f;
    FLOAT my = (screenH - mh) * 0.5f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 28, 38, 255));
    UI::Draw_Box(mx, my, mw, 3.0f, COLOR(231, 76, 60, 255));

    UI::Draw_Text("BUILD STANDALONE GAME", mx + 20.0f, my + 18.0f, COLOR_WHITE, 1.0f);
    UI::Draw_Text("Packages all .pak archives with encryption and prepares the release game client.", mx + 20.0f, my + 44.0f, COLOR(160, 175, 200, 255), 0.8f);

    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    BOOL hovBuild = Check_Click_Rect(mouse, mx + 20.0f, my + 120.0f, 160.0f, 35.0f);
    UI::Draw_Box(mx + 20.0f, my + 120.0f, 160.0f, 35.0f, hovBuild ? COLOR(46, 204, 113, 255) : COLOR(39, 174, 96, 255));
    UI::Draw_Text("Start Game Build", mx + 35.0f, my + 130.0f, COLOR_WHITE, 0.85f);
    if (hovBuild && bClick) {
        E_AssetPacker::Build_All("Build/Release/Packs", s_GeneratedKey.data(), s_GeneratedKey.size());
        Set_Status("Game build completed! Outputs in Build/Release/");
        M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
        s_ActiveModal = EditorModal::None;
    }

    BOOL hovCancel = Check_Click_Rect(mouse, mx + 200.0f, my + 120.0f, 100.0f, 35.0f);
    UI::Draw_Box(mx + 200.0f, my + 120.0f, 100.0f, 35.0f, hovCancel ? COLOR(70, 80, 95, 255) : COLOR(40, 50, 65, 255));
    UI::Draw_Text("Close", mx + 230.0f, my + 130.0f, COLOR_WHITE, 0.85f);
    if (hovCancel && bClick) {
        s_ActiveModal = EditorModal::None;
    }
}

void E_EditorApp::Draw_Modals(FLOAT screenW, FLOAT screenH) {
    if (s_ActiveModal == EditorModal::SaveProjectAs) {
        Draw_SaveProjectAs_Modal(screenW, screenH);
    } else if (s_ActiveModal == EditorModal::PluginsList) {
        Draw_PluginsList_Modal(screenW, screenH);
    } else if (s_ActiveModal == EditorModal::XORGenerator) {
        Draw_XOR_Modal(screenW, screenH);
    } else if (s_ActiveModal == EditorModal::BuildGame) {
        Draw_BuildGame_Modal(screenW, screenH);
    } else if (s_ActiveModal == EditorModal::AssetPacker) {
        s_BottomTab = EditorTab::Packs;
        s_ActiveModal = EditorModal::None;
    } else if (s_ActiveModal == EditorModal::Settings) {
        Draw_Settings_Modal(screenW, screenH);
    }
}

void E_EditorApp::Draw_Settings_Modal(FLOAT screenW, FLOAT screenH) {
    Vector2D mouse = I_Mouse::Get_Position();
    BOOL bClick = I_Mouse::Is_Pressed(MouseButton::MB_Left);

    // Dim Background Overlay (Unreal style)
    UI::Draw_Box(0, 0, screenW, screenH, COLOR(0, 0, 0, 170));

    // Centered Modal Window
    FLOAT mw = 560.0f;
    FLOAT mh = 380.0f;
    FLOAT mx = (screenW - mw) * 0.5f;
    FLOAT my = (screenH - mh) * 0.5f;

    UI::Draw_Box(mx, my, mw, mh, COLOR(24, 30, 42, 255));
    UI::Draw_Box(mx, my, mw, 3.0f, COLOR(52, 152, 219, 255));
    UI::Draw_Box(mx, my + 44.0f, mw, 1.0f, COLOR(45, 55, 75, 255));

    // Modal Title
    UI::Draw_Text("EDITOR & PROJECT SETTINGS", mx + 20.0f, my + 14.0f, COLOR_WHITE, 1.05f);

    // [ X ] Close button
    BOOL hovClose = Check_Click_Rect(mouse, mx + mw - 38.0f, my + 10.0f, 26.0f, 26.0f);
    UI::Draw_Box(mx + mw - 38.0f, my + 10.0f, 26.0f, 26.0f, hovClose ? COLOR(192, 57, 43, 255) : COLOR(35, 42, 56, 255));
    UI::Draw_Text("X", mx + mw - 30.0f, my + 16.0f, COLOR_WHITE, 0.85f);
    if (hovClose && bClick) {
        s_ActiveModal = EditorModal::None;
        return;
    }

    FLOAT curY = my + 60.0f;

    // 1. Graphics Backend
    UI::Draw_Text("Graphics API Backend:", mx + 24.0f, curY + 6.0f, COLOR(180, 195, 220, 255), 0.85f);
    String projBackend = E_ProjectManager::Get_ActiveProject().RHIBackend;
    if (projBackend.empty()) {
        projBackend = (RHI_Manager::Get_ActiveAPI() == API_Type::DirectX11) ? "DirectX 11" : "Vulkan";
    }
    BOOL isDX = (projBackend == "DirectX 11");
    BOOL isVK = (projBackend == "Vulkan");

    BOOL hovDX = Check_Click_Rect(mouse, mx + 220.0f, curY, 130.0f, 28.0f);
    UI::Draw_Box(mx + 220.0f, curY, 130.0f, 28.0f, isDX ? COLOR(41, 128, 185, 255) : (hovDX ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text("DirectX 11", mx + 248.0f, curY + 6.0f, COLOR_WHITE, 0.8f);

    BOOL hovVK = Check_Click_Rect(mouse, mx + 360.0f, curY, 130.0f, 28.0f);
    UI::Draw_Box(mx + 360.0f, curY, 130.0f, 28.0f, isVK ? COLOR(41, 128, 185, 255) : (hovVK ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text("Vulkan", mx + 400.0f, curY + 6.0f, COLOR_WHITE, 0.8f);

    if (hovDX && bClick && !isDX) {
        E_ProjectManager::Set_ActiveRHI("DirectX 11");
        E_ProjectManager::Save_Project();
        Set_Status("Graphics API saved as DirectX 11 (Restart Aurora Studio to apply)");
    }
    if (hovVK && bClick && !isVK) {
        E_ProjectManager::Set_ActiveRHI("Vulkan");
        E_ProjectManager::Save_Project();
        Set_Status("Graphics API saved as Vulkan (Restart Aurora Studio to apply)");
    }

    // Notice if pending restart
    API_Type activeAPI = RHI_Manager::Get_ActiveAPI();
    BOOL bMismatch = (isDX && activeAPI != API_Type::DirectX11) || (isVK && activeAPI != API_Type::Vulkan);
    if (bMismatch) {
        UI::Draw_Text("* Saved. Restart Aurora Studio to switch API safely.", mx + 220.0f, curY + 32.0f, COLOR(241, 196, 15, 255), 0.72f);
    }

    curY += 45.0f;

    // 2. VSync
    UI::Draw_Text("Vertical Sync (VSync):", mx + 24.0f, curY + 6.0f, COLOR(180, 195, 220, 255), 0.85f);
    BOOL vsync = RHI_Manager::Get_VSync();
    BOOL hovVS = Check_Click_Rect(mouse, mx + 220.0f, curY, 90.0f, 28.0f);
    UI::Draw_Box(mx + 220.0f, curY, 90.0f, 28.0f, vsync ? COLOR(39, 174, 96, 255) : (hovVS ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text(vsync ? "ENABLED" : "DISABLED", mx + 232.0f, curY + 6.0f, COLOR_WHITE, 0.8f);
    if (hovVS && bClick) RHI_Manager::Set_VSync(!vsync);

    curY += 45.0f;

    // 3. Editor Freecam Navigation
    UI::Draw_Text("Freecam Speed (Hold RMB):", mx + 24.0f, curY + 6.0f, COLOR(180, 195, 220, 255), 0.85f);
    FLOAT spd = E_SceneEditor::Get_MoveSpeed();
    BOOL hovSpd1 = Check_Click_Rect(mouse, mx + 220.0f, curY, 70.0f, 28.0f);
    BOOL hovSpd2 = Check_Click_Rect(mouse, mx + 298.0f, curY, 70.0f, 28.0f);
    BOOL hovSpd3 = Check_Click_Rect(mouse, mx + 376.0f, curY, 70.0f, 28.0f);

    UI::Draw_Box(mx + 220.0f, curY, 70.0f, 28.0f, (spd < 10.0f) ? COLOR(41, 128, 185, 255) : (hovSpd1 ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text("8 m/s", mx + 235.0f, curY + 6.0f, COLOR_WHITE, 0.8f);
    UI::Draw_Box(mx + 298.0f, curY, 70.0f, 28.0f, (spd >= 10.0f && spd < 25.0f) ? COLOR(41, 128, 185, 255) : (hovSpd2 ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text("15 m/s", mx + 310.0f, curY + 6.0f, COLOR_WHITE, 0.8f);
    UI::Draw_Box(mx + 376.0f, curY, 70.0f, 28.0f, (spd >= 25.0f) ? COLOR(41, 128, 185, 255) : (hovSpd3 ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text("30 m/s", mx + 388.0f, curY + 6.0f, COLOR_WHITE, 0.8f);

    if (hovSpd1 && bClick) E_SceneEditor::Set_MoveSpeed(8.0f);
    if (hovSpd2 && bClick) E_SceneEditor::Set_MoveSpeed(15.0f);
    if (hovSpd3 && bClick) E_SceneEditor::Set_MoveSpeed(30.0f);

    curY += 45.0f;

    // 4. Collision Wireframe Debug (F1)
    UI::Draw_Text("Collision Bounds (F1):", mx + 24.0f, curY + 6.0f, COLOR(180, 195, 220, 255), 0.85f);
    BOOL isDbg = M_Collision::Is_DebugDrawing();
    BOOL hovDbg = Check_Click_Rect(mouse, mx + 220.0f, curY, 90.0f, 28.0f);
    UI::Draw_Box(mx + 220.0f, curY, 90.0f, 28.0f, isDbg ? COLOR(39, 174, 96, 255) : (hovDbg ? COLOR(35, 45, 62, 255) : COLOR(20, 25, 35, 255)));
    UI::Draw_Text(isDbg ? "ON" : "OFF", mx + 252.0f, curY + 6.0f, COLOR_WHITE, 0.8f);
    if (hovDbg && bClick) M_Collision::Toggle_DebugDrawing();

    curY += 45.0f;

    // 5. Active Project Info
    UI::Draw_Text("Active Project:", mx + 24.0f, curY + 6.0f, COLOR(180, 195, 220, 255), 0.85f);
    UI::Draw_Text(E_ProjectManager::Get_ActiveProject().RootDirectory.generic_string(), mx + 160.0f, curY + 6.0f, COLOR_CYAN, 0.75f);

    // Done / Close button at bottom
    FLOAT btnY = my + mh - 48.0f;
    BOOL hovDone = Check_Click_Rect(mouse, mx + (mw - 130.0f) * 0.5f, btnY, 130.0f, 32.0f);
    UI::Draw_Box(mx + (mw - 130.0f) * 0.5f, btnY, 130.0f, 32.0f, hovDone ? COLOR(41, 128, 185, 255) : COLOR(32, 42, 60, 255));
    UI::Draw_Text("Done / Close", mx + (mw - 130.0f) * 0.5f + 18.0f, btnY + 8.0f, COLOR_WHITE, 0.85f);
    if (hovDone && bClick) {
        s_ActiveModal = EditorModal::None;
    }
}

void E_EditorApp::Render() {
    FLOAT winW = static_cast<FLOAT>(M_Window::Get_Width());
    FLOAT winH = static_cast<FLOAT>(M_Window::Get_Height());

    // MODE 1: Flax / Unreal Project Hub
    if (s_AppMode == AppMode::ProjectHub) {
        Draw_ProjectHub_Screen(winW, winH);
        return;
    }

    // MODE 2: Workspaces
    if (s_ActiveWorkspace == WorkspaceTab::Scene3D) {
        // 1. Render 3D World in Viewport
        M_World::Render();

        // 2. Roblox-Style In-Scene UI Overlay
        if (s_ShowInSceneUI) {
            E_UIDesigner::Draw_InScene_Overlay(s_ViewportMin, s_ViewportMax);
        }

        // 3. 3D Raycast Drag Marker on Ground
        if (s_IsDraggingAsset && s_DragHasHit) {
            U_Raycast::Draw_DebugLine(s_DragWorldHit, s_DragWorldHit + Vector3D{ 0.0f, 2.5f, 0.0f }, COLOR_YELLOW, 0.0f);
            U_Raycast::Draw_DebugLine(s_DragWorldHit - Vector3D{ 1.0f, 0.0f, 0.0f }, s_DragWorldHit + Vector3D{ 1.0f, 0.0f, 0.0f }, COLOR_YELLOW, 0.0f);
            U_Raycast::Draw_DebugLine(s_DragWorldHit - Vector3D{ 0.0f, 0.0f, 1.0f }, s_DragWorldHit + Vector3D{ 0.0f, 0.0f, 1.0f }, COLOR_YELLOW, 0.0f);
        }

        // 4. Render 2D Editor UI Overlay
        if (!s_IsPlayMode) {
            Draw_TopToolbar(winW);
            Draw_TabBar(winW);
            Draw_Outliner(0.0f, 72.0f, 270.0f, winH - 262.0f);
            Draw_Inspector(winW - 320.0f, 72.0f, 320.0f, winH - 262.0f);
            Draw_BottomPanel(0.0f, winH - 190.0f, winW, 190.0f);
            Draw_ContextMenu();
            Draw_Modals(winW, winH);

            // Render enabled plugins UI (Blueprint / CodeEditor)
            M_PluginManager::Render_UI();
        } else {
            // Minimal Play Mode HUD
            if (s_IsPlayPaused) {
                UI::Draw_Box(10.0f, 10.0f, 520.0f, 36.0f, COLOR(20, 24, 32, 235));
                UI::Draw_Box(10.0f, 10.0f, 520.0f, 2.0f, COLOR(241, 196, 15, 255));
                UI::Draw_Text("|| PIE PAUSED  [F6 Continue | F7 Step Frame | Esc Stop | Freecam Active]", 20.0f, 21.0f, COLOR(241, 196, 15, 255), 0.85f);
            } else {
                UI::Draw_Box(10.0f, 10.0f, 380.0f, 36.0f, COLOR(20, 24, 32, 235));
                UI::Draw_Box(10.0f, 10.0f, 380.0f, 2.0f, COLOR(46, 204, 113, 255));
                UI::Draw_Text("> PLAY IN EDITOR  [F6 Pause | Esc / F5 Stop]", 20.0f, 21.0f, COLOR(46, 204, 113, 255), 0.85f);
            }
        }

        // 3D Viewport Performance Overlay (Top Right of Viewport in Edit Mode)
        if (!s_IsPlayMode) {
            FLOAT statsW = 210.0f;
            FLOAT statsX = s_ViewportMax.x - statsW - 12.0f;
            FLOAT statsY = s_ViewportMin.y + 12.0f;
            UI::Draw_Box(statsX, statsY, statsW, 46.0f, COLOR(15, 18, 25, 210));
            UI::Draw_Box(statsX, statsY, statsW, 1.0f, COLOR(40, 50, 70, 200));

            FLOAT fps = M_Time::Get_FPS();
            FLOAT ms  = M_Time::Get_DeltaTime() * 1000.0f;
            COLOR fpsCol = (fps >= 55.0f) ? COLOR(46, 204, 113, 255) : ((fps >= 30.0f) ? COLOR_YELLOW : COLOR(231, 76, 60, 255));
            UI::Draw_Text(StringFormat("FPS: {:.0f} ({:.1f} ms)", fps, ms), statsX + 10.0f, statsY + 7.0f, fpsCol, 0.8f);

            UINT entCount = static_cast<UINT>(M_World::Get_Entities().size());
            UI::Draw_Text(StringFormat("Entities: {} | Collision: {}", entCount, M_Collision::Is_DebugDrawing() ? "ON" : "OFF"), statsX + 10.0f, statsY + 26.0f, COLOR(160, 175, 205, 255), 0.72f);
        }
    } else if (s_ActiveWorkspace == WorkspaceTab::UIDesigner) {
        Draw_TopToolbar(winW);
        Draw_TabBar(winW);
        E_UIDesigner::Render({ 0.0f, 72.0f }, { winW, winH });
        Draw_Modals(winW, winH);
    }

    // Floating Drag & Drop Asset Badge
    if (s_IsDraggingAsset) {
        Vector2D mouse = I_Mouse::Get_Position();
        FilePath ap(s_DraggedAssetPath);
        UI::Draw_Box(mouse.x + 15.0f, mouse.y + 15.0f, 210.0f, 32.0f, COLOR(26, 36, 52, 240));
        UI::Draw_Box(mouse.x + 15.0f, mouse.y + 15.0f, 210.0f, 2.0f, COLOR_GOLD);
        UI::Draw_Text("📦 Spawn: " + ap.filename().string(), mouse.x + 22.0f, mouse.y + 24.0f, COLOR_WHITE, 0.8f);
    }

    // Status Notification Banner
    if (s_StatusTimer > 0.0f) {
        FLOAT alpha = std::clamp(s_StatusTimer, 0.0f, 1.0f);
        UI::Draw_Box(winW - 400.0f, winH - 225.0f, 380.0f, 28.0f, COLOR(25, 35, 45, static_cast<BYTE>(230 * alpha)));
        UI::Draw_Text(s_StatusText, winW - 390.0f, winH - 219.0f, COLOR(241, 196, 15, static_cast<BYTE>(255 * alpha)), 0.8f);
    }
}

void E_EditorApp::Shutdown() {
    M_PluginManager::Shutdown();
    s_IsRunning = false;
}
