#include "common.h"

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    UDPServer server(Port);
    KCPObject kcpObject(Conv, Interval);
    kcpObject.bindSocket(&server);

    loop = aeCreateEventLoop(64);
    aeMain(loop);
    return 0;
}

