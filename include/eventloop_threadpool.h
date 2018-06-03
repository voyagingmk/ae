#ifndef WY_EVENTLOOP_THREADPOOL_H
#define WY_EVENTLOOP_THREADPOOL_H

#include "noncopyable.h"
#include "common.h"

namespace wynet
{

class EventLoop;
class EventLoopThread;
using ThreadInitCallback = std::function<void(EventLoop *)>;

class EventLoopThreadPool : Noncopyable
{
  public:
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg, int threadNum = 0);

    ~EventLoopThreadPool();

    void setThreadNum(int n);

    void start(const ThreadInitCallback &cb = {});

    // must after calling start()
    EventLoop *getNextLoop();

    // must after calling start()
    EventLoop *getLoopByHash(size_t hashCode);

    std::vector<EventLoop *> getAllLoops();

    bool isStarted() const
    {
        return m_started;
    }

    const std::string &name() const
    {
        return m_name;
    }

  private:
    EventLoop *m_baseLoop;
    std::string m_name;
    bool m_started;
    int m_numThreads;
    int m_next;
    std::vector<std::shared_ptr<EventLoopThread>> m_threads;
    std::vector<EventLoop *> m_loops;
};

}; // namespace wynet

#endif