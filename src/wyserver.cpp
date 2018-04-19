#include "wyserver.h"
#include "wyutils.h"
#include "wynet.h"
#include "eventloop.h"

namespace wynet
{

void OnUdpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    // Server *server = (Server *)(clientData);
    // server->m_udpServer.Recvfrom();
}

Server::Server(WyNet *net) : FDRef(0),
                             m_net(net),
                             m_tcpPort(0),
                             m_udpPort(0),
                             onTcpConnected(nullptr),
                             onTcpDisconnected(nullptr),
                             onTcpRecvMessage(nullptr)
{
}

Server::~Server()
{
    m_net->getLoop().deleteFileEvent(m_tcpServer->sockfd(), LOOP_EVT_READABLE);
    m_net->getLoop().deleteFileEvent(m_udpServer->sockfd(), LOOP_EVT_READABLE);
    m_net = nullptr;
    log_info("[Server] destoryed.");
}

void Server::initTcpServer(int tcpPort)
{
    m_tcpPort = tcpPort;
    m_tcpServer = std::make_shared<TCPServer>(shared_from_this(), m_tcpPort);
    log_info("[Server] TCPServer created, tcp sockfd: %d\n", m_tcpServer->sockfd());
}

void Server::initUdpServer(int udpPort)
{
    m_udpPort = udpPort;
    m_udpServer = std::make_shared<UDPServer>(m_udpPort);
    log_info("[Server] UDPServer created, udp sockfd: %d\n", m_udpServer->sockfd());

    m_net->getLoop().createFileEvent(m_udpServer, LOOP_EVT_READABLE,
                                     OnUdpMessage);
}
};
