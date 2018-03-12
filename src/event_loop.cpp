#include "event_loop.h"

namespace wynet
{
std::atomic<TimerId> TimerRef::g_numCreated;
    
int OnlyForWakeup(EventLoop *, TimerRef tr, void *userData)
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
    TimerRef tr = loop->timerId2ref[timerid];
    do {
        if (!tr.validate())
        {
            break;
        }
        std::shared_ptr<EventLoop::TimerData> p = loop->timerData[tr];
        if (!p || !p->onTimerEvent)
        {
             loop->timerData.erase(tr);
             break;
        }
        int ret = p->onTimerEvent(loop, tr, p->userData);
        if (ret == AE_NOMORE)
        {
            break;
        }
        return ret;
    } while(0);
    loop->timerId2ref.erase(timerid);
    return AE_NOMORE;
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
    TimerRef tr = createTimerInLoop(m_wakeupInterval, OnlyForWakeup, (void *)&m_wakeupInterval);
    while (!aeloop->stop)
    {
        if (aeloop->beforesleep != NULL)
            aeloop->beforesleep(aeloop);
        aeProcessEvents(aeloop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
        processTaskQueue();
    }
    deleteTimerInLoop(tr);
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

TimerRef EventLoop ::createTimerInLoop(TimerId ms, OnTimerEvent onTimerEvent, void *userData)
{
    TimerRef tr = TimerRef::newTimerRef();
    runInLoop([&]() {
        createTimer(tr, ms, onTimerEvent, userData);
    });
    return tr;
}

void EventLoop ::deleteTimerInLoop(TimerRef tr)
{
    runInLoop([&]() {
        deleteTimer(tr);
    });
}

bool EventLoop ::deleteTimer(TimerRef tr)
{
    std::shared_ptr<TimerData> p = timerData[tr];
    if (p) {
        timerData.erase(tr);
        int ret = aeDeleteTimeEvent(aeloop, p->timerid);
        return AE_ERR != ret;
    }
    return false;
}

TimerId EventLoop ::createTimer(TimerRef tr, TimerId ms, OnTimerEvent onTimerEvent, void *userData)
{
    TimerId timerid = aeCreateTimeEvent(aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != timerid);
    assert(timerData.find(tr) == timerData.end());
    std::shared_ptr<TimerData> p(new TimerData());
    timerData.insert({tr, p});
    timerId2ref.insert({timerid, tr});
    p->onTimerEvent = onTimerEvent;
    p->userData = userData;
    p->timerid = timerid;
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
