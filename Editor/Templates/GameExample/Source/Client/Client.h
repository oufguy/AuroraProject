#pragma once

#include <Aurora.h>

// ============================================================================
// Aurora Client (Game Layer)
// ============================================================================
// Inherits from B_Game.
// Uses C_Camera directly for camera controls and M_Asset for resources.
// ============================================================================
class Client : public B_Game {
public:
    // Called once when the Engine boots and World is ready
    void BeginPlay() override;

    // Called every frame with the elapsed delta time
    void Tick(FLOAT deltaTime) override;

    // Called every frame to render 2D HUD / UI elements on top of the 3D scene
    void DrawUI() override;

    // Called right before the game shuts down
    void EndPlay() override;

private:
    C_Camera    *m_Camera         = nullptr;
    C_StaticMesh *m_Ground        = nullptr;
    C_Character  *m_Player        = nullptr;
    FLOAT        m_CameraDistance = 8.0f;
    Array<C_StaticMesh *> m_TownBuildings;
    C_Entity    *m_SelectedEntity = nullptr;
    Vector3D     m_LastGroundClick = { 0.0f, 0.0f, 0.0f };
    BOOL         m_HasGroundClick  = false;

    // 2D Screen-space UI Demo (Ace Online 4-State UI)
    F_Box      m_HUDPanel;
    F_ImageBtn m_TestButton;
};
