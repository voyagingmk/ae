#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "tcpserver.h"
#include "udpserver.h"
#include "connection.h"
#include "noncopyable.h"
namespace wynet
{

class WyNet;
class EventLoop;

class Server : public FDRef, public Noncopyable
{
  WyNet *m_net;
  int m_udpPort;
  std::shared_ptr<TCPServer> m_tcpServer;
  std::shared_ptr<UDPServer> m_udpServer;

public:
  Server(WyNet *net);

  std::shared_ptr<TCPServer> initTcpServer(const char *host, int tcpPort);

  std::shared_ptr<UDPServer> initUdpServer(int udpPort);

  ~Server();

  std::shared_ptr<Server> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<Server>();
  }

  WyNet *getNet() const
  {
    return m_net;
  }

  std::shared_ptr<TCPServer> &getTCPServer()
  {
    return m_tcpServer;
  }
};
}; // namespace wynet

#endif
