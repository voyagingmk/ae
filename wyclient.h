#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpclient.h"
#include "wyudpclient.h"

namespace wynet
{

class WyNet;
class Test;

class Client
{
  public:
    friend class TCPClient;
    typedef void (*OnTcpConnected)(Client *);
    typedef void (*OnTcpDisconnected)(Client *);
    WyNet *net;
    TCPClient tcpClient;
    UDPClient *udpClient;
    ConvID convId;
    KCPObject *kcpDict;
    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;

    Client(WyNet *net, const char *host, int tcpPort);

    ~Client();

  private:
    void _onTcpConnected();
    
    void _onTcpDisconnected();
};
};

#endif
