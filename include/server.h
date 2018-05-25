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

class Server : public Noncopyable, public std::enable_shared_from_this<Server>
{
  WyNet *m_net;
  int m_udpPort;
  std::shared_ptr<TcpServer> m_tcpServer;
  std::shared_ptr<UdpServer> m_udpServer;

public:
  Server(WyNet *net);

  std::shared_ptr<TcpServer> initTcpServer(const char *host, int tcpPort);

  std::shared_ptr<UdpServer> initUdpServer(int udpPort);

  ~Server();

  WyNet *getNet() const
  {
    return m_net;
  }

  std::shared_ptr<TcpServer> &getTcpServer()
  {
    return m_tcpServer;
  }
};
}; // namespace wynet

#endif
