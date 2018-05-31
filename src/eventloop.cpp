#include "eventloop.h"
#include "utils.h"

namespace wynet
{

// thread local
__thread EventLoop *t_threadLoop = nullptr;

int OnlyForWakeup(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
{
    const int *ms = (const int *)(data);
    // log_debug("OnlyForWakeup %d", *ms);
    return *ms;
}

void aeOnFileEvent(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
    EventLoop *loop = (EventLoop *)(clientData);
    WeakPtrEvtListener wkListener = loop->m_fd2listener[sockfd];

    PtrEvtListener listener = wkListener.lock();
    if (!listener)
    {
        return;
    }
    if (listener->m_onFileEvent)
    {
        if ((mask & LOOP_EVT_READABLE) || (mask & LOOP_EVT_WRITABLE))
        {
            listener->m_onFileEvent(loop, listener, mask);
        }
    }
}

int aeOnTimerEvent(struct aeEventLoop *eventLoop, AeTimerId aeTimerId, void *clientData)
{
    EventLoop *loop = (EventLoop *)(clientData);
    int err = 0;
    do
    {
        if (loop->m_aeTimerId2ref.find(aeTimerId) == loop->m_aeTimerId2ref.end())
        {
            err = 1;
            break;
        }
        TimerRef tr = loop->m_aeTimerId2ref[aeTimerId];
        if (!tr.validate())
        {
            err = 2;
            break;
        }
        if (loop->m_timerData.find(tr) == loop->m_timerData.end())
        {
            err = 3;
            break;
        }
        EventLoop::TimerData &td = loop->m_timerData[tr];
        if (!td.m_onTimerEvent)
        {
            err = 4;
            break;
        }
        PtrEvtListener listener = td.m_listener.lock();
        if (!listener)
        {
            err = 5;
            break;
        }
        int milliseconds = td.m_onTimerEvent(loop, tr, listener, td.m_data);
        if (milliseconds == AE_NOMORE)
        {
            err = 6;
            break;
        }
        return milliseconds;
    } while (0);
    // log_debug("aeOnTimerEvent err %d", err);
    loop->deleteTimerInLoop(aeTimerId);
    return AE_NOMORE;
}

EventLoop::EventLoop(int wakeupInterval, int defaultSetsize) : m_threadId(CurrentThread::tid()),
                                                               m_ownEvtListener(new EventListener()),
                                                               m_wakeupInterval(wakeupInterval),
                                                               m_doingTask(false)
{
    log_ctor("EventLoop()");
    if (t_threadLoop)
    {
        log_fatal("Create 2 EventLoop For 1 thread?");
    }
    else
    {
        t_threadLoop = this;
    }
    m_aeloop = aeCreateEventLoop(defaultSetsize);
}

EventLoop::~EventLoop()
{
    log_dtor("~EventLoop()");
    assert(m_fd2listener.size() == 0);

    /*
    if (m_fd2listener.size() > 0)
    {
        log_error("m_fd2listener.size(): %d", m_fd2listener.size());
        for (auto it : m_fd2listener)
        {
            log_error("%s", it.second.lock()->getName().c_str());
        }
    }*/
    aeDeleteEventLoop(m_aeloop);
    t_threadLoop = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    m_aeloop->stop = 0;
    AeTimerId aeTimerId = createTimerInLoop(m_ownEvtListener, m_wakeupInterval, OnlyForWakeup, (void *)&m_wakeupInterval);
    while (!m_aeloop->stop)
    {
        if (m_aeloop->beforesleep != NULL)
            m_aeloop->beforesleep(m_aeloop);
        aeProcessEvents(m_aeloop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
        // log_debug("processTaskQueue");
        processTaskQueue();
    }
    processTaskQueue();
    deleteTimerInLoop(aeTimerId);
}

void EventLoop::stop()
{
    if (!m_aeloop->stop)
    {
        aeStop(m_aeloop);
        log_debug("EventLoop stop");
    }
}

bool EventLoop::hasFileEvent(PtrEvtListener listener, int mask)
{
    assertInLoopThread();
    assert((mask & AE_READABLE) || (mask & AE_WRITABLE));
    SockFd sockfd = listener->getSockFd();
    int oldMask = aeGetFileEvents(m_aeloop, sockfd);
    return (oldMask & mask);
}

void EventLoop::createFileEvent(PtrEvtListener listener, int mask)
{
    assertInLoopThread();
    assert((mask & AE_READABLE) || (mask & AE_WRITABLE));
    int setsize = aeGetSetSize(m_aeloop);
    SockFd sockfd = listener->getSockFd();
    while (sockfd >= setsize)
    {
        assert(AE_ERR != aeResizeSetSize(m_aeloop, setsize << 1));
        setsize = aeGetSetSize(m_aeloop);
    }
    int oldMask = aeGetFileEvents(m_aeloop, sockfd);
    if (oldMask == mask)
    {
        return;
    }
    if ((oldMask & AE_READABLE) && (mask & AE_READABLE))
    {
        mask &= ~AE_READABLE;
    }
    else if ((oldMask & AE_WRITABLE) && (mask & AE_WRITABLE))
    {
        mask &= ~AE_WRITABLE;
    }
    int ret = aeCreateFileEvent(m_aeloop, sockfd, mask, aeOnFileEvent, (void *)this);
    assert(AE_ERR != ret);
    m_fd2listener[sockfd] = listener;
}

void EventLoop ::deleteFileEvent(PtrEvtListener listener, int mask)
{
    SockFd sockfd = listener->getSockFd();
    deleteFileEvent(sockfd, mask);
}

void EventLoop ::deleteFileEvent(SockFd sockfd, int mask)
{
    assertInLoopThread();
    assert((mask & AE_READABLE) || (mask & AE_WRITABLE));
    int setsize = aeGetSetSize(m_aeloop);
    if (sockfd >= setsize)
    {
        log_warn("deleteFileEvent sockfd >= setsize  %d %d\n", sockfd, setsize);
    }
    aeDeleteFileEvent(m_aeloop, sockfd, mask);
    if (!aeGetFileEvents(m_aeloop, sockfd) && m_fd2listener.find(sockfd) != m_fd2listener.end())
    {
        m_fd2listener.erase(sockfd);
    }
}

void EventLoop ::deleteAllFileEvent(SockFd sockfd)
{
    deleteFileEvent(sockfd, AE_READABLE | AE_WRITABLE);
}

TimerRef EventLoop ::createTimer(PtrEvtListener listener, int ms, OnTimerEvent onTimerEvent, void *data)
{
    // 分配新的timer标识
    TimerRef tr = TimerRef::newTimerRef();
    runInLoop([&]() {
        createTimerInLoop(listener, tr, ms, onTimerEvent, data);
    });
    return tr;
}

void EventLoop ::deleteTimer(TimerRef tr)
{
    runInLoop([&]() {
        deleteTimerInLoop(tr);
    });
}

bool EventLoop ::deleteTimerInLoop(AeTimerId aeTimerId)
{
    assertInLoopThread();
    auto it = m_aeTimerId2ref.find(aeTimerId);
    if (it == m_aeTimerId2ref.end())
    {
        return false;
    }
    TimerRef tr = it->second;
    return deleteTimerInLoop(tr);
}

bool EventLoop ::deleteTimerInLoop(TimerRef tr)
{
    assertInLoopThread();
    if (m_timerData.find(tr) != m_timerData.end())
    {
        TimerData &data = m_timerData[tr];
        WyTimerId aeTimerId = data.m_aeTimerId;
        m_timerData.erase(tr);
        m_aeTimerId2ref.erase(aeTimerId);
        int ret = aeDeleteTimeEvent(m_aeloop, aeTimerId);
        return AE_ERR != ret;
    }
    return false;
}

AeTimerId EventLoop ::createTimerInLoop(PtrEvtListener listener, int ms, OnTimerEvent onTimerEvent, void *data)
{
    TimerRef tr = TimerRef::newTimerRef();
    return createTimerInLoop(listener, tr, ms, onTimerEvent, data);
}

AeTimerId EventLoop ::createTimerInLoop(PtrEvtListener listener, TimerRef tr, int ms, OnTimerEvent onTimerEvent, void *data)
{
    assertInLoopThread();
    // log_debug("createTimerInLoop ms %d", ms);
    AeTimerId aeTimerId = aeCreateTimeEvent(m_aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != aeTimerId);
    assert(m_timerData.find(tr) == m_timerData.end());
    m_timerData[tr] = {onTimerEvent, listener, data, aeTimerId};
    m_aeTimerId2ref.insert({aeTimerId, tr});
    return aeTimerId;
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

EventLoop *EventLoop::getCurrentThreadLoop()
{
    return t_threadLoop;
}
}; // namespace wynet
