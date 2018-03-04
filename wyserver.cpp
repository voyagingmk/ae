#include "wyserver.h"
#include "protocol.h"
#include "protocol_define.h"

namespace wynet
{

void onTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    printf("onTcpMessage fd=%d\n", fd);
    Server *server = (Server *)(clientData);
    UniqID clientID = server->connfd2cid[fd];
    TCPConnection &conn = server->connDict[clientID];
    int ret = conn.buf.readIn(fd);
    printf("ret=%d\n", ret);
    if (ret == 0) {
        Close(fd);
        server->connfd2cid.erase(fd);
        server->connDict.erase(clientID);
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

    UniqID clientID = server->clientIdGen.getNewID();
    server->connDict[clientID] = TCPConnection();
    TCPConnection &conn = server->connDict[clientID];
    conn.connfd = connfd;
    server->connfd2cid[connfd] = clientID;

    protocol::Handshake handshake;
    handshake.clientID = clientID;
    handshake.udpPort = server->udpPort;
    PacketHeader *header = SerializeProtocol<protocol::Handshake>(handshake);
    printf("send handshake %d\n", header->getUInt32(HeaderFlag::PacketLen));
    server->Send(clientID, (char *)header, header->getUInt32(HeaderFlag::PacketLen));

    printf("Client %d connected, connfd: %d \n", clientID, connfd);
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

    printf("Server created, tcp sockfd: %d, udp sockfd: %d\n",
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
    printf("Server destoryed.\n");
}

void Server::Send(UniqID clientID, const char *data, size_t len)
{
    auto it = connDict.find(clientID);
    if (it == connDict.end())
    {
        return;
    }
    TCPConnection &conn = it->second;

    ::Send(conn.connfd, data, len, 0);
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}
};
