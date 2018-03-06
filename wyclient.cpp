#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"
#include "wyutils.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    log_debug("OnTcpMessage fd %d", fd);
    Client *client = (Client *)(clientData);
    client->_onTcpMessage();
}

Client::Client(WyNet *net, const char *host, int tcpPort) :
    net(net),
    tcpClient(this, host, tcpPort),
    udpClient(nullptr),
    onTcpConnected(nullptr),
    onTcpDisconnected(nullptr),
    onTcpRecvUserData(nullptr)
{
}

Client::~Client()
{
    log_info("[Client] close tcp sockfd %d", tcpClient.m_sockfd);
    tcpClient.Close();
}
    
    
void Client::SendByTcp(const uint8_t *data, size_t len) {
    protocol::UserPacket* p = (protocol::UserPacket*)data;
    PacketHeader* header = SerializeProtocol<protocol::UserPacket>(*p, len);
    tcpClient.Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::SendByTcp(PacketHeader *header) {
    tcpClient.Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::_onTcpConnected()
{
    aeCreateFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE,
                      OnTcpMessage, (void *)this);
    LogSocketState(tcpClient.m_sockfd);
    if (onTcpConnected)
        onTcpConnected(this);
}

void Client::_onTcpDisconnected()
{
    aeDeleteFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE | AE_WRITABLE);
    if (onTcpDisconnected)
        onTcpDisconnected(this);
}

void Client::_onTcpMessage() {
    SockBuffer &sockBuffer = tcpClient.buf;
    // validate packet
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(tcpClient.m_sockfd, &nreadTotal);
        log_debug("readIn ret %d nreadTotal %d", ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            tcpClient.Close();
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
                case Protocol::TcpHandshake:
                {
                    protocol::TcpHandshake *handShake = (protocol::TcpHandshake *)(bufRef->buffer + header->getHeaderLength());
                    conn.key = handShake->key;
                    conn.udpPort = handShake->udpPort;
                    conn.clientId = handShake->clientId;
                    log_info("clientId %d, udpPort %d convId %d passwd %d",
                             handShake->clientId, handShake->udpPort,
                             conn.convId(),
                             conn.passwd());
                    break;
                }
                case Protocol::UserPacket:
                {
                    protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->buffer + header->getHeaderLength());
                    size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
                    // log_debug("getHeaderLength: %d", header->getHeaderLength());
                    if (onTcpRecvUserData) {
                        onTcpRecvUserData(this, (uint8_t*)p, dataLength);
                    }
                    break;
                }
                default:
                    break;
            }
            sockBuffer.resetBuffer();
        }
    } while (1);
}
    
};
