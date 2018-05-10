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

void TcpConnection::OnConnectionEvent(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
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
    if (getLoop()->isInLoopThread())
    {
        log_debug("[conn] close, already in loop");
        closeInLoop(force);
    }
    else
    {
        log_debug("[conn] close, not in loop");
        getLoop()->runInLoop(
            std::bind(&TcpConnection::closeInLoop,
                      shared_from_this(),
                      force));
    }
}

void TcpConnection ::closeInLoop(bool force)
{
    getLoop()->assertInLoopThread();
    if (m_state == State::Disconnected)
    {
        return;
    }
    log_debug("[conn] close in thread: %s", CurrentThread::name());
    m_state = State::Disconnected;
    getLoop()->deleteFileEvent(shared_from_this(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
    if (force)
    {
        struct linger l;
        l.l_onoff = 1; /* cause RST to be sent on close() */
        l.l_linger = 0;
        Setsockopt(connectFd(), SOL_SOCKET, SO_LINGER, &l, sizeof(l));
    }
    if (onTcpDisconnected)
        onTcpDisconnected(shared_from_this());
}

void TcpConnection::onEstablished()
{
    getLoop()->assertInLoopThread();
    assert(m_state == State::Connecting);
    log_debug("[conn] establish in thread: %s", CurrentThread::name());
    m_state = State::Connected;
    getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_READABLE, TcpConnection::OnConnectionEvent);
    if (onTcpConnected)
        onTcpConnected(shared_from_this());
}

void TcpConnection::onReadable()
{
    getLoop()->assertInLoopThread();
    SockBuffer &sockBuf = sockBuffer();
    int ret = sockBuf.readIn(connectFd());
    log_debug("[conn] onReadable, readIn connectFd %d ret %d", connectFd(), ret);
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
        closeInLoop(false);
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
    getLoop()->assertInLoopThread();
    int remain = m_pendingBuf.readableSize();
    log_debug("[conn] onWritable, remain:%d", remain);
    int nwrote = ::send(fd(), m_pendingBuf.readBegin(), m_pendingBuf.readableSize(), 0);
    log_debug("[conn] onWritable, nwrote:%d", nwrote);
    if (nwrote > 0)
    {
        m_pendingBuf.readOut(nwrote);
        if (m_pendingBuf.readableSize() == 0)
        {
            log_debug("[conn] onWritable, no remain");
            getLoop()->deleteFileEvent(shared_from_this(), LOOP_EVT_WRITABLE);
            if (onTcpSendComplete)
            {
                getLoop()->queueInLoop(std::bind(onTcpSendComplete, shared_from_this()));
            }
        }
    }
    else if (nwrote == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        log_error("[conn] onWritable send error: %d", errno);
        // has unknown error or has closed
        closeInLoop(false);
    }
}

void TcpConnection::send(const uint8_t *data, const size_t len)
{
    if (m_state != State::Connected)
    {
        return;
    }
    if (getLoop()->isInLoopThread())
    {
        log_debug("[conn] send, already in loop");
        sendInLoop(data, len);
    }
    else
    {
        log_debug("[conn] send, not in loop");
        getLoop()->runInLoop(
            std::bind((void (TcpConnection::*)(const std::string &)) & TcpConnection::sendInLoop,
                      shared_from_this(),
                      std::string((const char *)data, len)));
    }
}

void TcpConnection::send(const std::string &msg)
{
    send((const uint8_t *)(msg.data()), msg.size());
}

void TcpConnection::sendInLoop(const std::string &msg)
{
    sendInLoop((const uint8_t *)msg.data(), msg.size());
}

void TcpConnection::sendInLoop(const uint8_t *data, const size_t len)
{
    getLoop()->assertInLoopThread();
    if (m_state != State::Connected)
    {
        return;
    }
    if (m_pendingBuf.readableSize() > 0)
    {
        m_pendingBuf.append(data, len);
    }
    else
    {
        // write directly
        int nwrote = ::send(fd(), data, len, 0);
        if (nwrote > 0)
        {
            int remain = len - nwrote;
            log_debug("[conn] send, len:%d, nwrote:%d, remain:%d", len, nwrote, remain);
            if (remain > 0)
            {
                m_pendingBuf.append(data + nwrote, remain);
                getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_WRITABLE, TcpConnection::OnConnectionEvent);
            }
            else
            {
                if (onTcpSendComplete)
                {
                    getLoop()->queueInLoop(std::bind(onTcpSendComplete, shared_from_this()));
                }
            }
        }
        else if (nwrote == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            log_error("[conn] sendInLoop send error: %d", errno);
            // has unknown error or has closed
            closeInLoop(false);
        }
    }
}

bool TcpConnection::isPending()
{
    return getPendingSize() > 0;
}

int TcpConnection::getPendingSize()
{
    int remain = m_pendingBuf.readableSize();
    return remain;
}
