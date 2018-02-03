#include "common.h"

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    UDPClient client("127.0.0.1", Port);
    KCPObject kcpObject(Conv, Interval);
    kcpObject.bindSocket(&client);
    loop = aeCreateEventLoop(64);

    aeMain(loop);
    return 0;
}

