#include "wyconnection.h"
#include "wyutils.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wysockbuffer.h"

using namespace wynet;

int testOnTimerEvent(EventLoop *loop, TimerRef tr, std::weak_ptr<FDRef> fdRef, void *data)
{
    log_debug("[conn] testOnTimerEvent %lld", tr.Id());
    return LOOP_EVT_NOMORE;
}

void OnConnectionEvent(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return;
    }
    std::shared_ptr<TcpConnection> conn = std::dynamic_pointer_cast<TcpConnection>(sfdRef);
    if (mask & LOOP_EVT_READABLE)
    {
        log_debug("[conn] onReadable connfd=%d", conn->connectFd());
        conn->onReadable();
    }
    if (mask & LOOP_EVT_WRITABLE)
    {
        log_debug("[conn] onWritable connfd=%d", conn->connectFd());
        conn->onWritable();
    }
    // eventLoop->createTimerInLoop(1000, testOnTimerEvent, std::weak_ptr<FDRef>(), nullptr);
}

void TcpConnection ::close(bool force)
{
    getLoop()->assertInLoopThread();
    getLoop()->deleteFileEvent(shared_from_this(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
    if (force)
    {
        struct linger l;
        l.l_onoff = 1; /* cause RST to be sent on close() */
        l.l_linger = 0;
        Setsockopt(connectFd(), SOL_SOCKET, SO_LINGER, &l, sizeof(l));
    }

    // _onTcpDisconnected(connfdTcp);
}

void TcpConnection::onEstablished()
{
    getLoop()->assertInLoopThread();
    getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_READABLE, OnConnectionEvent);
    onTcpConnected(shared_from_this());
}

void TcpConnection::onReadable()
{
    getLoop()->assertInLoopThread();
    SockBuffer &sockBuf = sockBuffer();
    int ret = sockBuf.readIn(connectFd());
    log_debug("[conn] readIn connectFd %d ret %d", connectFd(), ret);
    if (ret <= 0)
    {
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            log_error("[conn] readIn error: %d", errno);
        }
        // has unknown error or has closed
        close(false);
        return;
    }
    onTcpRecvMessage(shared_from_this(), sockBuf);
    /*
        BufferRef &bufRef = sockBuf.getBufRef();
        PacketHeader *header = (PacketHeader *)(bufRef->data());
        Protocol protocol = static_cast<Protocol>(header->getProtocol());
        switch (protocol)
        {
        case Protocol::UdpHandshake:
        {
            break;
        }
        case Protocol::UserPacket:
        {
            protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->data() + header->getHeaderLength());
            size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
            // log_debug("getHeaderLength: %d", header->getHeaderLength());
           
                if (onTcpRecvMessage)
                {
                    onTcpRecvMessage(shared_from_this(), conn, (uint8_t *)p, dataLength);
                }
            break;
        }
        default:
            break;
        }
        sockBuf.resetBuffer();
    }*/
}

void TcpConnection::onWritable()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void TcpConnectionForServer::send(const uint8_t *data, size_t len)
{
    // getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_WRITABLE, OnConnectionEvent);
}

/*
void TcpConnectionForServer::onEstablished()
{
    protocol::TcpHandshake handshake;
    handshake.connectId = connectId();
    handshake.udpPort = udpPort();
    handshake.key = key();
    sendByTcp(connectId, SerializeProtocol<protocol::TcpHandshake>(handshake));
    log_info("[Server][tcp] connected, connectId: %d, connfdTcp: %d, key: %d", connectId(), connectFd(), handshake.key);

    // LogSocketState(fd());
}

void TcpConnectionForServer::onWritable()
{
}

void TcpConnectionForServer::onReadable()
{
}


void TcpConnectionForClient::onEstablished()
{
    getLoop()->assertInLoopThread();
    getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_READABLE, OnConnectionEvent);
}

void TcpConnectionForClient::onReadable()
{
}

void TcpConnectionForClient::onWritable()
{
}

*/