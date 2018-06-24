#include "eventloop.h"
#include "utils.h"

namespace wynet
{

// thread local
__thread EventLoop *t_threadLoop = nullptr;

int OnlyForWakeup(EventLoop *loop, TimerRef tr, PtrEvtListener listener, void *data)
{
    const int *ms = (const int *)(data);
    // log_info("OnlyForWakeup %d", *ms);
    return *ms;
}

int ForceStop(EventLoop *loop, TimerRef tr, PtrEvtListener listener, void *data)
{
    log_info("ForceStop");
    loop->stop();
    return MP_HALT;
}

int StatEventLoop(EventLoop *loop, TimerRef tr, PtrEvtListener listener, void *data)
{
    log_info("loop, taskqueue = %d listener = %d timer = %d",
             loop->queueSize(), loop->m_fd2listener.size(),
             loop->m_timerId2ref.size());
    loop->m_mploop.debugInfo();
    return 10 * 1000;
}

void OnSockEvent(MpEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
    assert(sockfd > 0);
    log_debug("file evt %d %s %s", sockfd,
              (mask & MP_READABLE) ? "rd" : "",
              (mask & MP_WRITABLE) ? "wr" : "");
    EventLoop *loop = (EventLoop *)(clientData);
    if (loop->m_fd2listener.find(sockfd) == loop->m_fd2listener.end())
    {
        log_fatal("OnSockEvent no wkListener sockfd:%d, m_fd2listener.size(): %d",
                  sockfd, loop->m_fd2listener.size());
        return;
    }
    WeakPtrEvtListener wkListener = loop->m_fd2listener[sockfd];
    PtrEvtListener listener = wkListener.lock();
    if (!listener)
    {
        log_error("OnSockEvent no listener %d", sockfd);
        // maybe deleteFileEvent job is in queue (async)
        loop->m_fd2listener.erase(sockfd);
        loop->deleteFileEventInLoop(sockfd, MP_READABLE | MP_WRITABLE);
        return;
    }
    assert(listener->getSockFd() == sockfd);
    if (listener->m_onFileEvent)
    {
        int lmask = listener->getFileEventMask();
        if ((lmask & mask) > 0)
        {
            assert(listener->hasFileEvent(mask));
            listener->m_onFileEvent(loop, listener, mask);
        }
        else
        {
            log_error("sockfd %d not listening this evt: %s", sockfd,
                      (mask & MP_READABLE) ? "rd" : "",
                      (mask & MP_WRITABLE) ? "wr" : "");
            loop->deleteFileEvent(listener, mask);
        }
    }
    else
    {
        log_fatal("OnSockEvent no m_onFileEvent %d", sockfd);
    }
}

int OnTimerEventTimeout(MpEventLoop *eventLoop, MpTimerId timerId, void *clientData)
{
    EventLoop *loop = (EventLoop *)(clientData);
    int err = 0;
    do
    {
        if (loop->m_timerId2ref.find(timerId) == loop->m_timerId2ref.end())
        {
            err = 1;
            break;
        }
        TimerRef tr = loop->m_timerId2ref[timerId];
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
        if (milliseconds == MP_HALT)
        {
            err = 6;
            break;
        }
        return milliseconds;
    } while (0);
    if (err != 6)
    {
        log_error("OnTimerEventTimeout err %d", err);
    }
    loop->deleteTimerInLoop(timerId);
    return MP_HALT;
}

EventLoop::EventLoop(int wakeupInterval, int defaultSetsize) : m_threadId(CurrentThread::tid()),
                                                               m_mploop(defaultSetsize),
                                                               m_ownEvtListener(new EventListener()),
                                                               m_wakeupInterval(wakeupInterval),
                                                               m_forceStopTime(60 * 1000),
                                                               m_doingTask(false),
                                                               m_stopping(false)
{
    log_ctor("EventLoop()");
    log_info("multiplexing:%s", m_mploop.getApiName());
    if (t_threadLoop)
    {
        log_fatal("Create 2 EventLoop For 1 thread?");
    }
    else
    {
        t_threadLoop = this;
    }
}

EventLoop::~EventLoop()
{
    log_dtor("~EventLoop()");
    if (m_taskFuncQueue.size() != 0)
    {
        log_error("~EventLoop() m_taskFuncQueue.size() = %d", m_taskFuncQueue.size());
    }
    if (m_fd2listener.size() != 0)
    {
        log_error("~EventLoop() m_fd2listener.size() = %d", m_fd2listener.size());
        for (auto it : m_fd2listener)
        {
            PtrEvtListener l = it.second.lock();
            log_error("name:%s mask:%d", l->getName().c_str(), l->getFileEventMask());
        }
    }
    t_threadLoop = nullptr;
    assert(m_taskFuncQueue.size() == 0);
    assert(m_fd2listener.size() == 0);
}

void EventLoop::loop()
{
    assertInLoopThread("loop");
    MpTimerId timerIdWakeup = createTimerInLoop(m_ownEvtListener, m_wakeupInterval, OnlyForWakeup, (void *)&m_wakeupInterval);
    MpTimerId timerIdStat = createTimerInLoop(m_ownEvtListener, 10 * 1000, StatEventLoop, nullptr);
    while (!m_mploop.isStopped())
    {
        log_timemeasure("loop");
        {
            log_timemeasure("processEvents");
            m_mploop.processEvents(MP_ALL_EVENTS | MP_CALL_AFTER_SLEEP);
        }
        {
            log_timemeasure("processTaskQueue");
            processTaskQueue();
        }
        if (m_stopping)
        {
            if (m_fd2listener.size() == 0)
            {
                log_info("stopSafely ok");
                stopInLoop();
            }
            else
            {
                // log_info("m_stopping size: %d", (int)m_fd2listener.size());
            }
        }
    }
    log_info("loop exited");
    processTaskQueue();
    deleteTimerInLoop(timerIdWakeup);
    deleteTimerInLoop(timerIdStat);
}

void EventLoop::stopSafely()
{
    log_info("EventLoop::stopSafely");
    runInLoop([=]() {
        m_stopping = true;
        createTimerInLoop(m_ownEvtListener, m_forceStopTime, ForceStop, nullptr);
    });
}

void EventLoop::stop()
{
    log_info("EventLoop::stop");
    runInLoop([=]() {
        stopInLoop();
    });
}

void EventLoop::stopInLoop()
{
    log_info("EventLoop::stopInLoop");
    assertInLoopThread("stop");
    if (!m_mploop.isStopped())
    {
        m_mploop.stop();
        log_info("EventLoop stop2");
    }
}

int EventLoop::getFileEvent(SockFd sockfd)
{
    assertInLoopThread("hasFileEvent");
    return m_mploop.getFileEvents(sockfd);
}

bool EventLoop::hasFileEvent(SockFd sockfd, int mask)
{
    assertInLoopThread("hasFileEvent");
    assert((mask & MP_READABLE) || (mask & MP_WRITABLE));
    int oldMask = m_mploop.getFileEvents(sockfd);
    return (oldMask & mask);
}

void EventLoop::createFileEvent(PtrEvtListener listener, int mask)
{
    runInLoop([=]() {
        createFileEventInLoop(listener, mask);
    });
}

void EventLoop::deleteFileEvent(PtrEvtListener listener, int mask)
{
    SockFd sockfd = listener->getSockFd();
    assert(sockfd > 0);
    runInLoop([=]() {
        deleteFileEventInLoop(sockfd, mask);
    });
}

void EventLoop::deleteFileEvent(SockFd sockfd, int mask)
{
    runInLoop([=]() {
        deleteFileEventInLoop(sockfd, mask);
    });
}

void EventLoop::deleteAllFileEvent(SockFd sockfd)
{
    runInLoop([=]() {
        deleteAllFileEventInLoop(sockfd);
    });
}

void EventLoop::createFileEventInLoop(const PtrEvtListener &listener, int mask)
{
    assertInLoopThread("createFileEventInLoop");
    assert((mask & MP_READABLE) || (mask & MP_WRITABLE));
    int setsize = m_mploop.getSetSize();
    SockFd sockfd = listener->getSockFd();
    if (sockfd >= setsize)
    {
        while (sockfd >= setsize)
        {
            setsize = setsize << 1;
        }
        assert(MP_ERR != m_mploop.resizeSetSize(setsize));
    }
    int oldMask = getFileEvent(sockfd);
    if ((oldMask & MP_READABLE) && (mask & MP_READABLE))
    {
        log_warn("already has MP_READABLE");
    }
    else if ((oldMask & MP_WRITABLE) && (mask & MP_WRITABLE))
    {
        log_warn("already has MP_WRITABLE");
    }
    int ret = m_mploop.createFileEvent(sockfd, mask, OnSockEvent, (void *)this);
    assert(MP_ERR != ret);
    auto it = m_fd2listener.find(sockfd);
    if (it != m_fd2listener.end())
    {
        PtrEvtListener l = it->second.lock();
        assert(l == listener);
    }
    m_fd2listener[sockfd] = listener;
}

void EventLoop ::deleteFileEventInLoop(SockFd sockfd, int mask)
{
    // log_info("deleteFileEventInLoop %d %d", sockfd, mask);
    assert(sockfd > 0);
    assertInLoopThread("deleteFileEventInLoop");
    assert((mask & MP_READABLE) || (mask & MP_WRITABLE));
    int setsize = m_mploop.getSetSize();
    if (sockfd >= setsize)
    {
        log_warn("deleteFileEventInLoop sockfd >= setsize  %d %d\n", sockfd, setsize);
    }
    int ret = m_mploop.deleteFileEvent(sockfd, mask);
    assert(MP_ERR != ret);
    if (!getFileEvent(sockfd) && m_fd2listener.find(sockfd) != m_fd2listener.end())
    {
        m_fd2listener.erase(sockfd);
    }
}

void EventLoop ::deleteAllFileEventInLoop(SockFd sockfd)
{
    assert(sockfd > 0);
    deleteFileEventInLoop(sockfd, MP_READABLE | MP_WRITABLE);
}

TimerRef EventLoop ::createTimer(PtrEvtListener listener, int ms, OnTimerEvent onTimerEvent, void *data)
{
    // 分配新的timer标识
    TimerRef tr = TimerRef::newTimerRef();
    runInLoop([=]() {
        createTimerInLoop(listener, tr, ms, onTimerEvent, data);
    });
    return tr;
}

void EventLoop ::deleteTimer(TimerRef tr)
{
    runInLoop([=]() {
        deleteTimerInLoop(tr);
    });
}

void EventLoop ::deleteTimerInLoop(MpTimerId timerId)
{
    assertInLoopThread("deleteTimerInLoop");
    auto it = m_timerId2ref.find(timerId);
    if (it == m_timerId2ref.end())
    {
        return;
    }
    TimerRef tr = it->second;
    deleteTimerInLoop(tr);
}

void EventLoop ::deleteTimerInLoop(TimerRef tr)
{
    assertInLoopThread("deleteTimerInLoop");
    if (m_timerData.find(tr) != m_timerData.end())
    {
        TimerData &data = m_timerData[tr];
        WyTimerId timerId = data.m_timerId;
        m_timerData.erase(tr);
        m_timerId2ref.erase(timerId);
        m_mploop.deleteTimeEvent(timerId);
    }
}

MpTimerId EventLoop ::createTimerInLoop(PtrEvtListener listener, int ms, OnTimerEvent onTimerEvent, void *data)
{
    TimerRef tr = TimerRef::newTimerRef();
    return createTimerInLoop(listener, tr, ms, onTimerEvent, data);
}

MpTimerId EventLoop ::createTimerInLoop(PtrEvtListener listener, TimerRef tr, int ms, OnTimerEvent onTimerEvent, void *data)
{
    assertInLoopThread("createTimerInLoop");
    // log_debug("createTimerInLoop ms %d", ms);
    MpTimerId timerId = m_mploop.createTimeEvent(ms, OnTimerEventTimeout, (void *)this, NULL);
    assert(MP_ERR != timerId);
    assert(m_timerData.find(tr) == m_timerData.end());
    m_timerData[tr] = {onTimerEvent, listener, data, timerId};
    m_timerId2ref.insert({timerId, tr});
    return timerId;
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
