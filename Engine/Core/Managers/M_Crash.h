#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>

// ============================================================================
// Cross-Platform Crash & Assert Manager (100% Pure C++23 — Windows & Linux)
// ============================================================================
class M_Crash {
private:
    using SourceLoc  = std::source_location;
    using StackTrace = std::stacktrace;

    static FilePath s_CrashLogPath;

    static FilePath BuildDatedLogPath();
    static String   CleanFunctionName(String func);
    static String   GetCrashLocation();
    static void     SignalHandler(INT signalCode);
    static void     TerminateHandler();

public:
    // Call ONCE at the start of application boot to catch OS signals & uncaught exceptions
    static void Init(const FilePath &crashLogPath = "Logs/Crash.log");

    // Forced crash utility (Debug & Dev only, no-op in Release)
    static void ForceCrash(const SourceLoc loc = SourceLoc::current());

    // Immediately halts the engine, logs [FATAL], and writes the full report to Crash.log
    static void Trigger(StringView reason, const SourceLoc loc = SourceLoc::current());

    // Verifies condition. If FALSE, triggers full crash report with line & function
    static inline void Check(BOOL condition, StringView reason, const SourceLoc loc = SourceLoc::current()) {
        if (!condition) {
            Trigger(reason, loc);
        }
    }
};
