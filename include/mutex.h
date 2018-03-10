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
      : m_pid(0)
  {
    pthread_mutex_init(&m_mutex, NULL);
  }

  ~MutexLock()
  {
    pthread_mutex_destroy(&m_mutex);
  }

  bool isLockedByThisThread() const
  {
    return m_pid == CurrentThread::tid();
  }

  void lock()
  {
    pthread_mutex_lock(&m_mutex);
    setPid();
  }

  void unlock()
  {
    resetPid();
    pthread_mutex_unlock(&m_mutex);
  }

  pthread_mutex_t *getPthreadMutex()
  {
    return &m_mutex;
  }

private:
  friend class Condition;

  void resetPid()
  {
    m_pid = 0;
  }

  void setPid()
  {
    m_pid = CurrentThread::tid();
  }

  pthread_mutex_t m_mutex;
  pid_t m_pid;
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
}

#endif
