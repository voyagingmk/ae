#include "net.h"
#include "logger/logger.h"
#include "utils.h"

using namespace wynet;
using namespace std::placeholders;

class TestClient
{
  public:
    TestClient(WyNet *net, const char *ip,
               int port, int blockSize,
               int sessions, int timeout) : m_net(net),
                                            m_numConnected(0),
                                            m_numTimeout(0),
                                            m_timeout(timeout),
                                            m_evtListener(EventListener::create())
    {
        m_bytesRead.resize(sessions);
        m_messagesRead.resize(sessions);
        for (int i = 0; i < sessions; i++)
        {
            m_bytesRead[i] = 0;
            m_messagesRead[i] = 0;
            EventLoop *loop = m_net->getThreadPool().getNextLoop();
            PtrTcpClient tcpClient = std::make_shared<TcpClient>(loop);
            tcpClient->setReconnectTimes(5);
            tcpClient->setReconnectInterval(1000);
            tcpClient->onTcpConnectFailed = std::bind(&TestClient::OnTcpConnectFailed, this, _1);
            tcpClient->onTcpConnected = std::bind(&TestClient::OnTcpConnected, this, _1);
            tcpClient->onTcpDisconnected = std::bind(&TestClient::OnTcpDisconnected, this, _1);
            tcpClient->onTcpRecvMessage = std::bind(&TestClient::OnTcpRecvMessage, this, _1, _2);
            m_tcpClients.push_back(tcpClient);
        }
        for (int i = 0; i < sessions; i++)
        {
            m_tcpClients[i]->connect(ip, port);
        }
        for (int i = 0; i < blockSize; ++i)
        {
            m_message.push_back(static_cast<char>(i % 128));
        }
        m_evtListener->setEventLoop(&m_net->getLoop());
        m_evtListener->createTimer(10 * 1000, std::bind(&TestClient::onStat, this, _1, _2, _3, _4), nullptr);
    }

    ~TestClient()
    {
        log_dtor(" ~TestClient()");
        m_tcpClients.clear();
    }

    int onStat(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_info("======== onStat ========");
        logStat();
        return 10 * 1000;
    }

    void logStat()
    {
        int64_t bytesRead = 0;
        int64_t messagesRead = 0;
        for (int i = 0; i < m_bytesRead.size(); i++)
        {
            bytesRead += m_bytesRead[i];
            messagesRead += m_messagesRead[i];
        }
        auto timeEnd = std::chrono::system_clock::now();
        size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - m_timeStart).count();
        log_info("took %d ms, m_timeout %d ms", ms, m_timeout);
        log_info("total bytes read: %lld", bytesRead);
        log_info("total messages read: %lld", messagesRead);
        log_info("average message size: %lld", static_cast<int64_t>(static_cast<double>(bytesRead) / static_cast<double>(messagesRead)));
        log_info("%f MiB/s throughput",
                 static_cast<double>(bytesRead) / (ms * 1024 * 1024 / 1000));
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        // log_info("[test.onTimeout]");
        PtrConnEvtListener l = std::dynamic_pointer_cast<TcpConnectionEventListener>(listener);
        auto conn = l->getTcpConnection();
        conn->shutdown();
        int num = ++m_numTimeout;
        log_info("numTimeout %d", num);
        // m_tcpClient = nullptr;
        return -1;
    }

    void OnTcpSendComplete(const PtrConn &conn)
    {
        int i = conn->getUserData();
        // log_info("send ok %d", i);
        //      std::string msg(buffer, ret_in);
        //      conn->send(msg);

        //      m_net->stopLoop();
        //
    }

    void OnTcpConnectFailed(const PtrTcpClient &tcpClient)
    {
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        // log_info("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        conn->setCallBack_SendComplete(std::bind(&TestClient::OnTcpSendComplete, this, _1));
        int i = m_numConnected;
        conn->setUserData(i);
        m_numConnected++;
        m_timeStart = std::chrono::system_clock::now();
        conn->send(m_message);
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
        conn->getListener()->createTimer(m_timeout, std::bind(&TestClient::onTimeout, this, _1, _2, _3, _4), nullptr);
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        // log_info("[test.OnTcpDisconnected] %d", conn->connectId());
        int num = --m_numConnected;
        conn->getCtrlAsClient()->setReconnectTimes(0);
        log_info("numConnected %d", num);
        if (m_numConnected == 0)
        {
            logStat();
            m_tcpClients.clear();
            m_net->stopLoop();
        }
    }

    void OnTcpRecvMessage(const PtrConn &conn, SockBuffer &sockBuf)
    {
        size_t bytes = sockBuf.readableSize();
        log_debug("[test.OnTcpRecvMessage] readableSize=%d, readOutSize=%d", bytes);
        conn->send(sockBuf.readBegin(), bytes);
        int i = conn->getUserData();
        m_bytesRead[i] += bytes;
        ++m_messagesRead[i];
        sockBuf.readOut(bytes);
    }

  private:
    WyNet *m_net;
    std::vector<PtrTcpClient> m_tcpClients;
    std::vector<int64_t> m_messagesRead;
    std::vector<int64_t> m_bytesRead;
    std::atomic_int m_numConnected;
    std::atomic_int m_numTimeout;
    int m_timeout;
    std::string m_message;
    std::chrono::system_clock::time_point m_timeStart;
    PtrEvtListener m_evtListener;
};

WyNet *g_net;

void Stop(int signo)
{
    log_info("Stop()");
    g_net->stopLoop();
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        fprintf(stderr, "cmd args: <host_ip> <port> <threads> <blockSize>");
        fprintf(stderr, "<sessions> <time>\n");
        return -1;
    }
    if (signal(SIGINT, Stop) == SIG_ERR)
    {
        fprintf(stderr, "can't catch SIGPIPE\n");
    }

    //  log_file("bench_client");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    //  log_console(false);
    //  log_file_start();

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int threadsNum = atoi(argv[3]);
    int blocksize = atoi(argv[4]);
    int sessions = atoi(argv[5]);
    double seconds = atof(argv[6]);
    if (threadsNum < 0)
    {
        threadsNum = 0;
    }
    WyNet net(threadsNum);
    g_net = &net;
    log_info("testClient");
    TestClient testClient(&net, ip, port, blocksize, sessions, seconds * 1000);
    net.startLoop();
    sleep(1); // avoid RST problem
    log_info("exit");
    return 0;
}
