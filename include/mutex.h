#ifndef WY_MUTEX_H
#define WY_MUTEX_H

#include "noncopyable.h"
#include <assert.h>
#include <pthread.h>

namespace wynet
{

class MutexLock : Noncopyable
{
public:
  MutexLock()
      :m_tid(0)
  {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init (&attr);
#ifdef DEBUG_MODE
      pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
      pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_DEFAULT);
#endif
      pthread_mutex_init(&m_mutex, &attr);
  }

  ~MutexLock()
  {
    pthread_mutex_destroy(&m_mutex);
  }

  bool isLockedByThisThread() const
  {
      uint64_t tid;
      pthread_threadid_np(NULL, &tid);
      return m_tid == tid;
  }

  void lock()
  {
    pthread_mutex_lock(&m_mutex);
    setTid();
  }

  void unlock()
  {
    resetTid();
    pthread_mutex_unlock(&m_mutex);
  }

  pthread_mutex_t *getPthreadMutex()
  {
    return &m_mutex;
  }

private:
  friend class Condition;

  void resetTid()
  {
    m_tid = 0;
  }

  void setTid()
  {
      pthread_threadid_np(NULL, &m_tid);
  }

  pthread_mutex_t m_mutex;
  uint64_t m_tid;
};

class MutexLockGuard : Noncopyable
{
public:
    explicit MutexLockGuard(MutexLock &mutex):
        m_mutex(mutex)
    {
        m_mutex.lock();
    }

  ~MutexLockGuard()
  {
    m_mutex.unlock();
  }

private:
  MutexLock &m_mutex;
};
  
#define MutexLockGuard(x) static_assert(false, "[MutexLockGuard] error use")

}

#endif
