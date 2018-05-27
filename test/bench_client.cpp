#include "net.h"
#include "logger/logger.h"
#include "utils.h"
using namespace wynet;
using namespace std::placeholders;
class TestClient
{
  public:
    TestClient(WyNet *net, const char *ip, int port, int timeout) : m_net(net),
                                                                    m_client(Client::create(net)),
                                                                    m_messagesRead(0),
                                                                    m_bytesRead(0),
                                                                    m_bytesWritten(0),
                                                                    m_timeout(timeout)
    {
        PtrTcpClient tcpClient = m_client->newTcpClient();
        tcpClient->onTcpConnected = std::bind(&TestClient::OnTcpConnected, this, _1);
        tcpClient->onTcpDisconnected = std::bind(&TestClient::OnTcpDisconnected, this, _1);
        tcpClient->onTcpRecvMessage = std::bind(&TestClient::OnTcpRecvMessage, this, _1, _2);
        tcpClient->connect(ip, port);
        m_net->getPeerManager().addClient(m_client);
        m_tcpClient = tcpClient;
    }

    int onTimeout(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
    {
        log_debug("[test.onTimeout]");
        m_tcpClient->getConn()->close(true);
        return -1;
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
        log_debug("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        conn->setCallBack_SendComplete(std::bind(&TestClient::OnTcpSendComplete, this, _1));

        m_timeStart = std::chrono::system_clock::now();
        const char *hello = "hello";
        conn->send((const uint8_t *)hello, sizeof(hello));
        log_debug("m_timeout %d", this->m_timeout);
        conn->getListener()->createTimer(m_timeout, std::bind(&TestClient::onTimeout, this, _1, _2, _3, _4), nullptr);

        // m_net->stopLoop();
        //client->getTcpClient();
    }

    void OnTcpDisconnected(PtrConn conn)
    {
        m_timeEnd = std::chrono::system_clock::now();
        log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
        size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_timeEnd - m_timeStart).count();
        log_info("took %d ms", ms);
        log_info("total bytes read: %d", m_bytesRead);
        log_info("total messages read: %d", m_messagesRead);
        log_info("average message size: %d", static_cast<int>(static_cast<double>(m_bytesRead) / static_cast<double>(m_messagesRead)));
        log_info("%f MiB/s throughput", static_cast<double>(m_bytesRead) / (m_timeout * 1024 * 1024));
        m_net->stopLoop();
    }

    void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
    {
        size_t bytes = sockBuf.readableSize();
        log_debug("[test.OnTcpRecvMessage] readableSize=%d, readOutSize=%d", bytes);
        conn->send(sockBuf.readBegin(), bytes);
        m_bytesRead += bytes;
        m_bytesWritten += bytes;
        sockBuf.readOut(bytes);
    }

  private:
    WyNet *m_net;
    PtrClient m_client;
    PtrTcpClient m_tcpClient;
    size_t m_messagesRead;
    size_t m_bytesRead;
    size_t m_bytesWritten;
    int m_timeout;
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
    if (argc < 3)
    {
        fprintf(stderr, "cmd args: <address> <port>\n");
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("bench_client");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_file_start();

    const char *ip = argv[1];
    int port = static_cast<int>(atoi(argv[2]));
    const int threadsNum = 1;
    WyNet net(threadsNum);
    g_net = &net;
    TestClient testClient(&net, ip, port, 2000);
    net.startLoop();
    log_info("exit");
    return 0;
}
