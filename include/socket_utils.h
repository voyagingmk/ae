#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

#include <sys/types.h>  /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <sys/un.h>     /* for Unix domain sockets */
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

}; // namespace socketUtils
}; // namespace wynet

#endif
