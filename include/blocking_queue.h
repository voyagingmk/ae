#ifndef WY_BLOCKINGQUEUE_H
#define WY_BLOCKINGQUEUE_H

#include <deque>
#include <assert.h>
#include "mutex.h"
#include "condition.h"

namespace wynet
{

template <typename T>
class BlockingQueue : Noncopyable
{
  public:
    BlockingQueue()
        : m_mutex(),
          m_notEmpty(m_mutex),
          m_queue()
    {
    }

    size_t size() const
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        return m_queue.size();
    }

    void put(const T &x)
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        m_queue.push_back(x);
        m_notEmpty.notify(); // wait morphing
    }

    void put(T &&x)
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        m_queue.push_back(std::move(x));
        m_notEmpty.notify();
    }

    T pop()
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        while (m_queue.empty())
        {
            m_notEmpty.wait();
        }
        assert(!m_queue.empty());
        T front(std::move(m_queue.front()));
        m_queue.pop_front();
        return front;
    }

  private:
    mutable MutexLock m_mutex;
    Condition m_notEmpty;
    std::deque<T> m_queue;
};
}

#endif