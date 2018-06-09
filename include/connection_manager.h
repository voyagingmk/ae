#ifndef WY_CONNECTION_MANAGER_H
#define WY_CONNECTION_MANAGER_H

#include "common.h"
#include "uniqid.h"
#include "noncopyable.h"
#include "connection.h"
#include "thread.h"

namespace wynet
{

// only for server
class ConnectionManager : public std::enable_shared_from_this<ConnectionManager>, public Noncopyable
{
public:
  ConnectionManager();

  ~ConnectionManager();

  bool addConnection(const PtrConn &conn);

  bool removeConnection(const PtrConn &conn);

  bool removeConnection(UniqID connectId);

  void removeAllConnection();

  PtrConn getConncetion(UniqID connectId);

protected:
  static void weakDeleteCallback(std::weak_ptr<ConnectionManager>, TcpConnection *);

  UniqID refConnection(const PtrConn &conn);

  bool unrefConnection(UniqID connectId);

  bool unrefConnection(const PtrConn &conn);

protected:
  MutexLock m_mutex;
  std::map<UniqID, PtrConn> m_connDict;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
};

}; // namespace wynet

#endif