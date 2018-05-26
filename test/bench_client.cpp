#include "net.h"
#include "logger/logger.h"
#include "utils.h"
using namespace wynet;
using namespace std::placeholders;
class TestClient
{
  public:
    TestClient(WyNet *net, const char *ip, int port) : m_net(net),
                                                       m_client(Client::create(net))
    {
        PtrTcpClient tcpClient = m_client->initTcpClient(ip, port);
        tcpClient->onTcpConnected = std::bind(&TestClient::OnTcpConnected, this, _1);
        tcpClient->onTcpDisconnected = std::bind(&TestClient::OnTcpDisconnected, this, _1);
        tcpClient->onTcpRecvMessage = std::bind(&TestClient::OnTcpRecvMessage, this, _1, _2);
        m_net->getPeerManager().addClient(m_client);
        m_tcpClient = tcpClient;
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
        m_timeStart = std::chrono::system_clock::now();
        log_debug("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        conn->setCallBack_SendComplete(std::bind(&TestClient::OnTcpSendComplete, this, _1));
        const char *hello = "hello";
        conn->send((const uint8_t *)hello, sizeof(hello));
        // m_net->stopLoop();
        //client->getTcpClient();
    }

    void OnTcpDisconnected(PtrConn conn)
    {
        m_timeEnd = std::chrono::system_clock::now();
        log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
        size_t us = std::chrono::duration_cast<std::chrono::microseconds>(m_timeEnd - m_timeStart).count();
        log_debug("took %d us", us);
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
    size_t m_bytesRead;
    size_t m_bytesWritten;
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
    TestClient testClient(&net, ip, port);
    net.startLoop();
    log_info("exit");
    return 0;
}
