#ifndef WY_TCPSERVER_H
#define WY_TCPSERVER_H

#include "common.h"
#include "uniqid.h"
#include "wysockbase.h"
#include "wyconnection.h"

namespace wynet
{
class Server;
typedef std::shared_ptr<Server> PtrServer;

class TCPServer : public SocketBase
{
  std::map<UniqID, PtrSerConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_convId2cid;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
  PtrServer m_parent;

public:
  TCPServer(PtrServer parent, int port);

  ~TCPServer();

  // only use in unusal cases
  void closeConnect(UniqID connectId);

  void sendByTcp(UniqID connectId, const uint8_t *data, size_t len);

  void sendByTcp(UniqID connectId, PacketHeader *header);

private:
  EventLoop &getLoop();

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