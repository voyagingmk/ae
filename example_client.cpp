#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)   
{  
    printf("Stop.\n");  
    net.StopLoop();
}  

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);
    
    printf("aeGetApiName: %s\n", aeGetApiName());
    Client* client = new Client(net.GetAeLoop(), "127.0.0.1", 9998);
    net.AddClient(client);
    net.Loop();
    return 0;
    /*
    UDPClient client("127.0.0.1", 9999);
    printf("aaa\n");
    client.Send("hello", 6);
    KCPObject kcpObject(9999, &client, &SocketOutput);
    */
}
