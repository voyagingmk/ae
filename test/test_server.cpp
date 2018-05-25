#include "net.h"
#include "logger/logger.h"
using namespace wynet;

WyNet *g_net;
char lineBuffer[1024];
PtrConn g_conn;

void Stop(int signo)
{
    g_net->stopLoop();
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected] %d", conn->connectId());
    conn->getCtrlAsServer()->addConnection(conn);
}

void OnTcpDisconnected(PtrConn conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
    conn->getCtrlAsServer()->removeConnection(conn);
}

void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
{
    log_debug("[test.OnTcpRecvMessage] readableSize=%d", sockBuf.readableSize());
    conn->send(sockBuf.readBegin(), sockBuf.readableSize());
    sockBuf.readOut(sockBuf.readableSize());
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("test_server");
    log_level(LOG_LEVEL::LOG_INFO);
    log_lineinfo(false);
    // log_file_start();

    WyNet net;
    g_net = &net;

    //  UdpServer server(9999);
    //  KCPObject kcpObject(9999, &server, &SocketOutput);
    log_info("aeGetApiName: %s", aeGetApiName());
    std::shared_ptr<Server> server = std::make_shared<Server>(&net);
    PtrTcpServer tcpServer = server->initTcpServer(NULL, 9998);
    server->initUdpServer(9999);
    tcpServer->onTcpConnected = &OnTcpConnected;
    tcpServer->onTcpDisconnected = &OnTcpDisconnected;
    tcpServer->onTcpRecvMessage = &OnTcpRecvMessage;
    net.addServer(server);
    net.startLoop();
    return 0;
}
