#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern "C" {
    #include "ae.h"
    #include "ikcp.h"
}

static aeEventLoop *loop;

static int Conv = 0x11223344;
static int Interval = 20;

class UDPServer { 
    sockaddr_in servaddr;
    int sockfd;
public:
    UDPServer(int port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);
        bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr));

    }
};


class UDPClient { 
    sockaddr_in cliaddr;
public:
    void Bind() {
      //  bind(sockfd, (SA*)&cliaddr, sizeof(cliaddr));
    }
};

class KCPObject {
    ikcpcb * m_kcp;
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
    static int kcp_output(const char *buf, int len, ikcpcb *kcp, void * user) {
        KCPObject* obj = (KCPObject*)user;
        assert(obj);
        obj->send_udp_package(buf, len);
        return len;
    }
    void send_udp_package(const char *buf, int len)
    {     
       // ptr->send_udp_packet(std::string(buf, len), udp_remote_endpoint_);
    }
};

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    /*
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        printf("Error: %s\n", c->errstr);
        return 1;
    }*/
    UDPServer server(9999);
    KCPObject kcpObject(Conv, Interval);

    loop = aeCreateEventLoop(64);
    /*
    redisAeAttach(loop, c);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc-1], strlen(argv[argc-1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");
    */
    aeMain(loop);
    return 0;
}

