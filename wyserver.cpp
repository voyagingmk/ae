#include "wyserver.h"
#include "protocol.h"
#include "protocol_define.h"

namespace wynet
{

void onTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    log_debug("onTcpMessage fd=%d", fd);
    Server *server = (Server *)(clientData);
    UniqID clientId = server->connfd2cid[fd];
    TCPConnection &conn = server->connDict[clientId];
    int ret = conn.buf.readIn(fd);
    if (ret == 0)
    {
        Close(fd);
        server->connfd2cid.erase(fd);
        server->convId2cid.erase(conn.convId());
        server->connDict.erase(clientId);
        aeDeleteFileEvent(server->aeloop, fd, AE_READABLE);
    }
}

void OnTcpNewConnection(struct aeEventLoop *eventLoop,
                        int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    int sockfd = server->tcpServer.m_sockfd;
    assert(sockfd == fd);
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    int connfd = Accept(sockfd, (SA *)&cliAddr, &len);
    if (connfd == -1)
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
    aeCreateFileEvent(server->aeloop, connfd, AE_READABLE,
                      onTcpMessage, server);

    UniqID clientId = server->clientIdGen.getNewID();
    UniqID convId = server->convIdGen.getNewID();
    server->connDict[clientId] = TCPConnection();
    TCPConnection &conn = server->connDict[clientId];
    conn.connfd = connfd;
    uint16_t password = random();
    conn.key = (password << 16) | convId;
    server->connfd2cid[connfd] = clientId;
    server->convId2cid[convId] = clientId;

    protocol::Handshake handshake;
    handshake.clientId = clientId;
    handshake.udpPort = server->udpPort;
    handshake.key = conn.key;

    PacketHeader *header = SerializeProtocol<protocol::Handshake>(handshake);
    log_debug("send handshake len %d", header->getUInt32(HeaderFlag::PacketLen));
    server->Send(clientId, (char *)header, header->getUInt32(HeaderFlag::PacketLen));

    log_info("Client %d connected, connfd: %d, key: %d ", handshake.key, clientId, connfd);
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

void Server::Send(UniqID clientId, const char *data, size_t len)
{
    auto it = connDict.find(clientId);
    if (it == connDict.end())
    {
        return;
    }
    TCPConnection &conn = it->second;

    ::Send(conn.connfd, data, len, 0);
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}
};
