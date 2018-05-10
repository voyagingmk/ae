#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "wysockbase.h"
#include "wyconnection.h"

namespace wynet
{

class Client;
typedef std::shared_ptr<Client> PtrClient;

class TCPClient : public SocketBase
{
public:
  sockaddr_in m_serSockaddr;
  struct hostent *h;
  PtrCliConn m_conn;
  PtrClient m_parent;
  bool m_connected;

public:
  TcpConnection::OnTcpConnected onTcpConnected;
  TcpConnection::OnTcpDisconnected onTcpDisconnected;
  TcpConnection::OnTcpRecvMessage onTcpRecvMessage;

public:
  std::shared_ptr<TCPClient> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<TCPClient>();
  }

  TCPClient(PtrClient client);

  void connect(const char *host, int port);

  void Close();

  void Send(uint8_t *data, size_t len);

  void Recvfrom();

  EventLoop &getLoop();

private:
  void onConnected();

  void _onTcpConnected();

  void _onTcpDisconnected();

  static void OnTcpWritable(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};

}; // namespace wynet

#endif
