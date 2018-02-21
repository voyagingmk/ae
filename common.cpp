#include "common.h"

UDPServer::UDPServer(int port)
{
    int n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    const char *host = NULL;
    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_server error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do
    {
        m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (m_sockfd < 0)
            continue; /* error, try next one */

        Setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(m_sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break; /* success */

        Close(m_sockfd); /* bind error, close and try next one */
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno from final socket() or bind() */
        err_sys("udp_server error for %s, %s", host, serv);

    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
    m_socklen = res->ai_addrlen; /* return size of protocol address */
    freeaddrinfo(ressave);

    char *str = Sock_ntop((struct sockaddr *)&m_sockaddr, m_socklen);
    printf("Server: %s\n", str);
}

UDPServer::~UDPServer()
{
    close(m_sockfd);
}

void UDPServer::Recvfrom()
{
    memset(msg, 0x0, MAX_MSG);
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    const int ret = recvfrom(m_sockfd, msg, MAX_MSG, 0,
                             (struct sockaddr *)&cliAddr, &len);
    if (ret == 0)
        return;
    if (ret < 0)
    {
        err_msg("Recvfrom %d", errno);
        return;
    }

    printf("Recvfrom %s : %s \n",
           Sock_ntop((SA *)&cliAddr, len),
           msg);

    switch (cliAddr.ss_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)&cliAddr;
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&cliAddr;
        break;
    }
    }
}

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
    printf("connected: %s\n", str);
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

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
