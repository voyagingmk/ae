#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

#include "common.h"

namespace wynet
{
namespace socketUtils
{

static bool valid(SockFd sockfd) { return sockfd > 0; }

static bool isIPv4(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET; }

static bool isIPv6(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET6; }

static sockaddr_storage getSrcAddr(SockFd sockfd);

static sockaddr_storage getDestAddr(SockFd sockfd);

static bool isSelfConnect(SockFd sockfd);

static void getNameInfo(struct sockaddr_storage *addr, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize);

static void log_debug_addr(struct sockaddr *addr, const char *tag = "");

static void log_debug_addr(struct sockaddr_storage *addr, const char *tag = "");

static int setTcpNoDelay(SockFd sockfd, bool enabled);

static int setTcpNonBlock(SockFd sockfd);

static int SetSockSendBufSize(SockFd sockfd, int newSndBuf, bool force = false);

static int SetSockRecvBufSize(SockFd sockfd, int newRcvBuf, bool force = false);

#ifdef DEBUG_MODE

static void LogSocketState(SockFd sockfd);

#else

#define LogSocketState(fd)

#endif

}; // namespace socketUtils
}; // namespace wynet

#endif
