#include "wynet.h"

namespace wynet {

WyNet::WyNet() {
    aeloop = aeCreateEventLoop(64);
}

UniqID WyNet::AddServer(Server * s) {
    UniqID serverId = serverIdGen.getNewID();
    servers[serverId] = s;
    return serverId;
}

bool WyNet::DestroyServer(UniqID serverId) {
    if(servers.find(serverId) == servers.end()) {
        return false;
    }
    Server* server = servers[serverId];
    delete servers[serverId];
    serverIdGen.recycleID(serverId);
    return true;
}

void WyNet::Loop() {
    aeMain(aeloop);
}

};