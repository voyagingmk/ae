#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "sockbase.h"
#include "connection.h"
#include "event_listener.h"

namespace wynet
{
class WyNet;
class Server;
typedef std::shared_ptr<Server> PtrServer;
class TCPServer;
typedef std::shared_ptr<TCPServer> PtrTCPServer;
typedef std::weak_ptr<TCPServer> WeakPtrTCPServer;
class ConnectionManager;
typedef std::shared_ptr<ConnectionManager> PtrConnMgr;
class TCPServerEventListener;
typedef std::shared_ptr<TCPServerEventListener> PtrTcpServerEvtListener;

class TCPServerEventListener : public EventListener
{
public:
  void setTCPServer(PtrTCPServer tcpServer)
  {
    m_tcpServer = tcpServer;
  }
  PtrTCPServer getTCPServer()
  {
    return m_tcpServer.lock();
  }

  static PtrTcpServerEvtListener create()
  {
    return std::make_shared<TCPServerEventListener>();
  }

protected:
  WeakPtrTCPServer m_tcpServer;
};

class TCPServer : public Noncopyable, public std::enable_shared_from_this<TCPServer>
{
  PtrServer m_parent;
  int m_tcpPort;
  PtrConnMgr m_connMgr;
  SockAddr m_sockAddr;
  PtrTcpServerEvtListener m_evtListener;

public:
  SocketFdCtrl m_sockFdCtrl;

  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

public:
  TCPServer(PtrServer parent);

  ~TCPServer();

  void init();

  void startListen(const char *host, int port);

  PtrConnMgr initConnMgr();

  PtrConnMgr getConnMgr() const { return m_connMgr; }

  bool addConnection(PtrConn conn);

  bool removeConnection(PtrConn conn);

protected:
  WyNet *getNet() const;

  EventLoop &getLoop();

  void acceptConnection();

  void _onTcpDisconnected(int connfdTcp);

  static void OnNewTcpConnection(EventLoop *eventLoop, PtrEvtListener, int mask);
};
}; // namespace wynet

#endif