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
using PtrServer = std::shared_ptr<Server>;
using WeakPtrServer = std::weak_ptr<Server>;

class Server : public Noncopyable, public std::enable_shared_from_this<Server>
{
  WyNet *m_net;
  WeakPtrTcpServer m_tcpServer;
  WeakPtrUdpServer m_udpServer;

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

  PtrTcpServer getTcpServer()
  {
    return m_tcpServer.lock();
  }

  PtrUdpServer getUdpServer()
  {
    return m_udpServer.lock();
  }
};
}; // namespace wynet

#endif
