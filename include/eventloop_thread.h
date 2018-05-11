#ifndef WY_EVENTLOOP_THREAD_H
#define WY_EVENTLOOP_THREAD_H

#include "common.h"
#include "noncopyable.h"
#include "thread.h"

namespace wynet
{
class EventLoop;
typedef std::function<void(EventLoop *)> ThreadInitCallback;

// 将一个thread和一个loop联合起来的对象
// 每个EventLoopThread拥有一个mutex
class EventLoopThread : Noncopyable
{
public:
  EventLoopThread(const ThreadInitCallback &cb = {},
                  const std::string &name = "");

  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadEntry();

private:
  EventLoop *m_loop;
  bool m_exiting;
  Thread m_thread;
  MutexLock m_mutex;
  Condition m_cond;
  ThreadInitCallback m_callback;
};
};

#endif