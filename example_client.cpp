#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    printf("Stop. %d\n", net.aeloop->stop);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, Stop);

    printf("aeGetApiName: %s\n", aeGetApiName());
    Client *client = new Client(net.GetAeLoop(), "127.0.0.1", 9998);
    net.AddClient(client);
    net.Loop();
    printf("exit\n");
    return 0;
}
