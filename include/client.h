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

class Client : public FDRef
{
    WyNet *m_net;
    PtrConn m_conn;
    std::shared_ptr<TCPClient> m_tcpClient;
    std::shared_ptr<UDPClient> m_udpClient;

  public:
    friend class TCPClient;
    std::shared_ptr<Client> shared_from_this()
    {
        return FDRef::downcasted_shared_from_this<Client>();
    }

    Client(WyNet *net);

    ~Client();

    std::shared_ptr<TCPClient> initTcpClient(const char *host, int tcpPort);

    const std::shared_ptr<TCPClient> getTcpClient() const
    {
        return m_tcpClient;
    }

    const std::shared_ptr<UDPClient> getUdpClient() const
    {
        return m_udpClient;
    }

    WyNet *getNet() const
    {
        return m_net;
    }
};
};

#endif
