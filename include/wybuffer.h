#ifndef WY_BUFFER_H
#define WY_BUFFER_H

#include "common.h"
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
    void expand(size_t n)
    {
        if (n > MaxBufferSize)
        {
            log_fatal("[DynamicBuffer] wrong n: %d\n", n);
        }
        return m_data.resize(n);
    }
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
    BufferSet()
    {
    }

    UniqID newBuffer()
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        UniqID uid = m_uniqIDGen.getNewID();
        if (uid > m_buffers.size())
        {
            m_buffers.push_back(std::make_shared<DynamicBuffer>());
        }
        return uid;
    }

    void recycleBuffer(UniqID uid)
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        m_uniqIDGen.recycleID(uid);
    }

    std::shared_ptr<DynamicBuffer> getBuffer(UniqID uid)
    {
        int32_t idx = uid - 1;
        MutexLockGuard<MutexLock> lock(m_mutex);
        if (idx < 0 || idx >= m_buffers.size())
        {
            return nullptr;
        }
        return m_buffers[idx];
    }

    std::shared_ptr<DynamicBuffer> getBufferByIdx(int32_t idx)
    {
        if (idx < 0)
        {
            return nullptr;
        }
        MutexLockGuard<MutexLock> lock(m_mutex);
        while ((idx + 1) > m_buffers.size())
        {
            m_buffers.push_back(std::make_shared<DynamicBuffer>());
        }
        return m_buffers[idx];
    }
};

class BufferRef : public Noncopyable
{
    UniqID uniqID;
    std::shared_ptr<DynamicBuffer> m_cachedPtr;

  public:
    BufferRef()
    {
        uniqID = BufferSet::getSingleton()->newBuffer();
        if (uniqID)
        {
            log_debug("BufferRef created %d", uniqID);
        }
    }

    ~BufferRef()
    {
        recycleBuffer();
    }

    BufferRef(BufferRef &&b)
    {
        recycleBuffer();
        uniqID = b.uniqID;
        b.uniqID = 0;
        log_debug("BufferRef moved %d", uniqID);
    }

    BufferRef &operator=(BufferRef &&b)
    {
        recycleBuffer();
        uniqID = b.uniqID;
        b.uniqID = 0;
        log_debug("BufferRef moved %d", uniqID);
        return (*this);
    }

    inline std::shared_ptr<DynamicBuffer> operator->()
    {
        return get();
    }

    std::shared_ptr<DynamicBuffer> get()
    {
        if (!uniqID)
        {
            return nullptr;
        }
        if (!m_cachedPtr)
        {
            m_cachedPtr = BufferSet::getSingleton()->getBuffer(uniqID);
        }
        // log_debug("BufferRef get %d", uniqID);
        return m_cachedPtr;
    }

  private:
    void recycleBuffer()
    {
        if (uniqID)
        {
            BufferSet::getSingleton()->recycleBuffer(uniqID);
            log_debug("BufferRef recycled %d", uniqID);
            uniqID = 0;
        }
    }
};
};

#endif
