#include <E_SceneEditor.h>
#include <M_World.h>
#include <I_Mouse.h>
#include <I_Keyboard.h>
#include <M_Window.h>
#include <U_Raycast.h>
#include <C_StaticMesh.h>
#include <C_Character.h>

C_Camera   *E_SceneEditor::s_Camera             = nullptr;
C_Entity   *E_SceneEditor::s_SelectedEntity     = nullptr;
GizmoMode   E_SceneEditor::s_GizmoMode          = GizmoMode::Translate;
FLOAT       E_SceneEditor::s_MoveSpeed          = 15.0f;
FLOAT       E_SceneEditor::s_TurnSpeed          = 40.0f;
FLOAT       E_SceneEditor::s_StepSize           = 1.0f;
BOOL        E_SceneEditor::s_IsNavigating       = false;
Vector2D    E_SceneEditor::s_NavSavedMousePos   = { 0.0f, 0.0f };
Vector2D    E_SceneEditor::s_NavCenterPos       = { 0.0f, 0.0f };

BOOL        E_SceneEditor::s_IsDraggingObject   = false;
INT         E_SceneEditor::s_ActiveGizmoAxis    = 0;
Vector3D    E_SceneEditor::s_DragStartEntityPos = { 0.0f, 0.0f, 0.0f };
Vector3D    E_SceneEditor::s_DragStartWorldHit  = { 0.0f, 0.0f, 0.0f };
Vector2D    E_SceneEditor::s_DragStartMouse     = { 0.0f, 0.0f };

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

void E_SceneEditor::Duplicate_Selected() {
    if (!s_SelectedEntity) return;

    if (auto *sm = s_SelectedEntity->As<C_StaticMesh>()) {
        auto *clone = M_World::Spawn_Entity<C_StaticMesh>();
        if (clone) {
            clone->Mesh      = sm->Mesh;
            clone->Position  = sm->Position + Vector3D{ 2.0f, 0.0f, 0.0f };
            clone->Rotation  = sm->Rotation;
            clone->Size      = sm->Size;
            clone->Color     = sm->Color;
            clone->Material  = sm->Material;
            clone->TextureID = sm->TextureID;
            Select_Entity(clone);
        }
    } else if (auto *chr = s_SelectedEntity->As<C_Character>()) {
        auto *clone = M_World::Spawn_Entity<C_Character>();
        if (clone) {
            clone->Position = chr->Position + Vector3D{ 2.0f, 0.0f, 0.0f };
            clone->Rotation = chr->Rotation;
            clone->Size     = chr->Size;
            clone->Color    = chr->Color;
            Select_Entity(clone);
        }
    }
}

void E_SceneEditor::Focus_On_Selected() {
    if (!s_SelectedEntity || !s_Camera) return;

    Vector3D target = s_SelectedEntity->Position;
    Vector3D offset = { 0.0f, 3.5f, -8.0f };
    s_Camera->Position = target + offset;

    Vector3D toTarget = target - s_Camera->Position;
    FLOAT dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
    if (dist > 0.001f) {
        s_Camera->Rotation.x = std::asin(-toTarget.y / dist) * (180.0f / 3.14159265f);
        s_Camera->Rotation.y = std::atan2(toTarget.x, toTarget.z) * (180.0f / 3.14159265f);
        s_Camera->Rotation.z = 0.0f;
    }
}

void E_SceneEditor::Render_Gizmo() {
    if (!s_SelectedEntity) return;

    Vector3D pos = s_SelectedEntity->Position;
    FLOAT axisLen = 3.5f;

    // 1. Draw 3D colored coordinate axes
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ axisLen, 0.0f, 0.0f }, COLOR(255, 60, 60, 255), 0.0f);   // Red   = +X
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ 0.0f, axisLen, 0.0f }, COLOR(60, 255, 60, 255), 0.0f);   // Green = +Y
    U_Raycast::Draw_DebugLine(pos, pos + Vector3D{ 0.0f, 0.0f, axisLen }, COLOR(60, 120, 255, 255), 0.0f);  // Blue  = +Z

    // 2. Draw Center Ground Plane Drag Quad (Yellow)
    Vector3D q1 = pos + Vector3D{  1.0f, 0.0f,  0.0f };
    Vector3D q2 = pos + Vector3D{  0.0f, 0.0f,  1.0f };
    Vector3D q3 = pos + Vector3D{ -1.0f, 0.0f,  0.0f };
    Vector3D q4 = pos + Vector3D{  0.0f, 0.0f, -1.0f };
    U_Raycast::Draw_DebugLine(q1, q2, COLOR(241, 196, 15, 200), 0.0f);
    U_Raycast::Draw_DebugLine(q2, q3, COLOR(241, 196, 15, 200), 0.0f);
    U_Raycast::Draw_DebugLine(q3, q4, COLOR(241, 196, 15, 200), 0.0f);
    U_Raycast::Draw_DebugLine(q4, q1, COLOR(241, 196, 15, 200), 0.0f);

    // 3. Visual feedback while dragging
    if (s_IsDraggingObject) {
        // Drop-line down to ground y = 0
        U_Raycast::Draw_DebugLine(pos, Vector3D{ pos.x, 0.0f, pos.z }, COLOR_YELLOW, 0.0f);

        // Ground crosshair
        U_Raycast::Draw_DebugLine(Vector3D{ pos.x - 1.5f, 0.05f, pos.z }, Vector3D{ pos.x + 1.5f, 0.05f, pos.z }, COLOR_YELLOW, 0.0f);
        U_Raycast::Draw_DebugLine(Vector3D{ pos.x, 0.05f, pos.z - 1.5f }, Vector3D{ pos.x, 0.05f, pos.z + 1.5f }, COLOR_YELLOW, 0.0f);

        // Bounding box wireframe in Cyan
        AABB b = s_SelectedEntity->Get_WorldAABB();
        Vector3D c0 = b.Min;
        Vector3D c1 = { b.Max.x, b.Min.y, b.Min.z };
        Vector3D c2 = { b.Max.x, b.Min.y, b.Max.z };
        Vector3D c3 = { b.Min.x, b.Min.y, b.Max.z };
        Vector3D c4 = { b.Min.x, b.Max.y, b.Min.z };
        Vector3D c5 = { b.Max.x, b.Max.y, b.Min.z };
        Vector3D c6 = b.Max;
        Vector3D c7 = { b.Min.x, b.Max.y, b.Max.z };

        COLOR cyan = COLOR(0, 240, 255, 220);
        U_Raycast::Draw_DebugLine(c0, c1, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c1, c2, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c2, c3, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c3, c0, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c4, c5, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c5, c6, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c6, c7, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c7, c4, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c0, c4, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c1, c5, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c2, c6, cyan, 0.0f);
        U_Raycast::Draw_DebugLine(c3, c7, cyan, 0.0f);
    }
}

void E_SceneEditor::Tick(FLOAT deltaTime, const Vector2D &viewportMin, const Vector2D &viewportMax) {
    if (!s_Camera) Init();
    if (!s_Camera) return;

    Vector2D mousePos = I_Mouse::Get_Position();
    BOOL bInViewport = (mousePos.x >= viewportMin.x && mousePos.x <= viewportMax.x &&
                        mousePos.y >= viewportMin.y && mousePos.y <= viewportMax.y);

    BOOL bRightDown     = I_Mouse::Is_Pressed(MouseButton::MB_Right);
    BOOL bRightHeld     = I_Mouse::Is_Down(MouseButton::MB_Right);
    BOOL bRightReleased = I_Mouse::Is_Released(MouseButton::MB_Right);

    // 1. Enter Flycam Navigation Mode when RMB is pressed inside the 3D Viewport
    if (bRightDown && bInViewport) {
        s_IsNavigating      = true;
        s_NavSavedMousePos  = mousePos; // Save exact cursor position before navigation
        s_NavCenterPos      = { (viewportMin.x + viewportMax.x) * 0.5f, (viewportMin.y + viewportMax.y) * 0.5f };

        M_Window::Set_MouseLocked(true); // Hide cursor

        // Warp mouse immediately to viewport center
        SDL_Window *win = M_Window::Get_Handle();
        if (win) {
            SDL_WarpMouseInWindow(win, s_NavCenterPos.x, s_NavCenterPos.y);
        }
        I_Mouse::Set_Position(s_NavCenterPos);
        I_Mouse::Reset_Delta();
    }

    // 2. Continuous 3D Camera Look & Movement (Center-Warp Anchor Navigation)
    if (s_IsNavigating && bRightHeld) {
        // Read current mouse position and measure offset from center anchor
        Vector2D curPos = I_Mouse::Get_Position();
        Vector2D offset = curPos - s_NavCenterPos;

        if (std::abs(offset.x) > 0.001f || std::abs(offset.y) > 0.001f) {
            const FLOAT mouseSens = 0.15f;
            s_Camera->Rotation.y += offset.x * mouseSens;
            s_Camera->Rotation.x += offset.y * mouseSens;
            s_Camera->Rotation.x = std::clamp(s_Camera->Rotation.x, -89.0f, 89.0f);

            // Instantly reset mouse position back to center anchor!
            // Mouse never moves across other editor panels or hits monitor edges!
            SDL_Window *win = M_Window::Get_Handle();
            if (win) {
                SDL_WarpMouseInWindow(win, s_NavCenterPos.x, s_NavCenterPos.y);
            }
            I_Mouse::Set_Position(s_NavCenterPos);
            I_Mouse::Reset_Delta();
        }

        Vector3D fwd   = U_Raycast::Get_ForwardVector(s_Camera->Rotation);
        Vector3D right = U_Raycast::Get_RightVector(s_Camera->Rotation);
        FLOAT speed    = s_MoveSpeed * (I_Keyboard::Is_Down(Key::Shift) ? 2.5f : 1.0f);

        if (I_Keyboard::Is_Down(Key::W)) s_Camera->Position += fwd * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::S)) s_Camera->Position -= fwd * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::D)) s_Camera->Position += right * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::A)) s_Camera->Position -= right * (speed * deltaTime);
        if (I_Keyboard::Is_Down(Key::E)) s_Camera->Position.y += speed * deltaTime;
        if (I_Keyboard::Is_Down(Key::Q)) s_Camera->Position.y -= speed * deltaTime;
    }

    // 3. Exit Flycam Navigation Mode when RMB is released or window focus is lost
    if (s_IsNavigating && (!bRightHeld || bRightReleased || !M_Window::Is_Focused())) {
        s_IsNavigating = false;

        // Restore mouse to the exact position where right-click was originally clicked!
        SDL_Window *win = M_Window::Get_Handle();
        if (win) {
            SDL_WarpMouseInWindow(win, s_NavSavedMousePos.x, s_NavSavedMousePos.y);
        }
        I_Mouse::Set_Position(s_NavSavedMousePos);
        I_Mouse::Reset_Delta();

        M_Window::Set_MouseLocked(false); // Show cursor again
    }

    // Adjust camera fly speed with mouse scroll wheel when not dragging
    if (bInViewport && !s_IsNavigating && !s_IsDraggingObject) {
        FLOAT scroll = I_Mouse::Get_Scroll();
        if (std::abs(scroll) > 0.01f) {
            s_MoveSpeed = std::clamp(s_MoveSpeed + scroll * 2.5f, 2.0f, 100.0f);
        }
    }

    // 4. Interactive 3D Mouse Dragging & Selection
    if (bInViewport && !s_IsNavigating) {
        if (I_Mouse::Is_Pressed(MouseButton::MB_Left)) {
            Ray3D ray = U_Raycast::Screen_To_World(mousePos, s_Camera->Position, s_Camera->Rotation, s_Camera->FOV);
            BOOL clickedTarget = false;

            if (s_SelectedEntity) {
                Vector3D pos = s_SelectedEntity->Position;
                FLOAT dX = 0.0f, dY = 0.0f, dZ = 0.0f, dCenter = 0.0f;
                Vector3D n;
                BOOL hitX = U_Raycast::Intersects_Ray_Sphere(ray, pos + Vector3D{ 3.5f, 0.0f, 0.0f }, 0.9f, dX, n);
                BOOL hitY = U_Raycast::Intersects_Ray_Sphere(ray, pos + Vector3D{ 0.0f, 3.5f, 0.0f }, 0.9f, dY, n);
                BOOL hitZ = U_Raycast::Intersects_Ray_Sphere(ray, pos + Vector3D{ 0.0f, 0.0f, 3.5f }, 0.9f, dZ, n);
                BOOL hitC = U_Raycast::Intersects_Ray_Sphere(ray, pos, 1.3f, dCenter, n);

                if (hitX && (!hitY || dX < dY) && (!hitZ || dX < dZ)) {
                    s_ActiveGizmoAxis = 1; // X Axis
                    clickedTarget = true;
                } else if (hitY && (!hitX || dY < dX) && (!hitZ || dY < dZ)) {
                    s_ActiveGizmoAxis = 2; // Y Axis
                    clickedTarget = true;
                } else if (hitZ && (!hitX || dZ < dX) && (!hitY || dZ < dY)) {
                    s_ActiveGizmoAxis = 3; // Z Axis
                    clickedTarget = true;
                } else if (hitC) {
                    s_ActiveGizmoAxis = 0; // Free XZ ground plane
                    clickedTarget = true;
                } else {
                    FLOAT hitDist = 0.0f;
                    if (U_Raycast::Intersects_Ray_AABB(ray, s_SelectedEntity->Get_WorldAABB(), hitDist, n)) {
                        s_ActiveGizmoAxis = 0;
                        clickedTarget = true;
                    }
                }
            }

            if (!clickedTarget) {
                // Raycast pick against all entities in the world
                FLOAT closestDist = 999999.0f;
                C_Entity *picked = nullptr;
                for (const auto &ent : M_World::Get_Entities()) {
                    if (!ent || !ent->bIsVisible) continue;
                    FLOAT hitDist = 0.0f;
                    Vector3D hitNormal;
                    if (U_Raycast::Intersects_Ray_AABB(ray, ent->Get_WorldAABB(), hitDist, hitNormal)) {
                        if (hitDist > 0.0f && hitDist < closestDist) {
                            closestDist = hitDist;
                            picked = ent.get();
                        }
                    }
                }

                Select_Entity(picked);
                if (picked) {
                    s_ActiveGizmoAxis = 0; // Free ground drag
                    clickedTarget = true;
                }
            }

            if (s_SelectedEntity && clickedTarget) {
                s_IsDraggingObject   = true;
                s_DragStartEntityPos = s_SelectedEntity->Position;
                s_DragStartMouse     = mousePos;

                if (std::abs(ray.Direction.y) > 0.0001f) {
                    FLOAT t = (s_SelectedEntity->Position.y - ray.Origin.y) / ray.Direction.y;
                    if (t > 0.0f) {
                        s_DragStartWorldHit = ray.Origin + (ray.Direction * t);
                    }
                }
            }
        }
    }

    // 5. Active Mouse Dragging Update (Smooth Real-time Motion)
    if (s_IsDraggingObject && s_SelectedEntity) {
        if (!I_Mouse::Is_Down(MouseButton::MB_Left)) {
            s_IsDraggingObject = false;
            s_ActiveGizmoAxis  = 0;
        } else {
            Ray3D ray = U_Raycast::Screen_To_World(mousePos, s_Camera->Position, s_Camera->Rotation, s_Camera->FOV);
            if (std::abs(ray.Direction.y) > 0.0001f) {
                FLOAT t = (s_DragStartEntityPos.y - ray.Origin.y) / ray.Direction.y;
                if (t > 0.0f) {
                    Vector3D curHit = ray.Origin + (ray.Direction * t);
                    Vector3D delta  = curHit - s_DragStartWorldHit;
                    Vector3D targetPos = s_DragStartEntityPos;

                    if (s_ActiveGizmoAxis == 0) {
                        targetPos.x += delta.x;
                        targetPos.z += delta.z;
                    } else if (s_ActiveGizmoAxis == 1) {
                        targetPos.x += delta.x;
                    } else if (s_ActiveGizmoAxis == 2) {
                        FLOAT dy = -(mousePos.y - s_DragStartMouse.y) * 0.06f;
                        targetPos.y += dy;
                    } else if (s_ActiveGizmoAxis == 3) {
                        targetPos.z += delta.z;
                    }

                    // Optional Grid Snap (Ctrl key = snap to 1.0m)
                    if (I_Keyboard::Is_Down(Key::Ctrl)) {
                        targetPos.x = std::round(targetPos.x);
                        targetPos.z = std::round(targetPos.z);
                    }

                    s_SelectedEntity->Position = targetPos;
                }
            }

            // Mouse wheel adjusts vertical height during drag
            FLOAT scroll = I_Mouse::Get_Scroll();
            if (std::abs(scroll) > 0.01f) {
                s_SelectedEntity->Position.y += scroll * 0.5f;
                s_DragStartEntityPos.y += scroll * 0.5f;
            }
        }
    }

    // 6. Hotkeys for Gizmo Manipulation & Selection
    if (!bRightHeld) {
        if (I_Keyboard::Is_Pressed(Key::W) || I_Keyboard::Is_Pressed(Key::Num1)) Set_GizmoMode(GizmoMode::Translate);
        if (I_Keyboard::Is_Pressed(Key::E) || I_Keyboard::Is_Pressed(Key::Num2)) Set_GizmoMode(GizmoMode::Rotate);
        if (I_Keyboard::Is_Pressed(Key::R) || I_Keyboard::Is_Pressed(Key::Num3)) Set_GizmoMode(GizmoMode::Scale);
    }
    if (I_Keyboard::Is_Pressed(Key::Delete)) Delete_Selected();
    if (I_Keyboard::Is_Pressed(Key::F)) Focus_On_Selected();
    if (I_Keyboard::Is_Down(Key::Ctrl) && I_Keyboard::Is_Pressed(Key::D)) Duplicate_Selected();

    // 7. Keyboard Arrow Key Transform Stepping
    if (s_SelectedEntity && !bRightHeld && !s_IsDraggingObject) {
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

    // 8. Render active 3D gizmo
    Render_Gizmo();
}
