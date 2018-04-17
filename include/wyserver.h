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

class Server : public Noncopyable
{
  WyNet *m_net;
  int m_tcpPort;
  int m_udpPort;
  TCPServer m_tcpServer;
  UDPServer m_udpServer;
  std::map<UniqID, PtrSerConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_cconvId2cid;

  UniqIDGenerator m_clientIdGen;
  UniqIDGenerator m_convIdGen;

public:
  typedef void (*OnTcpConnected)(Server *, UniqID clientId);
  typedef void (*OnTcpDisconnected)(Server *, UniqID clientId);
  typedef void (*OnTcpRecvUserData)(Server *, UniqID clientId, uint8_t *, size_t);

  OnTcpConnected onTcpConnected;
  OnTcpDisconnected onTcpDisconnected;
  OnTcpRecvUserData onTcpRecvUserData;

  Server(WyNet *net, int tcpPort, int udpPort = 0);

  ~Server();

  // only use in unusal cases
  void closeConnect(UniqID clientId);

  void sendByTcp(UniqID clientId, const uint8_t *data, size_t len);

  void sendByTcp(UniqID clientId, PacketHeader *header);

private:
  void _closeConnectByFd(int connfdTcp, bool force = false);

  void _onTcpConnected(int connfdTcp);

  void _onTcpMessage(int connfdTcp);

  void _onTcpDisconnected(int connfdTcp);

  friend void onTcpMessage(EventLoop *eventLoop,
                           int connfdTcp, void *clientData, int mask);

  friend void OnTcpNewConnection(EventLoop *eventLoop,
                                 int listenfdTcp, void *clientData, int mask);

  friend void OnUdpMessage(EventLoop *eventLoop,
                           int fd, void *clientData, int mask);
};
};

#endif
