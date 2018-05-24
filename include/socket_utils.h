#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

#include "common.h"

namespace wynet
{
namespace socketUtils
{

extern bool valid(SockFd sockfd);

extern bool isIPv4(sockaddr_storage sockAddr);

extern bool isIPv6(sockaddr_storage sockAddr);

extern sockaddr_storage getSrcAddr(SockFd sockfd);

extern sockaddr_storage getDestAddr(SockFd sockfd);

extern bool isSelfConnect(SockFd sockfd);

extern void getNameInfo(struct sockaddr_storage *addr, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize);

extern void log_debug_addr(struct sockaddr *addr, const char *tag = "");

extern void log_debug_addr(struct sockaddr_storage *addr, const char *tag = "");

extern int setTcpNoDelay(SockFd sockfd, bool enabled);

extern int setTcpNonBlock(SockFd sockfd);

extern int SetSockSendBufSize(SockFd sockfd, int newSndBuf, bool force = false);

extern int SetSockRecvBufSize(SockFd sockfd, int newRcvBuf, bool force = false);

#ifdef DEBUG_MODE

extern void LogSocketState(SockFd sockfd);

#else

#define LogSocketState(fd)

#endif

}; // namespace socketUtils
}; // namespace wynet

#endif
