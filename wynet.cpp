#include "wynet.h"

namespace wynet {

WyNet::WyNet() {
    aeloop = aeCreateEventLoop(64);
}

    
WyNet::~WyNet() {
    while(!servers.empty()) {
        UniqID serverID = servers.begin()->first;
        DestroyServer(serverID);
    }
    printf("WyNet destroyed.\n");
}

UniqID WyNet::AddServer(Server * s) {
    UniqID serverId = serverIdGen.getNewID();
    servers[serverId] = s;
    return serverId;
}

bool WyNet::DestroyServer(UniqID serverId) {
    Servers::iterator it = servers.find(serverId);
    if(it == servers.end()) {
        return false;
    }
    Server* server = it->second;
    servers.erase(it);
    delete server;
    serverIdGen.recycleID(serverId);
    return true;
}

void WyNet::StopLoop() {
   aeloop->stop = 1;
}

void WyNet::Loop() {
    aeMain(aeloop);
}

};