#include "net.h"
#include "logger/logger.h"
using namespace wynet;
using namespace std::placeholders;

class TestServer
{

  public:
    TestServer(WyNet *net, const char *ip, int port) : m_net(net),
                                                       m_server(nullptr),
                                                       m_numConnected(0),
                                                       m_tcpServer(nullptr),
                                                       m_numClient(0),
                                                       m_evtListener(EventListener::create())

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

        m_evtListener->setEventLoop(&m_net->getLoop());
        m_evtListener->createTimer(10 * 1000, std::bind(&TestServer::onStat, this, _1, _2, _3, _4), nullptr);
    }

    int onStat(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        logStat();
        return 10 * 1000;
    }

    void logStat()
    {
        auto conns = m_tcpServer->getConnMgr()->getAllConnection();
        int numConnected = 0;
        int numDisconnected = 0;
        int numDisconnecting = 0;
        int numNoConn = 0;
        int sdw = 0;
        int evtR = 0;
        int evtW = 0;
        int evtRW = 0;
        {
            for (auto it = conns.begin(); it != conns.end(); it++)
            {
                const PtrConn &conn = it->second;
                if (conn)
                {
                    switch (conn->getState())
                    {
                    case TcpConnection::State::Connected:
                        numConnected++;
                        break;
                    case TcpConnection::State::Disconnected:
                        numDisconnected++;
                        break;
                    case TcpConnection::State::Disconnecting:
                        numDisconnecting++;
                        break;
                    }
                    if (conn->hasShutdownWrite())
                    {
                        sdw++;
                    }
                    const PtrConnEvtListener &l = conn->getListener();
                    if (l)
                    {
                        int mask = l->getFileEventMask();
                        if (mask & (MP_READABLE | MP_WRITABLE))
                        {
                            evtRW++;
                        }
                        else if (mask & MP_READABLE)
                        {
                            evtR++;
                        }
                        else if (mask & MP_WRITABLE)
                        {
                            evtR++;
                        }
                    }
                }
                else
                {
                    numNoConn++;
                }
            }
        }
        log_info("======== onStat ======== \n [count] connected: %d, disconnected: %d, disconnecting: %d, noConn: %d, sdw: %d <%d,%d,%d>",
                 numConnected,
                 numDisconnected,
                 numDisconnecting,
                 numNoConn,
                 sdw, evtRW, evtR, evtR);
    }

    void terminate()
    {
        m_tcpServer->terminate();
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_debug("[test.onTimeout]");
        m_net->stopAllLoop();
        return -1;
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        m_numConnected++;
        int i = ++m_numClient;
        conn->setUserData(i);
        log_debug("[test.OnTcpConnected] sockfd %d", conn->sockfd());
        socketUtils::setTcpKeepAlive(conn->sockfd(), true);
        socketUtils::setTcpKeepInterval(conn->sockfd(), 5);
        socketUtils::setTcpKeepIdle(conn->sockfd(), 60);
        socketUtils::setTcpKeepCount(conn->sockfd(), 3);
        conn->setCallBack_SendComplete(std::bind(&TestServer::OnTcpSendComplete, this, _1));
        conn->getCtrlAsServer()->addConnection(conn);
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        int num = --m_numConnected;
        // log_info("numConnected %d", num);
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
    PtrEvtListener m_evtListener;
};

WyNet *g_net;
TestServer *g_testServer;

void Stop(int signo)
{
    log_info("Stop()");
    g_testServer->terminate();
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
    g_testServer = &server;
    log_info("TestServer start listen %s %d", ip, port);
    g_net = &net;
    net.startLoop();
    return 0;
}
