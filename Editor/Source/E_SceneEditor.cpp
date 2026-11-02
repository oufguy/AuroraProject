#include <E_SceneEditor.h>
#include <M_World.h>
#include <I_Mouse.h>
#include <I_Keyboard.h>
#include <M_Window.h>
#include <U_Raycast.h>
#include <C_StaticMesh.h>
#include <C_Character.h>

C_Camera   *E_SceneEditor::s_Camera         = nullptr;
C_Entity   *E_SceneEditor::s_SelectedEntity = nullptr;
GizmoMode   E_SceneEditor::s_GizmoMode      = GizmoMode::Translate;
FLOAT       E_SceneEditor::s_MoveSpeed      = 15.0f;
FLOAT       E_SceneEditor::s_TurnSpeed      = 40.0f;
FLOAT       E_SceneEditor::s_StepSize       = 1.0f;

void E_SceneEditor::Init() {
    s_Camera = M_World::Get_Camera();
    if (s_Camera) {
        s_Camera->Position = { 0.0f, 12.0f, -22.0f };
        s_Camera->Rotation = { 20.0f, 0.0f, 0.0f };
    }
}

void E_SceneEditor::Select_Entity(C_Entity *entity) {
    s_SelectedEntity = entity;
}

C_Entity *E_SceneEditor::Get_Selected() {
    return s_SelectedEntity;
}

void E_SceneEditor::Set_GizmoMode(GizmoMode mode) {
    s_GizmoMode = mode;
}

GizmoMode E_SceneEditor::Get_GizmoMode() {
    return s_GizmoMode;
}

void E_SceneEditor::Move_Selected(const Vector3D &offset) {
    if (!s_SelectedEntity) return;
    s_SelectedEntity->Position += offset;
}

void E_SceneEditor::Rotate_Selected(const Vector3D &deltaDegrees) {
    if (!s_SelectedEntity) return;
    s_SelectedEntity->Rotation += deltaDegrees;
}

void E_SceneEditor::Scale_Selected(const Vector3D &deltaScale) {
    if (!s_SelectedEntity) return;
    if (auto *sm = s_SelectedEntity->As<C_StaticMesh>()) {
        sm->Size += deltaScale;
        if (sm->Size.x < 0.1f) sm->Size.x = 0.1f;
        if (sm->Size.y < 0.1f) sm->Size.y = 0.1f;
        if (sm->Size.z < 0.1f) sm->Size.z = 0.1f;
    }
}

void E_SceneEditor::Delete_Selected() {
    if (!s_SelectedEntity) return;
    M_World::Destroy_Entity(s_SelectedEntity->ID);
    s_SelectedEntity = nullptr;
}

void E_SceneEditor::Render_Gizmo() {
    if (!s_SelectedEntity) return;

    Vector3D pos = s_SelectedEntity->Position;
    FLOAT axisLen = 3.5f;

    // Draw 3D colored coordinate axes
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ axisLen, 0.0f, 0.0f }, COLOR(255, 60, 60, 255), 0.0f);   // Red   = +X
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ 0.0f, axisLen, 0.0f }, COLOR(60, 255, 60, 255), 0.0f);   // Green = +Y
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ 0.0f, 0.0f, axisLen }, COLOR(60, 120, 255, 255), 0.0f);  // Blue  = +Z
}

void E_SceneEditor::Tick(FLOAT deltaTime, const Vector2D &viewportMin, const Vector2D &viewportMax) {
    if (!s_Camera) Init();
    if (!s_Camera) return;

    Vector2D mousePos = I_Mouse::Get_Position();
    BOOL bInViewport = (mousePos.x >= viewportMin.x && mousePos.x <= viewportMax.x &&
                        mousePos.y >= viewportMin.y && mousePos.y <= viewportMax.y);

    // 1. Flycam 3D Navigation (Hold Right Mouse Button)
    BOOL bRightHeld = I_Mouse::Is_Down(MouseButton::MB_Right);
    if (bInViewport && bRightHeld) {
        M_Window::Set_MouseLocked(true);
        Vector2D delta = I_Mouse::Get_Delta();
        s_Camera->Rotation.y += delta.x * s_TurnSpeed * deltaTime;
        s_Camera->Rotation.x += delta.y * s_TurnSpeed * deltaTime;
        s_Camera->Rotation.x = std::clamp(s_Camera->Rotation.x, -89.0f, 89.0f);

        Vector3D fwd   = U_Raycast::Get_ForwardVector(s_Camera->Rotation);
        Vector3D right = U_Raycast::Get_RightVector(s_Camera->Rotation);
        FLOAT speed    = s_MoveSpeed * (I_Keyboard::Is_Down(Key::Shift) ? 2.5f : 1.0f);

        if (I_Keyboard::Is_Down(Key::W)) s_Camera->Position += fwd * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::S)) s_Camera->Position -= fwd * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::D)) s_Camera->Position += right * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::A)) s_Camera->Position -= right * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::E)) s_Camera->Position.y += speed * deltaTime;
        if (I_Keyboard::Is_Down(Key::Q)) s_Camera->Position.y -= speed * deltaTime;
    } else if (!bRightHeld) {
        M_Window::Set_MouseLocked(false);
    }

    // 2. 3D Mouse Raycast Picking (Left Click to Select)
    if (bInViewport && I_Mouse::Is_Pressed(MouseButton::MB_Left)) {
        Ray3D ray = U_Raycast::Screen_To_World(mousePos, s_Camera->Position, s_Camera->Rotation, s_Camera->FOV);
        RayHit hit{};
        FLOAT closestDist = 999999.0f;
        C_Entity *picked = nullptr;

        for (const auto &ent : M_World::Get_Entities()) {
            if (!ent || !ent->bIsVisible) continue;
            AABB bounds = ent->Get_WorldAABB();
            FLOAT hitDist = 0.0f;
            Vector3D hitNormal;
            if (U_Raycast::Intersects_Ray_AABB(ray, bounds, hitDist, hitNormal)) {
                if (hitDist > 0.0f && hitDist < closestDist) {
                    closestDist = hitDist;
                    picked = ent.get();
                }
            }
        }

        if (picked) {
            Select_Entity(picked);
        }
    }

    // 3. Hotkeys for Gizmo Manipulation
    if (I_Keyboard::Is_Pressed(Key::Num1)) Set_GizmoMode(GizmoMode::Translate);
    if (I_Keyboard::Is_Pressed(Key::Num2)) Set_GizmoMode(GizmoMode::Rotate);
    if (I_Keyboard::Is_Pressed(Key::Num3)) Set_GizmoMode(GizmoMode::Scale);
    if (I_Keyboard::Is_Pressed(Key::Delete)) Delete_Selected();

    // 4. Keyboard Arrow Key Transform Stepping
    if (s_SelectedEntity && !bRightHeld) {
        FLOAT step = I_Keyboard::Is_Down(Key::Shift) ? 0.2f : 1.0f;

        if (s_GizmoMode == GizmoMode::Translate) {
            if (I_Keyboard::Is_Pressed(Key::Left))  Move_Selected({ -step, 0.0f, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::Right)) Move_Selected({  step, 0.0f, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::Up))    Move_Selected({ 0.0f, 0.0f,  step });
            if (I_Keyboard::Is_Pressed(Key::Down))  Move_Selected({ 0.0f, 0.0f, -step });
            if (I_Keyboard::Is_Pressed(Key::PageUp))     Move_Selected({ 0.0f,  step, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::PageDown))   Move_Selected({ 0.0f, -step, 0.0f });
        } else if (s_GizmoMode == GizmoMode::Rotate) {
            FLOAT rotStep = I_Keyboard::Is_Down(Key::Shift) ? 5.0f : 15.0f;
            if (I_Keyboard::Is_Pressed(Key::Left))  Rotate_Selected({ 0.0f, -rotStep, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::Right)) Rotate_Selected({ 0.0f,  rotStep, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::Up))    Rotate_Selected({ -rotStep, 0.0f, 0.0f });
            if (I_Keyboard::Is_Pressed(Key::Down))  Rotate_Selected({  rotStep, 0.0f, 0.0f });
        }
    }

    // 5. Render active 3D gizmo
    Render_Gizmo();
}
