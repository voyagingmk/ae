#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{

class TcpClientEventListener;
typedef std::shared_ptr<TcpClientEventListener> PtrTcpClientEvtListener;

class TcpClientEventListener : public EventListener
{
public:
  ctor_dtor_forlogging(TcpClientEventListener);

  void setTcpClient(PtrTcpClient tcpClient)
  {
    m_tcpClient = tcpClient;
  }

  PtrTcpClient getTcpClient()
  {
    return m_tcpClient.lock();
  }

  static PtrTcpClientEvtListener create()
  {
    return std::make_shared<TcpClientEventListener>();
  }

protected:
  WeakPtrTcpClient m_tcpClient;
};

typedef std::shared_ptr<TcpClient> PtrTcpClient;
typedef std::weak_ptr<TcpClient> WeakPtrTcpClient;

class TcpClient : public Noncopyable, public std::enable_shared_from_this<TcpClient>
{
public:
  TcpClient(EventLoop *loop);

  ~TcpClient();

  void connect(const char *host, int port);

  void setReconnectTimes(int times)
  {
    m_reconnectTimes = times;
  }

  PtrConn getConn();

  void disconnect();

  EventLoop &getLoop();

private:
  void connectInLoop(const char *host, int port);

  void whetherReconnect();

  void reconnect();

  void asyncConnect(int sockfd);

  bool isAsyncConnecting();

  void endAsyncConnect();

  void _onTcpConnected(int sockfd);

  static void OnTcpWritable(EventLoop *eventLoop, PtrEvtListener listener, int mask);

public:
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

private:
  PtrConn m_conn;
  EventLoop *m_loop;
  SockAddr m_sockAddr;
  PtrTcpClientEvtListener m_evtListener;
  bool m_asyncConnect;
  SockFd m_asyncSockfd;
  MutexLock m_mutex;
  int m_reconnectTimes; // -1: infinitely   0: no reconnect
};

}; // namespace wynet

#endif
