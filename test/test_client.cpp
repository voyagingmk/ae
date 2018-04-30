#include "wynet.h"
#include "logger/logger.h"
using namespace wynet;

WyNet *g_net;

void Stop(int signo)
{
    g_net->stopLoop();
}

void OnTcpConnected(PtrConn conn)
{
    //client->getTcpClient();
    //log_info("OnTcpConnected: %d", client->getTcpClient()->sockfd());
    // client->sendByTcp((const uint8_t *)"hello", 5);
}

void OnTcpDisconnected(PtrConn conn)
{
    //log_info("OnTcpDisconnected: %d", client->getTcpClient()->sockfd());
    g_net->stopLoop();
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("test_client");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_start();

    WyNet net;
    g_net = &net;

    log_info("aeGetApiName: %s", aeGetApiName());
    std::shared_ptr<Client> client = std::make_shared<Client>(&net);
    std::shared_ptr<TCPClient> tcpClient = client->initTcpClient("127.0.0.1", 9998);
    tcpClient->onTcpConnected = &OnTcpConnected;
    tcpClient->onTcpDisconnected = &OnTcpDisconnected;
    net.addClient(client);
    net.startLoop();
    log_info("exit");
    return 0;
}
