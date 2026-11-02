#pragma once

#include <pch.h>
#include <U_Variables.h>

// ============================================================================
// E_ProjectManager: Aurora Hub & Project Workspace Manager
// ============================================================================
// Stores recent projects in %APPDATA%/Aurora/RecentProjects.ini so project paths
// are never committed or shared with other people.
// Manages project creation from Templates/ and launches AuroraEditor.exe.
// ============================================================================

enum class ProjectTemplate : BYTE {
    Blank   = 0, // Clean scene with ground, lighting, character, camera
    Town    = 1  // Complete MMORPG Demo (buildings, characters, audio chime, combat)
};

struct ProjectInfo {
    String   Name;
    FilePath RootDirectory;
    String   DefaultScene = "Scenes/World.scene";
    String   RHIBackend   = "DirectX 11";
    BOOL     bIsLoaded    = false;
    BOOL     bIsTemporary = false; // True if running in a temporary directory until saved
};

struct RecentProjectEntry {
    String   Name;
    FilePath Path;
    String   LastOpened;
};

class E_ProjectManager {
private:
    static ProjectInfo s_ActiveProject;

public:
    // Path to %APPDATA%/Aurora/RecentProjects.ini (or OS preference directory)
    [[nodiscard]] static FilePath Get_AppDataConfigPath();

    // Returns all recent projects saved on this machine
    [[nodiscard]] static Array<RecentProjectEntry> Get_RecentProjects();

    // Adds or moves a project to the top of recent projects
    static void Add_RecentProject(const String &name, const FilePath &path);

    // Removes a project from the recent projects list
    static void Remove_RecentProject(size_t index);

    // Creates and opens a temporary blank project in Local Temp
    static BOOL Open_Temporary_Project();

    // Checks if the active project is currently untitled/temporary
    [[nodiscard]] static BOOL Is_Temporary() {
        return s_ActiveProject.bIsTemporary;
    }

    // Creates a new project workspace on disk by copying from Templates/
    static BOOL Create_Project(
        const FilePath  &rootDirectory,
        const String    &projectName,
        ProjectTemplate  templateType = ProjectTemplate::Town);

    // Opens an existing project workspace by loading its Project.aurora file
    static BOOL Open_Project(const FilePath &projectDirectory);

    // Saves current project metadata to Project.aurora
    static BOOL Save_Project();

    // Saves the active project (and copies temporary contents) to a new permanent location
    static BOOL Save_Project_As(const FilePath &destinationDir, const String &projectName);

    // Launches AuroraEditor.exe pointing to the given project directory
    static BOOL Launch_Editor(const FilePath &projectDirectory);

    // Returns the currently active project info
    [[nodiscard]] static const ProjectInfo &Get_ActiveProject() {
        return s_ActiveProject;
    }

    [[nodiscard]] static BOOL Has_ActiveProject() {
        return s_ActiveProject.bIsLoaded;
    }

    // Resolves a relative path to absolute within the active project directory
    [[nodiscard]] static FilePath Resolve_Path(const FilePath &relativePath);
};
