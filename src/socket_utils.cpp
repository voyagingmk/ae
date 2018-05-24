#include "socket_utils.h"
#include "logger/log.h"

namespace wynet
{
namespace socketUtils
{

int sock_fcntl(int sockfd, int cmd, int arg)
{
    int n;

    if ((n = fcntl(sockfd, cmd, arg)) == -1)
        log_fatal("fcntl error");
    return n;
}

sockaddr_storage getSrcAddr(int sockfd)
{
    struct sockaddr_storage addr = {0};
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    if (::getsockname(sockfd, (sockaddr *)(&addr), &addrlen) < 0)
    {
        log_fatal("socket_utils.getSrcAddr");
    }
    if (addrlen < static_cast<socklen_t>(sizeof(addr)))
    {
        log_warn("socket_utils.getSrcAddr truncated, %zu -> %zu", sizeof(addr), static_cast<size_t>(addrlen));
    }
    return addr;
}

sockaddr_storage getDestAddr(int sockfd)
{
    struct sockaddr_storage addr = {0};
    socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
    if (::getpeername(sockfd, (struct sockaddr *)(&addr), &addrlen) < 0)
    {
        log_fatal("socket_utils.getDestAddr");
    }
    if (addrlen < static_cast<socklen_t>(sizeof(addr)))
    {
        log_warn("socket_utils.getDestAddr truncated, %zu -> %zu", sizeof(addr), static_cast<size_t>(addrlen));
    }
    return addr;
}

bool isSelfConnect(int sockfd)
{
    sockaddr_storage srcAddr = getSrcAddr(sockfd);
    sockaddr_storage destAddr = getDestAddr(sockfd);
    log_debug_addr((struct sockaddr *)(&srcAddr), "socket_utils.isSelfConnect srcAddr");
    log_debug_addr((struct sockaddr *)(&destAddr), "socket_utils.isSelfConnect destAddr");

    if (srcAddr.ss_family == AF_INET)
    {
        sockaddr_in *srcAddr_v4 = (struct sockaddr_in *)(&srcAddr);
        sockaddr_in *destAddr_v4 = (struct sockaddr_in *)(&destAddr);
        return srcAddr_v4->sin_port == destAddr_v4->sin_port && srcAddr_v4->sin_addr.s_addr == destAddr_v4->sin_addr.s_addr;
    }
    else if (srcAddr.ss_family == AF_INET6)
    {
        sockaddr_in6 *srcAddr_v6 = (struct sockaddr_in6 *)(&srcAddr);
        sockaddr_in6 *destAddr_v6 = (struct sockaddr_in6 *)(&destAddr);
        return srcAddr_v6->sin6_port == destAddr_v6->sin6_port &&
               memcmp(&srcAddr_v6->sin6_addr,
                      &destAddr_v6->sin6_addr,
                      sizeof(struct in6_addr)) == 0;
    }
    return false;
}

void getNameInfo(struct sockaddr_storage *addr, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize)
{
    if (!ipBuf && !portBuf)
    {
        return;
    }
    int flag = 0;
    if (ipBuf)
    {
        flag |= NI_NUMERICHOST;
    }
    if (portBuf)
    {
        flag |= NI_NUMERICSERV;
    }
    int ret = getnameinfo((struct sockaddr *)addr, addr->ss_len,
                          ipBuf, ipBufSize,
                          portBuf, portBufSize,
                          NI_NUMERICHOST);
    if (ret != 0)
    {
        int errnosave = errno;
        log_warn("socketUtils.getAddrIP error: %d, %s", errnosave, strerror(errnosave));
    }
}

void log_debug_addr(struct sockaddr *addr, const char *tag)
{
    char ipBuf[NI_MAXHOST];
    char portBuf[NI_MAXSERV];
    int port = 0;
    if (addr->sa_family == AF_INET)
    {
        sockaddr_in *addr_v4 = (struct sockaddr_in *)(addr);
        port = (int)ntohs(addr_v4->sin_port);
    }
    else if (addr->sa_family == AF_INET6)
    {
        sockaddr_in6 *addr_v6 = (struct sockaddr_in6 *)(addr);
        port = (int)ntohs(addr_v6->sin6_port);
    }

    getNameInfo((struct sockaddr_storage *)addr, ipBuf, NI_MAXHOST, portBuf, NI_MAXSERV);
    log_debug("%s, ip=%s, port=%d|%s", tag, ipBuf, port, portBuf);
}

void log_debug_addr(struct sockaddr_storage *addr, const char *tag)
{
    log_debug_addr((struct sockaddr *)(addr), tag);
}

int setTcpNoDelay(int sockfd, bool enabled)
{
    int val = enabled ? 1 : 0;
    return ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, static_cast<socklen_t>(sizeof val));
}

int setTcpNonBlock(int sockfd)
{

    int flags = sock_fcntl(sockfd, F_GETFL, 0);
    return sock_fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int SetSockSendBufSize(int sockfd, int newSndBuf, bool force)
{
    if (!force)
    {
        int sndbuf = 0;
        socklen_t len = sizeof(sndbuf);
        Getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
        if (sndbuf >= newSndBuf)
        {
            return -1;
        }
    }
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void *)&newSndBuf, sizeof(int));
    log_debug("socket_utils.SetSockSendBufSize %d, %d", sockfd, newSndBuf);
}

int SetSockRecvBufSize(int sockfd, int newRcvBuf, bool force)
{
    if (!force)
    {
        int rcvbuf = 0;
        socklen_t len = sizeof(rcvbuf);
        Getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
        if (rcvbuf >= newRcvBuf)
        {
            return -1;
        }
    }
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *)&newRcvBuf, sizeof(int));
    log_debug("socket_utils.SetSockRecvBufSize %d, %d", sockfd, newRcvBuf);
}

}; // namespace socketUtils
}; // namespace wynet