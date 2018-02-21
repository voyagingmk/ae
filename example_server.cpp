#include "wynet.h"
using namespace wynet;

void CustomFileProc(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask)
{
    UDPServer *server = (UDPServer *)(clientData);
    server->Recvfrom();
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    UDPServer server(9999);
    KCPObject kcpObject(9999, &server, &SocketOutput);
    printf("aeGetApiName: %s\n", aeGetApiName());
    WyNet wynet;
    wynet.loop = aeCreateEventLoop(64);
    aeCreateFileEvent(wynet.loop, server.m_sockfd, AE_READABLE,
                      CustomFileProc, &server);
    aeMain(wynet.loop);
    return 0;
}
