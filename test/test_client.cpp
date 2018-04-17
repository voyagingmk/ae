#include "wynet.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.stopLoop();
}

void OnTcpConnected(Client *client)
{
    log_info("OnTcpConnected: %d", client->getTcpClient().m_sockfd);
    client->sendByTcp((const uint8_t *)"hello", 5);
}

void OnTcpDisconnected(Client *client)
{
    log_info("OnTcpDisconnected: %d", client->getTcpClient().m_sockfd);
    net.stopLoop();
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        log_set_level((int)(*argv[1]));
    }
    log_set_file("./client.log", "w+");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    log_info("aeGetApiName: %s", aeGetApiName());
    Client *client = new Client(&net, "127.0.0.1", 9998);
    client->onTcpConnected = &OnTcpConnected;
    client->onTcpDisconnected = &OnTcpDisconnected;
    net.addClient(client);
    net.startLoop();
    log_info("exit");
    return 0;
}
