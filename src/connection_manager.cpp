#include "connection_manager.h"

namespace wynet
{
void ConnectionManager::weakDeleteCallback(std::weak_ptr<ConnectionManager> wkConnMgr, TcpConnection *rawConn)
{
    log_info("<connMgr> weakDeleteCallback");
    std::shared_ptr<ConnectionManager> connMgr(wkConnMgr.lock());
    if (connMgr)
    {
        UniqID connectId = rawConn->connectId();
        connMgr->unrefConnection(connectId);
    }
    delete rawConn;
}

ConnectionManager::ConnectionManager()
{
    m_convIdGen.setRecycleThreshold(2 << 15);
    m_convIdGen.setRecycleEnabled(true);
}

PtrConn ConnectionManager::newConnection()
{
    using namespace std::placeholders;
    PtrConn conn(new TcpConnection(), std::bind(ConnectionManager::weakDeleteCallback,
                                                std::weak_ptr<ConnectionManager>(shared_from_this()), _1));
    refConnection(conn);
    return conn;
}

PtrConn ConnectionManager::getConncetion(UniqID connectId)
{
    auto it = m_connDict.find(connectId);
    if (it == m_connDict.end())
    {
        return PtrConn();
    }
    PtrConn conn(it->second.lock());
    if (!conn)
    {
        log_warn("<connMgr> getConncetion failed.");
    }
    return conn;
}

UniqID ConnectionManager::refConnection(PtrConn conn)
{
    UniqID connectId = m_connectIdGen.getNewID();
    UniqID convId = m_convIdGen.getNewID();
    m_connDict[connectId] = conn;
    conn->setConnectId(connectId);
    uint16_t password = random();
    conn->setKey((password << 16) | convId);
    log_info("<connMgr> ref %d", connectId);
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
    log_info("<connMgr> unref %d", connectId);
    m_connDict.erase(connectId);
    return true;
}

void ConnectionManager::onTcpDisconnected(PtrConn conn)
{
    unrefConnection(conn);
}

}; // namespace wynet