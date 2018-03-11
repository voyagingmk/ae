#include "wythread.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace wynet
{
namespace CurrentThread
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";
const bool sameType = boost::is_same<int, pid_t>::value;
BOOST_STATIC_ASSERT(sameType);
}

namespace detail
{

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
    muduo::CurrentThread::t_cachedTid = 0;
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
  public:
    ThreadNameInitializer()
    {
        muduo::CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

ThreadNameInitializer init;

struct ThreadData
{
    typedef muduo::Thread::ThreadFunc ThreadFunc;
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
        *m_tid = muduo::CurrentThread::tid();
        m_tid = NULL;
        m_latch->countDown();
        m_latch = NULL;

        muduo::CurrentThread::t_threadName = m_name.empty() ? "muduoThread" : m_name.c_str();
        ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
        try
        {
            m_func();
            muduo::CurrentThread::t_threadName = "finished";
        }
        catch (const Exception &ex)
        {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();
        }
        catch (const std::exception &ex)
        {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", m_name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...)
        {
            muduo::CurrentThread::t_threadName = "crashed";
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
}
}

using namespace muduo;

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
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
    setDefaultName();
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
    detail::ThreadData *data = new detail::ThreadData(m_func, m_name, &m_tid, &m_latch);
    if (pthread_create(&m_pthreadId, NULL, &detail::startThread, data))
    {
        m_started = false;
        delete data; // or no delete?
        LOG_SYSFATAL << "Failed in pthread_create";
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