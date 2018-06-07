#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{

class TcpConnection;
class TcpClientEventListener;
using PtrTcpClientEvtListener = std::shared_ptr<TcpClientEventListener>;

class TcpClientEventListener final : public EventListener
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

using PtrTcpClient = std::shared_ptr<TcpClient>;
using WeakPtrTcpClient = std::weak_ptr<TcpClient>;

class TcpClient final : public Noncopyable, public std::enable_shared_from_this<TcpClient>
{
public:
  friend class TcpConnection;

  TcpClient(EventLoop *loop);

  ~TcpClient();

  void reconnectWithDelay(int ms);

  void connect(const char *host, int port);

  bool needReconnect();

  void setReconnectTimes(int times)
  {
    m_reconnectTimes = times;
  }

  void setReconnectInterval(int ms)
  {
    m_reconnectInterval = ms;
  }

  PtrConn getConn();

  void disconnect();

  EventLoop &getLoop();

private:
  void connectInLoop(const char *host, int port);

  void reconnect();

  void asyncConnect(int sockfd);

  bool isAsyncConnecting();

  void endAsyncConnect();

  void afterAsyncConnect(int sockfd);

  void onConnected(int sockfd);

  void onConnectFailed();

  void onDisconnected(const PtrConn &);

  void resetEvtListener();

  // in order to access private members
  static void OnTcpWritable(EventLoop *eventLoop, const PtrEvtListener &listener, int mask);

  static int onReconnectTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data);

public:
  using OnTcpConnectFailed = std::function<void(const PtrTcpClient &)>;
  OnTcpConnectFailed onTcpConnectFailed;
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

private:
  PtrConn m_conn;
  EventLoop *m_loop;
  PtrTcpClientEvtListener m_evtListener;
  MutexLock m_mutex;
  bool m_asyncConnect;
  int m_reconnectTimes;    // -1: infinitely   0: no reconnect
  int m_reconnectInterval; // ms
  SockAddr m_sockAddr;
};

}; // namespace wynet

#endif
