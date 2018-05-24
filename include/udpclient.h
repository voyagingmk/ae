#ifndef WY_UDPCLIENT_H
#define WY_UDPCLIENT_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{
class UDPClient
{
  SockAddr m_sockAddr;

public:
  UDPClient(const char *host, int port);
  ~UDPClient();
  void Send(const char *data, size_t len);
};
}; // namespace wynet

#endif