#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern "C" {
    #include "unp.h"
    #include "ae.h"
    #include "ikcp.h"
}

static aeEventLoop *loop;

static int Conv = 0x11223344;
static int Interval = 20;

class Socket { 
public:
    sockaddr_in m_sockaddr;
    int m_sockfd;
};

class UDPServer: public Socket { 
public:
    UDPServer(int port) {
        m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&m_sockaddr, sizeof(m_sockaddr));
        m_sockaddr.sin_family = AF_INET;
        m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        m_sockaddr.sin_port = htons(port);
        bind(m_sockfd, (sockaddr*)&m_sockaddr, sizeof(m_sockaddr));
    }
};


class UDPClient: public Socket { 
public:
};

class KCPObject {
    ikcpcb * m_kcp;
    Socket* m_socket;
public:
    KCPObject(int conv, int interval) {
        m_kcp = ikcp_create(conv, this);
        m_kcp->output = KCPObject::kcp_output;
        ikcp_wndsize(m_kcp, 128, 128);
	    ikcp_nodelay(m_kcp, 0, interval, 0, 0);
    }
    ~KCPObject() {
        ikcp_release(m_kcp);
        m_kcp = NULL;
    }
    const ikcpcb * kcp() {
        return m_kcp;
    }
    void bindSocket(Socket* s) {
        m_socket = s;
    }
    void send_udp_package(const char *buf, int len)
    {     
        sendto(m_socket->m_sockfd, buf, len, 0, pcliaddr, len)
       // ptr->send_udp_packet(std::string(buf, len), udp_remote_endpoint_);
    }
    static int kcp_output(const char *buf, int len, ikcpcb *kcp, void * user) {
        KCPObject* obj = (KCPObject*)user;
        assert(obj);
        obj->send_udp_package(buf, len);
        return len;
    }
};