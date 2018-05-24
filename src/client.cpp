#include "client.h"
#include "protocol.h"
#include "protocol_define.h"
#include "net.h"
#include "utils.h"

namespace wynet
{

Client::Client(WyNet *net) : m_net(net)
{
}

Client::~Client()
{
}

std::shared_ptr<TCPClient> Client::initTcpClient(const char *host, int tcpPort)
{
    m_tcpClient = std::make_shared<TCPClient>(shared_from_this());
    m_tcpClient->init();
    m_tcpClient->connect(host, tcpPort);
    return m_tcpClient;
}

/*
void Client::_onTcpMessage()
{
    SockBuffer &sockBuffer = m_tcpClient->m_pendingRecvBuf;
    // validate packet
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(m_tcpClient->sockfd(), &nreadTotal);
        log_debug("readIn ret %d nreadTotal %d", ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            m_tcpClient->Close();
            return;
        }
        if (ret == 2)
        {
            break;
        }
        if (ret == 1)
        {
            BufferRef &bufRef = sockBuffer.bufRef;
            PacketHeader *header = (PacketHeader *)(bufRef->data());
            Protocol protocol = static_cast<Protocol>(header->getProtocol());
            switch (protocol)
            {
            case Protocol::TcpHandshake:
            {
                protocol::TcpHandshake *handShake = (protocol::TcpHandshake *)(bufRef->data() + header->getHeaderLength());
                m_conn->setKey(handShake->key);
                m_conn->setUdpPort(handShake->udpPort);
                m_conn->setConnectId(handShake->connectId);
                log_info("TcpHandshake connectId %d, udpPort %d convId %d passwd %d",
                         handShake->connectId, handShake->udpPort,
                         m_conn->convId(),
                         m_conn->passwd());
                break;
            }
            case Protocol::UserPacket:
            {
                protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->data() + header->getHeaderLength());
                size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
                // log_debug("getHeaderLength: %d", header->getHeaderLength());
                if (onTcpRecvMessage)
                {
                    onTcpRecvMessage(shared_from_this(), (uint8_t *)p, dataLength);
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
*/
}; // namespace wynet
