#ifndef WY_UDPSERVER_H
#define WY_UDPSERVER_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{

class UDPServer : public SocketBase
{
public:
  UDPServer(int port);
  ~UDPServer();
  void Recvfrom();
};
};

#endif