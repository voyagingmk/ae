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

void OnTcpSendComplete(PtrConn conn)
{
    log_debug("[test.OnTcpSendComplete]");
    //      std::string msg(buffer, ret_in);
    //      conn->send(msg);

    //      m_net->stopLoop();
    //
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected] sockfd %d", conn->sockfd());
    conn->setCallBack_SendComplete(&OnTcpSendComplete);
    conn->getCtrlAsServer()->addConnection(conn);
    log_debug("addConnection, connectId %d", conn->connectId());
    socketUtils::setTcpNoDelay(conn->sockfd(), true);
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
    if (argc < 4)
    {
        fprintf(stderr, "cmd args: <address> <port> <threads num>\n");
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("bench_server");
    log_level(LOG_LEVEL::LOG_INFO);
    log_lineinfo(false);
    // log_file_start();

    const char *ip = argv[1];
    int port = static_cast<int>(atoi(argv[2]));
    int threadsNum = atoi(argv[3]);
    if (threadsNum < 1)
    {
        threadsNum = 1;
    }
    WyNet net(threadsNum);
    g_net = &net;
    PtrServer server = Server::create(&net);
    PtrTcpServer tcpServer = server->initTcpServer(strcmp(ip, "") == 0 ? NULL : ip, port);
    tcpServer->onTcpConnected = &OnTcpConnected;
    tcpServer->onTcpDisconnected = &OnTcpDisconnected;
    tcpServer->onTcpRecvMessage = &OnTcpRecvMessage;
    net.getPeerManager().addServer(server);
    net.startLoop();
    return 0;
}
