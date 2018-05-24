#include "common.h"
#include "udpserver.h"

namespace wynet
{

UDPServer::UDPServer(int port)
{
    if (port > 0)
    {
        init(port);
    }
}

void UDPServer::init(int port)
{
    /*
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
        log_fatal("udp_server error for %s, %s: %s",
                  host, serv, gai_strerror(n));
    ressave = res;

    do
    {
        setSockfd(socket(res->ai_family, res->ai_socktype, res->ai_protocol));
        if (sockfd() < 0)
            continue; 

    socketUtils ::sock_setsockopt(sockfd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(sockfd(), res->ai_addr, res->ai_addrlen) == 0)
        break; 

        close(sockfd()); 
    }
    while ((res = res->ai_next) != NULL)
        ;

    if (res == NULL) 
        err_sys("udp_server error for %s, %s", host, serv);

    memcpy(&m_sockAddr.m_addr, res->ai_addr, res->ai_addrlen);
    m_sockAddr.m_socklen = res->ai_addrlen;

    freeaddrinfo(ressave);
    */
} // namespace wynet

UDPServer::~UDPServer()
{
    // close(sockfd());
}

void UDPServer::Recvfrom()
{
    /*
    memset(msg, 0x0, MAX_MSG);
    struct sockaddr_storage cliAddr;
    socklen_t len = sizeof(cliAddr);
    const int ret = recvfrom(sockfd(), msg, MAX_MSG, 0,
                             (struct sockaddr *)&cliAddr, &len);
    if (ret == 0)
        return;
    if (ret < 0)
    {
        err_msg("Recvfrom %d", errno);
        return;
    }
*/
}
}; // namespace wynet
