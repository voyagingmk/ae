#include "buffer.h"

using namespace wynet;

void DynamicBuffer::expand(size_t n)
{
    if (n > MaxBufferSize)
    {
        log_fatal("[DynamicBuffer] expand, wrong n: %d", n);
    }
    return m_data.resize(n);
}

UniqID BufferSet::newBuffer()
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    UniqID uid = m_uniqIDGen.getNewID();
    if (m_buffers.find(uid) == m_buffers.end())
    {
        m_buffers.insert({uid, std::make_shared<DynamicBuffer>()});
    }
    return uid;
}

size_t BufferSet::getSize()
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    return m_buffers.size();
}

void BufferSet::recycleBuffer(UniqID uid)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    m_uniqIDGen.recycleID(uid);
    if (m_uniqIDGen.getRecycledLength() > 10)
    {
        std::vector<UniqID> vec;
        m_uniqIDGen.popRecycleIDs(vec, 5);
        for (auto it = vec.begin(); it != vec.end(); it++)
        {
            UniqID uid = *it;
            auto it2 = m_buffers.find(uid);
            assert(it2 != m_buffers.end());
            m_buffers.erase(it2);
        }
    }
}

std::shared_ptr<DynamicBuffer> BufferSet::getBuffer(UniqID uid)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    if (m_buffers.find(uid) == m_buffers.end())
    {
        return std::shared_ptr<DynamicBuffer>();
    }
    return m_buffers[uid];
}

std::shared_ptr<DynamicBuffer> BufferSet::getBufferByIdx(int32_t idx)
{
    if (idx < 0)
    {
        return std::shared_ptr<DynamicBuffer>();
    }
    MutexLockGuard<MutexLock> lock(m_mutex);
    UniqID uid = idx + 1;
    if (m_buffers.find(uid) == m_buffers.end())
    {
        m_buffers.insert({uid, std::make_shared<DynamicBuffer>()});
    }
    return m_buffers[uid];
}

//////////////////////////////
//////////////////////////////
//////////////////////////////

BufferRef::BufferRef(const char *reason)
{
    m_uniqID = BufferSet::getSingleton()->newBuffer();
    m_cachedPtr = nullptr;
    log_debug("BufferRef created %d, reason: %s", m_uniqID, reason);
}

BufferRef::~BufferRef()
{
    recycleBuffer();
}

BufferRef::BufferRef(BufferRef &&b)
{
    recycleBuffer();
    m_uniqID = b.m_uniqID;
    b.m_uniqID = 0;
    b.m_cachedPtr = nullptr;
    log_debug("BufferRef moved %d", m_uniqID);
}

BufferRef &BufferRef::operator=(BufferRef &&b)
{
    recycleBuffer();
    m_uniqID = b.m_uniqID;
    b.m_uniqID = 0;
    b.m_cachedPtr = nullptr;
    log_debug("BufferRef moved %d", m_uniqID);
    return (*this);
}

std::shared_ptr<DynamicBuffer> BufferRef::get()
{
    if (m_cachedPtr)
    {
        return m_cachedPtr;
    }
    if (!m_uniqID)
    {
        log_fatal("BufferRef get, no m_uniqID");
        return nullptr;
    }
    m_cachedPtr = BufferSet::getSingleton()->getBuffer(m_uniqID);
    log_debug("BufferRef cache %d", m_uniqID);
    if (!m_cachedPtr)
    {
        log_fatal("BufferRef get, no m_cachedPtr");
    }
    return m_cachedPtr;
}

void BufferRef::recycleBuffer()
{
    if (m_uniqID)
    {
        BufferSet::getSingleton()->recycleBuffer(m_uniqID);
        log_debug("BufferRef recycled %d", m_uniqID);
        m_uniqID = 0;
    }
    m_cachedPtr = nullptr;
}