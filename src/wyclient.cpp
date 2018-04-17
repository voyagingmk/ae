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
    if (!sfdRef) {
        return;
    }
    log_debug("OnTcpMessage fd %d", sfdRef->fd());  
    std::shared_ptr<Client> client = std::dynamic_pointer_cast<Client>(sfdRef);
    client->_onTcpMessage();
}

Client::Client(WyNet *net, const char *host, int tcpPort) : m_net(net),
                                                            m_tcpClient(this, host, tcpPort),
                                                            m_udpClient(nullptr),
                                                            onTcpConnected(nullptr),
                                                            onTcpDisconnected(nullptr),
                                                            onTcpRecvUserData(nullptr)
{
}

Client::~Client()
{
    log_info("[Client] close tcp sockfd %d", m_tcpClient.sockfd());
    m_tcpClient.Close();
}

void Client::sendByTcp(const uint8_t *data, size_t len)
{
    protocol::UserPacket *p = (protocol::UserPacket *)data;
    PacketHeader *header = SerializeProtocol<protocol::UserPacket>(*p, len);
    m_tcpClient.Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::sendByTcp(PacketHeader *header)
{
    m_tcpClient.Send((uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen));
}

void Client::_onTcpConnected()
{
    m_net->getLoop().createFileEvent(shared_from_this(), LOOP_EVT_READABLE, OnTcpMessage);
    LogSocketState(m_tcpClient.sockfd());
    if (onTcpConnected)
        onTcpConnected(this);
}

void Client::_onTcpDisconnected()
{
    m_net->getLoop().deleteFileEvent(m_tcpClient.sockfd(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
    if (onTcpDisconnected)
        onTcpDisconnected(this);
}

void Client::_onTcpMessage()
{
    SockBuffer &sockBuffer = m_tcpClient.buf;
    // validate packet
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(m_tcpClient.sockfd(), &nreadTotal);
        log_debug("readIn ret %d nreadTotal %d", ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            m_tcpClient.Close();
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
                m_conn.key = handShake->key;
                m_conn.udpPort = handShake->udpPort;
                m_conn.clientId = handShake->clientId;
                log_info("clientId %d, udpPort %d convId %d passwd %d",
                         handShake->clientId, handShake->udpPort,
                         m_conn.convId(),
                         m_conn.passwd());
                break;
            }
            case Protocol::UserPacket:
            {
                protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->m_data + header->getHeaderLength());
                size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
                // log_debug("getHeaderLength: %d", header->getHeaderLength());
                if (onTcpRecvUserData)
                {
                    onTcpRecvUserData(this, (uint8_t *)p, dataLength);
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
