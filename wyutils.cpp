#include "wyutils.h"

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

    int keepAlive = 0;
    int keepIdle = 0;
    int keepInterval = 0;
    int keepCount = 0;
#ifdef TCP_KEEPALIVE
    len = sizeof(keepAlive);
    Getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPALIVE, &keepAlive, &len);
    log_debug("TCP_KEEPALIVE = %d", keepAlive);
#endif
#ifdef TCP_KEEPIDLE
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


int SetSockSendBufSize(int fd, int bytes) {
    return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&bytes, sizeof(int));
}

int SetSockRecvBufSize(int fd, int bytes) {
    return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&bytes, sizeof(int));
}
