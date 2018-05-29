#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{

class Client;
typedef std::shared_ptr<Client> PtrClient;
typedef std::weak_ptr<Client> WeakPtrClient;

class TcpClientEventListener;
typedef std::shared_ptr<TcpClientEventListener> PtrTcpClientEvtListener;

class TcpClientEventListener : public EventListener
{
public:
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
  TcpClient(PtrClient client);

  ~TcpClient();

  void connect(const char *host, int port);

  PtrConn getConn()
  {
    return m_conn;
  }

  void disconnect();

  EventLoop &getLoop();

private:
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
  PtrClient m_parent;
  SockAddr m_sockAddr;
  PtrTcpClientEvtListener m_evtListener;
  bool m_asyncConnect;
  SockFd m_asyncSockfd;
  MutexLock m_mutex;
};

}; // namespace wynet

#endif
