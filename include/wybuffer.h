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

};

template<size_t BUF_SIZE>
class StaticBuffer: public BufferBase
{
  public:
    uint8_t m_data[BUF_SIZE];
    size_t m_size;


};


class DynamicBuffer: public BufferBase
{
  public:
    uint8_t *m_data;
    size_t m_size;

  public:
    DynamicBuffer(size_t s = 0xff)
    {
        m_size = s;
        m_data = new uint8_t[m_size];
    }
    ~DynamicBuffer()
    {
        m_size = 0;
        delete[] m_data;
    }

    void clean() { 
        bzero(m_data, m_size); 
    }

    // keep old m_data
    uint8_t *expand(size_t n)
    {
        return reserve(n, true);
    }

    uint8_t *reserve(size_t n, bool keep = false)
    {
        if (n > MaxBufferSize)
        {
            return nullptr;
        }
        if (!m_data || n > m_size)
        {
            size_t oldSize = m_size;
            while (n > m_size)
            {
                m_size = m_size << 1;
            }
            uint8_t *newBuffer = new uint8_t[m_size]{0};
            if (keep)
            {
                memcpy(newBuffer, m_data, oldSize);
            }
            delete[] m_data;
            m_data = newBuffer;
        }
        return m_data;
    }
};

class BufferSet : public Noncopyable
{
    std::vector<std::shared_ptr<DynamicBuffer>> buffers;
    UniqIDGenerator uniqIDGen;
    MutexLock m_mutex;
    Condition m_cond;

  public:
    static BufferSet &dynamicSingleton()
    {
        static BufferSet gBufferSet;
        return gBufferSet;
    }

    static BufferSet &constSingleton()
    {
        static BufferSet gBufferSet;
        return gBufferSet;
    }

    BufferSet() : m_mutex(),
                  m_cond(m_mutex)
    {
    }

    UniqID newBuffer()
    {
        UniqID uid = uniqIDGen.getNewID();
        if (uid > buffers.size())
        {
            buffers.push_back(std::make_shared<DynamicBuffer>());
        }
        return uid;
    }

    void recycleBuffer(UniqID uid)
    {
        uniqIDGen.recycleID(uid);
    }

    std::shared_ptr<DynamicBuffer> getBuffer(UniqID uid)
    {
        int32_t idx = uid - 1;
        if (idx < 0 || idx >= buffers.size())
        {
            return nullptr;
        }
        return buffers[idx];
    }

    std::shared_ptr<DynamicBuffer> getBufferByIdx(int32_t idx)
    {
        if (idx < 0)
        {
            return nullptr;
        }
        while ((idx + 1) > buffers.size())
        {
            buffers.push_back(std::make_shared<DynamicBuffer>());
        }
        return buffers[idx];
    }
};

class BufferRef : public Noncopyable
{
    UniqID uniqID;

  public:
    BufferRef()
    {
        uniqID = BufferSet::dynamicSingleton().newBuffer();
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
        // log_debug("BufferRef get %d", uniqID);
        return BufferSet::dynamicSingleton().getBuffer(uniqID);
    }

  private:
    void recycleBuffer()
    {
        if (uniqID)
        {
            BufferSet::dynamicSingleton().recycleBuffer(uniqID);
            log_debug("BufferRef recycled %d", uniqID);
            uniqID = 0;
        }
    }
};
};

#endif
