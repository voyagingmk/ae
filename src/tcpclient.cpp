#include "tcpclient.h"
#include "client.h"
#include "net.h"
#include "utils.h"
#include "socket_utils.h"

namespace wynet
{

void removeConnection(EventLoop *loop, const PtrConn &conn)
{
    log_info("------removeConnection");
    loop->queueInLoop(std::bind(&TcpConnection::onDestroy, conn));
}

// http://man7.org/linux/man-pages/man2/connect.2.html
void TcpClient::OnTcpWritable(EventLoop *eventLoop, PtrEvtListener listener, int mask)
{
    log_debug("TcpClient::OnTcpWritable");
    PtrTcpClientEvtListener l = std::static_pointer_cast<TcpClientEventListener>(listener);
    std::shared_ptr<TcpClient> tcpClient = l->getTcpClient();
    if (!tcpClient)
    {
        log_debug("OnTcpWritable no tcpClient");
        return;
    }
    SockFd asyncSockfd = tcpClient->m_asyncSockfd;
    tcpClient->endAsyncConnect();
    int error;
    socklen_t len;
    len = sizeof(error);
    if (getsockopt(asyncSockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        log_debug("OnTcpWritable getsockopt failed, errno = %d", strerror(errno));
        tcpClient->disconnect();
        return;
    }
    if (error != 0)
    {
        log_debug("OnTcpWritable getsockopt SO_ERROR = %d", error);
        tcpClient->disconnect();
        return;
    }
    if (socketUtils::isSelfConnect(asyncSockfd))
    {
        log_warn("OnTcpWritable isSelfConnect = true");
        return;
    }

    log_debug("OnTcpWritable _onTcpConnected");
    // connect ok, remove event
    tcpClient->_onTcpConnected(asyncSockfd);
}

TcpClient::TcpClient(EventLoop *loop) : onTcpConnected(nullptr),
                                        onTcpDisconnected(nullptr),
                                        onTcpRecvMessage(nullptr),
                                        m_asyncConnect(false),
                                        m_asyncSockfd(0)
{
    if (LOG_CTOR_DTOR)
        log_info("TcpClient()");
    m_loop = loop;
}

TcpClient::~TcpClient()
{
    if (LOG_CTOR_DTOR)
        log_info("~TcpClient()");
    PtrConn conn;
    bool unique = false;
    {
        MutexLockGuard<MutexLock> lock(m_mutex);
        unique = m_conn.unique();
        conn = m_conn;
        m_conn = nullptr;
        log_info("conn->use_count() %d", conn.use_count());
    }
    if (conn)
    {
        log_info("~TcpClient() has conn unique %d", unique);
        assert(m_loop == conn->getLoop());
        TcpConnection::OnTcpClose cb = std::bind(&removeConnection, m_loop, std::placeholders::_1);
        m_loop->runInLoop(
            std::bind(
                &TcpConnection::setCloseCallback,
                conn,
                cb));
        if (unique)
        {
            log_info("~TcpClient() conn->close()");
            conn->close();
        }
    }
    else if (m_asyncConnect)
    {
        log_info("~TcpClient() endAsyncConnect");
        endAsyncConnect();
    }
}

void TcpClient::connect(const char *host, int port)
{
    m_loop->runInLoop(std::bind(&TcpClient::connectInLoop, shared_from_this(), host, port));
}

void TcpClient::connectInLoop(const char *host, int port)
{
    int n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        log_fatal("TcpClient.getaddrinfo error: %s, %s: %s",
                  host, serv, gai_strerror(n));
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
    }
    else
    {
        memcpy(&m_sockAddr.m_addr, res->ai_addr, res->ai_addrlen);
        m_sockAddr.m_socklen = res->ai_addrlen;
        if (ret == 0)
        {
            _onTcpConnected(sockfd);
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

void TcpClient::_onTcpConnected(int sockfd)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    m_conn = std::make_shared<TcpConnection>(sockfd);
    m_conn->setEventLoop(m_loop);
    m_conn->setCtrl(shared_from_this());
    m_conn->setCallBack_Connected(onTcpConnected);
    m_conn->setCallBack_Disconnected(onTcpDisconnected);
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
    if (isAsyncConnecting())
    {
        endAsyncConnect();
    }
    log_debug("asyncConnect %d", sockfd);
    m_evtListener = TcpClientEventListener::create();
    m_evtListener->setName(std::string("asyncConnect"));
    m_evtListener->setEventLoop(m_loop);
    m_evtListener->setTcpClient(shared_from_this());
    m_evtListener->setSockfd(sockfd);
    m_evtListener->createFileEvent(LOOP_EVT_WRITABLE, OnTcpWritable);
    m_asyncSockfd = sockfd;
}

bool TcpClient::isAsyncConnecting()
{
    return m_asyncConnect;
}

void TcpClient::endAsyncConnect()
{
    log_info("endAsyncConnect");
    if (m_evtListener)
    {
        m_evtListener->deleteAllFileEvent();
    }
    m_evtListener = nullptr;
    m_asyncConnect = false;
    m_asyncSockfd = 0;
}

}; // namespace wynet
