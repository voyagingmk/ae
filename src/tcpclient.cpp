#include "tcpclient.h"
#include "net.h"
#include "utils.h"
#include "socket_utils.h"

namespace wynet
{

void removeConnection(EventLoop *loop, const PtrConn &conn)
{
    log_debug("removeConnection");
    loop->runInLoop(std::bind(&TcpConnection::onDestroy, conn));
}

int TcpClient::onReconnectTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
{

    log_debug("onReconnectTimeout");
    PtrTcpClientEvtListener l = std::dynamic_pointer_cast<TcpClientEventListener>(listener);
    PtrTcpClient tcpClient = l->getTcpClient();
    if (tcpClient)
    {
        log_debug("onReconnectTimeout tcpClient");
        tcpClient->reconnect();
    }
    return -1;
}

// http://man7.org/linux/man-pages/man2/connect.2.html
void TcpClient::OnTcpWritable(EventLoop *eventLoop, PtrEvtListener listener, int mask)
{
    log_debug("TcpClient::OnTcpWritable");
    PtrTcpClientEvtListener l = std::static_pointer_cast<TcpClientEventListener>(listener);
    std::shared_ptr<TcpClient> tcpClient = l->getTcpClient();
    if (!tcpClient)
    {
        log_error("OnTcpWritable no tcpClient");
        return;
    }
    SockFd sockfd = l->getSockFd();
    tcpClient->endAsyncConnect();
    int error;
    socklen_t len;
    len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        log_error("OnTcpWritable getsockopt failed, errno = %d %s", errno, strerror(errno));
        tcpClient->afterAsyncConnect(0);
        return;
    }
    if (error != 0)
    {
        log_error("OnTcpWritable getsockopt SO_ERROR = %d", error);
        tcpClient->afterAsyncConnect(0);
        return;
    }
    if (socketUtils::isSelfConnect(sockfd))
    {
        log_warn("OnTcpWritable self connect");
        tcpClient->afterAsyncConnect(0);
        return;
    }
    log_debug("OnTcpWritable onConnected");
    tcpClient->afterAsyncConnect(sockfd);
}

TcpClient::TcpClient(EventLoop *loop) : onTcpConnectFailed(nullptr),
                                        onTcpConnected(nullptr),
                                        onTcpDisconnected(nullptr),
                                        onTcpRecvMessage(nullptr),
                                        m_asyncConnect(false),
                                        m_reconnectTimes(0),
                                        m_reconnectInterval(200)
{
    log_ctor("TcpClient()");
    m_loop = loop;
}

TcpClient::~TcpClient()
{
    log_dtor("~TcpClient()");
    resetEvtListener();
    PtrConn conn;
    bool unique = false;
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        unique = m_conn.unique();
        conn = m_conn;
        m_conn = nullptr;
        log_debug("conn->use_count() %d", conn.use_count());
    }
    if (conn)
    {
        log_debug("~TcpClient() has conn unique %d", unique);
        assert(m_loop == conn->getLoop());
        TcpConnection::OnTcpClose cb = std::bind(&removeConnection, m_loop, std::placeholders::_1);
        m_loop->runInLoop(
            std::bind(
                &TcpConnection::setCloseCallback,
                conn,
                cb));
        log_debug("~TcpClient() conn->close()");
        conn->close("~TcpClient");
    }
    else if (m_asyncConnect)
    {
        log_debug("~TcpClient() endAsyncConnect");
        endAsyncConnect();
    }
}

void TcpClient::connect(const char *host, int port)
{
    m_loop->runInLoop(std::bind(&TcpClient::connectInLoop, shared_from_this(), host, port));
}

void TcpClient::connectInLoop(const char *host, int port)
{
    m_loop->assertInLoopThread("connectInLoop");
    m_sockAddr = SockAddr(host, port);
    int err;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    if ((err = getaddrinfo(m_sockAddr.getHost(), serv, &hints, &res)) != 0)
    {
        log_warn("TcpClient.getaddrinfo error: %s, %s, %d, %s",
                 m_sockAddr.getHost(), serv, err, gai_strerror(err));
        switch (err)
        {
        case EAI_AGAIN:
        case EAI_NONAME:
            onConnectFailed();
            break;
        case EAI_SYSTEM:
            log_error("EAI_SYSTEM %d", errno);
            break;
        default:
            log_fatal("getaddrinfo %d", err);
        }
        return;
    }
    ressave = res;
    int ret;
    int sockfd;
    do
    {
        sockfd = 0;
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;

        bool needContinue = true;
        do
        {
            socketUtils::setTcpNonBlock(sockfd);
            // socketUtils::SetSockRecvBufSize(sockfd, 32 * 1024);
            // socketUtils::SetSockSendBufSize(sockfd, 32 * 1024);
            ret = ::connect(sockfd, res->ai_addr, res->ai_addrlen);
            log_debug("tcpclient.connect, ret = %d, errno = %s", ret, strerror(errno));
            if (ret == -1)
            {
                if (errno == EINPROGRESS)
                {
                    asyncConnect(sockfd);
                    needContinue = false;
                    break;
                }
                else
                {
                    log_warn("tcpclient.connect, error for %s, %s, %d, %s", host || "", serv, ret, strerror(errno));
                }
            }
            if (ret == 0)
            {
                needContinue = false;
                break;
            }
        } while (0);

        if (needContinue)
        {
            socketUtils ::sock_close(sockfd);
        }
        else
        {
            break;
        }
    } while ((res = res->ai_next) != NULL);

    if (res == NULL)
    {
        log_error("TcpClient.connect failed. %s, %s", host, serv);
        onConnectFailed();
    }
    else
    {
        if (ret == 0)
        {
            onConnected(sockfd);
        }
    }
    if (ressave)
    {
        freeaddrinfo(ressave);
    }
}

EventLoop &TcpClient::getLoop()
{
    return *m_loop;
}

void TcpClient::onConnected(int sockfd)
{
    resetEvtListener();
    m_loop->assertInLoopThread("onConnected");
    MutexLockGuard<MutexLock> lock(m_mutex);
    m_conn = std::make_shared<TcpConnection>(sockfd);
    m_conn->setEventLoop(m_loop);
    m_conn->setCtrl(shared_from_this());
    m_conn->setCallBack_Connected(onTcpConnected);
    m_conn->setCallBack_Message(onTcpRecvMessage);
    m_loop->runInLoop(std::bind(&TcpConnection::onEstablished, m_conn));
}

PtrConn TcpClient::getConn()
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    return m_conn;
}

void TcpClient::disconnect()
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    if (m_conn)
    {
        m_conn->shutdown();
    }
}

void TcpClient::asyncConnect(int sockfd)
{
    m_loop->assertInLoopThread("asyncConnect");
    if (isAsyncConnecting())
    {
        endAsyncConnect();
    }
    log_debug("asyncConnect %d", sockfd);
    m_asyncConnect = true;
    resetEvtListener();
    m_evtListener->setSockfd(sockfd);
    m_evtListener->createFileEvent(LOOP_EVT_WRITABLE, OnTcpWritable);
}

bool TcpClient::isAsyncConnecting()
{
    return m_asyncConnect;
}

void TcpClient::endAsyncConnect()
{
    m_loop->assertInLoopThread("endAsyncConnect");
    log_debug("endAsyncConnect");
    resetEvtListener();
    m_asyncConnect = false;
}

void TcpClient::afterAsyncConnect(int sockfd)
{
    m_loop->assertInLoopThread("afterAsyncConnect");
    if (sockfd)
    {
        onConnected(sockfd);
    }
    else
    {
        onConnectFailed();
    }
}

void TcpClient::onConnectFailed()
{
    if (needReconnect())
    {
        reconnectWithDelay(m_reconnectInterval);
    }
    if (onTcpConnectFailed)
    {
        onTcpConnectFailed(shared_from_this());
    }
}

void TcpClient::onDisconnected(const PtrConn &conn)
{
    if (onTcpDisconnected)
        onTcpDisconnected(conn);
    if (needReconnect())
    {
        reconnectWithDelay(m_reconnectInterval);
    }
}

bool TcpClient::needReconnect()
{
    return m_reconnectTimes == -1 || m_reconnectTimes > 0;
}

void TcpClient::reconnectWithDelay(int ms)
{
    log_info("reconnectWithDelay %d ms", ms);
    m_loop->assertInLoopThread("reconnectWithDelay");
    resetEvtListener();
    m_evtListener->createTimer(ms, onReconnectTimeout, nullptr);
}

void TcpClient::reconnect()
{
    m_loop->assertInLoopThread("reconnect");
    resetEvtListener();
    if (m_reconnectTimes > 0)
    {
        m_reconnectTimes--;
    }
    log_debug("reconnect, left times: %d", m_reconnectTimes);
    connect(m_sockAddr.getHost(), m_sockAddr.getPort());
}

void TcpClient::resetEvtListener()
{
    if (m_evtListener && m_evtListener->getSockFd())
    {
        m_evtListener->deleteAllFileEvent();
        m_evtListener->setSockfd(0);
    }
    if (!m_evtListener)
    {
        m_evtListener = TcpClientEventListener::create();
        m_evtListener->setEventLoop(m_loop);
        m_evtListener->setTcpClient(shared_from_this());
    }
}

}; // namespace wynet
