#pragma once

#include <pch.h>
#include <I_Plugin.h>
#include <UserInterface.h>

// ============================================================================
// CodeEditorPlugin: Integrated In-Editor C++ Code Editor
// ============================================================================
// Allows editing game and project C++ code directly inside Aurora Studio.
// Starts DISABLED by default.
// ============================================================================
class CodeEditorPlugin : public I_Plugin {
private:
    Array<String> m_CodeLines;
    String        m_ActiveFileName = "Client.cpp";
    INT           m_SelectedLine   = 0;
    INT           m_ScrollOffset   = 0;
    String        m_StatusMsg      = "";

public:
    [[nodiscard]] StringView Get_ID() const override { return "aurora.code_editor"; }
    [[nodiscard]] StringView Get_Name() const override { return "In-Editor Code Editor"; }
    [[nodiscard]] StringView Get_Description() const override {
        return "Integrated lightweight C++ code editor with syntax highlighting and instant compilation triggers.";
    }
    [[nodiscard]] StringView Get_Version() const override { return "1.0.0"; }
    [[nodiscard]] StringView Get_Category() const override { return "Developer Tools"; }

    BOOL On_Enable() override;
    void On_Disable() override;
    void On_Tick(FLOAT deltaTime) override;
    void On_Render_UI() override;

    void Load_File(const FilePath &path);
};
