#include "wynet.h"
using namespace wynet;


int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
  //  UDPServer server(9999);
  //  KCPObject kcpObject(9999, &server, &SocketOutput);
    printf("aeGetApiName: %s\n", aeGetApiName());
    WyNet wynet;
    Server * server = new Server(wynet.GetAeLoop(), 9998, 9999);
    wynet.AddServer(server);
    wynet.Loop();
    return 0;
}
