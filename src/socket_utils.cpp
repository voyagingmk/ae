#include "socket_utils.h"
#include "logger/log.h"

namespace wynet
{
namespace socketUtils
{

int sock_socket(int family, int type, int protocol)
{
    int n;
    if ((n = socket(family, type, protocol)) < 0)
        log_fatal("socket error");
    return n;
}

void sock_close(int fd)
{
    if (close(fd) == -1)
        log_fatal("sock_close error");
}

void sock_listen(int fd, int backlog)
{
    char *ptr;

    if ((ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if (listen(fd, backlog) < 0)
        log_fatal("listen error");
}

void sock_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(fd, level, optname, optval, optlen) < 0)
    {
        log_fatal("setsockopt error %d", errno);
    }
}

bool valid(SockFd sockfd) { return sockfd > 0; }

bool isIPv4(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET; }

bool isIPv6(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET6; }

void checkReturnValue(int ret)
{
    if (ret == 0)
    {
        return;
    }
    if (ret == -1)
    {
        log_error("getsockopt error %d %s", errno, strerror(errno));
    }
}

void sock_getsockopt(int sockfd, int level, int option_name,
                     void *option_value, socklen_t *option_len)
{
    int ret = getsockopt(sockfd, level, option_name, option_value, option_len);
    checkReturnValue(ret);
}

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
    assert(addrlen <= static_cast<socklen_t>(sizeof(addr)));
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
    assert(addrlen <= static_cast<socklen_t>(sizeof(addr)));
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
    socklen_t addrlen = sizeof(sockaddr_storage);
    int ret = getnameinfo((struct sockaddr *)addr, addrlen,
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

int setTcpNoDelay(SockFd sockfd, bool enabled)
{
    int val = enabled ? 1 : 0;
    return ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, static_cast<socklen_t>(sizeof val));
}

int setTcpNonBlock(SockFd sockfd)
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
        sock_getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
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
        sock_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
        if (rcvbuf >= newRcvBuf)
        {
            return -1;
        }
    }
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *)&newRcvBuf, sizeof(int));
    log_debug("socket_utils.SetSockRecvBufSize %d, %d", sockfd, newRcvBuf);
}

#ifdef DEBUG_MODE

void LogSocketState(int sockfd)
{
    log_debug("---- LogSocketState %d ----", sockfd);
    socklen_t len = 0;
    int sndbuf = 0;
    int rcvbuf = 0;
    int mss = 0;
    int isKeepAlive = 0;
    int isReuseAddr = 0;
    len = sizeof(rcvbuf);
    sock_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    log_debug("SO_RCVBUF = %d", rcvbuf);
    len = sizeof(sndbuf);
    sock_getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    log_debug("SO_SNDBUF = %d", sndbuf);
    len = sizeof(mss);
    sock_getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
    log_debug("TCP_MAXSEG = %d", mss);
    len = sizeof(isKeepAlive);
    sock_getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &isKeepAlive, &len);
    log_debug("SO_KEEPALIVE = %d", isKeepAlive);
    len = sizeof(isReuseAddr);
    sock_getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &isReuseAddr, &len);
    log_debug("SO_REUSEADDR = %d", isReuseAddr);

    int keepInterval = 0;
    int keepCount = 0;
#ifdef TCP_KEEPALIVE
    int keepAlive = 0;
    len = sizeof(keepAlive);
    sock_getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPALIVE, &keepAlive, &len);
    log_debug("TCP_KEEPALIVE = %d", keepAlive);
#endif
#ifdef TCP_KEEPIDLE
    int keepIdle = 0;
    len = sizeof(keepIdle);
    sock_getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, &len); // tcp_keepalive_time
    log_debug("TCP_KEEPIDLE = %d", keepIdle);
#endif
    len = sizeof(keepInterval);
    sock_getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, &len); // tcp_keepalive_intvl
    log_debug("TCP_KEEPINTVL = %d", keepInterval);
    len = sizeof(keepCount);
    sock_getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, &len); // tcp_keepalive_probes
    log_debug("TCP_KEEPCNT = %d", keepCount);

    log_debug("---- LogSocketState End %d ----", sockfd);
}

#endif

}; // namespace socketUtils
}; // namespace wynet