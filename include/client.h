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
    std::shared_ptr<TcpClient> m_tcpClient;
    std::shared_ptr<UdpClient> m_udpClient;

  public:
    friend class TcpClient;

    Client(WyNet *net);

    ~Client();

    std::shared_ptr<TcpClient> initTcpClient(const char *host, int tcpPort);

    const std::shared_ptr<TcpClient> getTcpClient() const
    {
        return m_tcpClient;
    }

    const std::shared_ptr<UdpClient> getUdpClient() const
    {
        return m_udpClient;
    }

    WyNet *getNet() const
    {
        return m_net;
    }
};
}; // namespace wynet

#endif
