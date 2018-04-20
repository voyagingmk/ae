#include "wybuffer.h"

using namespace wynet;

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