#include "event_loop.h"

namespace wynet
{

int OnlyForWakeup(EventLoop *, TimerId timerfd, void *userData)
{
    const int *ms = (const int *)(userData);
    return *ms;
}

void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    EventLoop *loop = (EventLoop *)(clientData);
    std::shared_ptr<EventLoop::FDData> p = loop->fdData[fd];
    if (p && p->onFileEvent)
    {
        if (mask & AE_READABLE)
        {
            p->onFileEvent(loop, fd, p->userDataRead, mask);
        }
        if (mask & AE_WRITABLE)
        {
            p->onFileEvent(loop, fd, p->userDataWrite, mask);
        }
    }
}

int aeOnTimerEvent(struct aeEventLoop *eventLoop, TimerId timerid, void *clientData)
{
    EventLoop *loop = (EventLoop *)(clientData);
    std::shared_ptr<EventLoop::TimerData> p = loop->timerData[timerid];
    if (!p || !p->onTimerEvent)
    {
        return AE_NOMORE;
    }
    int ret = p->onTimerEvent(loop, timerid, p->userData);
    if (ret == AE_NOMORE)
    {
        loop->timerData.erase(timerid);
    }
    return ret;
}

EventLoop::EventLoop(int wakeupInterval, int defaultSetsize) : m_threadId(CurrentThread::tid()),
                                                               m_wakeupInterval(wakeupInterval),
                                                               m_doingTask(false)
{
    aeloop = aeCreateEventLoop(defaultSetsize);
}

EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
    assertInLoopThread();
    aeloop->stop = 0;
    TimerId timerid = createTimer(m_wakeupInterval, OnlyForWakeup, (void *)&m_wakeupInterval);
    while (!aeloop->stop)
    {
        if (aeloop->beforesleep != NULL)
            aeloop->beforesleep(aeloop);
        aeProcessEvents(aeloop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
        processTaskQueue();
    }
    deleteTimer(timerid);
}

void EventLoop::stop()
{
    aeStop(aeloop);
    log_info("EventLoop stop");
}

void EventLoop ::createFileEvent(int fd, int mask, OnFileEvent onFileEvent, void *userData)
{
    int setsize = aeGetSetSize(aeloop);
    while (fd >= setsize)
    {
        assert(AE_ERR != aeResizeSetSize(aeloop, setsize << 1));
    }
    int ret = aeCreateFileEvent(aeloop, fd, mask, aeOnFileEvent, (void *)this);
    assert(AE_ERR != ret);
    if (fdData.find(fd) == fdData.end())
    {
        std::shared_ptr<FDData> p(new FDData());
        fdData.insert({fd, p});
    }
    std::shared_ptr<FDData> p = fdData[fd];
    p->onFileEvent = onFileEvent;
    if (mask & AE_READABLE)
    {
        p->userDataRead = userData;
    }
    if (mask & AE_WRITABLE)
    {
        p->userDataWrite = userData;
    }
}

void EventLoop ::deleteFileEvent(int fd, int mask)
{
    aeDeleteFileEvent(aeloop, fd, mask);
}

TimerId EventLoop ::createTimerInLoop(TimerId ms, OnTimerEvent onTimerEvent, void *userData)
{
    runInLoop([&]() {
        createTimer(ms, onTimerEvent, userData);
    });
}

bool EventLoop ::deleteTimerInLoop(TimerId timerid)
{
    runInLoop([&]() {
        deleteTimer(timerid);
    });
}

bool EventLoop ::deleteTimer(TimerId timerid)
{
    timerData.erase({timerid});
    int ret = aeDeleteTimeEvent(aeloop, timerid);
    return AE_ERR != ret;
}

TimerId EventLoop ::createTimer(TimerId ms, OnTimerEvent onTimerEvent, void *userData)
{
    TimerId timerid = aeCreateTimeEvent(aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != timerid);
    if (timerData.find(timerid) == timerData.end())
    {
        std::shared_ptr<TimerData> p(new TimerData());
        timerData.insert({timerid, p});
    }
    std::shared_ptr<TimerData> p = timerData[timerid];
    p->onTimerEvent = onTimerEvent;
    p->userData = userData;
    return timerid;
}

void EventLoop::runInLoop(const TaskFunction &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const TaskFunction &cb)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    m_taskFuncQueue.push_back(cb);
}

size_t EventLoop::queueSize() const
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    return m_taskFuncQueue.size();
}

void EventLoop::processTaskQueue()
{
    std::vector<TaskFunction> taskFuncQueue;
    m_doingTask = true;

    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        taskFuncQueue.swap(m_taskFuncQueue);
    }

    for (size_t i = 0; i < taskFuncQueue.size(); ++i)
    {
        taskFuncQueue[i]();
    }
    m_doingTask = false;
}
};
