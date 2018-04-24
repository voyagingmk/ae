#ifndef WY_SINGLETON_H
#define WY_SINGLETON_H

#include "common.h"
#include "noncopyable.h"
#include "mutex.h"

namespace wynet
{
template <typename Derived>
class Singleton : public Noncopyable
{
  public:
    static std::atomic<Derived *> s_instance;
    static MutexLock s_mutex;

    static Derived *getSingleton()
    {
        Derived *tmp = Singleton::s_instance.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr)
        {
            MutexLockGuard<MutexLock> lock(s_mutex);
            tmp = s_instance.load(std::memory_order_relaxed);
            if (tmp == nullptr)
            {
                tmp = new Derived;
                std::atomic_thread_fence(std::memory_order_release);
                s_instance.store(tmp, std::memory_order_relaxed);
            }
        }
        return tmp;
    }
};
};

#endif
