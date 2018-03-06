#include "wyutils.h"

void LogSocketState(int sockfd)
{
    log_info("---- LogSocketState %d ----", sockfd);
    int sndbuf = 0;
    int rcvbuf = 0;
    int mss = 0;
    socklen_t len = 0;
    len = sizeof(rcvbuf);
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
    len = sizeof(mss);
    getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
    len = sizeof(sndbuf);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    log_info("SO_RCVBUF = %d, SO_SNDBUF = %d, MSS = %d", rcvbuf, sndbuf, mss);
    
    int keepAlive = 0;
    int keepIdle = 0;
    int keepInterval = 0;
    int keepCount = 0;
    len = sizeof(keepAlive);
    getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPALIVE, &keepAlive, &len);
    log_info("TCP_KEEPALIVE = %d", keepAlive);
#ifdef TCP_KEEPIDLE
    len = sizeof(keepIdle);
    getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, &len); // tcp_keepalive_time
    log_info("TCP_KEEPIDLE = %d", keepIdle);
#endif
    len = sizeof(keepInterval);
    getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, &len); // tcp_keepalive_intvl
    log_info("TCP_KEEPINTVL = %d", keepInterval);
    len = sizeof(keepCount);
    getsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, &len); // tcp_keepalive_probes
    log_info("TCP_KEEPCNT = %d", keepCount);
    

    log_info("---- LogSocketState End %d ----", sockfd);
}
