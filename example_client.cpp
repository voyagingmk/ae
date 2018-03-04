#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    log_info("Stop. %d\n", net.aeloop->stop);
}

void OnTcpConnected(Client *client)
{
    log_info("OnTcpConnected: %d\n", client->tcpClient.m_sockfd);
    client->tcpClient.Send("hello", 6);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    log_info("aeGetApiName: %s\n", aeGetApiName());
    Client *client = new Client(&net, "127.0.0.1", 9998);
    client->onTcpConnected = &OnTcpConnected;
    net.AddClient(client);
    net.Loop();
    log_info("exit\n");
    return 0;
}
