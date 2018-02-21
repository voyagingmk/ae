#include "common.h"

#define MAX_MSG 1400
void CustomFileProc(struct aeEventLoop *eventLoop,
                    int fd, void *clientData, int mask)
{
    UDPServer *server = (UDPServer *)(clientData);
    char msg[MAX_MSG];
    memset(msg, 0x0, MAX_MSG);
    struct sockaddr_in cliAddr;
    socklen_t len = sizeof(cliAddr);
    Recvfrom(server->m_sockfd, msg, MAX_MSG, 0,
             (struct sockaddr *)&cliAddr, &len);

    printf("recv from UDP %s : %s \n",
           Sock_ntop((struct sockaddr *)&cliAddr, len),
           msg);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    UDPServer server(Port);
    KCPObject kcpObject(Conv, &server, &SocketOutput);
    printf("aeGetApiName: %s\n", aeGetApiName());
    loop = aeCreateEventLoop(64);
    aeCreateFileEvent(loop, server.m_sockfd, AE_READABLE,
                      CustomFileProc, &server);
    aeMain(loop);
    return 0;
}
