#include "wytcpclient.h"
#include "wyclient.h"
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
    std::shared_ptr<TCPClient> tcpClient = std::dynamic_pointer_cast<TCPClient>(sfdRef);
    tcpClient->_onTcpMessage();
}

// http://man7.org/linux/man-pages/man2/connect.2.html
void OnTcpWritable(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    log_debug("OnTcpWritable");
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return;
    }
    std::shared_ptr<TCPClient> tcpClient = std::dynamic_pointer_cast<TCPClient>(sfdRef);
    int error;
    socklen_t len;
    len = sizeof(error);
    if (getsockopt(tcpClient->sockfd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        // close it
    }
    else
    {
        PtrClient client = tcpClient->m_parent;
        // connect ok, remove event
        client->getNet()->getLoop().deleteFileEvent(tcpClient, LOOP_EVT_WRITABLE);
        tcpClient->onConnected();
    }
}

TCPClient::TCPClient(PtrClient client) : onTcpConnected(nullptr),
                                         onTcpDisconnected(nullptr),
                                         onTcpRecvMessage(nullptr)
{

    m_parent = client;
}

void TCPClient::connect(const char *host, int port)
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
        err_quit("tcp_connect error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;
    int ret;
    do
    {
        setSockfd(socket(res->ai_family, res->ai_socktype, res->ai_protocol));
        if (sockfd() < 0)
            continue; /* ignore this one */

        int flags = Fcntl(sockfd(), F_GETFL, 0);
        Fcntl(sockfd(), F_SETFL, flags | O_NONBLOCK);

        SetSockRecvBufSize(sockfd(), 32 * 1024);
        SetSockSendBufSize(sockfd(), 32 * 1024);

        ret = ::connect(sockfd(), res->ai_addr, res->ai_addrlen);
        log_debug("connect: %d", ret);
        if ((ret == -1) && (errno == EINPROGRESS))
        {
            m_parent->getNet()->getLoop().createFileEvent(shared_from_this(), LOOP_EVT_WRITABLE,
                                                          OnTcpWritable);
            break;
        }
        if (ret == 0)
            break; /* success */

        ::Close(sockfd()); /* ignore this one */
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno set from final connect() */
        err_sys("tcp_connect error for %s, %s", host, serv);

    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
    m_socklen = res->ai_addrlen; /* return size of protocol address */

    freeaddrinfo(ressave);
    if (ret == 0)
    {
        onConnected();
    }
}

void TCPClient::Close()
{
    if (!m_connected)
    {
        return;
    }
    log_info("tcp client closed");
    m_connected = false;
    close(sockfd());
    _onTcpDisconnected();
}

void TCPClient::Recvfrom()
{
}

void TCPClient::Send(uint8_t *data, size_t len)
{
    int ret = send(sockfd(), data, len, 0);
    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return;
        }
        Close();
    }
    // Sendto(sockfd(), data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}

EventLoop &TCPClient::getLoop()
{
    return m_parent->getNet()->getLoop();
}

void TCPClient::onConnected()
{
    m_connected = true;
    _onTcpConnected();
}

void TCPClient::_onTcpConnected()
{
    m_conn = std::make_shared<CliConn>(shared_from_this(), sockfd());
    m_conn->setEventLoop(&m_parent->getNet()->getLoop());
	m_conn->setCallBack_Connected(onTcpConnected);
	m_conn->setCallBack_Disconnected(onTcpDisconnected);
	m_conn->setCallBack_Message(onTcpRecvMessage);
	m_parent->getNet()->getLoop().runInLoop(std::bind(&TcpConnection::onEstablished, m_conn));
}

void TCPClient::_onTcpDisconnected()
{
    m_conn = nullptr;
}

void TCPClient::_onTcpMessage()
{
}
};
