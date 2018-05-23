#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "logger/log.h"
#include "buffer.h"
#include "protocol.h"
#include "multiple_inherit.h"

namespace wynet
{

class FDRef : public inheritable_enable_shared_from_this<FDRef>
{
public:
  FDRef(int fd)
  {
    setfd(fd);
  }
  virtual ~FDRef() {}

  inline int fd() const { return m_fd; }

protected:
  inline void setfd(int fd) { m_fd = fd; }

private:
  int m_fd;
};

class SocketBase : public FDRef
{
protected:
  SocketBase(int fd = 0) : FDRef(fd),
                           m_socklen(0)
  {
  }
  virtual ~SocketBase()
  {
    ::close(sockfd());
    setfd(0);
  }

public:
  inline void setSockfd(int fd) { setfd(fd); }
  inline int sockfd() const { return fd(); }
  inline bool valid() const { return fd() > 0; }
  inline bool isIPv4() const { return m_sockaddr.ss_family == PF_INET; }
  inline bool isIPv6() const { return m_sockaddr.ss_family == PF_INET6; }

public:
  sockaddr_storage m_sockaddr;
  socklen_t m_socklen;
};

typedef std::shared_ptr<SocketBase> PtrCtrl;
typedef std::weak_ptr<SocketBase> PtrCtrlWeak;

}; // namespace wynet

#endif
