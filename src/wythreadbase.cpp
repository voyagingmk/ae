#include "wythreadbase.h"

namespace wynet
{
namespace CurrentThread
{
static pid_t g_mainThreadId = 0;
__thread pid_t t_tidCached = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";

void cacheTid()
{
    if (t_tidCached)
        return;
    t_tidCached = gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_tidCached);
}

void setMainThreadId()
{
#if __APPLE__
    if (!g_mainThreadId)
    {
        g_mainThreadId = tid();
    }
#endif
}

pid_t mainThreadId()
{
#if __APPLE__
    return g_mainThreadId;
#else
    ::getpid();
#endif
}

bool isMainThread()
{
#if __APPLE__
    return tid() == g_mainThreadId;
#else
    // https://linux.die.net/man/2/gettid
    // In a single-threaded process, the thread ID is equal to the process ID
    return tid() == mainThreadId();
#endif
}

pid_t gettid()
{
#if __APPLE__
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return static_cast<pid_t>(tid);
#else
    return static_cast<pid_t>(::syscall(SYS_gettid));
#endif
}

void afterFork()
{
    CurrentThread::t_tidCached = 0;
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
}

class ThreadNameInit
{
  public:
    ThreadNameInit()
    {
        CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        setMainThreadId();
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

const static ThreadNameInit init;
};
};
