#ifndef WY_UDPSERVER_H
#define WY_UDPSERVER_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{

static const size_t MAX_MSG = 1400;

class UDPServer : public SocketBase
{
  char msg[MAX_MSG];

public:
  UDPServer(int port);
  ~UDPServer();
  void Recvfrom();
};
};

#endif