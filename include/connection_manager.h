#ifndef WY_CONNECTION_MANAGER_H
#define WY_CONNECTION_MANAGER_H

#include "common.h"
#include "uniqid.h"
#include "noncopyable.h"
#include "wyconnection.h"

namespace wynet
{
class ConnectionManager : public std::enable_shared_from_this<ConnectionManager>, public Noncopyable
{
public:
  ConnectionManager();

  PtrConn newConnection();

  PtrConn getConncetion(UniqID connectId);

  UniqID refConnection(PtrConn conn);

  bool unrefConnection(PtrConn conn);

  void onTcpDisconnected(PtrConn conn);

protected:
  static void weakDeleteCallback(std::weak_ptr<ConnectionManager>, TcpConnection *);

  bool unrefConnection(UniqID connectId);

protected:
  std::map<UniqID, PtrConnWeak> m_connDict;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
};

}; // namespace wynet

#endif