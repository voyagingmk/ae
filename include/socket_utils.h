#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdlib.h>
#include <cstring>
#include <arpa/inet.h>

namespace wynet
{
namespace socketUtils
{

bool valid(SockFd sockfd) { return sockfd > 0; }

bool isIPv4(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET; }

bool isIPv6(sockaddr_storage sockAddr) { return sockAddr.ss_family == PF_INET6; }

sockaddr_storage getSrcAddr(SockFd sockfd);

sockaddr_storage getDestAddr(SockFd sockfd);

bool isSelfConnect(SockFd sockfd);

void getNameInfo(struct sockaddr_storage *addr, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize);

void log_debug_addr(struct sockaddr *addr, const char *tag = "");

void log_debug_addr(struct sockaddr_storage *addr, const char *tag = "");

int setTcpNoDelay(SockFd sockfd, bool enabled);

int setTcpNonBlock(SockFd sockfd);

int SetSockSendBufSize(SockFd sockfd, int newSndBuf, bool force = false);

int SetSockRecvBufSize(SockFd sockfd, int newRcvBuf, bool force = false);

#ifdef DEBUG_MODE

void LogSocketState(SockFd sockfd);

#else

#define LogSocketState(fd)

#endif

}; // namespace socketUtils
}; // namespace wynet

#endif
