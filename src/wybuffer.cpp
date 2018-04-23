#include "wybuffer.h"

using namespace wynet;

void DynamicBuffer::expand(size_t n)
{
    if (n > MaxBufferSize)
    {
        log_fatal("[DynamicBuffer] wrong n: %d", n);
    }
    return m_data.resize(n);
}

UniqID BufferSet::newBuffer()
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    UniqID uid = m_uniqIDGen.getNewID();
    if (uid > m_buffers.size())
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
    if (idx < 0 || idx >= m_buffers.size())
    {
        return nullptr;
    }
    return m_buffers[idx];
}

std::shared_ptr<DynamicBuffer> BufferSet::getBufferByIdx(int32_t idx)
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

BufferRef::BufferRef()
{
    m_uniqID = BufferSet::getSingleton()->newBuffer();
    log_debug("BufferRef created %d", m_uniqID);
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
    if (!m_uniqID)
    {
        return nullptr;
    }
    if (!m_cachedPtr)
    {
        m_cachedPtr = BufferSet::getSingleton()->getBuffer(m_uniqID);
        log_debug("BufferRef cache %d", m_uniqID);
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