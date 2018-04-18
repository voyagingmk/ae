
#include "eventloop_threadpool.h"
#include "eventloop.h"
#include "eventloop_thread.h"

using namespace wynet;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name)
    : m_baseLoop(baseLoop),
      m_name(name),
      m_started(false),
      m_numThreads(0),
      m_next(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::setThreadNum(int n)
{
    assert(!m_started);
    m_numThreads = n;
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    assert(!m_started);
    m_baseLoop->assertInLoopThread();
    m_started = true;

    for (int i = 0; i < m_numThreads; ++i)
    {
        char nameBuf[m_name.size() + 1 + sizeof(int)];
        snprintf(nameBuf, sizeof nameBuf, "%s_%d", m_name.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, nameBuf);
        m_threads.push_back(t);
        m_loops.push_back(t->startLoop());
    }
    if (m_numThreads == 0 && cb) // 0个工作线程时，把自己这个loop作为线程池唯一线程
    {
        cb(m_baseLoop);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    m_baseLoop->assertInLoopThread();
    assert(m_started);
    if (m_loops.empty())
    {
        return m_baseLoop;
    }

    EventLoop *nextLoop = m_loops[m_next];
    ++m_next;
    m_next = (size_t)(m_next) % m_loops.size();
    return nextLoop;
}

EventLoop *EventLoopThreadPool::getLoopByHash(size_t hashCode)
{
    m_baseLoop->assertInLoopThread();
    assert(m_started);
    if (m_loops.empty())
    {
        return m_baseLoop;
    }

    return m_loops[hashCode % m_loops.size()];
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    m_baseLoop->assertInLoopThread();
    assert(m_started);
    if (m_loops.empty())
    {
        return std::vector<EventLoop *>(1, m_baseLoop);
    }
    return m_loops;
}
