#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpclient.h"
#include "wyudpclient.h"
#include "wyconnection.h"

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
    typedef void (*OnTcpRecvUserData)(Client *, uint8_t*, size_t);
    WyNet *net;
    ConnectionForClient conn;
    TCPClient tcpClient;
    UDPClient *udpClient;
    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;
    OnTcpRecvUserData onTcpRecvUserData;

    Client(WyNet *net, const char *host, int tcpPort);

    ~Client();

    void SendByTcp(const uint8_t *data, size_t len);
    
    void SendByTcp(PacketHeader *header);

private:
    
    void _onTcpConnected();

    void _onTcpDisconnected();
};
};

#endif
