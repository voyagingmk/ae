#ifndef WY_NET_H
#define WY_NET_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wyserver.h"
#include "wyclient.h"
#include "mutex.h"
#include "eventloop.h"
#include "eventloop_thread.h"
#include "eventloop_threadpool.h"

namespace wynet
{
class WyNet
{
public:
  typedef std::map<UniqID, Server *> Servers;
  typedef std::map<UniqID, Client *> Clients;

public:
  WyNet();

  ~WyNet();

  void startLoop();

  void stopLoop();

  EventLoop &getLoop()
  {
    return m_loop;
  }

  UniqID addServer(Server *server);
  bool destroyServer(UniqID serverId);

  UniqID addClient(Client *client);
  bool destroyClient(UniqID serverId);

private:
  EventLoop m_loop;
  Servers m_servers;
  Clients m_clients;
  UniqIDGenerator m_serverIdGen;
  UniqIDGenerator m_clientIdGen;
  MutexLock m_mutexLock;
};
};

#endif
