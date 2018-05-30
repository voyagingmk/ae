#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "logger/log.h"
#include "buffer.h"
#include "protocol.h"
#include "multiple_inherit.h"
#include "socket_utils.h"

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
    close();
    m_sockfd = 0;
  }

  void setSockfd(SockFd sockfd) { m_sockfd = sockfd; }

  inline SockFd sockfd() const { return m_sockfd; }

  void close()
  {
    if (m_sockfd)
    {
      log_debug("close %d", m_sockfd);
      socketUtils ::sock_close(m_sockfd);
    }
  }

public:
  SockFd m_sockfd;
};

class SockAddr
{
public:
  SockAddr() : m_port(0),
               m_addr(nullptr),
               m_socklen(0)
  {
  }
  SockAddr(const char *host, int port) : m_host(host),
                                         m_port(port),
                                         m_addr(nullptr),
                                         m_socklen(0)
  {
  }

  SockAddr(const std::string &host, int port) : m_host(host),
                                                m_port(port),
                                                m_addr(nullptr),
                                                m_socklen(0)
  {
  }

  const char *getHost() const
  {
    return m_host.c_str();
  }

  int getPort() const
  {
    return m_port;
  }

private:
  std::string m_host;
  int m_port;
  sockaddr_storage *m_addr;
  socklen_t m_socklen;
};

}; // namespace wynet

#endif
