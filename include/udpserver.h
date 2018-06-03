#ifndef WY_UDPSERVER_H
#define WY_UDPSERVER_H

#include "common.h"
#include "sockbase.h"

namespace wynet
{

class UdpServer;

using PtrUdpServer = std::shared_ptr<UdpServer>;
using WeakPtrUdpServer = std::weak_ptr<UdpServer>;

class UdpServer
{
public:
  UdpServer(int port);

  ~UdpServer();

  void Recvfrom();

private:
  void init(int port);
};
}; // namespace wynet

#endif