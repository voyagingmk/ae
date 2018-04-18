#ifndef WY_THREAD_H
#define WY_THREAD_H

#include "common.h"
#include "noncopyable.h"
#include "count_down_latch.h"

namespace wynet
{

class Thread : Noncopyable
{
public:
  typedef std::function<void()> ThreadMain;

  explicit Thread(const ThreadMain &, const std::string &name = std::string(""));

  explicit Thread(ThreadMain &&, const std::string &name = std::string(""));

  ~Thread();

  void start();

  int join();

  bool isStarted() const { return m_started; }

  pid_t tid() const { return m_tid; }

  const std::string &name() const { return m_name; }

  static int numCreated() { return m_numCreated; }

private:
  void setDefaultName(int num);

  bool m_started;
  bool m_joined;
  pthread_t m_pthreadId;
  pid_t m_tid;
  ThreadMain m_func;
  std::string m_name;
  CountDownLatch m_latch;

  static std::atomic<int32_t> m_numCreated; // increase only
};
};

#endif
