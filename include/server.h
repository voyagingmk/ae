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
class Server;
typedef std::shared_ptr<Server> PtrServer;
typedef std::weak_ptr<Server> WeakPtrServer;

class Server : public Noncopyable, public std::enable_shared_from_this<Server>
{
  WyNet *m_net;
  int m_udpPort;
  PtrTcpServer m_tcpServer;
  PtrUdpServer m_udpServer;

protected:
  Server(WyNet *net);

public:
  static PtrServer create(WyNet *net)
  {
    return PtrServer(new Server(net));
  }

  PtrTcpServer initTcpServer(const char *host, int tcpPort);

  PtrUdpServer initUdpServer(int udpPort);

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
