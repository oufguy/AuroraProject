#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <C_Entity.h>
#include <C_StaticMesh.h>
#include <C_DynamicMesh.h>
#include <C_Character.h>
#include <C_Camera.h>

// ============================================================================
// M_SceneSerializer: Aurora Engine 3D World Serialization (.scene format)
// ============================================================================
// Saves and loads all world entities to/from clean human-readable .scene files.
// Eliminates the need to hardcode entity transforms in C++ source code!
// ============================================================================

struct SceneEntityRecord {
    String     Name;
    String     Type;         // "StaticMesh", "Character", "DynamicMesh", "Camera"
    String     MeshPath;
    Vector3D   Position      = { 0.0f, 0.0f, 0.0f };
    Vector3D   Rotation      = { 0.0f, 0.0f, 0.0f };
    Vector3D   Scale         = { 1.0f, 1.0f, 1.0f };
    COLOR      Color         = COLOR_WHITE;
    String     MaterialType  = "Matte";
    FLOAT      Roughness     = 32.0f;
    String     Collision     = "BlockAll";
};

class M_SceneSerializer {
public:
    // Serializes all entities currently in M_World into a .scene file
    static BOOL Save(const FilePath &scenePath, const String &sceneName = "DefaultWorld");

    // Clears the current world and spawns entities described in the .scene file
    static BOOL Load(const FilePath &scenePath);

    // Parses a .scene file without modifying the active world
    static Array<SceneEntityRecord> Parse_Records(const FilePath &scenePath);

    // Creates a template town scene file if none exists
    static BOOL Create_Town_Template(const FilePath &scenePath);

    // Creates an empty blank scene template (just ground and player spawn)
    static BOOL Create_Blank_Template(const FilePath &scenePath);
};
