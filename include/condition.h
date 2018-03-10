#ifndef WY_CONDITION_H
#define WY_CONDITION_H

#include <pthread.h>
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

    bool waitForSeconds(double seconds);

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
