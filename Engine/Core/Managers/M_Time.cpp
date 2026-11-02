#include <M_Time.h>

M_Time::TimePoint M_Time::s_StartTime{};
M_Time::TimePoint M_Time::s_LastFrameTime{};
FLOAT  M_Time::s_DeltaTime  = 0.0f;
DOUBLE M_Time::s_TotalTime  = 0.0;
FLOAT  M_Time::s_FPSTimer   = 0.0f;
INT    M_Time::s_FrameCount = 0;
INT    M_Time::s_CurrentFPS = 0;

std::mutex                M_Time::s_TimerMutex;
UINT                      M_Time::s_NextTimerID = 0;
Array<M_Time::TimerEntry> M_Time::s_ActiveTimers;
Array<M_Time::TimerEntry> M_Time::s_PendingTimers;

void M_Time::Init() {
    s_StartTime     = Clock::now();
    s_LastFrameTime = s_StartTime;
}

void M_Time::Update() {
    TimePoint now = Clock::now();

    // Single integer nanosecond subtraction -> multiply by 1e-9 (faster than 2 chrono conversions!)
    DOUBLE rawDt = static_cast<DOUBLE>((now - s_LastFrameTime).count()) * 1e-9;
    s_LastFrameTime = now;

    // Accumulate total uptime in DOUBLE and cap DeltaTime at 0.1s (10 FPS minimum)
    s_TotalTime += rawDt;
    FLOAT dt     = static_cast<FLOAT>(rawDt);
    s_DeltaTime  = (dt > 0.1f) ? 0.1f : dt;

    // Count frames and update FPS once every 1.0 second
    s_FrameCount++;
    s_FPSTimer += dt;
    if (s_FPSTimer >= 1.0f) [[unlikely]] {
        s_CurrentFPS = s_FrameCount;
        s_FrameCount = 0;
        s_FPSTimer  -= 1.0f;
    }

    // Process all scheduled intervals and timeouts
    Process_Timers();
}

void M_Time::Sleep(DWORD milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

String M_Time::Get_TimeStamp(BOOL forFileName) {
    auto now = std::chrono::system_clock::now();
    std::time_t rawTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &rawTime);
#else
    localtime_r(&rawTime, &localTm);
#endif
    INT day    = localTm.tm_mday;
    INT month  = localTm.tm_mon + 1;
    INT year   = (localTm.tm_year + 1900) % 100;
    INT hour   = localTm.tm_hour;
    INT minute = localTm.tm_min;

    if (forFileName) {
        return StringFormat("{:02}.{:02}.{:02}_{:02}.{:02}", day, month, year, hour, minute);
    }
    return StringFormat("{:02}.{:02}.{:02} {:02}:{:02}", day, month, year, hour, minute);
}

UINT M_Time::Set_Interval(FLOAT intervalSeconds, Function<void()> callback) {
    if (intervalSeconds <= 0.0001f || !callback) return 0;

    std::lock_guard<std::mutex> lock(s_TimerMutex);
    TimerEntry entry{};
    entry.ID        = ++s_NextTimerID;
    entry.Interval  = intervalSeconds;
    entry.NextFire  = s_TotalTime + intervalSeconds;
    entry.Callback  = std::move(callback);
    entry.bIsActive = true;
    s_PendingTimers.push_back(std::move(entry));
    return s_NextTimerID;
}

UINT M_Time::Set_Timeout(FLOAT delaySeconds, Function<void()> callback) {
    if (!callback) return 0;
    FLOAT d = (delaySeconds < 0.0f) ? 0.0f : delaySeconds;

    std::lock_guard<std::mutex> lock(s_TimerMutex);
    TimerEntry entry{};
    entry.ID        = ++s_NextTimerID;
    entry.Interval  = 0.0f; // 0 indicates one-shot
    entry.NextFire  = s_TotalTime + d;
    entry.Callback  = std::move(callback);
    entry.bIsActive = true;
    s_PendingTimers.push_back(std::move(entry));
    return s_NextTimerID;
}

BOOL M_Time::Clear_Interval(UINT timerID) {
    if (timerID == 0) return false;

    std::lock_guard<std::mutex> lock(s_TimerMutex);
    for (auto &t : s_ActiveTimers) {
        if (t.ID == timerID) {
            t.bIsActive = false;
            return true;
        }
    }
    for (auto &t : s_PendingTimers) {
        if (t.ID == timerID) {
            t.bIsActive = false;
            return true;
        }
    }
    return false;
}

void M_Time::Process_Timers() {
    Array<Function<void()>> toExecute;
    {
        std::lock_guard<std::mutex> lock(s_TimerMutex);

        if (!s_PendingTimers.empty()) {
            s_ActiveTimers.insert(
                s_ActiveTimers.end(),
                std::make_move_iterator(s_PendingTimers.begin()),
                std::make_move_iterator(s_PendingTimers.end())
            );
            s_PendingTimers.clear();
        }

        DOUBLE now = s_TotalTime;
        for (auto &timer : s_ActiveTimers) {
            if (!timer.bIsActive) continue;

            if (now >= timer.NextFire) {
                if (timer.Callback) {
                    toExecute.push_back(timer.Callback);
                }

                if (timer.Interval > 0.0f) {
                    timer.NextFire = now + timer.Interval;
                } else {
                    timer.bIsActive = false;
                }
            }
        }

        s_ActiveTimers.erase(
            std::remove_if(s_ActiveTimers.begin(), s_ActiveTimers.end(),
                [](const TimerEntry &t) { return !t.bIsActive; }),
            s_ActiveTimers.end()
        );
    }

    for (const auto &cb : toExecute) {
        if (cb) cb();
    }
}
