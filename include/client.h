#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "kcp.h"
#include "tcpclient.h"
#include "udpclient.h"
#include "connection.h"
#include "noncopyable.h"
#include "eventloop.h"

namespace wynet
{

class WyNet;
class Test;
class Client;

typedef std::shared_ptr<Client> PtrClient;

class Client : public Noncopyable, public std::enable_shared_from_this<Client>
{
    WyNet *m_net;
    WeakPtrTcpClient m_tcpClient;

  protected:
    Client(WyNet *net);

  public:
    static PtrClient create(WyNet *net);

    ~Client();

    PtrTcpClient initTcpClient(const char *host, int tcpPort);

    PtrTcpClient getTcpClient() const
    {
        return m_tcpClient.lock();
    }

    WyNet *getNet() const
    {
        return m_net;
    }

    friend class TcpClient;
};
}; // namespace wynet

#endif
