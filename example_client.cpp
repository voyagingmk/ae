#include "wynet.h"
using namespace wynet;

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    UDPClient client("127.0.0.1", 9999);
    printf("aaa\n");
    client.Send("hello", 6);
    KCPObject kcpObject(9999, &client, &SocketOutput);

    return 0;
}
