#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpclient.h"
#include "wyudpclient.h"

namespace wynet
{

class Client {
public:
    aeEventLoop *aeloop;
    TCPClient tcpClient;
    UDPClient* udpClient;
    ConvID convID;
    KCPObject* kcpDict;
    Client(aeEventLoop *aeloop, const char *host, int tcpPort);
    ~Client();
};

};

#endif