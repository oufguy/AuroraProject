#include <M_Threads.h>

Array<std::thread>      M_Threads::s_Workers;
std::queue<Task>        M_Threads::s_TaskQueue;
std::mutex              M_Threads::s_QueueMutex;
std::condition_variable M_Threads::s_CV;
std::condition_variable M_Threads::s_WaitAllCV;
std::atomic<UINT>       M_Threads::s_ActiveTasks{ 0 };
std::atomic<BOOL>       M_Threads::s_Stop{ false };
BOOL                    M_Threads::s_IsInitialized = false;
UINT                    M_Threads::s_ThreadCount = 0;

void M_Threads::WorkerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(s_QueueMutex);
            s_CV.wait(lock, [] {
                return s_Stop.load() || !s_TaskQueue.empty();
            });

            if (s_Stop.load() && s_TaskQueue.empty()) {
                return;
            }

            task = std::move(s_TaskQueue.front());
            s_TaskQueue.pop();
            s_ActiveTasks++;
        }

        if (task) {
            try {
                task();
            } catch (const std::exception &e) {
                LOG(Error, LogLevel::Core, "M_Threads: Worker task threw an exception: {}", e.what());
            } catch (...) {
                LOG(Error, LogLevel::Core, "M_Threads: Worker task threw an unknown exception!");
            }
        }

        {
            std::lock_guard<std::mutex> lock(s_QueueMutex);
            s_ActiveTasks--;
        }
        s_WaitAllCV.notify_all();
    }
}

BOOL M_Threads::Init(UINT threadCount) {
    if (s_IsInitialized) return true;

    UINT hwCores = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        threadCount = (hwCores > 1) ? (hwCores - 1) : 1;
    }

    s_Stop          = false;
    s_ThreadCount   = threadCount;
    s_ActiveTasks   = 0;

    s_Workers.reserve(threadCount);
    for (UINT i = 0; i < threadCount; ++i) {
        s_Workers.emplace_back(WorkerLoop);
    }

    s_IsInitialized = true;
    LOG(Info, LogLevel::Core, "M_Threads: Initialized with [{}] worker threads on [{}] CPU cores.",
        s_ThreadCount, hwCores);
    return true;
}

void M_Threads::Shutdown() {
    if (!s_IsInitialized) return;

    s_Stop = true;
    s_CV.notify_all();

    for (auto &worker : s_Workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    s_Workers.clear();

    {
        std::lock_guard<std::mutex> lock(s_QueueMutex);
        while (!s_TaskQueue.empty()) {
            s_TaskQueue.pop();
        }
    }

    s_IsInitialized = false;
    s_ThreadCount   = 0;
    LOG(Info, LogLevel::Core, "M_Threads: Worker threads shutdown cleanly.");
}

void M_Threads::Run(Task task) {
    if (!s_IsInitialized) {
        if (task) {
            try {
                task();
            } catch (const std::exception &e) {
                LOG(Error, LogLevel::Core, "M_Threads::Run (Fallback): Exception thrown: {}", e.what());
            } catch (...) {
                LOG(Error, LogLevel::Core, "M_Threads::Run (Fallback): Unknown exception thrown!");
            }
        }
        return;
    }

    {
        std::lock_guard<std::mutex> lock(s_QueueMutex);
        s_TaskQueue.push(std::move(task));
    }
    s_CV.notify_one();
}

void M_Threads::Parallel_For(UINT start, UINT end, const Function<void(UINT)> &loopBody) {
    if (start >= end || !loopBody) return;
    UINT totalItems = end - start;

    if (!s_IsInitialized || s_Workers.empty() || totalItems <= 1) {
        for (UINT i = start; i < end; ++i) {
            loopBody(i);
        }
        return;
    }

    UINT numWorkers = static_cast<UINT>(s_Workers.size());
    UINT chunkSize  = (totalItems + numWorkers - 1) / numWorkers;
    std::atomic<UINT> remainingChunks{ 0 };

    for (UINT i = start; i < end; i += chunkSize) {
        UINT chunkEnd = std::min(i + chunkSize, end);
        remainingChunks++;

        Run([i, chunkEnd, &loopBody, &remainingChunks]() {
            for (UINT j = i; j < chunkEnd; ++j) {
                loopBody(j);
            }
            remainingChunks--;
        });
    }

    while (remainingChunks.load() > 0) {
        std::this_thread::yield();
    }
}

void M_Threads::Create_NamedThread(StringView name, Task task) {
    std::thread t([nameStr = String(name), task = std::move(task)]() {
        LOG(Info, LogLevel::Core, "Thread [{}] started.", nameStr);
        if (task) {
            try {
                task();
            } catch (const std::exception &e) {
                LOG(Error, LogLevel::Core, "Thread [{}]: Exception thrown: {}", nameStr, e.what());
            } catch (...) {
                LOG(Error, LogLevel::Core, "Thread [{}]: Unknown exception thrown!", nameStr);
            }
        }
        LOG(Info, LogLevel::Core, "Thread [{}] terminated.", nameStr);
    });
    t.detach();
}

void M_Threads::Wait_All() {
    if (!s_IsInitialized) return;
    std::unique_lock<std::mutex> lock(s_QueueMutex);
    s_WaitAllCV.wait(lock, [] {
        return s_TaskQueue.empty() && s_ActiveTasks.load() == 0;
    });
}
