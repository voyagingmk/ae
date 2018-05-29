#ifndef WY_CONNECTION_MANAGER_H
#define WY_CONNECTION_MANAGER_H

#include "common.h"
#include "uniqid.h"
#include "noncopyable.h"
#include "connection.h"
#include "thread.h"

namespace wynet
{

// optional tool:
// in order to collect PtrConn (keep the PtrConn alive)
// so user need to explicitly add and delete the connection
class ConnectionManager : public std::enable_shared_from_this<ConnectionManager>, public Noncopyable
{
public:
  ConnectionManager();

  ~ConnectionManager();

  bool addConnection(PtrConn conn);

  bool removeConnection(PtrConn conn);

  bool removeConnection(UniqID connectId);

  PtrConn getConncetion(UniqID connectId);

protected:
  static void weakDeleteCallback(std::weak_ptr<ConnectionManager>, TcpConnection *);

  UniqID refConnection(PtrConn conn);

  bool unrefConnection(UniqID connectId);

  bool unrefConnection(PtrConn conn);

protected:
  MutexLock m_mutex;
  std::map<UniqID, PtrConn> m_connDict;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
};

}; // namespace wynet

#endif