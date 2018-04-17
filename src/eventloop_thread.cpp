#include "eventloop_thread.h"
#include "eventloop.h"

using namespace wynet;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : m_loop(NULL),
      m_exiting(false),
      m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
      m_mutex(),
      m_cond(m_mutex),
      m_callback(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if (m_loop != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
    {
        // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        m_loop->stop();
        m_thread.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    assert(!m_thread.isStarted());
    m_thread.start();

    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        while (m_loop == NULL)
        {
            m_cond.wait();
        }
    }

    return m_loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify();
    }

    loop.loop();
    //assert(exiting_);
    m_loop = NULL;
}
