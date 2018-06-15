#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "sockbase.h"
#include "connection.h"
#include "event_listener.h"
#include "connection_manager.h"

namespace wynet
{
class WyNet;
class Server;
class TcpConnection;
using PtrServer = std::shared_ptr<Server>;
class TcpServer;
using PtrTcpServer = std::shared_ptr<TcpServer>;
using WeakPtrTcpServer = std::weak_ptr<TcpServer>;
class ConnectionManager;
using PtrConnMgr = std::shared_ptr<ConnectionManager>;
class TcpServerEventListener;
using PtrTcpServerEvtListener = std::shared_ptr<TcpServerEventListener>;

class TcpServerEventListener final : public EventListener
{
public:
  ctor_dtor_forlogging(TcpServerEventListener);

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

class TcpServer final : public Noncopyable, public std::enable_shared_from_this<TcpServer>
{
public:
  friend class TcpConnection;
  friend class Server;

  ~TcpServer();

  void init();

  void startListen(const char *host, int port);

  void terminate();

  PtrConnMgr initConnMgr();

  PtrConnMgr getConnMgr() const { return m_connMgr; }

  bool addConnection(const PtrConn &conn);

  bool removeConnection(const PtrConn &conn);

  PtrTcpServerEvtListener getListener() const { return m_evtListener; }

protected:
  TcpServer(PtrServer parent);

  WyNet *getNet() const;

  EventLoop &getLoop();

  void acceptConnection();

  void onDisconnected(const PtrConn &);

  static void OnNewTcpConnection(EventLoop *eventLoop, const PtrEvtListener &, int mask);

public:
  SocketFdCtrl m_sockFdCtrl;
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

private:
  PtrServer m_parent;
  int m_tcpPort;
  bool m_terminated;
  PtrConnMgr m_connMgr;
  PtrTcpServerEvtListener m_evtListener;
};
}; // namespace wynet

#endif