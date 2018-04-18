#ifndef WY_SERVER_H
#define WY_SERVER_H

#include "common.h"
#include "uniqid.h"
#include "wytcpserver.h"
#include "wyudpserver.h"
#include "wyconnection.h"
#include "noncopyable.h"
namespace wynet
{

class WyNet;
class EventLoop;
class Server;
typedef std::shared_ptr<Server> PtrServer;

class Server : public FDRef, public Noncopyable
{
  WyNet *m_net;
  int m_tcpPort;
  int m_udpPort;
  std::shared_ptr<TCPServer> m_tcpServer;
  std::shared_ptr<UDPServer> m_udpServer;
  std::map<UniqID, PtrSerConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_convId2cid;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;

public:
  typedef void (*OnTcpConnected)(PtrServer, PtrSerConn conn);
  typedef void (*OnTcpDisconnected)(PtrServer, PtrSerConn conn);
  typedef void (*OnTcpRecvMessage)(PtrServer, PtrSerConn conn, uint8_t *, size_t);

  OnTcpConnected onTcpConnected;
  OnTcpDisconnected onTcpDisconnected;
  OnTcpRecvMessage onTcpRecvMessage;

public:
  Server(WyNet *net);

  void initTcpServer(int tcpPort);

  void initUdpServer(int udpPort);

  ~Server();

  std::shared_ptr<Server> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<Server>();
  }

  // only use in unusal cases
  void closeConnect(UniqID connectId);

  void sendByTcp(UniqID connectId, const uint8_t *data, size_t len);

  void sendByTcp(UniqID connectId, PacketHeader *header);

  WyNet *getNet() const
  {
    return m_net;
  }

  std::shared_ptr<TCPServer> &getTCPServer()
  {
    return m_tcpServer;
  }

private:
  UniqID refConnection(PtrSerConn conn);

  bool unrefConnection(UniqID connectId);

  void _closeConnectByFd(int connfdTcp, bool force = false);

  void _onTcpConnected(int connfdTcp);

  void _onTcpMessage(int connfdTcp);

  void _onTcpDisconnected(int connfdTcp);

  friend void OnTcpNewConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

  friend void OnUdpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};
};

#endif
