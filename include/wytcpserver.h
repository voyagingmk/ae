#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "wysockbase.h"
#include "wyconnection.h"

namespace wynet
{
class WyNet;
class Server;
typedef std::shared_ptr<Server> PtrServer;
class TCPServer;
typedef std::shared_ptr<TCPServer> PtrTCPServer;

class TCPServer : public SocketBase
{
  std::map<UniqID, PtrSerConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_convId2cid;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
  PtrServer m_parent;
  int m_tcpPort;

public:
  OnTcpConnected onTcpConnected;
  OnTcpDisconnected onTcpDisconnected;
  OnTcpRecvMessage onTcpRecvMessage;

public:
  TCPServer(PtrServer parent);

  ~TCPServer();

  void startListen(int port);

  // only use in unusal cases
  void closeConnect(UniqID connectId);

  void sendByTcp(UniqID connectId, const uint8_t *data, size_t len);

  void sendByTcp(UniqID connectId, PacketHeader *header);

private:
  std::shared_ptr<TCPServer> shared_from_this()
  {
    return FDRef::downcasted_shared_from_this<TCPServer>();
  }

  WyNet *getNet() const;

  EventLoop &getLoop();

  UniqID refConnection(PtrSerConn conn);

  bool unrefConnection(UniqID connectId);

  void acceptConnection();

  void _closeConnectByFd(int connfdTcp, bool force = false);

  void _onTcpMessage(int connfdTcp);

  void _onTcpDisconnected(int connfdTcp);

  friend void OnTcpNewConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

  friend void OnUdpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};
};

#endif