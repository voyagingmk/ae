#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "uniqid.h"
#include "kcp.h"
#include "server.h"
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

public:
  PeerManager();

  ~PeerManager();

  UniqID addServer(PtrServer);

  bool removeServer(UniqID serverId);

private:
  Servers m_servers;
  UniqIDGenerator m_serverIdGen;
  UniqIDGenerator m_clientIdGen;
};

class WyNet
{

public:
  WyNet(int threadNum = 0);

  ~WyNet();

  void startLoop();

  void stopLoop();

  EventLoop &getLoop()
  {
    return m_loop;
  }

  EventLoopThreadPool &getThreadPool()
  {
    return m_threadPool;
  }
  PeerManager &getPeerManager()
  {
    return m_peerMgr;
  }

private:
  EventLoop m_loop;
  EventLoopThreadPool m_threadPool;
  PeerManager m_peerMgr;
};
}; // namespace wynet

#endif
