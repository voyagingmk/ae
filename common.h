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
static int Interval = 20;
static int Port = 9999;


class SocketBase { 
public:
    sockaddr_in m_sockaddr;
    int m_sockfd;
};

class UDPServer: public SocketBase { 
public:
    UDPServer(int port);
};


class UDPClient: public SocketBase { 
    sockaddr_in m_serSockaddr;
    struct hostent *h;
public:
    UDPClient(const char* host, int port);
};

class KCPObject {
    ikcpcb * m_kcp;
    SocketBase* m_socket;
public:
    KCPObject(int conv, int interval);

    ~KCPObject();

    const ikcpcb * kcp();

    void bindSocket(SocketBase* s);

    void sendPackage(const char *buf, int len);

    static int kcp_output(const char *buf, int len, ikcpcb *kcp, void * user) {
        KCPObject* obj = (KCPObject*)user;
        assert(obj);
        obj->sendPackage(buf, len);
        return len;
    }
};