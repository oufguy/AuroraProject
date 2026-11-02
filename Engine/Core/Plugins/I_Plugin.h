#pragma once

#include <pch.h>
#include <U_Variables.h>

// ============================================================================
// I_Plugin: Base Interface for Modular Aurora Plugins
// ============================================================================
// Crash-safe, high-performance plugin lifecycle.
// All base plugins (e.g. Blueprint, CodeEditor) inherit from this interface
// and start in a DISABLED state by default.
// ============================================================================
class I_Plugin {
public:
    virtual ~I_Plugin() = default;

    [[nodiscard]] virtual StringView Get_ID() const = 0;
    [[nodiscard]] virtual StringView Get_Name() const = 0;
    [[nodiscard]] virtual StringView Get_Description() const = 0;
    [[nodiscard]] virtual StringView Get_Version() const = 0;
    [[nodiscard]] virtual StringView Get_Category() const { return "General"; }

    virtual BOOL On_Enable()  = 0;
    virtual void On_Disable() = 0;

    virtual void On_Tick(FLOAT deltaTime) {}
    virtual void On_Render_UI() {}

    [[nodiscard]] BOOL Is_Enabled() const { return m_bIsEnabled; }

    void Set_Enabled(BOOL bEnable) {
        if (m_bIsEnabled == bEnable) return;
        m_bIsEnabled = bEnable;
        if (m_bIsEnabled) {
            On_Enable();
        } else {
            On_Disable();
        }
    }

protected:
    BOOL m_bIsEnabled = false; // Always DISABLED by default!
};
