#include "wyserver.h"

namespace wynet {

void onTcpMessage(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask) 
{
    Server *server = (Server *)(clientData);
    
}


void OnTcpNewConnection(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    int sockfd = server->tcpServer.m_sockfd;
    printf("==%d %d\n", sockfd, fd);
    assert(sockfd == fd);
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    int connfd = Accept(sockfd, (SA *) &cliAddr, &len);
   // server->Recvfrom();
    aeCreateFileEvent(server->aeloop, connfd, AE_READABLE,
                      onTcpMessage, server);
    TCPClientInfo info;
    info.connfd = connfd;
    UniqID clientID = server->clientIdGen.getNewID();
    server->clientDict[clientID] = info;
}

void OnUdpMessage(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    server->udpServer.Recvfrom();
}

Server::Server(aeEventLoop *aeloop, int tcpPort, int udpPort):
    aeloop(aeloop),
    tcpServer(tcpPort),
    udpServer(udpPort) 
{

    aeCreateFileEvent(aeloop,tcpServer.m_sockfd, AE_READABLE,
                      OnTcpNewConnection, (void*)this);

    aeCreateFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE,
                      OnUdpMessage, (void*)this);
}

Server::~Server() {
    aeDeleteFileEvent(aeloop, tcpServer.m_sockfd, AE_READABLE); 
    aeDeleteFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE); 
    aeloop = NULL;
    printf("Server destoryed.\n");
}

};
