#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "sockbase.h"
#include "connection.h"

namespace wynet
{

class Client;
typedef std::shared_ptr<Client> PtrClient;

class TCPClient : public SocketBase
{
public:
  sockaddr_in m_serSockaddr;
  struct hostent *h;
  PtrConn m_conn;
  PtrClient m_parent;
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

public:
  std::shared_ptr<TCPClient> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<TCPClient>();
  }

  TCPClient(PtrClient client);

  ~TCPClient();

  void connect(const char *host, int port);

  EventLoop &getLoop();

private:
  void listenWritable(bool);

  void _onTcpConnected();

  void _onTcpDisconnected();

  static void OnTcpWritable(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

private:
  bool m_listenWritable;
};

}; // namespace wynet

#endif
