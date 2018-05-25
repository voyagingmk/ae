#ifndef WY_UDPCLIENT_H
#define WY_UDPCLIENT_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{
class UdpClient
{
  SockAddr m_sockAddr;

public:
  UdpClient(const char *host, int port);
  ~UdpClient();
  void Send(const char *data, size_t len);
};
}; // namespace wynet

#endif