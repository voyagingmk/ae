#include "common.h"
#include "udpclient.h"
#include "socket_utils.h"

namespace wynet
{

UdpClient::UdpClient(const char *host, int port)
{
    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    int fd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_ALL;
    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        log_fatal("udp_client error for %s, %s: %s",
                  host, serv, gai_strerror(n));
    ressave = res;

    do
    {
        if (res->ai_family == PF_INET || res->ai_family == PF_INET6)
        {
            fd = socketUtils ::sock_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (fd >= 0)
                break; /* success */
        }
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno set from final socket() */
        log_error("udp_client error %d for %s, %s", errno, host, serv);
    freeaddrinfo(ressave);
}

UdpClient::~UdpClient()
{
    // close(sockfd());
}

void UdpClient::Send(const char *data, size_t len)
{
    //  ::Send(sockfd(), data, len, 0);
}
}; // namespace wynet