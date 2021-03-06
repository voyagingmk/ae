#include "utils.h"

void CheckReturnValue(int ret)
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

void Getsockopt(int socket, int level, int option_name,
                void *&option_value, socklen_t *&option_len)
{
    int ret = getsockopt(socket, level, option_name, option_value, option_len);
    CheckReturnValue(ret);
}

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
    Getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    log_debug("SO_RCVBUF = %d", rcvbuf);
    len = sizeof(sndbuf);
    Getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    log_debug("SO_SNDBUF = %d", sndbuf);
    len = sizeof(mss);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
    log_debug("TCP_MAXSEG = %d", mss);
    len = sizeof(isKeepAlive);
    Getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &isKeepAlive, &len);
    log_debug("SO_KEEPALIVE = %d", isKeepAlive);
    len = sizeof(isReuseAddr);
    Getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &isReuseAddr, &len);
    log_debug("SO_REUSEADDR = %d", isReuseAddr);

    int keepInterval = 0;
    int keepCount = 0;
#ifdef TCP_KEEPALIVE
    int keepAlive = 0;
    len = sizeof(keepAlive);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPALIVE, &keepAlive, &len);
    log_debug("TCP_KEEPALIVE = %d", keepAlive);
#endif
#ifdef TCP_KEEPIDLE
    int keepIdle = 0;
    len = sizeof(keepIdle);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, &len); // tcp_keepalive_time
    log_debug("TCP_KEEPIDLE = %d", keepIdle);
#endif
    len = sizeof(keepInterval);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, &len); // tcp_keepalive_intvl
    log_debug("TCP_KEEPINTVL = %d", keepInterval);
    len = sizeof(keepCount);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, &len); // tcp_keepalive_probes
    log_debug("TCP_KEEPCNT = %d", keepCount);

    log_debug("---- LogSocketState End %d ----", sockfd);
}

int SetSockSendBufSize(int fd, int newSndbuf, bool force)
{
    if (!force)
    {
        int sndbuf = 0;
        socklen_t len = sizeof(sndbuf);
        Getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
        if (sndbuf >= newSndbuf)
        {
            return -1;
        }
    }
    return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&newSndbuf, sizeof(int));
    log_debug("SetSockSendBufSize = %d", newSndbuf);
}

int SetSockRecvBufSize(int fd, int newRcvbuf, bool force)
{
    if (!force)
    {
        int rcvbuf = 0;
        socklen_t len = sizeof(rcvbuf);
        Getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
        if (rcvbuf >= newRcvbuf)
        {
            return -1;
        }
    }
    return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&newRcvbuf, sizeof(int));
    log_debug("SetSockSendBufSize = %d", newRcvbuf);
}

std::string hostname()
{
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}
