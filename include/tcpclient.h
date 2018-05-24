#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{

class Client;
typedef std::shared_ptr<Client> PtrClient;

class TCPClientEventListener : public EventListener
{
public:
  void setTCPClient(PtrTCPClient tcpClient)
  {
    m_tcpClient = tcpClient;
  }
  PtrTCPClient getTCPClient()
  {
    return m_tcpClient.lock();
  }

  static PtrEvtListener create()
  {
    return std::make_shared<TCPClientEventListener>();
  }

protected:
  WeakPtrTCPClient m_tcpClient;
};

typedef std::shared_ptr<TCPClientEventListener> PtrTcpClientEvtListener;

class TCPClient : public Noncopyable, public std::enable_shared_from_this<TCPClient>
{
public:
  TCPClient(PtrClient client);

  ~TCPClient();

  void init();

  void connect(const char *host, int port);

  EventLoop &getLoop();

private:
  void asyncConnect(int sockfd);

  bool isAsyncConnecting();

  void endAsyncConnect();

  void _onTcpConnected(int sockfd);

  void _onTcpDisconnected();

  static void OnTcpWritable(EventLoop *eventLoop, PtrEvtListener listener, int mask);

public:
  PtrConn m_conn;
  PtrClient m_parent;
  SockAddr m_sockAddr;
  PtrTcpClientEvtListener m_evtListener;
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

private:
  bool m_asyncConnect;
  SockFd m_asyncSockfd;
};

}; // namespace wynet

#endif
