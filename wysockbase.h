#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "wybuffer.h"

namespace wynet
{

class SocketBase
{
protected:
  SocketBase() {}

public:
  sockaddr_in6 m_sockaddr;
  socklen_t m_socklen;
  int m_sockfd;
  int m_family;
  Buffer buffer;
  bool isIPv4() { return m_family == PF_INET; }
  bool isIPv6() { return m_family == PF_INET6; }
};
};

#endif
