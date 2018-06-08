#include "net.h"
#include "logger/logger.h"
using namespace wynet;
using namespace std::placeholders;

class TestServer
{

  public:
    TestServer(WyNet *net, const char *ip, int port) : m_net(net),
                                                       m_numClient(0),
                                                       m_numConnected(0)
    {
        PtrServer server = Server::create(net);
        m_server = server;
        PtrTcpServer tcpServer = server->initTcpServer(strcmp(ip, "") == 0 ? NULL : ip, port);
        tcpServer->onTcpConnected = std::bind(&TestServer::OnTcpConnected, this, _1);
        tcpServer->onTcpDisconnected = std::bind(&TestServer::OnTcpDisconnected, this, _1);
        tcpServer->onTcpRecvMessage = std::bind(&TestServer::OnTcpRecvMessage, this, _1, _2);
        // tcpServer->getListener()->createTimer(5000, std::bind(&TestServer::onTimeout, this, _1, _2, _3, _4), nullptr);
        m_tcpServer = tcpServer;

        net->getPeerManager().addServer(server);
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_debug("[test.onTimeout]");
        m_net->stopLoop();
        return -1;
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        m_numConnected++;
        int i = ++m_numClient;
        conn->setUserData(i);
        log_debug("[test.OnTcpConnected] sockfd %d", conn->sockfd());
        conn->setCallBack_SendComplete(std::bind(&TestServer::OnTcpSendComplete, this, _1));
        conn->getCtrlAsServer()->addConnection(conn);
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        int num = --m_numConnected;
        log_info("numConnected %d", num);
        conn->getCtrlAsServer()->removeConnection(conn);
    }

    void OnTcpRecvMessage(const PtrConn &conn, SockBuffer &sockBuf)
    {
        log_debug("[test.recv] fd=%d size=%d", conn.sockfd(), sockBuf.readableSize());
        conn->send(sockBuf.readBegin(), sockBuf.readableSize());
        sockBuf.readOut(sockBuf.readableSize());
    }

    void OnTcpSendComplete(const PtrConn &conn)
    {
        int i = conn->getUserData();
        //  log_info("send ok %d", i);
        //      std::string msg(buffer, ret_in);
        //      conn->send(msg);

        //      m_net->stopLoop();
        //
    }

  private:
    WyNet *m_net;
    PtrServer m_server;
    std::atomic_int m_numConnected;
    PtrTcpServer m_tcpServer;
    std::atomic_int m_numClient;
};

WyNet *g_net;

void Stop(int signo)
{
    log_info("Stop()");
    g_net->stopAllLoop();
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "cmd args: <address> <port> <threads num>\n");
        return -1;
    }
    if (signal(SIGINT, Stop) == SIG_ERR)
    {
        fprintf(stderr, "can't catch SIGPIPE\n");
    }
    //  log_file("bench_server");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_console(false);
    // log_file_start();

    const char *ip = argv[1];
    int port = static_cast<int>(atoi(argv[2]));
    int threadsNum = atoi(argv[3]);
    if (threadsNum < 0)
    {
        threadsNum = 0;
    }
    WyNet net(threadsNum);
    TestServer server(&net, ip, port);
    log_info("TestServer start listen %s %d", ip, port);
    g_net = &net;
    net.startLoop();
    return 0;
}
