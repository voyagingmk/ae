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
  EventLoop loop;
  typedef std::map<UniqID, Server *> Servers;
  typedef std::map<UniqID, Client *> Clients;
  Servers servers;
  Clients clients;
  UniqIDGenerator serverIdGen;
  UniqIDGenerator clientIdGen;
  MutexLock mutexLock;

public:
  WyNet();

  ~WyNet();

  void Loop();

  void StopLoop();
  UniqID AddServer(Server *server);
  bool DestroyServer(UniqID serverId);

  UniqID AddClient(Client *client);
  bool DestroyClient(UniqID serverId);
};
};

#endif
