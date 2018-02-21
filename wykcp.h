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
}

namespace wynet
{

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
};