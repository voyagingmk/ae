#include "event_loop.h"

namespace wynet
{

void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    EventLoop* loop = (EventLoop*)(clientData);
    if (loop->onFileEvent) {
        loop->onFileEvent(fd, mask);
    }
}
    
int aeOnTimerEvent(struct aeEventLoop *eventLoop, long long timerid, void *clientData) {
    EventLoop* loop = (EventLoop*)(clientData);
    if (loop->onTimerEvent) {
       return loop->onTimerEvent(timerid);
    } else {
        return AE_NOMORE;
    }
}

EventLoop::EventLoop():
    onFileEvent(nullptr),
    onTimerEvent(nullptr)
{
    aeloop = aeCreateEventLoop(64);
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
}

void EventLoop ::createFileEvent(int fd, int mask)
{
    int setsize = aeGetSetSize(aeloop) ;
    while (fd >= setsize) {
        assert(AE_ERR != aeResizeSetSize(aeloop, setsize << 1));
    }
    int ret = aeCreateFileEvent(aeloop, fd, mask, aeOnFileEvent, (void *)this);
     assert(AE_ERR != ret);
}

void EventLoop ::deleteFileEvent(int fd, int mask)
{
    aeDeleteFileEvent(aeloop, fd, mask);
}
    
long long EventLoop ::createTimerEvent(long long ms)
{
    long long timerid = aeCreateTimeEvent(aeloop, ms, aeOnTimerEvent, (void *)this, NULL);
    assert(AE_ERR != timerid);
    return timerid;
}
};
