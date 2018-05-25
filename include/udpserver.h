#ifndef WY_UDPSERVER_H
#define WY_UDPSERVER_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{

class UdpServer
{
  SockAddr m_sockAddr;

public:
  UdpServer(int port);

  ~UdpServer();

  void Recvfrom();

private:
  void init(int port);
};
}; // namespace wynet

#endif