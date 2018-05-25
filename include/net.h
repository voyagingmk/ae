#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "uniqid.h"
#include "kcp.h"
#include "server.h"
#include "client.h"
#include "thread.h"
#include "mutex.h"
#include "eventloop.h"
#include "eventloop_thread.h"
#include "eventloop_threadpool.h"
#include "socket_utils.h"

namespace wynet
{

// strong ref
class PeerManager
{
  typedef std::map<UniqID, PtrServer> Servers;
  typedef std::map<UniqID, PtrClient> Clients;

public:
  ~PeerManager();

  UniqID addServer(PtrServer);

  bool removeServer(UniqID serverId);

  UniqID addClient(PtrClient);

  bool removeClient(UniqID serverId);

private:
  Servers m_servers;
  Clients m_clients;
  UniqIDGenerator m_serverIdGen;
  UniqIDGenerator m_clientIdGen;
};

class WyNet
{

public:
  WyNet(int threadNum = 4);

  void startLoop();

  void stopLoop();

  EventLoop &getLoop()
  {
    return m_loop;
  }

  PtrThreadPool getThreadPool()
  {
    return m_threadPool;
  }
  PeerManager &getPeerManager()
  {
    return m_peerMgr;
  }

private:
  EventLoop m_loop;
  PtrThreadPool m_threadPool;
  PeerManager m_peerMgr;
};
}; // namespace wynet

#endif
