#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "wykcp.h"
#include "wyudpserver.h"
#include "wyudpclient.h"

namespace wynet
{
class WyNet
{
  public:
    aeEventLoop *loop;
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