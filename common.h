#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern "C" {
#include "wrapsock.h"
#include "error.h"
#include "ae.h"
#include "ikcp.h"
}

static aeEventLoop *loop;

static int Conv = 0x11223344;
static int Port = 12001;

class SocketBase
{
public:
  sockaddr_in6 m_sockaddr;
  socklen_t m_socklen;
  int m_sockfd;
};

class UDPServer : public SocketBase
{
public:
  UDPServer(int port);
  ~UDPServer();
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

class KCPObject
{
  ikcpcb *m_kcp;

public:
  typedef int (*OutputFunc)(const char *buf, int len,
                            ikcpcb *kcp, void *user);

public:
  KCPObject(int conv, void *userdata, OutputFunc outputFunc);

  ~KCPObject();

  const ikcpcb *kcp();

  int getSendWin();

  int getRecvWin();

  void setSendWin(int wnd);

  void setRecvWin(int wnd);

  void setNodelay(int nodelay, int interval, int resend, int nc);

  int send(const char *buf, int len);

  int recv(char *buf, int len);

  int nextRecvSize();

  void update(IUINT32 current);

  IUINT32 check(IUINT32 current);

  int input(const char *data, long size);

  void flush();

  int setmtu(int mtu);

  int waitsnd();

  static void setAllocator(void *(*new_malloc)(size_t), void (*new_free)(void *))
  {
    ikcp_allocator(new_malloc, new_free);
  }
};

static int SocketOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
  SocketBase *s = (SocketBase *)user;
  assert(s);
  // s->send(buf, len);
  return len;
}