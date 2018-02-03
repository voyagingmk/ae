#include "common.h"

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    ikcpcb *kcp1 = ikcp_create(0x11223344, (void*)0);
    loop = aeCreateEventLoop(64);

    aeMain(loop);
    return 0;
}

