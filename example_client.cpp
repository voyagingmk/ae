#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    log_info("Stop. %d", net.aeloop->stop);
}

void OnTcpConnected(Client *client)
{
    log_info("OnTcpConnected: %d", client->tcpClient.m_sockfd);
    client->tcpClient.Send("hello", 6);
}

int main(int argc, char **argv)
{
    log_set_file("./client.log", "w+");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    log_info("aeGetApiName: %s", aeGetApiName());
    Client *client = new Client(&net, "127.0.0.1", 9998);
    client->onTcpConnected = &OnTcpConnected;
    net.AddClient(client);
    net.Loop();
    log_info("exit");
    return 0;
}
