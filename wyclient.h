#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpclient.h"
#include "wyudpclient.h"

namespace wynet
{

class WyNet;
    
class Client {
public:
    typedef void(*OnTcpConnected)(Client *);
    WyNet *net;
    TCPClient tcpClient;
    UDPClient* udpClient;
    ConvID convID;
    KCPObject* kcpDict;
    OnTcpConnected onTcpConnected;
    Client(WyNet *net, const char *host, int tcpPort);
    ~Client();
};

};

#endif
