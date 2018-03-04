#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    log_info("Stop. %d\n", net.aeloop->stop);
}

int main(int argc, char **argv)
{
    log_set_file("./server.log", "w+");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);
    //  UDPServer server(9999);
    //  KCPObject kcpObject(9999, &server, &SocketOutput);
    log_info("aeGetApiName: %s\n", aeGetApiName());
    Server *server = new Server(net.GetAeLoop(), 9998, 9999);
    net.AddServer(server);
    net.Loop();
    return 0;
}
