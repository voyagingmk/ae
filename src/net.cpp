#include "net.h"

namespace wynet
{

WyNet::WyNet(int threadNum)
{
    m_threadPool = std::make_shared<EventLoopThreadPool>(&m_loop, "WyNet");
    m_threadPool->setThreadNum(threadNum);
    m_threadPool->start([](EventLoop *loop) -> void {
        log_debug("ThreadInitCallback");
    });
}

WyNet::~WyNet()
{
    while (!m_servers.empty())
    {
        UniqID serverId = m_servers.begin()->first;
        destroyServer(serverId);
    }
    while (!m_clients.empty())
    {
        UniqID clientId = m_clients.begin()->first;
        destroyClient(clientId);
    }
    log_info("WyNet destroyed.");
}

UniqID WyNet::addServer(std::shared_ptr<Server> server)
{
    UniqID serverId = m_serverIdGen.getNewID();
    m_servers[serverId] = server;
    return serverId;
}

bool WyNet::destroyServer(UniqID serverId)
{
    Servers::iterator it = m_servers.find(serverId);
    if (it == m_servers.end())
    {
        return false;
    }
    std::shared_ptr<Server> server = it->second;
    m_servers.erase(it);
    m_serverIdGen.recycleID(serverId);
    return true;
}

UniqID WyNet::addClient(std::shared_ptr<Client> c)
{
    UniqID clientId = m_clientIdGen.getNewID();
    m_clients[clientId] = c;
    return clientId;
}

bool WyNet::destroyClient(UniqID clientId)
{
    Clients::iterator it = m_clients.find(clientId);
    if (it == m_clients.end())
    {
        return false;
    }
    std::shared_ptr<Client> client = it->second;
    m_clients.erase(it);
    m_serverIdGen.recycleID(clientId);
    return true;
}

void WyNet::stopLoop()
{
    m_loop.stop();
}

void WyNet::startLoop()
{
    m_loop.loop();
}
};
