#include "wyclient.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    printf("OnTcpMessage\n");
    Client *client = (Client *)(clientData);
    client->tcpClient.Recvfrom();
}

Client::Client(aeEventLoop *aeloop, const char *host, int tcpPort) : tcpClient(host, tcpPort),
                                                                     udpClient(NULL),
                                                                     kcpDict(NULL)
{

    aeCreateFileEvent(aeloop, tcpClient.m_sockfd, AE_READABLE,
                      OnTcpMessage, (void *)this);

    tcpClient.Send("hello", 6);
}

Client::~Client()
{
}
};