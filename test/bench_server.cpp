#include "net.h"
#include "logger/logger.h"
using namespace wynet;
using namespace std::placeholders;

class TestServer
{

  public:
    TestServer(WyNet *net, const char *ip, int port) : m_net(net)
    {
        PtrServer server = Server::create(net);
        m_server = server;
        PtrTcpServer tcpServer = server->initTcpServer(strcmp(ip, "") == 0 ? NULL : ip, port);
        tcpServer->onTcpConnected = std::bind(&TestServer::OnTcpConnected, this, _1);
        tcpServer->onTcpDisconnected = std::bind(&TestServer::OnTcpDisconnected, this, _1);
        tcpServer->onTcpRecvMessage = std::bind(&TestServer::OnTcpRecvMessage, this, _1, _2);
        m_tcpServer = tcpServer;
        net->getPeerManager().addServer(server);
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        log_info("[test.OnTcpConnected] sockfd %d", conn->sockfd());
        // conn->setCallBack_SendComplete(std::bind(&TestServer::OnTcpSendComplete, this, _1));
        conn->getCtrlAsServer()->addConnection(conn);
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        log_info("[test.OnTcpDisconnected] sockfd %d", conn->sockfd());
        conn->getCtrlAsServer()->removeConnection(conn);
    }

    void OnTcpRecvMessage(const PtrConn &conn, SockBuffer &sockBuf)
    {
        log_debug("[test.OnTcpRecvMessage] readableSize=%d", sockBuf.readableSize());
        conn->send(sockBuf.readBegin(), sockBuf.readableSize());
        sockBuf.readOut(sockBuf.readableSize());
    }

    void OnTcpSendComplete(const PtrConn &conn)
    {
        log_info("[test.OnTcpSendComplete]");
        //      std::string msg(buffer, ret_in);
        //      conn->send(msg);

        //      m_net->stopLoop();
        //
    }

  private:
    WyNet *m_net;
    PtrServer m_server;
    PtrTcpServer m_tcpServer;
};

WyNet *g_net;

void Stop(int signo)
{
    g_net->stopLoop();
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

    //  log_file("bench_server");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_console(false);
    // log_file_start();

    const char *ip = argv[1];
    int port = static_cast<int>(atoi(argv[2]));
    int threadsNum = atoi(argv[3]);
    if (threadsNum < 1)
    {
        threadsNum = 1;
    }
    WyNet net(threadsNum);
    TestServer server(&net, ip, port);
    log_info("TestServer start listen %s %d", ip, port);
    g_net = &net;
    net.startLoop();
    return 0;
}
