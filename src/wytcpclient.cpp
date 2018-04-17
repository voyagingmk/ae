#include "wytcpclient.h"
#include "wyclient.h"
#include "wynet.h"
#include "wyutils.h"

namespace wynet
{

// http://man7.org/linux/man-pages/man2/connect.2.html
void OnTcpWritable(EventLoop *eventLoop,
                   int fd, void *clientData, int mask)
{
    log_debug("OnTcpWritable");
    TCPClient *tcpClient = (TCPClient *)(clientData);
    int error;
    socklen_t len;
    len = sizeof(error);
    if (getsockopt(tcpClient->m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        // close it
    }
    else
    {
        Client *client = tcpClient->parent;
        // connect ok, remove event
        client->getNet()->getLoop().deleteFileEvent(tcpClient->m_sockfd, LOOP_EVT_WRITABLE);
        tcpClient->onConnected();
    }
}

TCPClient::TCPClient(Client *client, const char *host, int port)
{
    parent = client;
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
    int i;
    do
    {
        m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (m_sockfd < 0)
            continue; /* ignore this one */

        int flags = Fcntl(m_sockfd, F_GETFL, 0);
        Fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

        SetSockRecvBufSize(m_sockfd, 32 * 1024);
        SetSockSendBufSize(m_sockfd, 32 * 1024);

        i = connect(m_sockfd, res->ai_addr, res->ai_addrlen);

        if ((i == -1) && (errno == EINPROGRESS))
        {
            client->getNet()->getLoop().createFileEvent(m_sockfd, LOOP_EVT_WRITABLE,
                                              OnTcpWritable, (void *)this);
            break;
        }
        if (i == 0)
            break; /* success */

        ::Close(m_sockfd); /* ignore this one */
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno set from final connect() */
        err_sys("tcp_connect error for %s, %s", host, serv);

    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
    m_socklen = res->ai_addrlen; /* return size of protocol address */

    freeaddrinfo(ressave);
    if (i == 0)
    {
        onConnected();
    }
}

void TCPClient::Close()
{
    if (!connected)
    {
        return;
    }
    log_info("tcp client closed");
    connected = false;
    close(m_sockfd);
    parent->_onTcpDisconnected();
}

void TCPClient::Recvfrom()
{
}

void TCPClient::Send(uint8_t *data, size_t len)
{
    int ret = send(m_sockfd, data, len, 0);
    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return;
        }
        Close();
    }
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}

void TCPClient::onConnected()
{
    connected = true;
    parent->_onTcpConnected();
}
};
