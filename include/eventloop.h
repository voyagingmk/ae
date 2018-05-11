#ifndef WY_EVENTLOOP_H
#define WY_EVENTLOOP_H

#include "noncopyable.h"
#include "common.h"
#include "thread.h"
#include "sockbase.h"
#include <functional>

namespace wynet
{

#define LOOP_EVT_NONE AE_NONE
#define LOOP_EVT_READABLE AE_READABLE
#define LOOP_EVT_WRITABLE AE_WRITABLE
#define LOOP_EVT_NOMORE AE_NOMORE

typedef long long WyTimerId;

typedef long long AeTimerId;

class TimerRef
{
    WyTimerId m_id;

  public:
    explicit TimerRef(const WyTimerId _id = 0) : m_id(_id)
    {
    }

    TimerRef(const TimerRef &tr) : m_id(tr.m_id)
    {
    }

    TimerRef &operator=(const TimerRef &tr)
    {
        m_id = tr.m_id;
        return *this;
    }

    TimerRef(TimerRef &&tr) : m_id(tr.m_id)
    {
    }

    TimerRef &operator=(TimerRef &&tr)
    {
        m_id = tr.m_id;
        return *this;
    }

    WyTimerId Id() const
    {
        return m_id;
    }

    bool validate() const
    {
        return m_id > 0;
    }

    bool operator<(const TimerRef &tr) const
    {
        return m_id < tr.m_id;
    }

    static TimerRef newTimerRef()
    {
        WyTimerId _id = ++g_numCreated;
        return TimerRef(_id);
    }
    static std::atomic<WyTimerId> g_numCreated;
};

class EventLoop : Noncopyable
{
  public:
    typedef std::function<void()> TaskFunction;
    typedef void (*OnFileEvent)(EventLoop *, std::weak_ptr<FDRef> fdRef, int mask);
    typedef int (*OnTimerEvent)(EventLoop *, TimerRef tr, std::weak_ptr<FDRef> fdRef, void *data);

    EventLoop(int wakeupInterval = 10, int defaultSetsize = 64);

    ~EventLoop();

    void loop();

    void stop();

    void createFileEvent(std::shared_ptr<FDRef> fdRef, int mask, OnFileEvent onFileEvent);

    void deleteFileEvent(int fd, int mask);

    void deleteFileEvent(std::shared_ptr<FDRef> fdRef, int mask);

    TimerRef createTimer(int ms, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data);

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

    class FDData
    {
      public:
        FDData() {}
        FDData(OnFileEvent _evt, std::weak_ptr<FDRef> _fdRef) : onFileEvent(_evt),
                                                                fdRef(_fdRef)
        {
        }
        OnFileEvent onFileEvent;
        std::weak_ptr<FDRef> fdRef;
    };

    struct TimerData
    {
        TimerData() : onTimerEvent(nullptr),
                      data(nullptr),
                      timerId(0)
        {
        }
        TimerData(OnTimerEvent _evt,
                  std::weak_ptr<FDRef> _fdRef,
                  void *_data,
                  WyTimerId _timerid) : onTimerEvent(_evt),
                                        fdRef(_fdRef),
                                        data(_data),
                                        timerId(_timerid)
        {
        }
        OnTimerEvent onTimerEvent;
        std::weak_ptr<FDRef> fdRef;
        void *data;
        WyTimerId timerId;
    };

  private:
    void processTaskQueue();

    AeTimerId createTimerInLoop(int delay, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data);

    AeTimerId createTimerInLoop(TimerRef tr, int delay, OnTimerEvent onTimerEvent, std::weak_ptr<FDRef> fdRef, void *data);

    bool deleteTimerInLoop(TimerRef tr);

    bool deleteTimerInLoop(AeTimerId aeTimerId);

    friend void aeOnFileEvent(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
    friend int aeOnTimerEvent(struct aeEventLoop *eventLoop, WyTimerId timerId, void *clientData);

  private:
    const pid_t m_threadId;
    aeEventLoop *m_aeloop;
    std::map<int, FDData> m_fdData;
    std::map<TimerRef, TimerData> m_timerData;
    std::map<AeTimerId, TimerRef> m_aeTimerId2ref;
    const int m_wakeupInterval;
    bool m_doingTask;
    mutable MutexLock m_mutex;
    std::vector<TaskFunction> m_taskFuncQueue;
};
}; // namespace wynet

#endif
