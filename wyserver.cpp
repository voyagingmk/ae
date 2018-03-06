#include "wyserver.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wyutils.h"

namespace wynet
{

void onTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    log_debug("onTcpMessage fd=%d", fd);
    Server *server = (Server *)(clientData);
    UniqID clientId = server->connfd2cid[fd];
    ConnectionForServer &conn = server->connDict[clientId];
    SockBuffer& sockBuffer = conn.buf;
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(fd, &nreadTotal);
        log_debug("[tcp] readIn ret %d nreadTotal %d", ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            server->CloseConnect(fd);
            return;
        }
        if (ret == 2)
        {
            break;
        }
        if (ret == 1)
        {
            BufferRef &bufRef = sockBuffer.bufRef;
            PacketHeader *header = (PacketHeader *)(bufRef->buffer);
            Protocol protocol = static_cast<Protocol>(header->getProtocol());
            switch (protocol)
            {
                case Protocol::UdpHandshake:
                {
                    break;
                }
                case Protocol::UserPacket:
                {
                    protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->buffer + header->getHeaderLength());
                    
                    log_debug("getHeaderLength: %d", header->getHeaderLength());
                    log_debug("UserPacket: %s", (const char*)p);
                    break;
                }
                default:
                    break;
            }
            sockBuffer.resetBuffer();
        }
    } while (1);
}

void OnTcpNewConnection(struct aeEventLoop *eventLoop,
                        int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    int sockfd = server->tcpServer.m_sockfd;
    assert(sockfd == fd);
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    int connfdTcp = Accept(sockfd, (SA *)&cliAddr, &len);
    if (connfdTcp == -1)
    {
        if ((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) ||
            (errno == ECONNABORTED) ||
            (errno == EINTR))
        {
            // already closed
            return;
        }
        err_msg("[server] new connection err: %d %s", errno, strerror(errno));
        return;
    }
    aeCreateFileEvent(server->aeloop, connfdTcp, AE_READABLE,
                      onTcpMessage, server);

    UniqID clientId = server->clientIdGen.getNewID();
    UniqID convId = server->convIdGen.getNewID();
    server->connDict[clientId] = ConnectionForServer();
    ConnectionForServer &conn = server->connDict[clientId];
    conn.connfdTcp = connfdTcp;
    uint16_t password = random();
    conn.key = (password << 16) | convId;
    server->connfd2cid[connfdTcp] = clientId;
    server->convId2cid[convId] = clientId;

    protocol::TcpHandshake handshake;
    handshake.clientId = clientId;
    handshake.udpPort = server->udpPort;
    handshake.key = conn.key;
    server->SendByTcp(clientId, SerializeProtocol<protocol::TcpHandshake>(handshake));

    log_info("Client %d connected, connfdTcp: %d, key: %d ", clientId, connfdTcp, handshake.key);
    LogSocketState(connfdTcp);
}

void OnUdpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    server->udpServer.Recvfrom();
}

Server::Server(aeEventLoop *aeloop, int tcpPort, int udpPort) : aeloop(aeloop),
                                                                tcpPort(tcpPort),
                                                                udpPort(udpPort),
                                                                tcpServer(tcpPort),
                                                                udpServer(udpPort)
{

    convIdGen.setRecycleThreshold(2 << 15);
    convIdGen.setRecycleEnabled(true);

    log_info("Server created, tcp sockfd: %d, udp sockfd: %d",
             tcpServer.m_sockfd,
             udpServer.m_sockfd);

    aeCreateFileEvent(aeloop, tcpServer.m_sockfd, AE_READABLE,
                      OnTcpNewConnection, (void *)this);

    aeCreateFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE,
                      OnUdpMessage, (void *)this);
}

Server::~Server()
{
    aeDeleteFileEvent(aeloop, tcpServer.m_sockfd, AE_READABLE);
    aeDeleteFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE);
    aeloop = NULL;
    log_info("Server destoryed.");
}
    
void Server::CloseConnect(int connfdTcp) {
    Close(connfdTcp);
    aeDeleteFileEvent(aeloop, connfdTcp, AE_READABLE);
    if (connfd2cid.find(connfdTcp) != connfd2cid.end()) {
        UniqID clientId = connfd2cid[connfdTcp];
        connfd2cid.erase(connfdTcp);
        ConnectionForServer &conn = connDict[clientId];
        convId2cid.erase(conn.convId());
        connDict.erase(clientId);
        log_info("CloseConnect %d connected, connfdTcp: %d", clientId, connfdTcp);
    }
}

void Server::SendByTcp(UniqID clientId, const uint8_t *data, size_t len)
{
     protocol::UserPacket* p = (protocol::UserPacket*)data;
    PacketHeader* header = SerializeProtocol<protocol::UserPacket>(*p, len);
    SendByTcp(clientId, header);
}
    
    
void Server::SendByTcp(UniqID clientId, PacketHeader *header) {
    assert(header != nullptr);
    auto it = connDict.find(clientId);
    if (it == connDict.end())
    {
        return;
    }
    ConnectionForServer &conn = it->second;
    ::Send(conn.connfdTcp, (uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen), 0);
}
    
};
