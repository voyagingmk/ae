#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "wysockbase.h"
#include "wyconnection.h"

namespace wynet
{
class WyNet;
class Server;
typedef std::shared_ptr<Server> PtrServer;
class TCPServer;
typedef std::shared_ptr<TCPServer> PtrTCPServer;

class TCPServer : public SocketBase
{
  PtrServer m_parent;
  int m_tcpPort;

public:
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

public:
  TCPServer(PtrServer parent);

  ~TCPServer();

  void startListen(int port);

protected:
  std::shared_ptr<TCPServer> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<TCPServer>();
  }

  WyNet *getNet() const;

  EventLoop &getLoop();

  void acceptConnection();

  void _onTcpDisconnected(int connfdTcp);

  static void OnNewTcpConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};
}; // namespace wynet

#endif