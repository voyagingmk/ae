#ifndef WY_EVENT_LOOP_H
#define WY_EVENT_LOOP_H

#include "noncopyable.h"
#include "common.h"

namespace wynet
{
class EventLoop : Noncopyable
{
  public:
    typedef void (*OnFileEvent)(int filefd, int mask);
    typedef int (*OnTimerEvent)(int timerfd);
    
    EventLoop();
    
    ~EventLoop();

    void loop();

    void stop();
    
    void createFileEvent(int fd, int mask);

    void deleteFileEvent(int fd, int mask);
    
    long long createTimerEvent(long long ms);
public:
    OnFileEvent onFileEvent;
    OnTimerEvent onTimerEvent;
private:
    friend void aeOnFileEvent(struct aeEventLoop *eventLoop, int filefd, void *clientData, int mask);
    friend int aeOnTimerEvent(struct aeEventLoop *eventLoop, long long timerid, void *clientData);
    
    aeEventLoop *aeloop;
};
};

#endif
