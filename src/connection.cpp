#include "connection.h"
#include "utils.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"
#include "sockbuffer.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include "socket_utils.h"

using namespace wynet;

int testOnTimerEvent(EventLoop *loop, TimerRef tr, PtrEvtListener listener, void *data)
{
    log_debug("[conn] testOnTimerEvent %lld", tr.Id());
    return MP_HALT;
}

TcpConnection::TcpConnection(SockFd sockfd) : m_loop(nullptr),
                                              m_sockFdCtrl(sockfd),
                                              m_evtListener(nullptr),
                                              m_ctrlType(0),
                                              m_state(State::Connecting),
                                              m_key(0),
                                              m_kcpObj(nullptr),
                                              m_connectId(0),
                                              m_pendingRecvBuf("pendingRecvBuf"),
                                              m_pendingSendBuf("pendingSendBuf"),
                                              m_shutdownWrite(false),
                                              onTcpConnected(nullptr),
                                              onTcpRecvMessage(nullptr),
                                              onTcpSendComplete(nullptr),
                                              onTcpClose(nullptr)
{
    log_ctor("TcpConnection() %d", sockfd);
    m_evtListener = TcpConnectionEventListener::create(sockfd);
    m_evtListener->setName(std::string("TcpConnectionEventListener") + std::to_string(sockfd));
}

TcpConnection::~TcpConnection()
{
    log_dtor("~TcpConnection() %d", m_sockFdCtrl.sockfd());
    assert(m_state == State::Disconnected);
}

void TcpConnection::OnConnectionEvent(EventLoop *eventLoop, const PtrEvtListener &listener, int mask)
{
    log_debug("[conn] OnConnectionEvent");
    assert(listener->hasFileEvent(mask));
    PtrConnEvtListener l = std::static_pointer_cast<TcpConnectionEventListener>(listener);
    if (!l)
    {
        log_fatal("[conn] no l");
        return;
    }
    PtrConn conn = l->getTcpConnection();
    if (!conn)
    {
        log_fatal("[conn] no conn");
        return;
    }
    assert(conn->m_evtListener == listener);
    bool handled = false;
    if ((mask & MP_WRITABLE) && listener->hasFileEvent(MP_WRITABLE))
    {
        handled = true;
        log_debug("[conn] onWritable sockfd=%d", conn->sockfd());
        conn->onWritable();
    }
    // 可读事件可能会把连接关了，要后处理
    if ((mask & MP_READABLE) && listener->hasFileEvent(MP_READABLE))
    {
        handled = true;
        log_debug("[conn] onReadable sockfd=%d", conn->sockfd());
        conn->onReadable();
    }

    if (!handled)
    {
        log_fatal("OnConnectionEvent");
    }
    // m_evtListener->createTimer(1000, testOnTimerEvent, nullptr);
}

void TcpConnection ::shutdown()
{
    if (m_state == State::Connected || m_state == State::Disconnecting)
    {
        m_state = State::Disconnecting;
        const PtrConn &conn = shared_from_this();
        getLoop()->runInLoop(std::bind(&TcpConnection::shutdownInLoop, conn));
    }
    else if (m_state == State::Connecting)
    {
        log_fatal("TcpConnection shutdown m_state = Connecting");
    }
}

void TcpConnection ::shutdownInLoop()
{
    // log_info("[conn] shutdownInLoop %d", sockfd());
    if (m_state == State::Connected || m_state == State::Disconnecting)
    {
        getLoop()->assertInLoopThread("shutdownInLoop");
        // shutdown WR only if not writing, in order to send out pendingBuf
        if (!m_shutdownWrite && !m_evtListener->hasFileEvent(MP_WRITABLE))
        {
            assert(m_pendingSendBuf.readableSize() == 0);
            if (::shutdown(sockfd(), SHUT_WR) < 0)
            {
                log_error("shutdown SHUT_WR failed %d %s", errno, strerror(errno));
            }
            else
            {
                log_debug("shutdown SHUT_WR");
            }
            m_shutdownWrite = true;
            assert(m_evtListener->hasFileEvent(MP_READABLE));
        }
    }
    else
    {
        State tmp = m_state;
        log_fatal("TcpConnection shutdownInLoop m_state: %d", static_cast<int>(tmp));
    }
}

void TcpConnection ::forceClose()
{
    if (m_state == State::Connected || m_state == State::Disconnecting)
    {
        m_state = State::Disconnecting;
        getLoop()->runInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection ::forceCloseInLoop()
{
    getLoop()->assertInLoopThread("forceCloseInLoop");
    if (m_state == State::Connected || m_state == State::Disconnecting)
    {
        close("force");
    }
}

void TcpConnection ::close(const char *reason)
{
    log_debug("[conn] close() isInLoopThread: %d", getLoop()->isInLoopThread());
    if (reason)
    {
        log_debug("[conn] close() reason %s", reason);
    }
    if (getLoop()->isInLoopThread())
    {
        closeInLoop();
    }
    else
    {
        getLoop()->runInLoop(
            std::bind(&TcpConnection::closeInLoop,
                      shared_from_this()));
    }
}

void TcpConnection ::closeInLoop()
{
    getLoop()->assertInLoopThread("closeInLoop");
    onDestroy();
}

void TcpConnection::setCloseCallback(const OnTcpClose &cb)
{
    onTcpClose = cb;
}

void TcpConnection::onDestroy()
{
    // log_info("[conn] onDestroy");
    getLoop()->assertInLoopThread("onDestroy");
    if (m_state == State::Disconnected)
    {
        return;
    }
    m_state = State::Disconnected;
    m_evtListener->deleteAllFileEvent();
    m_evtListener = nullptr;
    log_debug("[conn] m_state = State::Disconnected");
    PtrConn self(shared_from_this());
    if (m_ctrlType == 1)
    {
        getCtrlAsServer()->onDisconnected(self);
    }
    else if (m_ctrlType == 2)
    {
        getCtrlAsClient()->onDisconnected(self);
    }
    if (onTcpClose)
        onTcpClose(self);
}

void TcpConnection::onEstablished()
{
    getLoop()->assertInLoopThread("onEstablished");
    // assert(m_state == State::Connecting);
    log_debug("[conn] establish in thread: %s", CurrentThread::name());
    State expectedState = State::Connecting;
    if (!m_state.compare_exchange_weak(expectedState, State::Connected))
    {
        log_fatal("onEstablished %d", static_cast<int>(expectedState));
    }
    m_evtListener->setTcpConnection(shared_from_this());
    m_evtListener->createFileEvent(MP_READABLE, TcpConnection::OnConnectionEvent);
    if (onTcpConnected)
        onTcpConnected(shared_from_this());
}

void TcpConnection::onReadable()
{
    getLoop()->assertInLoopThread("onReadable");
    if (m_state == State::Disconnected)
    {
        log_fatal("onReadable Disconnected");
        return;
    }
    SockBuffer &sockBuf = sockBuffer();
    int ret = sockBuf.readIn(sockfd());
    log_debug("[conn] readIn sockfd %d ret %d free %d", sockfd(), ret, sockBuf.tailFreeSize());
    if (ret <= 0)
    {
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            log_error("[conn] readIn error: %d %s", errno, strerror(errno));
        }
        // has unknown error or has closed
        close(ret == 0 ? "readInClose" : "readInError");
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
    getLoop()->assertInLoopThread("onWritable");
    if (m_evtListener)
    {
        assert(m_evtListener->hasFileEvent(MP_WRITABLE));
    }
    if (m_state != State::Connected && m_state != State::Disconnecting)
    {
        return;
    }
    int remain = m_pendingSendBuf.readableSize();
    if (remain <= 0)
    {
        if (m_evtListener)
        {
            m_evtListener->deleteFileEvent(MP_WRITABLE);
        }
        log_error("TcpConnection::onWritable remain <= 0 %d", remain);
        return;
    }
    if (m_shutdownWrite)
    {
        log_fatal("TcpConnection::onWritable after shutdownWrite");
        return;
    }
    assert(!m_shutdownWrite);
    int nwrote = ::write(sockfd(), m_pendingSendBuf.readBegin(), remain);
    // log_info("write2 %d %d", remain, nwrote);
    log_debug("[conn] onWritable, remain:%d, nwrote:%d", remain, nwrote);
    if (nwrote > 0)
    {
        m_pendingSendBuf.readOut(nwrote);
        if (m_pendingSendBuf.readableSize() == 0)
        {
            m_evtListener->deleteFileEvent(MP_WRITABLE);
            if (onTcpSendComplete)
            {
                getLoop()->runInLoop(std::bind(onTcpSendComplete, shared_from_this()));
            }
            if (m_state == State::Disconnecting)
            {
                shutdownInLoop();
            }
        }
    }
    else if (nwrote == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        log_error("[conn] onWritable send error: %d %s", errno, strerror(errno));
        // has unknown error or has closed
        close("writeError");
    }
    else
    {
        log_fatal("write 0 bytes");
    }
}

void TcpConnection::send(const uint8_t *data, const size_t len)
{
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
    getLoop()->assertInLoopThread("sendInLoop");
    if (m_state != State::Connected)
    {
        return;
    }
    if (m_pendingSendBuf.readableSize() > 0)
    {
        m_pendingSendBuf.append(data, len);
    }
    else
    {
        // write directly
        assert(!m_shutdownWrite);
        int nwrote = ::write(sockfd(), data, len);
        //   log_info("write1 %d %d", len, nwrote);
        log_debug("[conn] sendInLoop send nwrote: %d", nwrote);
        if (nwrote > 0)
        {
            int remain = len - nwrote;
            log_debug("[conn] sendInLoop sockfd %d, len:%d, nwrote:%d, remain:%d", sockfd(), len, nwrote, remain);
            if (remain > 0)
            {
                m_pendingSendBuf.append(data + nwrote, remain);
                log_debug("[conn] remain > 0 sockfd %d", sockfd());
            }
            else
            {
                if (onTcpSendComplete)
                {
                    getLoop()->runInLoop(std::bind(onTcpSendComplete, shared_from_this()));
                }
                if (m_state == State::Disconnecting)
                {
                    m_evtListener->deleteFileEvent(MP_WRITABLE);
                    shutdownInLoop();
                }
            }
        }
        else if (nwrote == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                m_evtListener->createFileEvent(MP_WRITABLE, TcpConnection::OnConnectionEvent);
                return;
            }
            log_error("[conn] sendInLoop send error: %d", errno);
            // has unknown error or has closed
            close("writeError");
        }
    }
}

bool TcpConnection::isPending()
{
    return getPendingSize() > 0;
}

int TcpConnection::getPendingSize()
{
    return m_pendingSendBuf.readableSize();
}
