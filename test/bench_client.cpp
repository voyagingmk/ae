#include "net.h"
#include "logger/logger.h"
#include "utils.h"

using namespace wynet;
using namespace std::placeholders;

class TestClient
{
  public:
    TestClient(WyNet *net, const char *ip, int port, int blockSize, int sessions, int timeout) : m_net(net),
                                                                                                 m_messagesRead(0),
                                                                                                 m_bytesRead(0),
                                                                                                 m_bytesWritten(0),
                                                                                                 m_timeout(timeout)
    {
        EventLoop *loop = m_net->getThreadPool().getNextLoop();
        for (int i = 0; i < sessions; i++)
        {
            PtrTcpClient tcpClient = std::make_shared<TcpClient>(loop);
            tcpClient->setReconnectTimes(5);
            tcpClient->setReconnectInterval(1000);
            tcpClient->onTcpConnectFailed = std::bind(&TestClient::OnTcpConnectFailed, this, _1);
            tcpClient->onTcpConnected = std::bind(&TestClient::OnTcpConnected, this, _1);
            tcpClient->onTcpDisconnected = std::bind(&TestClient::OnTcpDisconnected, this, _1);
            tcpClient->onTcpRecvMessage = std::bind(&TestClient::OnTcpRecvMessage, this, _1, _2);
            tcpClient->connect(ip, port);
            m_tcpClients.push_back(tcpClient);
        }

        for (int i = 0; i < blockSize; ++i)
        {
            m_message.push_back(static_cast<char>(i % 128));
        }
    }

    ~TestClient()
    {
        log_dtor(" ~TestClient()");
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_info("[test.onTimeout]");
        PtrConnEvtListener l = std::dynamic_pointer_cast<TcpConnectionEventListener>(listener);
        auto conn = l->getTcpConnection();
        conn->shutdown();
        // m_tcpClient = nullptr;
        return -1;
    }

    void OnTcpSendComplete(const PtrConn &conn)
    {
        log_info("[test.OnTcpSendComplete]");
        //      std::string msg(buffer, ret_in);
        //      conn->send(msg);

        //      m_net->stopLoop();
        //
    }

    void OnTcpConnectFailed(const PtrTcpClient &tcpClient)
    {
        if (!tcpClient->needReconnect())
        {
            m_net->stopLoop();
        }
    }

    void OnTcpConnected(const PtrConn &conn)
    {
        log_info("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        // conn->setCallBack_SendComplete(std::bind(&TestClient::OnTcpSendComplete, this, _1));

        m_timeStart = std::chrono::system_clock::now();
        conn->send(m_message);
        socketUtils::setTcpNoDelay(conn->sockfd(), true);
        log_info("m_timeout %d", this->m_timeout);
        conn->getListener()->createTimer(m_timeout, std::bind(&TestClient::onTimeout, this, _1, _2, _3, _4), nullptr);
        // m_net->stopLoop();
        //client->getTcpClient();
    }

    void OnTcpDisconnected(const PtrConn &conn)
    {
        m_timeEnd = std::chrono::system_clock::now();
        log_info("[test.OnTcpDisconnected] %d", conn->connectId());

        PtrTcpClient tcpClient = conn->getCtrlAsClient();
        auto it = std::find(m_tcpClients.begin(), m_tcpClients.end(), tcpClient);
        m_tcpClients.erase(it);
        if (m_tcpClients.size() == 0)
        {
            size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_timeEnd - m_timeStart).count();
            log_info("took %d ms, m_timeout %d ms", ms, m_timeout);
            log_info("total bytes read: %lld", m_bytesRead);
            log_info("total messages read: %lld", m_messagesRead);
            log_info("average message size: %lld", static_cast<int64_t>(static_cast<double>(m_bytesRead) / static_cast<double>(m_messagesRead)));
            log_info("%f MiB/s throughput",
                     static_cast<double>(m_bytesRead) / (ms * 1024 * 1024 / 1000));
            m_net->stopLoop();
        }
    }

    void OnTcpRecvMessage(const PtrConn &conn, SockBuffer &sockBuf)
    {
        size_t bytes = sockBuf.readableSize();
        log_debug("[test.OnTcpRecvMessage] readableSize=%d, readOutSize=%d", bytes);
        conn->send(sockBuf.readBegin(), bytes);
        m_bytesRead += bytes;
        m_bytesWritten += bytes;
        ++m_messagesRead;
        sockBuf.readOut(bytes);
    }

  private:
    WyNet *m_net;
    std::list<PtrTcpClient> m_tcpClients;
    int64_t m_messagesRead;
    int64_t m_bytesRead;
    int64_t m_bytesWritten;
    int m_timeout;
    std::string m_message;
    std::chrono::system_clock::time_point m_timeStart;
    std::chrono::system_clock::time_point m_timeEnd;
};

WyNet *g_net;

void Stop(int signo)
{
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
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

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
