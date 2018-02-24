#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    printf("OnTcpMessage\n");
    Client *client = (Client *)(clientData);
    char buf[MAXLINE + 1];
    int n = Readn(client->tcpClient.m_sockfd, buf, MAXLINE);
    printf("Readn n = %d\n", n);
    buf[n] = 0;    /* null terminate */
    PacketHeader* header = (PacketHeader*)buf;
    Protocol protocol = header->getProtocol();
    uint32_t packetLen = header->getUInt(HeaderFlag::PacketLen);
    switch (protocol) {
        case Protocol::Handshake:
           {
               protocol::Handshake* handShake = (protocol::Handshake*)((char*)buf + packetLen);
               printf("clientID %d, udpPort %d\n", handShake->clientID, handShake->udpPort);
               break;
           }
        default:
            break;
    }
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
