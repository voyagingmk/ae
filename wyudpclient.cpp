#include "common.h"
#include "wyudpclient.h"

namespace wynet
{

UDPClient::UDPClient(const char *host, int port)
{
    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_ALL;
    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_client error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do
    {
        if (res->ai_family == PF_INET || res->ai_family == PF_INET6)
        {
            sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd >= 0)
                break; /* success */
        }
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno set from final socket() */
        err_sys("udp_client error %d for %s, %s", errno, host, serv);

    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
    m_socklen = res->ai_addrlen;
    freeaddrinfo(ressave);
    m_sockfd = sockfd;
    m_family = res->ai_family;

    Connect(m_sockfd, (struct sockaddr *)&m_sockaddr, m_socklen);
    char *str = Sock_ntop((struct sockaddr *)&m_sockaddr, m_socklen);
    log_info("connected: %s", str);
}

UDPClient::~UDPClient()
{
    close(m_sockfd);
}

void UDPClient::Send(const char *data, size_t len)
{
    ::Send(m_sockfd, data, len, 0);
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}
};