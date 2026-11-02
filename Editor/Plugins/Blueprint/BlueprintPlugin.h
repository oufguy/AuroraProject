#pragma once

#include <pch.h>
#include <I_Plugin.h>
#include <UserInterface.h>

// ============================================================================
// BlueprintPlugin: Visual C++ Node Scripting (Unreal-style, Native Speed)
// ============================================================================
// Compiles directly into native C++ calls (99% execution speed, zero bytecode VM).
// Starts DISABLED by default.
// ============================================================================
struct BP_Node {
    INT      ID    = 0;
    String   Title = "Node";
    Vector2D Pos   = { 100.0f, 100.0f };
    Vector2D Size  = { 180.0f, 90.0f };
    COLOR    Color = COLOR(41, 128, 185, 255);
    String   Code  = "";
};

class BlueprintPlugin : public I_Plugin {
private:
    Array<BP_Node> m_Nodes;
    INT            m_SelectedNode = -1;
    BOOL           m_bIsDragging   = false;
    Vector2D       m_DragOffset   = {};
    String         m_GeneratedCpp = "";

public:
    [[nodiscard]] StringView Get_ID() const override { return "aurora.blueprint"; }
    [[nodiscard]] StringView Get_Name() const override { return "Blueprint Visual Scripting"; }
    [[nodiscard]] StringView Get_Description() const override {
        return "Unreal-style visual node graph that compiles directly into high-performance C++ calls (99% speed, zero VM).";
    }
    [[nodiscard]] StringView Get_Version() const override { return "1.0.0"; }
    [[nodiscard]] StringView Get_Category() const override { return "Scripting"; }

    BOOL On_Enable() override;
    void On_Disable() override;
    void On_Tick(FLOAT deltaTime) override;
    void On_Render_UI() override;

    void Compile_To_Cpp();
};
