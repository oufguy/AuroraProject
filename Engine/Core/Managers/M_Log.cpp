#include <M_Log.h>
#include <iostream>

#if defined(_RELEASE)
LogLevel M_Log::s_CurrentLevel = User;      // Release: Only User logs by default
#elif defined(_DEVELOPMENT)
LogLevel M_Log::s_CurrentLevel = Developer; // Development: Developer + User logs
#else
LogLevel M_Log::s_CurrentLevel = Core;      // Debug: All logs including Engine Core
#endif

std::mutex M_Log::g_LogMutex;

void M_Log::Output(LogType type, LogLevel level, StringView message) {
    // Filter out logs below the active configuration level!
    // (e.g. In Release [User=2], Core[0] and Developer[1] are skipped automatically)
    if (level < s_CurrentLevel) return;

    ScopedLock lock(g_LogMutex);

    switch (type) {
        case Info:
            std::cout << LOG_COLOR_INFO << "[INFO] " << message << '\n';
            break;
        case Warn:
            std::cout << LOG_COLOR_WARN << "[WARN] " << message << '\n';
            break;
        case Error:
            std::cerr << LOG_COLOR_ERROR << "[ERROR] " << message << '\n';
            break;
        case Fatal:
            std::cerr << LOG_COLOR_FATAL << "[FATAL] " << message << '\n';
            break;
        case Print:
        default:
            std::cout << LOG_COLOR_PRINT << "[PRINT] " << message << '\n';
            break;
    }
    std::cout << std::flush;
}
