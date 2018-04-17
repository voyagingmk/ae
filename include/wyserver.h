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

class Server : public Noncopyable, FDRef
{
  WyNet *m_net;
  int m_tcpPort;
  int m_udpPort;
  TCPServer m_tcpServer;
  UDPServer m_udpServer;
  std::map<UniqID, PtrSerConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_convId2cid;
  UniqIDGenerator m_clientIdGen;
  UniqIDGenerator m_convIdGen;

public:
  typedef void (*OnTcpConnected)(Server *, UniqID clientId);
  typedef void (*OnTcpDisconnected)(Server *, UniqID clientId);
  typedef void (*OnTcpRecvUserData)(Server *, UniqID clientId, uint8_t *, size_t);

  OnTcpConnected onTcpConnected;
  OnTcpDisconnected onTcpDisconnected;
  OnTcpRecvUserData onTcpRecvUserData;

public:
  Server(WyNet *net, int tcpPort, int udpPort = 0);

  ~Server();

  // only use in unusal cases
  void closeConnect(UniqID clientId);

  void sendByTcp(UniqID clientId, const uint8_t *data, size_t len);

  void sendByTcp(UniqID clientId, PacketHeader *header);

  WyNet* getNet() const {
      return m_net;
  }
  TCPServer& getTCPServer() {
    return m_tcpServer;
  }

private:
  void _closeConnectByFd(int connfdTcp, bool force = false);

  void _onTcpConnected(int connfdTcp);

  void _onTcpMessage(int connfdTcp);

  void _onTcpDisconnected(int connfdTcp);

  friend void OnTcpNewConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

  friend void OnUdpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};
};

#endif
