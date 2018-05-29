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
class TcpServer;
typedef std::shared_ptr<TcpServer> PtrTcpServer;
typedef std::weak_ptr<TcpServer> WeakPtrTcpServer;
class ConnectionManager;
typedef std::shared_ptr<ConnectionManager> PtrConnMgr;
class TcpServerEventListener;
typedef std::shared_ptr<TcpServerEventListener> PtrTcpServerEvtListener;

class TcpServerEventListener : public EventListener
{
public:
  TcpServerEventListener()
  {
    if (LOG_CTOR_DTOR)
      log_info("TcpServerEventListener()");
  }

  ~TcpServerEventListener()
  {
    if (LOG_CTOR_DTOR)
      log_info("~TcpServerEventListener()");
  }

  void setTcpServer(PtrTcpServer tcpServer)
  {
    m_tcpServer = tcpServer;
  }

  PtrTcpServer getTcpServer()
  {
    return m_tcpServer.lock();
  }

  static PtrTcpServerEvtListener create()
  {
    return std::make_shared<TcpServerEventListener>();
  }

protected:
  WeakPtrTcpServer m_tcpServer;
};

class TcpServer : public Noncopyable, public std::enable_shared_from_this<TcpServer>
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
  friend class Server;

  ~TcpServer();

  void init();

  void startListen(const char *host, int port);

  PtrConnMgr initConnMgr();

  PtrConnMgr getConnMgr() const { return m_connMgr; }

  bool addConnection(PtrConn conn);

  bool removeConnection(PtrConn conn);

protected:
  TcpServer(PtrServer parent);

  WyNet *getNet() const;

  EventLoop &getLoop();

  void acceptConnection();

  static void OnNewTcpConnection(EventLoop *eventLoop, PtrEvtListener, int mask);
};
}; // namespace wynet

#endif