#include <Core.h>
#include <Editor.h>
#include <RHI_Manager.h>
#include <M_Time.h>
#include <M_Window.h>
#include <E_ProjectManager.h>

INT main(INT argc, Char *argv[]) {
    // 1. Parse Project Directory from command line argument
    FilePath projectPath = "";
    for (INT i = 1; i < argc; ++i) {
        String arg = argv[i];
        FilePath p(arg);
        String ext = p.extension().string();
        for (char &c : ext) c = static_cast<char>(std::tolower(c));

        if (arg == "--project" && i + 1 < argc) {
            projectPath = std::filesystem::absolute(argv[++i]);
        } else if (ext == ".aurora") {
            // User double-clicked a .aurora file! Resolve parent folder as absolute project root
            projectPath = std::filesystem::absolute(p).parent_path();
        } else if (std::filesystem::is_directory(p)) {
            projectPath = std::filesystem::absolute(p);
        }
    }

    // 2. Direct Double-Click vs Bare Launch Detection:
    // If a project / .aurora was passed, skip the Hub and open directly into the 3D Level Editor!
    // If opened directly without arguments, start with the Flax/Unreal Project Hub!
    String title = "Aurora Engine — Project Hub";

    if (!projectPath.empty() && std::filesystem::exists(projectPath)) {
        E_ProjectManager::Open_Project(projectPath);
        E_EditorApp::Set_AppMode(AppMode::LevelEditor);
        title = std::format("Aurora Studio — [{}]", E_ProjectManager::Get_ActiveProject().Name);

        // Pre-configure Graphics API before window creation
        API_Type api = RHI_Manager::Parse_API(E_ProjectManager::Get_ActiveProject().RHIBackend);
        RHI_Manager::Swap_API(api);
    } else {
        E_EditorApp::Set_AppMode(AppMode::ProjectHub);
    }

    // 3. Boot Aurora Engine in Windowed mode (1280x720)
    if (!Core::BeginPlay("Aurora Studio", ProcessType::Window, title)) {
        return 1;
    }

    if (!E_EditorApp::Init()) {
        Core::EndPlay();
        return 1;
    }

    while (Core::Tick()) {
        FLOAT dt = M_Time::Get_DeltaTime();

        // Process Editor / Hub Interactions & Scene Navigation
        E_EditorApp::Tick(dt);

        // Begin Frame & Render Active Mode (Project Hub or 3D Level Editor)
        if (RHI_Manager::Begin_Frame()) {
            E_EditorApp::Render();
            RHI_Manager::End_Frame();
        }
    }

    E_EditorApp::Shutdown();
    Core::EndPlay();
    return 0;
}
