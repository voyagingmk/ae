#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "wytcpserver.h"
#include "wyudpserver.h"
#include "wyconnection.h"

namespace wynet
{

class WyNet;
    
class Server
{
    WyNet *net;
    int tcpPort;
    int udpPort;
    TCPServer tcpServer;
    UDPServer udpServer;
    std::map<UniqID, ConnectionForServer> connDict;
    std::map<int, UniqID> connfd2cid;
    std::map<ConvID, UniqID> convId2cid;
    
    UniqIDGenerator clientIdGen;
    UniqIDGenerator convIdGen;
    
public:
    typedef void (*OnTcpConnected)(Server *, UniqID clientId);
    typedef void (*OnTcpDisconnected)(Server *, UniqID clientId);
    typedef void (*OnTcpRecvUserData)(Server *, UniqID clientId, uint8_t*, size_t);

    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;
    OnTcpRecvUserData onTcpRecvUserData;

    Server(WyNet *net, int tcpPort, int udpPort);

    ~Server();

    void CloseConnect(int connfdTcp);
    
    void SendByTcp(UniqID clientId, const uint8_t *data, size_t len);

    void SendByTcp(UniqID clientId, PacketHeader *header);

private:
    
    void _onTcpConnected(int connfdTcp);
    
    void _onTcpMessage(int connfdTcp);
    
    void _onTcpDisconnected(int connfdTcp);
    
    friend void onTcpMessage(struct aeEventLoop *eventLoop,
                      int connfdTcp, void *clientData, int mask);
    
    friend void OnTcpNewConnection(struct aeEventLoop *eventLoop,
                                   int listenfdTcp, void *clientData, int mask);
};
};

#endif
