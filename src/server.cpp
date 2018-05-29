#include "server.h"
#include "utils.h"
#include "net.h"
#include "eventloop.h"

namespace wynet
{

Server::Server(WyNet *net) : m_net(net)
{
}

Server::~Server()
{
    log_debug("[Server] destoryed.");
}

PtrTcpServer Server::initTcpServer(const char *host, int tcpPort)
{
    PtrTcpServer server = PtrTcpServer(new TcpServer(shared_from_this()));
    server->init();
    server->startListen(host, tcpPort);
    log_debug("[Server] TcpServer created, tcp sockfd: %d", server->m_sockFdCtrl.sockfd());
    m_tcpServer = server;
    return server;
}

PtrUdpServer Server::initUdpServer(int udpPort)
{
    PtrUdpServer server = std::make_shared<UdpServer>(udpPort);
    m_udpServer = server;
    return server;
}
}; // namespace wynet
