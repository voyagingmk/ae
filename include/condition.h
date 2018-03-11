#ifndef WY_CONDITION_H
#define WY_CONDITION_H

#include <pthread.h>
#include <errno.h>
#include "noncopyable.h"
#include "mutex.h"

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
        pthread_cond_wait(&m_cond, m_mutex.getMutexPointer());
    }

    bool waitForSeconds(double seconds)
    {
        struct timespec abstime;
        // FIXME: use CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW to prevent time rewind.
        clock_gettime(CLOCK_REALTIME, &abstime);

        const int64_t kNanoSecondsPerSecond = 1000000000;
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

        abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
        abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

        MutexLockGuard<MutexLock> ug(m_mutex);
        return ETIMEDOUT == pthread_cond_timedwait(&m_cond, m_mutex.getMutexPointer(), &abstime);
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
