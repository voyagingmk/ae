#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "wykcp.h"
#include "wytcpserver.h"
#include "wyudpserver.h"
#include "wytcpclient.h"
#include "wyudpclient.h"

namespace wynet
{

class Server {
public:
    TCPServer tcpServer;
    UDPServer udpServer;
    std::map<ConvID, KCPObject> kcpDict;

    Server(aeEventLoop *aeloop, int tcpPort, int udpPort);

    void Release(aeEventLoop *aeloop);

    bool hasConv(ConvID conv) {
       return kcpDict.find(conv) != kcpDict.end();
    }
};

};

#endif