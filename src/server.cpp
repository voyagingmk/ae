#include "server.h"
#include "utils.h"
#include "net.h"
#include "eventloop.h"

namespace wynet
{

Server::Server(WyNet *net) : m_net(net),
                             m_udpPort(0),
                             m_tcpServer(nullptr),
                             m_udpServer(nullptr)
{
}

Server::~Server()
{
    m_tcpServer = nullptr;
    m_udpServer = nullptr;
    m_net = nullptr;
    log_info("[Server] destoryed.");
}

PtrTcpServer Server::initTcpServer(const char *host, int tcpPort)
{
    if (!m_tcpServer)
    {
        m_tcpServer = PtrTcpServer(new TcpServer(shared_from_this()));
        m_tcpServer->init();
        m_tcpServer->startListen(host, tcpPort);
        log_info("[Server] TcpServer created, tcp sockfd: %d", m_tcpServer->m_sockFdCtrl.sockfd());
    }
    return m_tcpServer;
}

std::shared_ptr<UdpServer> Server::initUdpServer(int udpPort)
{
    m_udpPort = udpPort;
    m_udpServer = std::make_shared<UdpServer>(m_udpPort);
    return m_udpServer;
}
}; // namespace wynet
