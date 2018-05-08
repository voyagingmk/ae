#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wyserver.h"
#include "wyclient.h"
#include "wythread.h"
#include "mutex.h"
#include "eventloop.h"
#include "eventloop_thread.h"
#include "eventloop_threadpool.h"

namespace wynet
{
class WyNet
{
public:
  typedef std::map<UniqID, std::shared_ptr<Server>> Servers;
  typedef std::map<UniqID, std::shared_ptr<Client>> Clients;

public:
  WyNet(int threadNum = 4);

  ~WyNet();

  void startLoop();

  void stopLoop();

  EventLoop &getLoop()
  {
    return m_loop;
  }

  UniqID addServer(std::shared_ptr<Server> s);
  bool destroyServer(UniqID serverId);

  UniqID addClient(std::shared_ptr<Client> c);
  bool destroyClient(UniqID serverId);

  PtrThreadPool getThreadPool()
  {
    return m_threadPool;
  }

private:
  EventLoop m_loop;
  Servers m_servers;
  Clients m_clients;
  UniqIDGenerator m_serverIdGen;
  UniqIDGenerator m_clientIdGen;
  MutexLock m_mutexLock;
  PtrThreadPool m_threadPool;
};
};

#endif
