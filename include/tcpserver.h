#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{
class WyNet;
class Server;
typedef std::shared_ptr<Server> PtrServer;
class TCPServer;
typedef std::shared_ptr<TCPServer> PtrTCPServer;
class ConnectionManager;
typedef std::shared_ptr<ConnectionManager> PtrConnMgr;

class TCPServer : public SocketBase
{
  PtrServer m_parent;
  int m_tcpPort;
  PtrConnMgr m_connMgr;

public:
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

public:
  TCPServer(PtrServer parent);

  ~TCPServer();

  void startListen(int port);

  PtrConnMgr initConnMgr();

  PtrConnMgr getConnMgr() const { return m_connMgr; }

  bool addConnection(PtrConn conn);

  bool removeConnection(PtrConn conn);

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