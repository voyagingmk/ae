#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "wytcpserver.h"
#include "wyudpserver.h"
#include "wyconnection.h"

namespace wynet
{

class Server
{
public:
    aeEventLoop *aeloop;
    int tcpPort;
    int udpPort;
    TCPServer tcpServer;
    UDPServer udpServer;
    std::map<UniqID, ConnectionForServer> connDict;
    std::map<int, UniqID> connfd2cid;
    std::map<ConvID, UniqID> convId2cid;

    UniqIDGenerator clientIdGen;
    UniqIDGenerator convIdGen;

    Server(aeEventLoop *aeloop, int tcpPort, int udpPort);

    ~Server();

    void CloseConnect(int fd);
    
    void SendByTcp(UniqID clientId, const uint8_t *data, size_t len);

    void SendByTcp(UniqID clientId, PacketHeader *header);

};
};

#endif
