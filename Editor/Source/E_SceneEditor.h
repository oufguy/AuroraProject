#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <C_Entity.h>
#include <C_Camera.h>

// ============================================================================
// E_SceneEditor: Aurora Editor 3D Viewport, Flycam & Visual Transform Gizmo
// ============================================================================
// Handles:
//   1. 3D Flycam navigation (WASD + Right-Click look)
//   2. 3D Mouse raycast entity picking
//   3. 3D Coordinate axes gizmo rendering
//   4. Interactive object translation, rotation, and scaling
// ============================================================================

enum class GizmoMode : BYTE {
    Translate = 0, // Move along X, Y, Z
    Rotate    = 1, // Rotate Pitch, Yaw, Roll
    Scale     = 2  // Scale X, Y, Z
};

class E_SceneEditor {
private:
    static C_Camera   *s_Camera;
    static C_Entity   *s_SelectedEntity;
    static GizmoMode   s_GizmoMode;
    static FLOAT       s_MoveSpeed;
    static FLOAT       s_TurnSpeed;
    static FLOAT       s_StepSize;

public:
    static void Init();
    static void Tick(FLOAT deltaTime, const Vector2D &viewportMin, const Vector2D &viewportMax);
    static void Render_Gizmo();

    static void Select_Entity(C_Entity *entity);
    [[nodiscard]] static C_Entity *Get_Selected();

    static void Set_GizmoMode(GizmoMode mode);
    [[nodiscard]] static GizmoMode Get_GizmoMode();

    static void Move_Selected(const Vector3D &offset);
    static void Rotate_Selected(const Vector3D &deltaDegrees);
    static void Scale_Selected(const Vector3D &deltaScale);

    static void Delete_Selected();

    [[nodiscard]] static FLOAT Get_MoveSpeed() { return s_MoveSpeed; }
    static void Set_MoveSpeed(FLOAT speed)     { s_MoveSpeed = speed; }

    [[nodiscard]] static C_Camera *Get_Camera() { return s_Camera; }
};
