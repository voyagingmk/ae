#ifndef WY_BUFFER_H
#define WY_BUFFER_H

#include "common.h"
#include "logger/log.h"
#include "uniqid.h"
#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"

namespace wynet
{

const size_t MaxBufferSize = 1024 * 1024 * 256; // 256 MB

class BufferBase : public Noncopyable
{
  protected:
    BufferBase() {}

  public:
    void clean() {}

    uint8_t *data()
    {
        return nullptr;
    }

    const uint8_t *data() const
    {
        return nullptr;
    }
    // buffer capacity
    size_t length() const { return 0; }

    void expand(size_t n) {}
};

template <int BUF_SIZE>
class StaticBuffer : public BufferBase
{
  public:
    uint8_t m_data[BUF_SIZE];

  public:
    void clean()
    {
        bzero(m_data, BUF_SIZE);
    }

    uint8_t *data()
    {
        return m_data;
    }

    const uint8_t *data() const
    {
        return m_data;
    }

    size_t length() const
    {
        return BUF_SIZE;
    }
};

class DynamicBuffer : public BufferBase
{
  public:
    std::vector<uint8_t> m_data;

  public:
    DynamicBuffer(size_t s = 0xff) : m_data(s)
    {
    }

    ~DynamicBuffer()
    {
    }

    void clean()
    {
        std::fill(m_data.begin(), m_data.end(), 0);
    }

    uint8_t *data()
    {
        return m_data.data();
    }

    const uint8_t *data() const
    {
        return m_data.data();
    }

    size_t length() const
    {
        return m_data.size();
    }

    // will keep old m_data
    void expand(size_t n);
};

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

template <typename Derived>
std::atomic<Derived *> Singleton<Derived>::s_instance;

template <typename Derived>
MutexLock Singleton<Derived>::s_mutex;

class BufferSet;

// thread-safe

class BufferSet : public Singleton<BufferSet>
{
    MutexLock m_mutex;
    std::vector<std::shared_ptr<DynamicBuffer>> m_buffers;
    UniqIDGenerator m_uniqIDGen;

  public:
    UniqID newBuffer();

    size_t getSize();

    void recycleBuffer(UniqID uid);

    std::shared_ptr<DynamicBuffer> getBuffer(UniqID uid);

    std::shared_ptr<DynamicBuffer> getBufferByIdx(int32_t idx);
};

class BufferRef : public Noncopyable
{
    UniqID m_uniqID;
    std::shared_ptr<DynamicBuffer> m_cachedPtr; // lock only once

  public:
    BufferRef();

    ~BufferRef();

    BufferRef(BufferRef &&b);

    BufferRef &operator=(BufferRef &&b);

    inline std::shared_ptr<DynamicBuffer> operator->()
    {
        return get();
    }

  private:
    std::shared_ptr<DynamicBuffer> get();

    void recycleBuffer();
};
};

#endif
