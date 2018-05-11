#ifndef WY_UDPCLIENT_H
#define WY_UDPCLIENT_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{
class UDPClient : public SocketBase
{
  // sockaddr_in m_serSockaddr;
  // struct hostent *h;

public:
  UDPClient(const char *host, int port);
  ~UDPClient();
  void Send(const char *data, size_t len);
};
};

#endif