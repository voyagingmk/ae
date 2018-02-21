#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wyserver.h"

namespace wynet
{
class WyNet
{
protected:
    aeEventLoop * aeloop;
    typedef std::map<UniqID, Server*> Servers;
    Servers servers;
    UniqIDGenerator serverIdGen;
  public:
    WyNet();
    ~WyNet();
    void Loop();
    aeEventLoop * GetAeLoop() {
        return aeloop;
    }
    UniqID AddServer(Server* server);
    bool DestroyServer(UniqID serverId);
};

static int SocketOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
    SocketBase *s = (SocketBase *)user;
    assert(s);
    // s->send(buf, len);
    return len;
}
};

#endif