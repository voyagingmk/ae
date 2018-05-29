#ifndef WY_EVENTLOOP_H
#define WY_EVENTLOOP_H

#include "common.h"
#include "noncopyable.h"
#include "thread.h"
#include "sockbase.h"
#include "timer_ref.h"
#include "event_listener.h"

namespace wynet
{

#define LOOP_EVT_NONE AE_NONE
#define LOOP_EVT_READABLE AE_READABLE
#define LOOP_EVT_WRITABLE AE_WRITABLE
#define LOOP_EVT_NOMORE AE_NOMORE

typedef long long AeTimerId;

class EventLoop : Noncopyable
{
  public:
    typedef std::function<void()> TaskFunction;

    EventLoop(int wakeupInterval = 10, int defaultSetsize = 64);

    ~EventLoop();

    void loop();

    void stop();

    TimerRef createTimer(PtrEvtListener listener, int ms, OnTimerEvent onTimerEvent, void *data);

    void deleteTimer(TimerRef tr);

    bool isInLoopThread() const
    {
        return m_threadId == CurrentThread::tid();
    }

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abort("notInLoopThread");
        }
    }

    void runInLoop(const TaskFunction &cb);

    void queueInLoop(const TaskFunction &cb);

    size_t queueSize() const;

    static EventLoop *getCurrentThreadLoop();

    void abort(std::string reason)
    {
        log_fatal("EventLoop abort. %s. m_threadId %d curThreadId %d", reason.c_str(), m_threadId, CurrentThread::tid());
    }

    struct TimerData
    {
        TimerData() : m_onTimerEvent(nullptr),
                      m_data(nullptr),
                      m_aeTimerId(0)
        {
        }
        TimerData(OnTimerEvent _evt,
                  PtrEvtListener listener,
                  void *_data,
                  AeTimerId _aeTimerId) : m_onTimerEvent(_evt),
                                          m_listener(listener),
                                          m_data(_data),
                                          m_aeTimerId(_aeTimerId)
        {
        }
        OnTimerEvent m_onTimerEvent;
        WeakPtrEvtListener m_listener;
        void *m_data;
        AeTimerId m_aeTimerId;
    };

  private:
    void processTaskQueue();

    bool hasFileEvent(PtrEvtListener, int mask);

    void createFileEvent(PtrEvtListener, int mask);

    void deleteFileEvent(PtrEvtListener, int mask);

    void deleteFileEvent(SockFd fd, int mask);

    void deleteAllFileEvent(SockFd fd);

    AeTimerId createTimerInLoop(PtrEvtListener listener, int delay, OnTimerEvent onTimerEvent, void *data);

    AeTimerId createTimerInLoop(PtrEvtListener listener, TimerRef tr, int delay, OnTimerEvent onTimerEvent, void *data);

    bool deleteTimerInLoop(TimerRef tr);

    bool deleteTimerInLoop(AeTimerId aeTimerId);

    friend void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

    friend int aeOnTimerEvent(struct aeEventLoop *eventLoop, AeTimerId aeTimerId, void *clientData);

    friend class EventListener;

  private:
    const pid_t m_threadId;
    aeEventLoop *m_aeloop;
    PtrEvtListener m_ownEvtListener;
    std::map<SockFd, WeakPtrEvtListener> m_fd2listener;
    std::map<TimerRef, TimerData> m_timerData;
    std::map<AeTimerId, TimerRef> m_aeTimerId2ref;
    const int m_wakeupInterval;
    bool m_doingTask;
    mutable MutexLock m_mutex;
    std::vector<TaskFunction> m_taskFuncQueue;
};
}; // namespace wynet

#endif
