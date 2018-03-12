#ifndef WY_EVENT_LOOP_H
#define WY_EVENT_LOOP_H

#include "noncopyable.h"
#include "common.h"
#include "wythread.h"
#include <functional>

namespace wynet
{
    
#define LOOP_EVT_NONE AE_NONE
#define LOOP_EVT_READABLE AE_READABLE
#define LOOP_EVT_WRITABLE AE_WRITABLE
#define LOOP_EVT_NOMORE AE_NOMORE
    
class EventLoop : Noncopyable
{
  public:
    typedef std::function<void()> TaskFunction;
    typedef void (*OnFileEvent)(EventLoop *, int fd, void *userData, int mask);
    typedef int (*OnTimerEvent)(EventLoop *, long long timerfd, void *userData);

    EventLoop(int wakeupInterval = 10, int defaultSetsize = 64);

    ~EventLoop();

    void loop();

    void stop();

    void createFileEvent(int fd, int mask, OnFileEvent onFileEvent, void *userData);

    void deleteFileEvent(int fd, int mask);

    // must called by owned thread (becaused event loop blocked)
    long long createTimer(long long delay, OnTimerEvent onTimerEvent, void *userData);

    bool deleteTimer(long long timerid);

    bool isInLoopThread() const {
        return m_threadId == CurrentThread::tid();
    }
    
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abort("notInLoopThread");
        }
    }
    
    void runInLoop(const TaskFunction& cb);
    
    void queueInLoop(const TaskFunction& cb);
    
    size_t queueSize() const;
    
    void abort(std::string reason)
    {
        printf("EventLoop abort. %s. m_threadId %d curThreadId %d", reason.c_str(), m_threadId, CurrentThread::tid());
    }
    
    struct FDData
    {
        FDData() : onFileEvent(nullptr),
                   userDataRead(nullptr),
                   userDataWrite(nullptr)
        {
        }
        OnFileEvent onFileEvent;
        void *userDataRead;
        void *userDataWrite;
    };

    struct TimerData
    {
        TimerData() : onTimerEvent(nullptr),
                      userData(nullptr)
        {
        }
        OnTimerEvent onTimerEvent;
        void *userData;
    };

  private:
    void processTaskQueue();
    
    
    const pid_t m_threadId;
    aeEventLoop *aeloop;
    std::map<int, std::shared_ptr<FDData>> fdData;
    std::map<long long, std::shared_ptr<TimerData>> timerData;
    const int m_wakeupInterval;
    bool m_doingTask;
    mutable MutexLock m_mutex;
    std::vector<TaskFunction> m_taskFuncQueue;
    
    friend void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
    friend int aeOnTimerEvent(struct aeEventLoop *eventLoop, long long timerid, void *clientData);

};
};

#endif
