#include "wythread.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
// #include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include "exception.h"
#include "wythreadbase.h"

namespace wynet
{
using namespace std;

///////////////////////////////////////////

struct ThreadData
{
    typedef Thread::ThreadMain ThreadMain;
    ThreadMain m_func;
    string m_name;
    pid_t *m_tid;
    CountDownLatch *m_latch;

    ThreadData(const ThreadMain &func,
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

        CurrentThread::t_threadName = m_name.empty() ? "thread" : m_name.c_str();
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

Thread::Thread(const ThreadMain &func, const string &n)
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

Thread::Thread(ThreadMain &&func, const std::string &n)
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
    int ret = pthread_create(&m_pthreadId, NULL, &startThread, data);
    if (ret)
    {
        m_started = false;
        delete data;
        fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
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
