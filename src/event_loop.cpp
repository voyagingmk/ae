#include "event_loop.h"

namespace wynet
{

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

int aeOnTimerEvent(struct aeEventLoop *eventLoop, long long timerid, void *clientData)
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

EventLoop::EventLoop(int defaultSetsize):
    m_threadId(CurrentThread::tid())
{
    aeloop = aeCreateEventLoop(defaultSetsize);
}

EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
    aeloop->stop = 0;
    while (!aeloop->stop)
    {
        if (aeloop->beforesleep != NULL)
            aeloop->beforesleep(aeloop);
        aeProcessEvents(aeloop, AE_ALL_EVENTS | AE_DONT_WAIT | AE_CALL_AFTER_SLEEP);
    }
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

long long EventLoop ::createTimer(long long ms, OnTimerEvent onTimerEvent, void *userData)
{
    long long timerid = aeCreateTimeEvent(aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != timerid);
    if (timerData.find(timerid) == timerData.end())
    {
        std::shared_ptr<TimerData> p(new TimerData());
        timerData.insert({timerid, p});
    }
    std::shared_ptr<TimerData> p = timerData[timerid];
    p->onTimerEvent = onTimerEvent;
    return timerid;
}

bool EventLoop ::deleteTimer(long long timerid)
{
    timerData.erase({timerid});
    int ret = aeDeleteTimeEvent(aeloop, timerid);
    return AE_ERR != ret;
}
};
