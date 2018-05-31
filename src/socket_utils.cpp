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

void sock_close(SockFd sockfd)
{
    if (close(sockfd) == -1)
        log_fatal("sock_close error");
}

// to avoid large numbers of connections sitting in the TIME_WAIT state
// tying up all the available resources
void sock_linger(SockFd sockfd)
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms739165(v=vs.85).aspx
    struct linger l;
    // Specifies whether a socket should remain open for a specified amount of time
    // after a closesocket function call to enable queued data to be sent.
    l.l_onoff = 1;
    /*
    The linger time in seconds. This member specifies how long to remain open after 
    a closesocket function call to enable queued data to be sent. This member is only 
    applicable if the l_onoff member of the linger structure is set to a nonzero value.
    */
    l.l_linger = 0;
    /*
    组合  l_onoff    l_linger   作用
    A     0           忽略      closesocket立即返回，发送队列保持直到发完，意思是操作会负责把数据发完，防止丢失最后的包

    B     非0          0        closesocket立即返回，发送队列里的数据立即全部丢弃，直接发RST，不经过2MSL状态

    C     非0         非0       阻塞模式下，closesocket阻塞到l_linger超时或者数据发送完成，
                               发送队列在超时前会继续发送，但超时时还是得丢弃，超时后同情况2.
    */
    socketUtils ::sock_setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}

void sock_listen(SockFd sockfd, int backlog)
{
    char *ptr;

    if ((ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if (listen(sockfd, backlog) < 0)
        log_fatal("listen error %d, %s", errno, strerror(errno));
}

void sock_setsockopt(SockFd sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(sockfd, level, optname, optval, optlen) < 0)
    {
        log_fatal("setsockopt %d error %d %s", sockfd, errno, strerror(errno));
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

void getSrcAddr(int sockfd, sockaddr_storage *addr, socklen_t &addrlen)
{
    if (::getsockname(sockfd, (struct sockaddr *)(addr), &addrlen) < 0)
    {
        log_fatal("socket_utils.getSrcAddr");
    }
    assert(addrlen <= static_cast<socklen_t>(sizeof(sockaddr_storage)));
}

void getDestAddr(int sockfd, sockaddr_storage *addr, socklen_t &addrlen)
{
    if (::getpeername(sockfd, (struct sockaddr *)(addr), &addrlen) < 0)
    {
        log_fatal("socket_utils.getDestAddr");
    }
    assert(addrlen <= static_cast<socklen_t>(sizeof(sockaddr_storage)));
}

bool isSelfConnect(int sockfd)
{
    sockaddr_storage srcAddr;
    socklen_t srcAddrlen = sizeof(sockaddr_storage);
    sockaddr_storage destAddr;
    socklen_t destAddrlen = sizeof(sockaddr_storage);
    getSrcAddr(sockfd, &srcAddr, srcAddrlen);
    getDestAddr(sockfd, &destAddr, destAddrlen);
    // log_debug_addr(&srcAddr, srcAddrlen, "socket_utils.isSelfConnect srcAddr");
    // log_debug_addr(&destAddr, destAddrlen, "socket_utils.isSelfConnect destAddr");

    if (srcAddr.ss_family == AF_INET)
    {
        sockaddr_in *srcAddr_v4 = (struct sockaddr_in *)(&srcAddr);
        sockaddr_in *destAddr_v4 = (struct sockaddr_in *)(&destAddr);
        // log_info("isSelfConnect ipv4 port %d %d addr %d %d",
        //        srcAddr_v4->sin_port, destAddr_v4->sin_port,
        //        srcAddr_v4->sin_addr.s_addr, destAddr_v4->sin_addr.s_addr);
        return srcAddr_v4->sin_port == destAddr_v4->sin_port &&
               srcAddr_v4->sin_addr.s_addr == destAddr_v4->sin_addr.s_addr;
    }
    else if (srcAddr.ss_family == AF_INET6)
    {
        sockaddr_in6 *srcAddr_v6 = (struct sockaddr_in6 *)(&srcAddr);
        sockaddr_in6 *destAddr_v6 = (struct sockaddr_in6 *)(&destAddr);
        // log_info("isSelfConnect ipv6 port %d %d",
        //         srcAddr_v6->sin6_port, destAddr_v6->sin6_port);
        return srcAddr_v6->sin6_port == destAddr_v6->sin6_port &&
               memcmp(&srcAddr_v6->sin6_addr,
                      &destAddr_v6->sin6_addr,
                      sizeof(struct in6_addr)) == 0;
    }
    else
    {
        log_fatal("isSelfConnect wrong srcAddr.ss_family %d", srcAddr.ss_family);
    }
    return false;
}

void getNameInfo(struct sockaddr *addr, socklen_t addrlen, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize)
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
    socklen_t len;
    switch (addr->sa_family)
    {
    case AF_INET:
        len = sizeof(sockaddr_in);
        break;
    case AF_INET6:
        len = sizeof(sockaddr_in6);
        break;
    }
    // log_warn("getnameinfo addr sa_family %d len %d", addr->sa_family, len);
    int err = getnameinfo(addr, addrlen,
                          ipBuf, ipBufSize,
                          portBuf, portBufSize,
                          flag);
    if (err != 0)
    {
        log_warn("socketUtils.getNameInfo addrlen %d error: %d, %s", addrlen, err, gai_strerror(err));
    }
}

void log_debug_addr(struct sockaddr *addr, socklen_t addrlen, const char *tag)
{
    char ipBuf[NI_MAXHOST];
    char portBuf[NI_MAXSERV];
    /*
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
    */
    getNameInfo(addr, addrlen, ipBuf, NI_MAXHOST, portBuf, NI_MAXSERV);
    log_info("%s, ip=%s, port=%s", tag, ipBuf, portBuf);
    // log_info("%s, ip=%s, port=%d", tag, ipBuf, port);
}

void log_debug_addr(struct sockaddr_storage *addr, socklen_t addrlen, const char *tag)
{
    log_debug_addr((struct sockaddr *)(addr), addrlen, tag);
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