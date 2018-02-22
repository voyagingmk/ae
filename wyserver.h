#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wytcpserver.h"
#include "wyudpserver.h"

namespace wynet
{

struct TCPClientInfo {
    int connfd;

};

class Server {
public:
    aeEventLoop *aeloop;
    TCPServer tcpServer;
    UDPServer udpServer;
    std::map<UniqID, TCPClientInfo> clientDict;
    std::map<ConvID, KCPObject> kcpDict;
    UniqIDGenerator clientIdGen;
    
    Server(aeEventLoop *aeloop, int tcpPort, int udpPort);
     
    ~Server();

    bool hasConv(ConvID conv) {
       return kcpDict.find(conv) != kcpDict.end();
    }
};

};

#endif