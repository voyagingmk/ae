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
                                            m_shutdown(false),
                                            m_evtListener(EventListener::create())
    {
        m_bytesRead.resize(sessions);
        m_messagesRead.resize(sessions);
        m_timeStart = std::chrono::system_clock::now();
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
            m_tcpClients.insert(tcpClient);
        }
        for (auto it = m_tcpClients.begin(); it != m_tcpClients.end(); it++)
        {
            (*it)->connect(ip, port);
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
    }

    void shutdownAll()
    {
        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(0, 60 * 1000);
        MutexLockGuard<MutexLock> lock(m_mutex);
        m_shutdown = true;
        int i = 0;
        for (auto it = m_tcpClients.begin(); it != m_tcpClients.end(); it++)
        {
            i++;
            const PtrTcpClient &tcpClient = *it;
            int delay = distribution(generator);
            tcpClient->getLoop().createTimer(
                m_evtListener, 2 * 1000 * (i % 10),
                [=](EventLoop *, TimerRef tr, PtrEvtListener listener, void *data) -> int {
                    tcpClient->disconnect();
                    return MP_HALT;
                },
                nullptr);
        }
    }

    int onStat(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
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

        int numConnected = 0;
        int numDisconnected = 0;
        int numDisconnecting = 0;
        int numNoConn = 0;
        int sdw = 0;
        int evtR = 0;
        int evtW = 0;
        int evtRW = 0;
        {

            std::set<PtrTcpClient> tcpClients;
            {
                MutexLockGuard<MutexLock> lock(m_mutex);
                tcpClients = m_tcpClients;
            }
            for (auto it = tcpClients.begin(); it != tcpClients.end(); it++)
            {
                PtrConn conn = (*it)->getConn();
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
        log_info("======== onStat ========\ntook %d ms", ms);
        log_info("[atomic] connected: %d", (int)m_numConnected);
        log_info("[count] connected: %d, disconnected: %d, disconnecting: %d, noConn: %d, sdw: %d, <%d,%d,%d>",
                 numConnected,
                 numDisconnected,
                 numDisconnecting,
                 numNoConn,
                 sdw, evtRW, evtR, evtR);
        log_info("total bytes read: %lld", bytesRead);
        log_info("total messages read: %lld", messagesRead);
        log_info("average message size: %lld", static_cast<int64_t>(static_cast<double>(bytesRead) / static_cast<double>(messagesRead)));
        log_info("%f MiB/s throughput",
                 static_cast<double>(bytesRead) / (ms * 1024 * 1024 / 1000));
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_info("[test.onTimeout]");
        PtrConnEvtListener l = std::dynamic_pointer_cast<TcpConnectionEventListener>(listener);
        auto conn = l->getTcpConnection();
        conn->shutdown();
        int num = ++m_numTimeout;
        log_info("numTimeout %d", num);
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
        if (!tcpClient->needReconnect())
        {
            log_info("fail too many.");
            MutexLockGuard<MutexLock> lock(m_mutex);
            auto it = m_tcpClients.find(tcpClient);
            assert(it != m_tcpClients.end());
            m_tcpClients.erase(it);
        }
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        // log_info("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        conn->setCallBack_SendComplete(std::bind(&TestClient::OnTcpSendComplete, this, _1));
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
        int num = m_numConnected;
        // log_info("[connected] numConnected %d, sockfd: %d", num, conn->sockfd());
        conn->setUserData(num);
        m_numConnected++;
        {
            MutexLockGuard<MutexLock> lock(m_mutex);
            if (m_shutdown)
            {
                auto tcpClient = conn->getCtrlAsClient();
                tcpClient->disconnect();
                return;
            }
        }
        conn->send(m_message);
        conn->getListener()->createTimer(m_timeout, std::bind(&TestClient::onTimeout, this, _1, _2, _3, _4), nullptr);
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        // log_info("[test.OnTcpDisconnected] %d", conn->connectId());
        int num = --m_numConnected;
        auto tcpClient = conn->getCtrlAsClient();
        tcpClient->setReconnectTimes(0);

        MutexLockGuard<MutexLock> lock(m_mutex);
        auto it = m_tcpClients.find(tcpClient);
        assert(it != m_tcpClients.end());
        m_tcpClients.erase(it);
        // log_info("[disconnected] numConnected %d, sockfd: %d", num, conn->sockfd());
        if (m_numConnected == 0)
        {
            logStat();
            m_tcpClients.clear();
            m_net->stopAllLoop();
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
    std::set<PtrTcpClient> m_tcpClients;
    std::vector<int64_t> m_messagesRead;
    std::vector<int64_t> m_bytesRead;
    std::atomic_int m_numConnected;
    std::atomic_int m_numTimeout;
    int m_timeout;
    bool m_shutdown;
    std::string m_message;
    std::chrono::system_clock::time_point m_timeStart;
    PtrEvtListener m_evtListener;
    MutexLock m_mutex; // guard m_tcpClients
};

typedef std::shared_ptr<TestClient> PtrTestClient;
PtrTestClient ptrTestClient;
WyNet *g_net;

void Stop(int signo)
{
    log_info("Stop()");
    if (ptrTestClient)
    {
        ptrTestClient->shutdownAll();
    }
    g_net->stopAllLoop();
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
    ptrTestClient = std::make_shared<TestClient>(&net, ip, port, blocksize, sessions, seconds * 1000);
    net.startLoop();
    sleep(1); // avoid RST problem
    log_info("exit");
    return 0;
}
