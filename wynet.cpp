#include "wynet.h"

namespace wynet
{

WyNet::WyNet()
{
    aeloop = aeCreateEventLoop(64);
}

WyNet::~WyNet()
{
    while (!servers.empty())
    {
        UniqID serverID = servers.begin()->first;
        DestroyServer(serverID);
    }
    printf("WyNet destroyed.\n");
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
    aeStop(aeloop);
}

void WyNet::Loop()
{
    aeloop->stop = 0;
    while (!aeloop->stop)
    {
        if (aeloop->beforesleep != NULL)
            aeloop->beforesleep(aeloop);
        aeProcessEvents(aeloop, AE_ALL_EVENTS | AE_DONT_WAIT | AE_CALL_AFTER_SLEEP);
    }
}
};
