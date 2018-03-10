#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{

class TCPServer : public SocketBase
{
public:
  TCPServer(int port);
  ~TCPServer();
};
};

#endif