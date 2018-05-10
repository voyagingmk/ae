#ifndef WY_CONNECTION_MANAGER_H
#define WY_CONNECTION_MANAGER_H

#include "common.h"
#include "uniqid.h"
#include "noncopyable.h"
#include "wyconnection.h"

namespace wynet
{
class ConnectionManager : public Noncopyable
{
public:
  ConnectionManager();

  PtrConn getConncetion(UniqID connectId);

  UniqID refConnection(PtrConn conn);

  bool unrefConnection(PtrConn conn);

  void onTcpDisconnected(PtrConn conn);

protected:
  bool unrefConnection(UniqID connectId);

protected:
  std::map<UniqID, PtrConn> m_connDict;
  std::map<int, UniqID> m_connfd2cid;
  std::map<ConvID, UniqID> m_convId2cid;
  UniqIDGenerator m_connectIdGen;
  UniqIDGenerator m_convIdGen;
};

}; // namespace wynet

#endif