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
    char recvline[MAXLINE + 1];
    int n = read(fd, recvline, MAXLINE);
    recvline[n] = '\0';
    printf("recv n = %d msg: %s\n", n, recvline);
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
    aeCreateFileEvent(server->aeloop, connfd, AE_READABLE,
                      onTcpMessage, server);
    TCPClientInfo info;
    info.connfd = connfd;
    UniqID clientID = server->clientIdGen.getNewID();
    server->clientDict[clientID] = info;

    
    protocol::Handshake handshake;
    handshake.clientID = clientID;
    handshake.udpPort = server->udpPort;
    char* buf = SerializeProtocol<protocol::Handshake>(handshake);
    server->Send(clientID, buf, strlen(buf));
    
    printf("Client %d connected, connfd: %d \n", clientID, connfd);
}

void OnUdpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    server->udpServer.Recvfrom();
}

Server::Server(aeEventLoop *aeloop, int tcpPort, int udpPort) :
    aeloop(aeloop),
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
    auto it = clientDict.find(clientID);
    if(it == clientDict.end()) {
        return;
    }
    TCPClientInfo& info = it->second;

    ::Send(info.connfd, data, len, 0);
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}
};
