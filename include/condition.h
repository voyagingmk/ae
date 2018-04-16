#ifndef WY_CONDITION_H
#define WY_CONDITION_H

#include "common.h"
#include "noncopyable.h"
#include "mutex.h"
#include "clock_time.h"

namespace wynet
{

class Condition : Noncopyable
{
  public:
    explicit Condition(MutexLock &mutex)
        : m_mutex(mutex)
    {
        pthread_cond_init(&m_cond, NULL);
    }

    ~Condition()
    {
        pthread_cond_destroy(&m_cond);
    }

    void wait()
    {
        MutexLockGuard<MutexLock> g(m_mutex);
        pthread_cond_wait(&m_cond, m_mutex.getRawPointer());
    }

    bool waitForSeconds(double seconds)
    {
        ClockTime nowtime(ClockTime::getNowTime());
        ClockTime duration(seconds);
        nowtime += duration;
        MutexLockGuard<MutexLock> g(m_mutex);
        return ETIMEDOUT == pthread_cond_timedwait(&m_cond, m_mutex.getRawPointer(), &nowtime.ts);
    }

    void notify()
    {
        pthread_cond_signal(&m_cond);
    }

    void notifyAll()
    {
        pthread_cond_broadcast(&m_cond);
    }

  private:
    MutexLock &m_mutex;
    pthread_cond_t m_cond;
};
}

#endif
