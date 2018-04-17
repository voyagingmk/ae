#include "wyserver.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wyutils.h"
#include "wynet.h"
#include "eventloop.h"

namespace wynet
{

void OnTcpNewConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef) {
        return;
    }
    std::shared_ptr<Server> server = std::dynamic_pointer_cast<Server>(sfdRef);
    int listenfdTcp = server->getTCPServer().sockfd();
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    int connfdTcp = accept(listenfdTcp, (SA *)&cliAddr, &len);
    if (connfdTcp < 0)
    {
        if ((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) ||
            (errno == ECONNABORTED) ||
#ifdef EPROTO
            (errno == EPROTO) ||
#endif
            (errno == EINTR))
        {
            // already closed
            return;
        }
        log_error("[Server] Accept err: %d %s", errno, strerror(errno));
        return;
    }
    server->_onTcpConnected(connfdTcp);
}

void OnUdpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    // Server *server = (Server *)(clientData);
    // server->m_udpServer.Recvfrom();
}

Server::Server(WyNet *net, int tcpPortArg, int udpPortArg) : m_net(net),
                                                             m_tcpPort(tcpPortArg),
                                                             m_udpPort(udpPortArg),
                                                             m_tcpServer(m_tcpPort),
                                                             m_udpServer(m_udpPort),
                                                             onTcpConnected(nullptr),
                                                             onTcpDisconnected(nullptr),
                                                             onTcpRecvUserData(nullptr)
{

    m_convIdGen.setRecycleThreshold(2 << 15);
    m_convIdGen.setRecycleEnabled(true);

    log_info("[Server] created, tcp sockfd: %d, udp sockfd: %d",
             m_tcpServer.sockfd(),
             m_udpServer.sockfd());

    m_net->getLoop().createFileEvent(m_tcpServer.shared_from_this(), LOOP_EVT_READABLE,
                                   OnTcpNewConnection);

    if (m_udpServer.valid())
    {
        m_net->getLoop().createFileEvent(m_udpServer.shared_from_this(), LOOP_EVT_READABLE,
                                       OnUdpMessage);
    }
    LogSocketState(m_tcpServer.sockfd());
}

Server::~Server()
{
    m_net->getLoop().deleteFileEvent(m_tcpServer.sockfd(), LOOP_EVT_READABLE);
    m_net->getLoop().deleteFileEvent(m_udpServer.sockfd(), LOOP_EVT_READABLE);
    m_net = nullptr;
    log_info("[Server] destoryed.");
}

void Server::closeConnect(UniqID clientId)
{
    auto it = m_connDict.find(clientId);
    if (it == m_connDict.end())
    {
        return;
    }
    _closeConnectByFd(it->second->fd(), true);
}

void Server::_closeConnectByFd(int connfdTcp, bool force)
{
    if (force)
    {
        struct linger l;
        l.l_onoff = 1; /* cause RST to be sent on close() */
        l.l_linger = 0;
        Setsockopt(connfdTcp, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
    }
    _onTcpDisconnected(connfdTcp);
}

void Server::sendByTcp(UniqID clientId, const uint8_t *m_data, size_t len)
{
    protocol::UserPacket *p = (protocol::UserPacket *)m_data;
    PacketHeader *header = SerializeProtocol<protocol::UserPacket>(*p, len);
    sendByTcp(clientId, header);
}

void Server::sendByTcp(UniqID clientId, PacketHeader *header)
{
    assert(header != nullptr);
    auto it = m_connDict.find(clientId);
    if (it == m_connDict.end())
    {
        return;
    }
    PtrSerConn conn = it->second;
    int ret = send(conn->fd(), (uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen), 0);
    if (ret < 0)
    {
        // should never EMSGSIZE ENOBUFS
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return;
        }
        // close the client
        log_error("[Server][tcp] sendByTcp err %d", errno);
        _closeConnectByFd(conn->fd(), true);
    }
}

void Server::_onTcpConnected(int connfdTcp)
{
    m_net->getLoop().assertInLoopThread();
    EventLoop* ioLoop = m_net->getThreadPool()->getNextLoop();
    uint16_t password = random();
    UniqID clientId = m_clientIdGen.getNewID();
    UniqID convId = m_convIdGen.getNewID();
    m_connDict[clientId] = std::make_shared<SerConn>();
    PtrSerConn conn = m_connDict[clientId];
    conn->clientId = clientId;
    conn->setfd(connfdTcp);
    conn->setEventLoop(ioLoop);
    conn->setKey((password << 16) | convId);
    m_connfd2cid[connfdTcp] = clientId;
    m_convId2cid[convId] = clientId;
    if (onTcpConnected)
        onTcpConnected(this, clientId);
    ioLoop->runInLoop(std::bind(&SerConn::onConnectEstablished, conn));
}

void Server::_onTcpDisconnected(int connfdTcp)
{
    int ret = close(connfdTcp);
    if (ret < 0)
    {
        log_error("[Server][tcp] close err %d", ret);
    }
    m_net->getLoop().deleteFileEvent(connfdTcp, LOOP_EVT_READABLE);
    if (m_connfd2cid.find(connfdTcp) != m_connfd2cid.end())
    {
        UniqID clientId = m_connfd2cid[connfdTcp];
        m_connfd2cid.erase(connfdTcp);
        PtrSerConn conn = m_connDict[clientId];
        m_convId2cid.erase(conn->convId());
        m_connDict.erase(clientId);
        log_info("[Server][tcp] closed, clientId: %d connfdTcp: %d", clientId, connfdTcp);
        if (onTcpDisconnected)
            onTcpDisconnected(this, clientId);
    }
}

void Server::_onTcpMessage(int connfdTcp)
{
    UniqID clientId = m_connfd2cid[connfdTcp];
    PtrSerConn conn = m_connDict[clientId];
    SockBuffer &sockBuffer = conn->buf;
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(connfdTcp, &nreadTotal);
        log_debug("[Server][tcp] readIn connfdTcp %d ret %d nreadTotal %d", connfdTcp, ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            _closeConnectByFd(connfdTcp, true);
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
            case Protocol::UdpHandshake:
            {
                break;
            }
            case Protocol::UserPacket:
            {
                protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->m_data + header->getHeaderLength());
                size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
                // log_debug("getHeaderLength: %d", header->getHeaderLength());
                if (onTcpRecvUserData)
                {
                    onTcpRecvUserData(this, clientId, (uint8_t *)p, dataLength);
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
