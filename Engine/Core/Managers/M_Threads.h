#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>

// ============================================================================
// Cross-Platform Thread & Job Manager (Thread Pool & Parallel Simulation)
// ============================================================================
class M_Threads {
private:
    static Array<std::thread>      s_Workers;
    static std::queue<Task>        s_TaskQueue;
    static std::mutex              s_QueueMutex;
    static std::condition_variable s_CV;
    static std::condition_variable s_WaitAllCV;
    static std::atomic<UINT>       s_ActiveTasks;
    static std::atomic<BOOL>       s_Stop;
    static BOOL                    s_IsInitialized;
    static UINT                    s_ThreadCount;

    static void WorkerLoop();

public:
    // Initializes thread pool matching CPU core count (defaults to logical cores - 1)
    static BOOL Init(UINT threadCount = 0);

    // Gracefully finishes all tasks and joins all worker threads
    static void Shutdown();

    // Runs a fire-and-forget task asynchronously on a background worker thread
    static void Run(Task task);

    // Backward-compatible alias for Run
    static inline void Run_Async(Task task) { Run(std::move(task)); }

    // Divides a range [start, end) across all CPU worker threads simultaneously
    static void Parallel_For(UINT start, UINT end, const Function<void(UINT)> &loopBody);

    // Spawns a dedicated long-running thread with a descriptive name
    static void Create_NamedThread(StringView name, Task task);

    // Blocks caller until all queued and active tasks complete
    static void Wait_All();

    // Fast inline queries
    static inline UINT Get_ThreadCount()      { return s_ThreadCount; }
    static inline UINT Get_HardwareCores()    { return std::thread::hardware_concurrency(); }
    static inline UINT Get_ActiveTaskCount()  { return s_ActiveTasks.load(); }
    static inline BOOL Is_Initialized()       { return s_IsInitialized; }

    static inline UINT Get_PendingTaskCount() {
        std::lock_guard<std::mutex> lock(s_QueueMutex);
        return static_cast<UINT>(s_TaskQueue.size());
    }
};
