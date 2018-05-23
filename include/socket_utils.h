#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

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
sockaddr_storage getSrcAddr(int sockfd);
sockaddr_storage getDestAddr(int sockfd);
bool isSelfConnect(int sockfd);
void getNameInfo(struct sockaddr_storage *addr, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize);
void log_debug_addr(struct sockaddr *addr, const char *tag = "");
void log_debug_addr(struct sockaddr_storage *addr, const char *tag = "");
int setTcpNoDelay(int sockfd, bool enabled);
}; // namespace socketUtils
}; // namespace wynet

#endif
