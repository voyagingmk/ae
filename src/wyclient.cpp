#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"
#include "wyutils.h"

namespace wynet
{

void OnTcpMessage(EventLoop *loop, std::weak_ptr<FDRef> fdRef, int mask)
{
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return;
    }
    log_debug("OnTcpMessage fd %d", sfdRef->fd());
    std::shared_ptr<Client> client = std::dynamic_pointer_cast<Client>(sfdRef);
    client->_onTcpMessage();
}

Client::Client(WyNet *net) : FDRef(0),
                             m_net(net),
                             onTcpConnected(nullptr),
                             onTcpDisconnected(nullptr),
                             onTcpRecvMessage(nullptr)
{
}

Client::~Client()
{
    log_info("[Client] close tcp sockfd %d", m_tcpClient->sockfd());
    m_tcpClient->Close();
}

void Client::initTcpClient(const char *host, int tcpPort)
{
    m_tcpClient = std::make_shared<TCPClient>(shared_from_this(), host, tcpPort);
}

void Client::sendByTcp(const uint8_t *data, size_t len)
{
    protocol::UserPacket *p = (protocol::UserPacket *)data;
    PacketHeader *header = SerializeProtocol<protocol::UserPacket>(*p, len);
    m_tcpClient->Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::sendByTcp(PacketHeader *header)
{
    m_tcpClient->Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::_onTcpConnected()
{
    m_net->getLoop().createFileEvent(m_tcpClient->shared_from_this(), LOOP_EVT_READABLE, OnTcpMessage);
    LogSocketState(m_tcpClient->sockfd());
    m_conn = std::make_shared<CliConn>(m_tcpClient->sockfd());
    if (onTcpConnected)
        onTcpConnected(shared_from_this());
}

void Client::_onTcpDisconnected()
{
    m_net->getLoop().deleteFileEvent(m_tcpClient->sockfd(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
    if (onTcpDisconnected)
        onTcpDisconnected(shared_from_this());
}

void Client::_onTcpMessage()
{
    SockBuffer &sockBuffer = m_tcpClient->m_buf;
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
            PacketHeader *header = (PacketHeader *)(bufRef->m_data);
            Protocol protocol = static_cast<Protocol>(header->getProtocol());
            switch (protocol)
            {
            case Protocol::TcpHandshake:
            {
                protocol::TcpHandshake *handShake = (protocol::TcpHandshake *)(bufRef->m_data + header->getHeaderLength());
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
                protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->m_data + header->getHeaderLength());
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
};
