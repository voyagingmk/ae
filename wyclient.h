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
    uint32_t key;
    uint32_t clientId;
    uint16_t udpPort;
    KCPObject *kcpDict;
    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;

    Client(WyNet *net, const char *host, int tcpPort);

    ~Client();
    
    ConvID convId()
    {
        return key & 0x0000ffff;
    }
    
    uint16_t passwd()
    {
        return key >> 16;
    }

  private:
    void _onTcpConnected();
    
    void _onTcpDisconnected();
};
};

#endif
