#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

extern "C" {
    #include "ae.h"
    #include "ikcp.h"
}

static aeEventLoop *loop;

static int conv = 0x11223344;
static int interval = 20;

static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user) {
    
}

class KCPObject {
    ikcpcb *kcp;
public:
    KCPObject() {
        kcp = ikcp_create(conv, this);
    }
    ~KCPObject() {
        ikcp_release(kcp);
        kcp = NULL;
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
    ikcpcb *kcp = ikcp_create(conv, (void*)0);
    kcp->output = kcp_output;
    ikcp_wndsize(kcp, 128, 128);
	ikcp_nodelay(kcp, 0, interval, 0, 0);

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

