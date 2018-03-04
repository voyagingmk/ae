#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wytcpserver.h"
#include "wyudpserver.h"

namespace wynet
{

class TCPConnection
{
  public:
    int connfd;
    SockBuffer buf;
    uint32_t key;
    KCPObject *kcpDict;

    ConvID convId()
    {
        return key & 0x0000ffff;
    }

    uint16_t passwd()
    {
        return key >> 16;
    }
};

class Server
{
  public:
    aeEventLoop *aeloop;
    int tcpPort;
    int udpPort;
    TCPServer tcpServer;
    UDPServer udpServer;
    std::map<UniqID, TCPConnection> connDict;
    std::map<int, UniqID> connfd2cid;
    std::map<ConvID, UniqID> convId2cid;

    UniqIDGenerator clientIdGen;
    UniqIDGenerator convIdGen;

    Server(aeEventLoop *aeloop, int tcpPort, int udpPort);

    ~Server();

    void Send(UniqID clientId, const char *data, size_t len);
};
};

#endif
