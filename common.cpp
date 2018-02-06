#include "common.h"

UDPServer::UDPServer(int port)
{
    m_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&m_sockaddr, sizeof(m_sockaddr));

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sockaddr.sin_port = htons(port);

    Bind(m_sockfd, (struct sockaddr *)&m_sockaddr, sizeof(m_sockaddr));
}

UDPServer::~UDPServer()
{
    close(m_sockfd);
}

UDPClient::UDPClient(const char *host, int port)
{
    h = gethostbyname(host);
    if (h == NULL)
    {
        printf("unknown host '%s' \n", host);
        exit(1);
    }
    printf("host: '%s'  IP : %s \n", h->h_name,
           inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));
    m_serSockaddr.sin_family = h->h_addrtype;
    memcpy((char *)&m_serSockaddr.sin_addr.s_addr,
           h->h_addr_list[0], h->h_length);
    m_serSockaddr.sin_port = htons(port);
    m_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sockaddr.sin_port = htons(0);

    Bind(m_sockfd, (struct sockaddr *)&m_sockaddr, sizeof(m_sockaddr));
}

UDPClient::~UDPClient()
{
    close(m_sockfd);
}

void UDPClient::Send(const char *data)
{
    Sendto(m_sockfd, data, strlen(data) + 1, 0, (struct sockaddr *)&m_serSockaddr, sizeof(m_serSockaddr));
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

KCPObject::KCPObject(int conv)
{
    m_kcp = ikcp_create(conv, this);
    m_kcp->output = KCPObject::kcp_output;
}

KCPObject::~KCPObject()
{
    ikcp_release(m_kcp);
    m_kcp = NULL;
}

const ikcpcb *KCPObject::kcp()
{
    return m_kcp;
}

void KCPObject::bindSocket(SocketBase *s)
{
    m_socket = s;
}

int KCPObject::getSendWin()
{
    return m_kcp->snd_wnd;
}

int KCPObject::getRecvWin()
{
    return m_kcp->rcv_wnd;
}

void KCPObject::setSendWin(int wnd)
{
    if (wnd > 0)
    {
        m_kcp->snd_wnd = wnd;
    }
}

void KCPObject::setRecvWin(int wnd)
{
    if (wnd > 0)
    {
        m_kcp->rcv_wnd = wnd;
    }
}

void KCPObject::setNodelay(int nodelay, int interval, int resend, int nc)
{
    ikcp_nodelay(m_kcp, nodelay, interval, resend, nc);
}

int KCPObject::send(const char *buf, int len)
{
    int ret = ikcp_send(m_kcp, buf, len);
    if (ret < 0)
    {
        if (ret == -1)
        {
            printf("len < 0 \n");
        }
        if (ret == -2)
        {
            printf("seg == NULL or count > 255 \n");
        }
    }
    return ret;
}

int KCPObject::recv(char *buf, int len)
{
    int ret = ikcp_recv(m_kcp, buf, len);
    if (ret < 0)
    {
        if (ret == -1)
        {
            printf("rcv_queue is empty \n");
        }
        if (ret == -2)
        {
            printf("peeksize < 0 \n");
        }

        if (ret == -2)
        {
            printf("peeksize > len \n");
        }
    }
    return ret;
}

void KCPObject::sendBySocket(const char *buf, int len)
{
    //  Sendto(m_socket->m_sockfd, buf, len, 0, pcliaddr, len);
    // ptr->send_udp_packet(std::string(buf, len), udp_remote_endpoint_);
}