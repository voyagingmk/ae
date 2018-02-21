#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern "C" {
#include "wrapsock.h"
#include "error.h"
#include "ikcp.h"
#include "ae.h"
}

#include "kcpwrapper.h"

static aeEventLoop *loop;

static int Conv = 0x11223344;
static int Port = 12001;
static const size_t MAX_MSG = 1400;

class SocketBase
{
public:
  sockaddr_in6 m_sockaddr;
  socklen_t m_socklen;
  int m_sockfd;
  int m_family;
  bool isIPv4() { return m_family == PF_INET; }
  bool isIPv6() { return m_family == PF_INET6; }
};

class UDPServer : public SocketBase
{
  char msg[MAX_MSG];

public:
  UDPServer(int port);
  ~UDPServer();
  void Recvfrom();
};

class UDPClient : public SocketBase
{
  sockaddr_in m_serSockaddr;
  struct hostent *h;

public:
  UDPClient(const char *host, int port);
  ~UDPClient();
  void Send(const char *data, size_t len);
};

static int SocketOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
  SocketBase *s = (SocketBase *)user;
  assert(s);
  // s->send(buf, len);
  return len;
}
