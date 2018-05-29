#include "net.h"

namespace wynet
{

PeerManager::~PeerManager()
{
    while (!m_servers.empty())
    {
        UniqID serverId = m_servers.begin()->first;
        removeServer(serverId);
    }
    while (!m_clients.empty())
    {
        UniqID clientId = m_clients.begin()->first;
        removeClient(clientId);
    }
    log_debug("PeerManager destroyed.");
}

UniqID PeerManager::addServer(PtrServer server)
{
    UniqID serverId = m_serverIdGen.getNewID();
    m_servers[serverId] = server;
    return serverId;
}

bool PeerManager::removeServer(UniqID serverId)
{
    Servers::iterator it = m_servers.find(serverId);
    if (it == m_servers.end())
    {
        return false;
    }
    m_servers.erase(it);
    m_serverIdGen.recycleID(serverId);
    return true;
}

UniqID PeerManager::addClient(PtrClient c)
{
    UniqID clientId = m_clientIdGen.getNewID();
    m_clients[clientId] = c;
    return clientId;
}

bool PeerManager::removeClient(UniqID clientId)
{
    Clients::iterator it = m_clients.find(clientId);
    if (it == m_clients.end())
    {
        return false;
    }
    m_clients.erase(it);
    m_serverIdGen.recycleID(clientId);
    return true;
}

WyNet::WyNet(int threadNum)
{
    m_threadPool = std::make_shared<EventLoopThreadPool>(&m_loop, "WyNet");
    m_threadPool->setThreadNum(threadNum);
    m_threadPool->start([](EventLoop *loop) -> void {
        log_debug("ThreadInitCallback");
    });
}

void WyNet::stopLoop()
{
    m_loop.stop();
}

void WyNet::startLoop()
{
    m_loop.loop();
}
}; // namespace wynet
