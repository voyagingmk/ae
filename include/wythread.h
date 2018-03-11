#ifndef WY_THREAD_H
#define WY_THREAD_H

#include <atomic>
#include <functional>
#include <string>
#include <stdint.h>
#include <pthread.h>
#include "noncopyable.h"
#include "count_down_latch.h"

namespace wynet
{

namespace CurrentThread
{
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  
  pid_t gettid();
    
  void cacheTid();

  inline int tid()
  {
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString()
  {
    return t_tidString;
  }

  inline int tidStringLength()
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

  void sleepUsec(int64_t usec);
};


class Thread : Noncopyable
{
  public:
    typedef std::function<void()> ThreadFunc;

    explicit Thread(const ThreadFunc &, const std::string &name = std::string(""));

    explicit Thread(ThreadFunc &&, const std::string &name = std::string(""));

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
    ThreadFunc m_func;
    std::string m_name;
    CountDownLatch m_latch;

    static std::atomic<int32_t> m_numCreated; // increase only
};
}
#endif
