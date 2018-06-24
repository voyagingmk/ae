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
    if (static_cast<size_t>(uid) > m_buffers.size())
    {
        m_buffers.push_back(std::make_shared<DynamicBuffer>());
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
}

std::shared_ptr<DynamicBuffer> BufferSet::getBuffer(UniqID uid)
{
    int32_t idx = uid - 1;
    MutexLockGuard<MutexLock> lock(m_mutex);
    if (idx < 0 || idx >= static_cast<int32_t>(m_buffers.size()))
    {
        return std::shared_ptr<DynamicBuffer>();
    }
    return m_buffers[idx];
}

std::shared_ptr<DynamicBuffer> BufferSet::getBufferByIdx(int32_t idx)
{
    if (idx < 0)
    {
        return std::shared_ptr<DynamicBuffer>();
    }
    MutexLockGuard<MutexLock> lock(m_mutex);
    while ((idx + 1) > static_cast<int32_t>(m_buffers.size()))
    {
        m_buffers.push_back(std::make_shared<DynamicBuffer>());
    }
    return m_buffers[idx];
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