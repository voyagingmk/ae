#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    printf("OnTcpMessage\n");
    Client *client = (Client *)(clientData);
    PacketHeader header;
    int n = Readn(fd, (char*)(&header), HeaderBaseLength);
    Readn(fd, (char*)(&header) + HeaderBaseLength, header.getHeaderLength() - HeaderBaseLength);
    if(!header.isFlagOn(HeaderFlag::PacketLen)) {
        return;
    }
    Protocol protocol = header.getProtocol();
    uint32_t packetLen = header.getUInt(HeaderFlag::PacketLen);
    switch (protocol) {
        case Protocol::Handshake:
           {
               protocol::Handshake handShake;
               assert(packetLen == header.getHeaderLength() + sizeof(protocol::Handshake));
               Readn(fd, (char*)(&handShake), sizeof(protocol::Handshake));
               printf("clientID %d, udpPort %d\n", handShake.clientID, handShake.udpPort);
               break;
           }
        default:
            break;
    }
}

Client::Client(WyNet *net, const char *host, int tcpPort) :
    net(net),
    tcpClient(this, host, tcpPort),
    udpClient(NULL),
    kcpDict(NULL)
{

    aeCreateFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE,
                      OnTcpMessage, (void *)this);

}

Client::~Client()
{
}
};
