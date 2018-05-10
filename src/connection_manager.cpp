#include "connection_manager.h"

namespace wynet
{

ConnectionManager::ConnectionManager()
{
    m_convIdGen.setRecycleThreshold(2 << 15);
    m_convIdGen.setRecycleEnabled(true);
}

UniqID ConnectionManager::refConnection(PtrConn conn)
{
    UniqID connectId = m_connectIdGen.getNewID();
    UniqID convId = m_convIdGen.getNewID();
    m_connDict[connectId] = conn;
    conn->setConnectId(connectId);
    uint16_t password = random();
    conn->setKey((password << 16) | convId);
    m_connfd2cid[conn->connectFd()] = connectId;
    m_convId2cid[convId] = connectId;
    return connectId;
}

bool ConnectionManager::unrefConnection(PtrConn conn)
{
    UniqID connectId = conn->connectId();
    return unrefConnection(connectId);
}

bool ConnectionManager::unrefConnection(UniqID connectId)
{
    if (m_connDict.find(connectId) == m_connDict.end())
    {
        return false;
    }
    PtrConn conn = m_connDict[connectId];
    m_connfd2cid.erase(conn->connectFd());
    m_convId2cid.erase(conn->convId());
    m_connDict.erase(connectId);
    return true;
}

void ConnectionManager::onTcpDisconnected(int connfdTcp)
{
    if (m_connfd2cid.find(connfdTcp) != m_connfd2cid.end())
    {
        UniqID connectId = m_connfd2cid[connfdTcp];
        unrefConnection(connectId);
        log_info("[connMgr]closed, connectId: %d connfdTcp: %d", connectId, connfdTcp);
    }
}

}; // namespace wynet