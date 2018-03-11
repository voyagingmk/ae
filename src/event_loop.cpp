#include "event_loop.h"

namespace wynet
{

EventLoop::EventLoop()
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
};
