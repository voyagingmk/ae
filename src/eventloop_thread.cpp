#include "eventloop_thread.h"
#include "eventloop.h"

using namespace wynet;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : m_loop(NULL),
      m_exiting(false),
      m_thread(std::bind(&EventLoopThread::threadEntry, this), name),
      m_mutex(),
      m_cond(m_mutex),
      m_callback(cb)
{
    if (LOG_CTOR_DTOR)
        log_info("EventLoopThread()");
}

EventLoopThread::~EventLoopThread()
{
    if (LOG_CTOR_DTOR)
        log_info("~EventLoopThread()");
    m_exiting = true;
    if (m_loop != NULL)
    {
        m_loop->stop();
        m_thread.join();
        log_info("~EventLoopThread() m_thread end");
    }
}

EventLoop *EventLoopThread::startLoop()
{
    assert(!m_thread.isStarted());
    m_thread.start(); // 启动线程
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        while (m_loop == NULL) // 线程此时还没执行threadFunc，进入等待
        {
            m_cond.wait();
        }
    }
    return m_loop;
}

void EventLoopThread::threadEntry()
{
    log_info("EventLoopThread::threadEntry()");
    EventLoop loop;

    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        MutexLockGuard<MutexLock> lock(m_mutex); // 和startLoop里的lock的竞争
        m_loop = &loop;
        m_cond.notify();
    }

    loop.loop();
    m_loop = NULL; // TODO
}
