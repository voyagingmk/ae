#include "common.h"
#include "wyudpserver.h"

namespace wynet
{

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
    log_info("UDP Server created: %s", str);
}

UDPServer::~UDPServer()
{
    close(m_sockfd);
}

void UDPServer::Recvfrom()
{
    /*
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

    log_debug("Recvfrom %s : %s ",
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
    }*/
}
};
