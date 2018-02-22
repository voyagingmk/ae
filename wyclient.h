#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpserver.h"
#include "wyudpserver.h"
#include "wytcpclient.h"
#include "wyudpclient.h"

namespace wynet
{

class Client {
public:
    aeEventLoop *aeloop;
    TCPClient tcpClient;
    UDPClient udpClient;
    ConvID convID;
    KCPObject kcpDict;
    Client(aeEventLoop *aeloop, int tcpPort);
    ~Client();
};

};

#endif