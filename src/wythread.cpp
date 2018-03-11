#include "wythread.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
// #include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include "exception.h"

namespace wynet
{
using namespace std;
namespace CurrentThread
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";

void cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

bool isMainThread()
{
    return tid() == ::getpid();
}

void sleepUsec(int64_t usec)
{
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / (1000 * 1000));
    ts.tv_nsec = static_cast<long>(usec % (1000 * 1000) * 1000);
    ::nanosleep(&ts, NULL);
}

pid_t gettid()
{
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return static_cast<pid_t>(tid);
    // return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
    CurrentThread::t_cachedTid = 0;
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
}

class ThreadNameInitializer
{
  public:
    ThreadNameInitializer()
    {
        CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

ThreadNameInitializer init;
};

///////////////////////////////////////////

struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc m_func;
    string m_name;
    pid_t *m_tid;
    CountDownLatch *m_latch;

    ThreadData(const ThreadFunc &func,
               const string &name,
               pid_t *tid,
               CountDownLatch *latch)
        : m_func(func),
          m_name(name),
          m_tid(tid),
          m_latch(latch)
    {
    }

    void runInThread()
    {
        *m_tid = CurrentThread::tid();
        m_tid = NULL;
        m_latch->countDown();
        m_latch = NULL;

        CurrentThread::t_threadName = m_name.empty() ? "Thread" : m_name.c_str();
        try
        {
            m_func();
            CurrentThread::t_threadName = "finished";
        }
        catch (const Exception &ex)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();
        }
        catch (const std::exception &ex)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...)
        {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", m_name.c_str());
            throw; // rethrow
        }
    }
};

void *startThread(void *obj)
{
    ThreadData *data = static_cast<ThreadData *>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

Thread::Thread(const ThreadFunc &func, const string &n)
    : m_started(false),
      m_joined(false),
      m_pthreadId(0),
      m_tid(0),
      m_func(func),
      m_name(n),
      m_latch(1)
{
    int num = ++m_numCreated;
    setDefaultName(num);
}

Thread::Thread(ThreadFunc &&func, const std::string &n)
    : m_started(false),
      m_joined(false),
      m_pthreadId(0),
      m_tid(0),
      m_func(std::move(func)),
      m_name(n),
      m_latch(1)
{
    int num = ++m_numCreated;
    setDefaultName(num);
}

Thread::~Thread()
{
    if (m_started && !m_joined)
    {
        pthread_detach(m_pthreadId);
    }
}

void Thread::setDefaultName(int num)
{
    if (m_name.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread-%d", num);
        m_name = buf;
    }
}

void Thread::start()
{
    assert(!m_started);
    m_started = true;
    // FIXME: move(m_func)
    ThreadData *data = new ThreadData(m_func, m_name, &m_tid, &m_latch);
    if (pthread_create(&m_pthreadId, NULL, &startThread, data))
    {
        m_started = false;
        delete data; // or no delete?
        // LOG_SYSFATAL << "Failed in pthread_create";
    }
    else
    {
        m_latch.wait();
        assert(m_tid > 0);
    }
}

int Thread::join()
{
    assert(m_started);
    assert(!m_joined);
    m_joined = true;
    return pthread_join(m_pthreadId, NULL);
}

std::atomic<int32_t> Thread::m_numCreated;
};
