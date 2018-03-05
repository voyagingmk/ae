#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    log_debug("OnTcpMessage");
    Client *client = (Client *)(clientData);
    TCPClient& tcpClient = client->tcpClient;
    SockBuffer& buf = tcpClient.buf;
    // validate packet
    do {
        int nreadTotal = 0;
        int ret = buf.readIn(fd, &nreadTotal);
        log_info("readIn ret %d nreadTotal %d", ret, nreadTotal);
        if (ret <= 0) {
            // has error or has closed
            tcpClient.Close();
            return;
        }
        if (ret == 2) {
            break;
        }
        if (ret == 1) {
            Buffer* p = buf.bufRef.get();
            uint8_t* buffer = p->buffer;
            PacketHeader* header = (PacketHeader*)(buffer);
            Protocol protocol = static_cast<Protocol>(header->getProtocol());
            switch (protocol)
            {
                case Protocol::Handshake:
                {
                    protocol::Handshake* handShake = (protocol::Handshake*)(buffer + header->getHeaderLength());
                    log_debug("clientId %d, udpPort %d", handShake->clientId, handShake->udpPort);
                    break;
                }
                default:
                    break;
            }
            buf.resetBuffer();
        }
    } while(1);
}

Client::Client(WyNet *net, const char *host, int tcpPort) : net(net),
                                                            tcpClient(this, host, tcpPort),
                                                            udpClient(NULL),
                                                            kcpDict(NULL),
                                                            onTcpConnected(NULL)
{
}

Client::~Client()
{
    log_info("[Client] close tcp sockfd %d", tcpClient.m_sockfd);
    tcpClient.Close();
}

void Client::_onTcpConnected()
{
    aeCreateFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE,
                      OnTcpMessage, (void *)this);
    if (onTcpConnected)
        onTcpConnected(this);
}
    
    
void Client::_onTcpDisconnected() {
    aeDeleteFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE | AE_WRITABLE);
    if (onTcpDisconnected)
        onTcpDisconnected(this);
}
};
