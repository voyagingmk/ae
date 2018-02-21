#include "wyserver.h"

namespace wynet {
void OnUdpMessage(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
   // server->Recvfrom();
}

Server::Server(aeEventLoop *aeloop, int tcpPort, int udpPort):
    udpServer(udpPort) 
{
    aeCreateFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE,
                      OnUdpMessage, (void*)this);
}

void Server::Release(aeEventLoop *aeloop) {
    aeDeleteFileEvent(aeloop, udpServer.m_sockfd, AE_READABLE);
}

};
