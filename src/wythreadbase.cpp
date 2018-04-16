#include "wythreadbase.h"

namespace wynet
{
namespace CurrentThread
{
__thread int t_tidCached = 0;
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

bool isMainThread()
{
    // https://linux.die.net/man/2/gettid
    // In a single-threaded process, the thread ID is equal to the process ID
    return tid() == ::getpid();
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
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

const static ThreadNameInit init;
};
};
