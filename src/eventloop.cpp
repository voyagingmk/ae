#include "eventloop.h"

namespace wynet
{

// thread local
__thread EventLoop *t_threadLoop = nullptr;
std::atomic<WyTimerId> TimerRef::g_numCreated;

int OnlyForWakeup(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
{
    const int *ms = (const int *)(data);
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
    TimerRef tr = loop->m_aeTimerId2ref[aeTimerId];
    do
    {
        if (!tr.validate())
        {
            break;
        }
        if (loop->m_timerData.find(tr) == loop->m_timerData.end())
        {
            break;
        }
        EventLoop::TimerData &td = loop->m_timerData[tr];
        if (!td.onTimerEvent)
        {
            loop->m_timerData.erase(tr);
            break;
        }
        int ret = td.onTimerEvent(loop, tr, td.m_listener.lock(), td.data);
        if (ret == AE_NOMORE)
        {
            break;
        }
        return ret;
    } while (0);
    loop->m_aeTimerId2ref.erase(aeTimerId);
    return AE_NOMORE;
}

EventListener::~EventListener()
{
    if (m_loop && m_sockFd)
    {
        m_loop->deleteAllFileEvent(m_sockFd);
    }
    m_loop = nullptr;
    m_sockFd = 0;
}

void EventListener::createFileEvent(int mask, OnFileEvent onFileEvent)
{
    if (m_loop && m_sockFd)
    {
        m_onFileEvent = onFileEvent;
        m_loop->createFileEvent(shared_from_this(), mask);
    }
    else
    {
        log_error("EventListener::createFileEvent");
    }
}

void EventListener::deleteFileEvent(int mask)
{
    if (m_loop && m_sockFd)
    {
        m_loop->deleteFileEvent(shared_from_this(), mask);
    }
    else
    {
        log_error("EventListener::deleteFileEvent");
    }
}

TimerRef EventListener::createTimer(int ms, OnTimerEvent onTimerEvent, void *data)
{
    if (m_loop)
    {
        return m_loop->createTimer(shared_from_this(), ms, onTimerEvent, data);
    }
    else
    {
        log_error("EventListener::deleteFileEvent");
        return TimerRef(0);
    }
}

EventLoop::EventLoop(int wakeupInterval, int defaultSetsize) : m_threadId(CurrentThread::tid()),
                                                               m_wakeupInterval(wakeupInterval),
                                                               m_doingTask(false)
{
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
    aeDeleteEventLoop(m_aeloop);
    t_threadLoop = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    m_aeloop->stop = 0;
    AeTimerId aeTimerId = createTimerInLoop(nullptr, m_wakeupInterval, OnlyForWakeup, (void *)&m_wakeupInterval);
    while (!m_aeloop->stop)
    {
        if (m_aeloop->beforesleep != NULL)
            m_aeloop->beforesleep(m_aeloop);
        aeProcessEvents(m_aeloop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
        processTaskQueue();
    }
    deleteTimerInLoop(aeTimerId);
}

void EventLoop::stop()
{
    if (!m_aeloop->stop)
    {
        aeStop(m_aeloop);
        log_info("EventLoop stop");
    }
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
        WyTimerId timerId = m_timerData[tr].timerId;
        m_timerData.erase(tr);
        int ret = aeDeleteTimeEvent(m_aeloop, timerId);
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
