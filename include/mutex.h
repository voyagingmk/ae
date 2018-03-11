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
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
#ifdef DEBUG_MODE
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
#endif
        pthread_mutex_init(&m_mutex, &attr);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    pthread_mutex_t *getMutexPointer()
    {
        return &m_mutex;
    }

  protected:
    friend class Condition;
    pthread_mutex_t m_mutex;
};

class MutexLock2 : public MutexLock
{
    MutexLock2() : m_tid(0)
    {
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

    void resetTid()
    {
        m_tid = 0;
    }

    void setTid()
    {
        pthread_threadid_np(NULL, &m_tid);
    }

    uint64_t m_tid;
};

template <typename ML>
class MutexLockGuard : Noncopyable
{
  public:
    explicit MutexLockGuard(ML &mutex) : m_mutex(mutex)
    {
        m_mutex.lock();
    }

    ~MutexLockGuard()
    {
        m_mutex.unlock();
    }

  private:
    ML &m_mutex;
};

#define MutexLockGuard(x) static_assert(false, "MutexLockGuard error use")
}

#endif
