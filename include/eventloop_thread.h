#ifndef WY_EVENTLOOP_THREAD_H
#define WY_EVENTLOOP_THREAD_H

#include "noncopyable.h"
#include "common.h"
#include "wythread.h"

namespace wynet
{
class EventLoop;
typedef std::function<void(EventLoop *)> ThreadInitCallback;

class EventLoopThread : Noncopyable
{
public:
  EventLoopThread(const ThreadInitCallback &cb = {},
                  const std::string &name = std::string());

  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadMain();

  EventLoop *m_loop;
  bool m_exiting;
  Thread m_thread;
  MutexLock m_mutex;
  Condition m_cond;
  ThreadInitCallback m_callback;
};
};

#endif