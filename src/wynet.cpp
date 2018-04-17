#include "wynet.h"

namespace wynet
{

WyNet::WyNet()
{ 
    m_threadPool = std::make_shared<EventLoopThreadPool>(&m_loop, "WyNet");
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

UniqID WyNet::addServer(Server *s)
{
    UniqID serverId = m_serverIdGen.getNewID();
    m_servers[serverId] = s;
    return serverId;
}

bool WyNet::destroyServer(UniqID serverId)
{
    Servers::iterator it = m_servers.find(serverId);
    if (it == m_servers.end())
    {
        return false;
    }
    Server *server = it->second;
    m_servers.erase(it);
    delete server;
    m_serverIdGen.recycleID(serverId);
    return true;
}

UniqID WyNet::addClient(Client *s)
{
    UniqID clientId = m_clientIdGen.getNewID();
    m_clients[clientId] = s;
    return clientId;
}

bool WyNet::destroyClient(UniqID clientId)
{
    Clients::iterator it = m_clients.find(clientId);
    if (it == m_clients.end())
    {
        return false;
    }
    Client *client = it->second;
    m_clients.erase(it);
    delete client;
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
