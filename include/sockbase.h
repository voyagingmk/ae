#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "logger/log.h"
#include "buffer.h"
#include "protocol.h"
#include "multiple_inherit.h"

namespace wynet
{

// 为了自动close
class SocketFdCtrl : public Noncopyable
{
public:
  SocketFdCtrl(SockFd sockfd) : m_sockfd(sockfd)
  {
  }

  virtual ~SocketFdCtrl()
  {
    if (m_sockfd)
    {
      ::close(m_sockfd);
    }
    m_sockfd = 0;
  }

  void setSockfd(SockFd sockfd) { m_sockfd = sockfd; }

  inline SockFd sockfd() const { return m_sockfd; }

public:
  SockFd m_sockfd;
};

class SockAddr
{
public:
  sockaddr_storage m_addr;
  socklen_t m_socklen;
};

}; // namespace wynet

#endif
