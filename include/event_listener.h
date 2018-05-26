#ifndef WY_EVENT_LISTENER_H
#define WY_EVENT_LISTENER_H

#include "common.h"
#include "timer_ref.h"

namespace wynet
{

class TimerRef;
class EventLoop;
class EventListener;

typedef std::shared_ptr<EventListener> PtrEvtListener;
typedef std::weak_ptr<EventListener> WeakPtrEvtListener;
typedef std::function<void(EventLoop *, PtrEvtListener listener, int mask)> OnFileEvent;
typedef std::function<int(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)> OnTimerEvent;

// 用来发起事件监听
// 触发~EventListener()时，从eventloop删除相关的事件，保证不会有无用的事件监听
// 必须要用shared_ptr
// Note: 如果创建了多个指向同个sockfd的EventListener，会有问题
// 可以被继承，就能够绑定更多的参数
class EventListener : public std::enable_shared_from_this<EventListener>
{

  public:
    EventListener() : m_loop(nullptr),
                      m_sockfd(0)
    {
    }

    virtual ~EventListener();

  public:
    // 可重载
    static PtrEvtListener create()
    {
        return std::make_shared<EventListener>();
    }

    void setSockfd(SockFd sockfd)
    {
        m_sockfd = sockfd;
    }

    void setEventLoop(EventLoop *loop)
    {
        m_loop = loop;
    }

    SockFd getSockFd()
    {
        return m_sockfd;
    }

    EventLoop *getEventLoop()
    {
        return m_loop;
    }

    void createFileEvent(int mask, OnFileEvent onFileEvent);

    void deleteFileEvent(int mask);

    TimerRef createTimer(int ms, OnTimerEvent onTimerEvent, void *data);

  public:
    EventLoop *m_loop;
    SockFd m_sockfd;
    OnFileEvent m_onFileEvent;
};
}; // namespace wynet

#endif