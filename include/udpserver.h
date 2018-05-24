#ifndef WY_UDPSERVER_H
#define WY_UDPSERVER_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{

class UDPServer
{
  SockAddr m_sockAddr;

public:
  UDPServer(int port);

  ~UDPServer();

  void Recvfrom();

private:
  void init(int port);
};
}; // namespace wynet

#endif