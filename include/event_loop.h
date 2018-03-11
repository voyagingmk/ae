#ifndef WY_EVENT_LOOP_H
#define WY_EVENT_LOOP_H

#include "noncopyable.h"
#include "common.h"

namespace wynet
{
class EventLoop : Noncopyable
{
    aeEventLoop *aeloop;

  public:
    EventLoop();
    ~EventLoop();

    void loop();

    void stop();
};
};

#endif