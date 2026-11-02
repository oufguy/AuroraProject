#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Math.h>

// ============================================================================
// Game & Server Time Manager (DeltaTime, FPS, Uptime, Cooldowns, Sleep)
// ============================================================================
class M_Time {
private:
    // Internal high-precision clock (steady_clock never jumps backward if OS clock changes!)
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    static TimePoint s_StartTime;
    static TimePoint s_LastFrameTime;

    static FLOAT  s_DeltaTime;
    static DOUBLE s_TotalTime;
    static FLOAT  s_FPSTimer;
    static INT    s_FrameCount;
    static INT    s_CurrentFPS;

    struct TimerEntry {
        UINT             ID        = 0;
        DOUBLE           NextFire  = 0.0;
        FLOAT            Interval  = 0.0f; // <= 0 for one-shot, > 0 for repeating interval
        Function<void()> Callback;
        BOOL             bIsActive = true;
    };

    static std::mutex        s_TimerMutex;
    static UINT              s_NextTimerID;
    static Array<TimerEntry> s_ActiveTimers;
    static Array<TimerEntry> s_PendingTimers;

    static void Process_Timers();

public:
    // Time unit for Get_TotalTime(...)
    enum TimeUnit : BYTE {
        Seconds      = 0, // Total time in Seconds (e.g. 12.50s)
        Milliseconds = 1  // Total time in Milliseconds (e.g. 12500.0ms)
    };

    // ========================================================================
    // Core Frame / Tick Loop
    // ========================================================================

    // Resets the Engine start clock (called automatically by Core::BeginPlay).
    static void Init();

    // Call this ONCE at the very top of your Client or Server while(running) loop!
    // Automatically updates DeltaTime, TotalTime, and calculates FPS every 1 second.
    // Example: M_Time::Update();
    static void Update();

    // Pauses the current thread for 'milliseconds' (Saves CPU in Server loops or caps FPS!).
    // Example: M_Time::Sleep(10);
    static void Sleep(DWORD milliseconds);

    // ========================================================================
    // Get_... (DeltaTime, FPS, Total Uptime, Timestamp)
    // ========================================================================

    // Returns the time in Seconds between the last frame and this frame (e.g. 0.016f at 60 FPS).
    // Multiply all movement/speeds by this so 60 FPS and 240 FPS move at the exact same speed!
    // Example: FLOAT dt = M_Time::Get_DeltaTime();
    static inline FLOAT Get_DeltaTime() {
        return s_DeltaTime;
    }

    // Returns how many frames (Client) or ticks (Server) ran in the last second.
    // Example: INT fps = M_Time::Get_FPS();
    static inline INT Get_FPS() {
        return s_CurrentFPS;
    }

    // Returns total time elapsed since the Engine/Server started (Seconds or Milliseconds).
    // Example: DOUBLE timeSec = M_Time::Get_TotalTime(M_Time::Seconds);
    // Example: DOUBLE timeMS  = M_Time::Get_TotalTime(M_Time::Milliseconds);
    static inline DOUBLE Get_TotalTime(TimeUnit unit) {
        if (unit == Milliseconds) {
            return s_TotalTime * 1000.0;
        }
        return s_TotalTime;
    }

    // Returns current local Date & Time as "DD.MM.YY_HH.MM" (safe for filenames) or "DD.MM.YY HH:MM".
    // (Note: Windows forbids ':' inside filenames, so file mode uses '.' or '_' instead!)
    // Example: String stamp = M_Time::Get_TimeStamp(true); // "19.10.26_23.15"
    static String Get_TimeStamp(BOOL forFileName = true);

    // ========================================================================
    // Cooldown / Timer Helper (Skills, Fire Rate, Monster Spawns)
    // ========================================================================

    // Checks if 'cooldownSeconds' has passed since 'lastUsedTime'.
    // If ready: automatically updates 'lastUsedTime' to NOW and returns true!
    // Example: if (M_Time::Check_Cooldown(lastMissileShotTime, 0.5)) { ShootMissile(); }
    static inline BOOL Check_Cooldown(DOUBLE &lastUsedTime, DOUBLE cooldownSeconds) {
        if ((s_TotalTime - lastUsedTime) >= cooldownSeconds) {
            lastUsedTime = s_TotalTime;
            return true;
        }
        return false;
    }

    // ========================================================================
    // Scheduled Timers (Repeating Intervals & One-shot Timeouts)
    // ========================================================================

    // Runs 'callback' repeatedly every 'intervalSeconds' (looping interval)
    // Example: M_Time::Set_Interval(1.0f, [this]() { ... });
    // Example: M_Time::Every(1.0f, [this]() { ... });
    static UINT Set_Interval(FLOAT intervalSeconds, Function<void()> callback);
    static inline UINT Every(FLOAT intervalSeconds, Function<void()> callback) {
        return Set_Interval(intervalSeconds, std::move(callback));
    }

    // Runs 'callback' once after 'delaySeconds' (one-shot timeout)
    // Example: M_Time::Set_Timeout(2.5f, []() { PRINT("Done!"); });
    // Example: M_Time::After(2.5f, []() { PRINT("Done!"); });
    static UINT Set_Timeout(FLOAT delaySeconds, Function<void()> callback);
    static inline UINT After(FLOAT delaySeconds, Function<void()> callback) {
        return Set_Timeout(delaySeconds, std::move(callback));
    }

    // Unreal Engine style Set_Timer (bLoop = true for interval, false for one-shot timeout)
    // Example: M_Time::Set_Timer(1.0f, [this]() { ... }, /*bLoop=*/true);
    static inline UINT Set_Timer(FLOAT seconds, Function<void()> callback, BOOL bLoop = true) {
        return bLoop ? Set_Interval(seconds, std::move(callback)) : Set_Timeout(seconds, std::move(callback));
    }

    // Cancels an active timer by ID (returns true if found & cancelled)
    // Example: M_Time::Clear_Interval(timerID);
    // Example: M_Time::Clear_Timer(timerID);
    static BOOL Clear_Interval(UINT timerID);
    static inline BOOL Clear_Timer(UINT timerID) {
        return Clear_Interval(timerID);
    }
};
