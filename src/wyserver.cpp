#include "wyserver.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wyutils.h"
#include "wynet.h"
#include "eventloop.h"

namespace wynet
{

int testOnTimerEvent(EventLoop *loop, TimerRef tr, void *userData)
{
    printf("testOnTimerEvent %lld\n", tr.Id());
    return LOOP_EVT_NOMORE;
}

void onTcpMessage(EventLoop *eventLoop,
                  int connfdTcp, void *clientData, int mask)
{
    log_debug("onTcpMessage connfd=%d", connfdTcp);
    Server *server = (Server *)(clientData);
    server->_onTcpMessage(connfdTcp);

    eventLoop->createTimerInLoop(1000, testOnTimerEvent, NULL);
}

void OnTcpNewConnection(EventLoop *eventLoop, int listenfdTcp, void *clientData, int mask)
{
    Server *server = (Server *)(clientData);
    int sockfd = server->tcpServer.m_sockfd;
    assert(sockfd == listenfdTcp);
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

void OnUdpMessage(EventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    // Server *server = (Server *)(clientData);
    // server->udpServer.Recvfrom();
}

Server::Server(WyNet *net, int tcpPortArg, int udpPortArg) : net(net),
                                                             tcpPort(tcpPortArg),
                                                             udpPort(udpPortArg),
                                                             tcpServer(tcpPort),
                                                             udpServer(udpPort),
                                                             onTcpConnected(nullptr),
                                                             onTcpDisconnected(nullptr),
                                                             onTcpRecvUserData(nullptr)
{

    convIdGen.setRecycleThreshold(2 << 15);
    convIdGen.setRecycleEnabled(true);

    log_info("[Server] created, tcp sockfd: %d, udp sockfd: %d",
             tcpServer.m_sockfd,
             udpServer.m_sockfd);

    net->loop.createFileEvent(tcpServer.m_sockfd, LOOP_EVT_READABLE,
                              OnTcpNewConnection, (void *)this);

    if (udpServer.valid())
    {
        net->loop.createFileEvent(udpServer.m_sockfd, LOOP_EVT_READABLE,
                                  OnUdpMessage, (void *)this);
    }
    LogSocketState(tcpServer.m_sockfd);
}

Server::~Server()
{
    net->loop.deleteFileEvent(tcpServer.m_sockfd, LOOP_EVT_READABLE);
    net->loop.deleteFileEvent(udpServer.m_sockfd, LOOP_EVT_READABLE);
    net = nullptr;
    log_info("[Server] destoryed.");
}

void Server::CloseConnect(UniqID clientId)
{
    auto it = connDict.find(clientId);
    if (it == connDict.end())
    {
        return;
    }
    CloseConnectByFd(it->second.connfdTcp, true);
}

void Server::CloseConnectByFd(int connfdTcp, bool force)
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

void Server::SendByTcp(UniqID clientId, const uint8_t *m_data, size_t len)
{
    protocol::UserPacket *p = (protocol::UserPacket *)m_data;
    PacketHeader *header = SerializeProtocol<protocol::UserPacket>(*p, len);
    SendByTcp(clientId, header);
}

void Server::SendByTcp(UniqID clientId, PacketHeader *header)
{
    assert(header != nullptr);
    auto it = connDict.find(clientId);
    if (it == connDict.end())
    {
        return;
    }
    ConnectionForServer &conn = it->second;
    int ret = send(conn.connfdTcp, (uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen), 0);
    if (ret < 0)
    {
        // should never EMSGSIZE ENOBUFS
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return;
        }
        // close the client
        log_error("[Server][tcp] SendByTcp err %d", errno);
        CloseConnectByFd(conn.connfdTcp, true);
    }
}

void Server::_onTcpConnected(int connfdTcp)
{
    net->loop.createFileEvent(connfdTcp, LOOP_EVT_READABLE,
                              onTcpMessage, this);

    UniqID clientId = clientIdGen.getNewID();
    UniqID convId = convIdGen.getNewID();
    connDict[clientId] = ConnectionForServer();
    ConnectionForServer &conn = connDict[clientId];
    conn.connfdTcp = connfdTcp;
    uint16_t password = random();
    conn.key = (password << 16) | convId;
    connfd2cid[connfdTcp] = clientId;
    convId2cid[convId] = clientId;

    protocol::TcpHandshake handshake;
    handshake.clientId = clientId;
    handshake.udpPort = udpPort;
    handshake.key = conn.key;
    SendByTcp(clientId, SerializeProtocol<protocol::TcpHandshake>(handshake));

    log_info("[Server][tcp] connected, clientId: %d, connfdTcp: %d, key: %d", clientId, connfdTcp, handshake.key);
    LogSocketState(connfdTcp);
    if (onTcpConnected)
        onTcpConnected(this, clientId);
}

void Server::_onTcpDisconnected(int connfdTcp)
{
    int ret = close(connfdTcp);
    if (ret < 0)
    {
        log_error("[Server][tcp] close err %d", ret);
    }
    net->loop.deleteFileEvent(connfdTcp, LOOP_EVT_READABLE);
    if (connfd2cid.find(connfdTcp) != connfd2cid.end())
    {
        UniqID clientId = connfd2cid[connfdTcp];
        connfd2cid.erase(connfdTcp);
        ConnectionForServer &conn = connDict[clientId];
        convId2cid.erase(conn.convId());
        connDict.erase(clientId);
        log_info("[Server][tcp] closed, clientId: %d connfdTcp: %d", clientId, connfdTcp);
        if (onTcpDisconnected)
            onTcpDisconnected(this, clientId);
    }
}

void Server::_onTcpMessage(int connfdTcp)
{
    UniqID clientId = connfd2cid[connfdTcp];
    ConnectionForServer &conn = connDict[clientId];
    SockBuffer &sockBuffer = conn.buf;
    do
    {
        int nreadTotal = 0;
        int ret = sockBuffer.readIn(connfdTcp, &nreadTotal);
        log_debug("[Server][tcp] readIn connfdTcp %d ret %d nreadTotal %d", connfdTcp, ret, nreadTotal);
        if (ret <= 0)
        {
            // has error or has closed
            CloseConnectByFd(connfdTcp, true);
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
