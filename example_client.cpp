#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    printf("Stop. %d\n", net.aeloop->stop);
}

void OnTcpConnected(Client *client) {
    printf("OnTcpConnected: %d\n", client->tcpClient.m_sockfd);
    client->tcpClient.Send("hello", 6);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, Stop);

    printf("aeGetApiName: %s\n", aeGetApiName());
    Client *client = new Client(&net, "127.0.0.1", 9998);
    client->onTcpConnected = &OnTcpConnected;
    net.AddClient(client);
    net.Loop();
    printf("exit\n");
    return 0;
}
