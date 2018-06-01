#include "connection_manager.h"

namespace wynet
{

ConnectionManager::ConnectionManager()
{
    log_ctor("ConnectionManager()");
    m_convIdGen.setRecycleThreshold(2 << 15);
    m_convIdGen.setRecycleEnabled(true);
}
ConnectionManager::~ConnectionManager()
{
    log_dtor("~ConnectionManager() %d", m_connDict.size());
    for (auto it : m_connDict)
    {
        const PtrConn &conn = it.second;
        conn->forceClose();
    }
    m_connDict.clear();
}

bool ConnectionManager::addConnection(const PtrConn &conn)
{
    refConnection(conn);
    return true;
}

bool ConnectionManager::removeConnection(const PtrConn &conn)
{
    return unrefConnection(conn);
}

bool ConnectionManager::removeConnection(UniqID connectId)
{
    return unrefConnection(connectId);
}

PtrConn ConnectionManager::getConncetion(UniqID connectId)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    auto it = m_connDict.find(connectId);
    if (it == m_connDict.end())
    {
        return PtrConn();
    }
    return it->second;
}

UniqID ConnectionManager::refConnection(const PtrConn &conn)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    UniqID connectId = m_connectIdGen.getNewID();
    UniqID convId = m_convIdGen.getNewID();
    m_connDict[connectId] = conn;
    conn->setConnectId(connectId);
    uint16_t password = random();
    conn->setKey((password << 16) | convId);
    log_debug("<connMgr> ref %d", connectId);
    return connectId;
}

bool ConnectionManager::unrefConnection(const PtrConn &conn)
{
    UniqID connectId = conn->connectId();
    return unrefConnection(connectId);
}

bool ConnectionManager::unrefConnection(UniqID connectId)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    if (m_connDict.find(connectId) == m_connDict.end())
    {
        return false;
    }
    log_debug("<connMgr> unref %d", connectId);
    m_connDict.erase(connectId);
    return true;
}

}; // namespace wynet