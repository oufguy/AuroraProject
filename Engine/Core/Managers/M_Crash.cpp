#include <M_Crash.h>
#include <M_File.h>
#include <M_Time.h>

FilePath M_Crash::s_CrashLogPath = "Logs/Crash.log";

FilePath M_Crash::BuildDatedLogPath() {
    FilePath folder = s_CrashLogPath.parent_path();
    String stem     = s_CrashLogPath.stem().string();
    String ext      = s_CrashLogPath.extension().string();
    if (ext.empty()) ext = ".log";

    String datedName = StringFormat("{}_{}{}", stem, M_Time::Get_TimeStamp(true), ext);
    return folder.empty() ? FilePath(datedName) : (folder / datedName);
}

String M_Crash::CleanFunctionName(String func) {
    size_t bangPos = func.find('!');
    if (bangPos != String::npos) func = func.substr(bangPos + 1);
    size_t plusPos = func.find('+');
    if (plusPos != String::npos) func = func.substr(0, plusPos);
    return func;
}

String M_Crash::GetCrashLocation() {
    StackTrace trace = StackTrace::current();

    for (const auto &entry : trace) {
        String fullPath = entry.source_file();
        UINT64 line     = entry.source_line();
        if (fullPath.empty() || line == 0) continue;

        String shortFile = FilePath(fullPath).filename().string();

        if (shortFile == "M_Crash.h" || shortFile == "M_Crash.cpp") continue;
        if (fullPath.find("Engine") == String::npos && fullPath.find("Game") == String::npos) continue;

        String func = CleanFunctionName(entry.description());
        return StringFormat(
            "File     : {} (Line {})\n"
            "Function : {}\n",
            shortFile, line, func
        );
    }

    return "Function : Unknown (Run with Debug symbols enabled to see File & Line)\n";
}

void M_Crash::SignalHandler(INT signalCode) {
    String crashType = "Unknown Hardware Crash";
    String plainWhat = "An unknown hardware fault stopped the program.";

    switch (signalCode) {
        case SIGSEGV:
            crashType = "Memory Access Crash (Null / Invalid Pointer)";
            plainWhat = "Code tried to read or write to an empty (nullptr) or deleted variable/pointer!";
            break;
        case SIGFPE:
            crashType = "Math Crash (Divide by Zero)";
            plainWhat = "Code tried to divide a number by 0 (or produced an invalid math result)!";
            break;
        case SIGILL:
            crashType = "CPU Instruction Crash";
            plainWhat = "CPU hit a corrupted instruction or bad function pointer!";
            break;
        case SIGABRT:
            crashType = "Emergency Abort Crash";
            plainWhat = "A fatal memory or library check forced the program to stop immediately!";
            break;
    }

    String report = StringFormat(
        "============================================================\n"
        "[FATAL CRASH REPORT]\n"
        "Type     : {}\n"
        "Why      : {}\n"
        "{}"
        "============================================================\n",
        crashType,
        plainWhat,
        GetCrashLocation()
    );

    LOG(Fatal, LogLevel::Core, "\n{}", report);
    M_File::Write(BuildDatedLogPath(), report, true);

    std::_Exit(signalCode);
}

void M_Crash::TerminateHandler() {
    String report = StringFormat(
        "============================================================\n"
        "[FATAL CRASH REPORT]\n"
        "Type     : Unhandled C++ Exception\n"
        "Why      : A C++ error was thrown (e.g. out-of-bounds or bad format) and never caught!\n"
        "{}"
        "============================================================\n",
        GetCrashLocation()
    );

    LOG(Fatal, LogLevel::Core, "\n{}", report);
    M_File::Write(BuildDatedLogPath(), report, true);

    std::_Exit(1);
}

void M_Crash::Init(const FilePath &crashLogPath) {
    s_CrashLogPath = crashLogPath;

    std::signal(SIGSEGV, SignalHandler);
    std::signal(SIGFPE,  SignalHandler);
    std::signal(SIGILL,  SignalHandler);
    std::signal(SIGABRT, SignalHandler);
    std::set_terminate(TerminateHandler);

    LOG(Info, LogLevel::Core, "M_Crash::Init registered crash handlers -> {}", s_CrashLogPath.string());
}

void M_Crash::ForceCrash(const SourceLoc loc) {
#if !defined(_RELEASE)
    Trigger("Forced Crash (Debug/Dev Build)", loc);
#else
    (void)loc;
    LOG(Warn, LogLevel::User, "M_Crash::ForceCrash() called in Release build — ignoring!");
#endif
}

void M_Crash::Trigger(StringView reason, const SourceLoc loc) {
    String shortFile = FilePath(loc.file_name()).filename().string();

    String report = StringFormat(
        "============================================================\n"
        "[FATAL CRASH REPORT]\n"
        "Type     : Manual Engine Crash\n"
        "Why      : {}\n"
        "File     : {} (Line {})\n"
        "Function : {}\n"
        "============================================================\n",
        reason,
        shortFile,
        loc.line(),
        loc.function_name()
    );

    LOG(Fatal, LogLevel::Core, "\n{}", report);
    M_File::Write(BuildDatedLogPath(), report, true);

    std::_Exit(1);
}
