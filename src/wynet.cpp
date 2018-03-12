#include "wynet.h"

namespace wynet
{

WyNet::WyNet()
{
}

WyNet::~WyNet()
{
    while (!servers.empty())
    {
        UniqID serverId = servers.begin()->first;
        DestroyServer(serverId);
    }
    while (!clients.empty())
    {
        UniqID clientId = clients.begin()->first;
        DestroyClient(clientId);
    }
    log_info("WyNet destroyed.");
}

UniqID WyNet::AddServer(Server *s)
{
    UniqID serverId = serverIdGen.getNewID();
    servers[serverId] = s;
    return serverId;
}

bool WyNet::DestroyServer(UniqID serverId)
{
    Servers::iterator it = servers.find(serverId);
    if (it == servers.end())
    {
        return false;
    }
    Server *server = it->second;
    servers.erase(it);
    delete server;
    serverIdGen.recycleID(serverId);
    return true;
}

UniqID WyNet::AddClient(Client *s)
{
    UniqID clientId = clientIdGen.getNewID();
    clients[clientId] = s;
    return clientId;
}

bool WyNet::DestroyClient(UniqID clientId)
{
    Clients::iterator it = clients.find(clientId);
    if (it == clients.end())
    {
        return false;
    }
    Client *client = it->second;
    clients.erase(it);
    delete client;
    serverIdGen.recycleID(clientId);
    return true;
}

void WyNet::StopLoop()
{
    loop.stop();
}

void WyNet::Loop()
{
     loop.loop();
}
};
